#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "morningllvm.hpp"

#include <boost/algorithm/string.hpp>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Alignment.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

#include "codegen/arithmetic.hpp"
#include "env.h"
#include "logger.hpp"
#include "parser/MorningLangGrammar.h"
#include "tracelogger.hpp"
#include "utils/cast.hpp"
#include "utils/convert.hpp"

namespace {
    /**
     * @brief Replace regex in string
     *
     * @param str string for replacing text in string by regex templates
     * @return std::string
     **/
    auto replace_regex_in_string(const std::string& str) -> std::string {
        auto regex_newline = std::regex("\\\\n");
        auto regex_tab = std::regex("\\\\t");

        auto newlined = std::regex_replace(str, regex_newline, "\n");
        auto tabed = std::regex_replace(newlined, regex_tab, "\t");

        return tabed;
    }

    /**
     * @brief Extract name from variable
     *
     * @param exp exp
     * @return std::string
     **/
    auto extract_var_name(const Exp& exp) -> std::string {
        return exp.type == ExpType::LIST ? exp.list[0].string : exp.string;
    }

    /**
     * @brief Check if function has a return type
     *
     * @param fn_exp function exp
     * @return true
     * @return false
     **/
    auto has_return_type(const Exp& fn_exp) -> bool {
        return fn_exp.list[3].type == ExpType::SYMBOL && fn_exp.list[3].string == "->";
    }

    /**
     * @brief Safe expression converting to string
     *
     * @param exp expression for converting
     * @return std::string converted string expression
     **/
    auto safe_expr_to_string(const Exp& exp) -> std::string {
        switch (exp.type) {
            case ExpType::LIST: {
                if (exp.list.empty()) {
                    return "[]";
                }

                std::string s = "[";
                for (const auto& e : exp.list) {
                    s += safe_expr_to_string(e) + " ";
                }
                s.pop_back();    // Remove last space
                s += "]";

                // Trim long expressions
                if (s.length() > 120) {
                    return s.substr(0, 117) + "...";
                }
                return s;
            }
            case ExpType::SYMBOL:
                return exp.string;
            case ExpType::NUMBER:
                return std::to_string(exp.number);
            case ExpType::FRACTIONAL:
                return std::to_string(exp.fractional);
            case ExpType::STRING: {
                auto str1 = "\"" + exp.string + "\"";
                boost::replace_all(str1, "\n", "\\n");
                return str1;
            }
            default:
                return "<?>";
        }
    }

    /**
     * @brief Add expression to traceback expressions stack.
     *
     * @param exp expression for adding
     **/
    void add_expression_to_traceback_stack(const Exp& exp) {
        std::string context = "expr";
        std::string const EXPR_STR = safe_expr_to_string(exp);

        if (EXPR_STR == "<?>") {
            return;
        }

        if (exp.type == ExpType::LIST || exp.type == ExpType::SYMBOL || exp.type == ExpType::NUMBER
            || exp.type == ExpType::FRACTIONAL || exp.type == ExpType::STRING)
        {
            if (exp.type == ExpType::LIST && !exp.list.empty()) {
                if (exp.list[0].type == ExpType::SYMBOL) {
                    context = exp.list[0].string;
                } else {
                    context = "list";
                }
            } else {
                switch (exp.type) {
                    case ExpType::SYMBOL:
                        context = "symbol";
                        break;
                    case ExpType::NUMBER:
                        context = "number";
                        break;
                    case ExpType::FRACTIONAL:
                        context = "fractional";
                        break;
                    case ExpType::STRING:
                        context = "string";
                        break;
                    default:
                        context = "value";
                }
            }
        }

        PUSH_EXPR_STACK(context, EXPR_STR);
    }
}    // namespace

MorningLanguageLLVM::MorningLanguageLLVM()
    : m_PARSER(std::make_unique<syntax::MorningLangGrammar>()) {
    LOG_TRACE

    initialize_module();
    setup_triple();
    setup_extern_functions();
    setup_global_environment();
}

auto MorningLanguageLLVM::execute(const std::string& program, const std::string& output_base) -> int {
    LOG_TRACE

    auto ast = m_PARSER->parse("[scope " + program + "]");
    generate_ir(ast);

    llvm::verifyModule(*m_MODULE, &llvm::errs());
    save_module_to_file(output_base + ".ll");

    return 0;
}

void MorningLanguageLLVM::setup_triple() {
    m_MODULE->setTargetTriple("x86_64-unknown-linux-gnu");
}

void MorningLanguageLLVM::setup_global_environment() {
    LOG_TRACE

    std::map<std::string, llvm::Value*> const GLOBAL_OBJECT {
        {"_VERSION", m_IR_BUILDER->getInt64(300)},
    };

    std::map<std::string, llvm::Value*> global_rec {};

    for (const auto& entry : GLOBAL_OBJECT) {
        global_rec[entry.first] = create_global_variable(entry.first, (llvm::Constant*)entry.second);
    }

    m_GLOBAL_ENV = std::make_shared<Environment>(global_rec, nullptr);
}

void MorningLanguageLLVM::generate_ir(const Exp& ast) {
    LOG_TRACE

    // Create function type: i32 main()
    auto* main_type = llvm::FunctionType::get(m_IR_BUILDER->getInt64Ty(),    // Return type = 32-bit integer
                                              /* Parameters */ {},    // Empty list = no arguments
                                              /* Varargs */ false    // No "..."
    );

    m_ACTIVE_FUNCTION = create_function("main", main_type, m_GLOBAL_ENV);

    generate_expression(ast, m_GLOBAL_ENV);

    m_IR_BUILDER->CreateRet(m_IR_BUILDER->getInt64(0));
}

auto MorningLanguageLLVM::create_global_variable(const std::string& name,
                                                 llvm::Constant* init_value,
                                                 bool is_mutable) -> llvm::GlobalVariable* {
    LOG_TRACE

    m_MODULE->getOrInsertGlobal(name, init_value->getType());

    auto* variable = m_MODULE->getNamedGlobal(name);

    variable->setAlignment(llvm::MaybeAlign(4));
    variable->setConstant(!is_mutable);
    variable->setInitializer(init_value);

    return variable;
}

auto MorningLanguageLLVM::get_type(const std::string& type_string, const std::string& var_name)
    -> llvm::Type* {
    if (type_string == "!int" || type_string == "!int64") {
        return m_IR_BUILDER->getInt64Ty();
    }

    if (type_string == "!int32") {
        return m_IR_BUILDER->getInt64Ty();
    }

    if (type_string == "!int16") {
        return m_IR_BUILDER->getInt16Ty();
    }

    if (type_string == "!int8") {
        return m_IR_BUILDER->getInt8Ty();
    }

    if (type_string == "!str") {
        return m_IR_BUILDER->getInt8Ty()->getPointerTo();
    }

    if (type_string == "!ptr") {
        return m_IR_BUILDER->getInt8Ty()->getPointerTo();
    }

    if (type_string == "!frac") {
        return m_IR_BUILDER->getDoubleTy();
    }

    if (type_string == "!bool") {
        return m_IR_BUILDER->getInt8Ty();
    }

    if (type_string == "!none") {
        return m_IR_BUILDER->getVoidTy();
    }

    if (type_string.find("!size:") == 0) {
        size_t colon_pos = type_string.find(':');
        if (colon_pos == std::string::npos) {
            LOG_CRITICAL("Invalid size constraint for '%s'", var_name.c_str());
        }

        std::string base_type = type_string.substr(colon_pos + 1);
        llvm::Type* type = get_type(base_type, var_name);
        uint64_t expected_size = std::stoull(type_string.substr(6, colon_pos - 6));
        uint64_t actual_size = get_type_size(type);

        if (actual_size != expected_size) {
            LOG_CRITICAL("Size mismatch for '%s': expected %d bytes, actual %d bytes",
                         var_name.c_str(),
                         expected_size,
                         actual_size);
        }
        return type;
    }

    if (type_string.find("!ptr<") == 0) {
        size_t start = type_string.find('<');
        size_t end = type_string.rfind('>');
        if (end == std::string::npos) {
            LOG_CRITICAL("Invalid pointer type for '%s': missing '>'", var_name.c_str());
        }

        std::string inner = type_string.substr(start + 1, end - start - 1);
        llvm::Type* pointee_type = get_type(inner, var_name);
        return llvm::PointerType::get(*m_CONTEXT, 0);
    }

    if (type_string.find("!array<") == 0) {
        size_t start = type_string.find('<');
        size_t end = type_string.rfind('>');
        if (end == std::string::npos) {
            LOG_CRITICAL("Invalid array type for '%s': missing '>'", var_name.c_str());
        }

        std::string inner = type_string.substr(start + 1, end - start - 1);

        // Parse with nested type support
        int bracket_level = 0;
        size_t comma_pos = std::string::npos;
        for (size_t i = 0; i < inner.length(); i++) {
            char c = inner[i];
            if (c == '<') {
                bracket_level++;
            } else if (c == '>') {
                bracket_level--;
            } else if (c == ',' && bracket_level == 0) {
                comma_pos = i;
                break;
            }
        }

        if (comma_pos == std::string::npos) {
            LOG_CRITICAL("Invalid array type for '%s': expected comma", var_name.c_str());
        }

        // Extract element type and size
        std::string element_type_str = inner.substr(0, comma_pos);
        std::string size_str = inner.substr(comma_pos + 1);

        // Trim whitespace
        auto trim = [](std::string& s)
        {
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
            s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); }).base(),
                    s.end());
        };

        trim(element_type_str);
        trim(size_str);

        // Parse size
        int size;
        try {
            size = std::stoi(size_str);
            if (size <= 0) {
                LOG_CRITICAL("Invalid array size for '%s': must be positive integer", var_name.c_str());
            }
        } catch (...) {
            LOG_CRITICAL("Invalid array size for '%s': not a number", var_name.c_str());
        }

        // Recursively get element type
        llvm::Type* element_type = get_type(element_type_str, var_name);
        return llvm::ArrayType::get(element_type, size);
    }

    LOG_WARN("Variable \"%s\" does not have typing: set by auto (!int)", var_name.c_str());

    return m_IR_BUILDER->getInt64Ty();
}

auto MorningLanguageLLVM::extract_var_type(const Exp& exp) -> llvm::Type* {
    return exp.type == ExpType::LIST ? get_type(exp.list[1].string, exp.list[0].string)
                                     : m_IR_BUILDER->getInt64Ty();
}

auto MorningLanguageLLVM::extract_function_type(const Exp& fn_exp) -> llvm::FunctionType* {
    auto params = fn_exp.list[2];

    auto* return_type = has_return_type(fn_exp) ? get_type(fn_exp.list[4].string, fn_exp.list[0].string)
                                                : m_IR_BUILDER->getInt64Ty();

    std::vector<llvm::Type*> param_types {};

    for (auto& param : params.list) {
        auto* param_type = extract_var_type(param);
        param_types.push_back(param_type);
    }

    return llvm::FunctionType::get(return_type, param_types, /* varargs */ false);
}

auto MorningLanguageLLVM::alloc_var(const std::string& name, llvm::Type* var_type, const env& env)
    -> llvm::Value* {
    LOG_TRACE

    if (var_type == nullptr) {
        LOG_ERROR("Null type for variable '%s'", name.c_str());
        return nullptr;
    }

    if (env->lookup_by_name(name) != nullptr) {
        LOG_WARN("Redeclaration of variable '%s'", name.c_str());
    }

    m_VARS_BUILDER->SetInsertPoint(&m_ACTIVE_FUNCTION->getEntryBlock());

    auto* allocated_var = m_VARS_BUILDER->CreateAlloca(var_type, nullptr, name);

    env->define(name, allocated_var);

    return allocated_var;
}

auto MorningLanguageLLVM::compile_function(const Exp& fn_exp, const std::string& fn_name, const env& env)
    -> llvm::Value* {
    auto params = fn_exp.list[2];
    auto body = has_return_type(fn_exp) ? fn_exp.list[5] : fn_exp.list[3];

    auto* prev_fn = m_ACTIVE_FUNCTION;
    auto* prev_block = m_IR_BUILDER->GetInsertBlock();

    auto* new_fn = create_function(fn_name, extract_function_type(fn_exp), env);
    m_ACTIVE_FUNCTION = new_fn;

    auto idx = 0;

    // auto fn_env = std::make_shared<Environment>(std::map<std::string, llvm::Value*> {}, env);
    auto fn_env = std::make_shared<Environment>(std::map<std::string, llvm::Value*>(), env);
    fn_env->define(fn_name, new_fn);

    // Process parameters
    for (auto& arg : m_ACTIVE_FUNCTION->args()) {
        if (idx >= params.list.size()) {
            LOG_CRITICAL("Too many arguments for function '%s'", fn_name.c_str());
        }

        auto param = params.list[idx];
        auto arg_name = extract_var_name(param);
        arg.setName(arg_name);

        // Get parameter type
        llvm::Type* param_type = nullptr;
        if (param.type == ExpType::LIST && param.list.size() >= 2) {
            param_type = get_type(param.list[1].string, arg_name);
        } else {
            param_type = arg.getType();
        }

        // Allocate and store parameter
        auto* arg_binding = alloc_var(arg_name, param_type, fn_env);
        m_IR_BUILDER->CreateStore(&arg, arg_binding);
        idx++;
    }

    m_IR_BUILDER->CreateRet(generate_expression(body, fn_env));

    m_IR_BUILDER->SetInsertPoint(prev_block);
    m_ACTIVE_FUNCTION = prev_fn;

    return new_fn;
}

auto MorningLanguageLLVM::generate_expression(const Exp& exp, const env& env) -> llvm::Value* {
    LOG_TRACE

    add_expression_to_traceback_stack(exp);

    switch (exp.type) {
        case ExpType::NUMBER: {
            int64_t value = exp.number;

            if (value >= INT8_MIN && value <= INT8_MAX) {
                return m_IR_BUILDER->getInt8(static_cast<int8_t>(value));
            }
            if (value >= INT16_MIN && value <= INT16_MAX) {
                return m_IR_BUILDER->getInt16(static_cast<int16_t>(value));
            }
            if (value >= INT32_MIN && value <= INT32_MAX) {
                return m_IR_BUILDER->getInt32(static_cast<int32_t>(value));
            }
            return m_IR_BUILDER->getInt64(value);
        }
        case ExpType::FRACTIONAL:
            return llvm::ConstantFP::get(m_IR_BUILDER->getDoubleTy(), exp.fractional);
        case ExpType::STRING: {
            auto str = replace_regex_in_string(exp.string);

            return m_IR_BUILDER->CreateGlobalStringPtr(str);
        }
        case ExpType::SYMBOL:
            if (exp.string == "true" || exp.string == "false") {
                return m_IR_BUILDER->getInt8(static_cast<uint8_t>(exp.string == "true"));
            } else {
                auto var_name = exp.string;
                auto* value = env->lookup_by_name(var_name);

                // Handle functions separately
                if (llvm::isa<llvm::Function>(value)) {
                    return value;
                }

                // Handle opaque pointers (LLVM 19+)
                if (value->getType()->isPointerTy()) {
                    // Get pointee type from allocation
                    llvm::Type* pointee_type = nullptr;
                    if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(value)) {
                        pointee_type = alloca->getAllocatedType();
                    }
                    else if (auto* global = llvm::dyn_cast<llvm::GlobalVariable>(value)) {
                        pointee_type = global->getValueType();
                    }

                    if (pointee_type) {
                        return m_IR_BUILDER->CreateLoad(pointee_type, value, var_name.c_str());
                    }
                }

                // Default load
                return m_IR_BUILDER->CreateLoad(value->getType(), value, var_name.c_str());
            }
        case ExpType::LIST:
            if (exp.list.empty()) {
                LOG_CRITICAL("Empty list expression at line %s", exp.list[1].string.c_str());
            }

            auto oper = exp.list[0].string;
            if (oper == "+" && exp.list.size() < 3) {
                LOG_CRITICAL("Operator '+' requires two operands at line %s", exp.list[1].string.c_str());
            }

            auto tag = exp.list[0];

            if (tag.type == ExpType::SYMBOL) {
                auto oper = tag.string;

                if (oper == "+" || oper == "-" || oper == "*" || oper == "/" || oper == ">" || oper == "<"
                    || oper == ">=" || oper == "<=" || oper == "==" || oper == "!="
                    || oper == "__PLUS_OPERAND__" || oper == "__SUB_OPERAND__" || oper == "__MUL_OPERAND__"
                    || oper == "__DIV_OPERAND__" || oper == "__CMPG__" || oper == "__CMPL__"
                    || oper == "__CMPGE__" || oper == "__CMPLE__" || oper == "__CMPEQ__"
                    || oper == "__CMPNE__")
                {
                    auto* left = generate_expression(exp.list[1], env);
                    auto* right = generate_expression(exp.list[2], env);
                    return ArithmeticCodegen::generate_binary_op(oper, left, right, *m_IR_BUILDER);
                }

                if (oper == "array") {
                    LOG_DEBUG("Process array creation");

                    // Handle nested arrays
                    bool is_nested_array = false;
                    llvm::Type* element_type = nullptr;
                    std::vector<llvm::Constant*> elements;

                    for (size_t i = 1; i < exp.list.size(); i++) {
                        auto* element_val = generate_expression(exp.list[i], env);

                        if (auto* constant = llvm::dyn_cast<llvm::Constant>(element_val)) {
                            // First element determines type
                            if (element_type == nullptr) {
                                element_type = element_val->getType();
                                is_nested_array = element_type->isArrayTy();
                            }

                            // Validate type consistency
                            if (element_val->getType() != element_type) {
                                LOG_CRITICAL("Array element type mismatch at index %zu", i - 1);
                            }

                            elements.push_back(constant);
                        } else {
                            LOG_CRITICAL("Array element must be constant expression");
                        }
                    }

                    if (elements.empty()) {
                        LOG_CRITICAL("Array cannot be empty");
                    }

                    // For nested arrays, use the first element's type
                    if (is_nested_array) {
                        return llvm::ConstantArray::get(llvm::ArrayType::get(element_type, elements.size()),
                                                        elements);
                    }

                    // For simple arrays
                    return llvm::ConstantArray::get(llvm::ArrayType::get(element_type, elements.size()),
                                                    elements);
                }

                if (oper == "sizeof") {
                    LOG_DEBUG("Process sizeof operator");

                    if (exp.list.size() < 2) {
                        LOG_CRITICAL("sizeof requires a type argument");
                    }

                    std::string type_str = exp.list[1].string;
                    llvm::Type* target_type = get_type(type_str, "sizeof");

                    llvm::DataLayout data_layout(m_MODULE.get());
                    llvm::TypeSize type_size = data_layout.getTypeAllocSize(target_type);
                    return m_IR_BUILDER->getInt64(type_size.getFixedValue());
                }

                if (oper == "mem-alloc") {
                    LOG_DEBUG("Process memory allocation");

                    auto size_exp = exp.list[1];
                    auto* size_val = generate_expression(size_exp, env);
                    auto* malloc_fn = m_MODULE->getFunction("malloc");

                    if (!malloc_fn) {
                        auto* malloc_type = llvm::FunctionType::get(
                            m_IR_BUILDER->getInt8Ty()->getPointerTo(), {m_IR_BUILDER->getInt64Ty()}, false);
                        malloc_fn = llvm::Function::Create(
                            malloc_type, llvm::Function::ExternalLinkage, "malloc", m_MODULE.get());
                    }

                    return m_IR_BUILDER->CreateCall(malloc_fn, {size_val}, "malloc");
                }

                if (oper == "mem-free") {
                    LOG_DEBUG("Process memory free");

                    auto ptr_exp = exp.list[1];
                    auto* ptr_val = generate_expression(ptr_exp, env);
                    auto* free_fn = m_MODULE->getFunction("free");

                    if (!free_fn) {
                        auto* free_type = llvm::FunctionType::get(
                            m_IR_BUILDER->getVoidTy(), {m_IR_BUILDER->getInt8Ty()->getPointerTo()}, false);
                        free_fn = llvm::Function::Create(
                            free_type, llvm::Function::ExternalLinkage, "free", m_MODULE.get());
                    }

                    m_IR_BUILDER->CreateCall(free_fn, {ptr_val});
                    return m_IR_BUILDER->getInt64(0);
                }

                if (oper == "bit-and" || oper == "bit-or" || oper == "bit-xor" ||
                    oper == "bit-shl" || oper == "bit-shr" || oper == "bit-not") {

                    if (oper == "bit-not") {
                        auto* value = generate_expression(exp.list[1], env);

                        if (!value->getType()->isIntegerTy()) {
                            LOG_CRITICAL("Bitwise operation requires integer operand, got %s",
                                        type_to_string(value->getType()).c_str());
                        }

                        return m_IR_BUILDER->CreateNot(value, "bit_not");
                    }

                    auto* left = generate_expression(exp.list[1], env);
                    auto* right = generate_expression(exp.list[2], env);

                    if (!left->getType()->isIntegerTy() || !right->getType()->isIntegerTy()) {
                        LOG_CRITICAL("Bitwise operation requires integer operands, got %s and %s",
                                    type_to_string(left->getType()).c_str(),
                                    type_to_string(right->getType()).c_str());
                    }

                    llvm::Type* common_type = nullptr;
                    if (left->getType() != right->getType()) {
                        unsigned left_size = left->getType()->getIntegerBitWidth();
                        unsigned right_size = right->getType()->getIntegerBitWidth();
                        unsigned max_size = std::max(left_size, right_size);
                        common_type = m_IR_BUILDER->getIntNTy(max_size);

                        left = m_IR_BUILDER->CreateZExtOrTrunc(left, common_type);
                        right = m_IR_BUILDER->CreateZExtOrTrunc(right, common_type);
                    } else {
                        common_type = left->getType();
                    }

                    if (oper == "bit-and") {
                        return m_IR_BUILDER->CreateAnd(left, right, "bit_and");
                    }
                    if (oper == "bit-or") {
                        return m_IR_BUILDER->CreateOr(left, right, "bit_or");
                    }
                    if (oper == "bit-xor") {
                        return m_IR_BUILDER->CreateXor(left, right, "bit_xor");
                    }
                    if (oper == "bit-shl") {
                        return m_IR_BUILDER->CreateShl(left, right, "bit_shl");
                    }
                    if (oper == "bit-shr") {
                        return m_IR_BUILDER->CreateLShr(left, right, "bit_shr");
                    }
                }

                if (oper == "byte-read") {
                    auto* ptr = generate_expression(exp.list[1], env);
                    auto* casted_ptr =
                        m_IR_BUILDER->CreateBitCast(ptr, m_IR_BUILDER->getInt8Ty()->getPointerTo());
                    return m_IR_BUILDER->CreateLoad(m_IR_BUILDER->getInt8Ty(), casted_ptr, "byte_read");
                }
                if (oper == "byte-write") {
                    auto* ptr = generate_expression(exp.list[1], env);
                    auto* value = generate_expression(exp.list[2], env);
                    auto* casted_ptr =
                        m_IR_BUILDER->CreateBitCast(ptr, m_IR_BUILDER->getInt8Ty()->getPointerTo());
                    return m_IR_BUILDER->CreateStore(
                        m_IR_BUILDER->CreateTrunc(value, m_IR_BUILDER->getInt8Ty()), casted_ptr);
                }

                if (oper == "mem-write") {
                    LOG_DEBUG("Process memory write");

                    auto ptr_exp = exp.list[1];
                    auto value_exp = exp.list[2];
                    auto* ptr_val = generate_expression(ptr_exp, env);
                    auto* value_val = generate_expression(value_exp, env);

                    auto* casted_ptr = m_IR_BUILDER->CreateBitCast(
                        ptr_val, value_val->getType()->getPointerTo(), "cast_ptr");
                    m_IR_BUILDER->CreateStore(value_val, casted_ptr);
                    return value_val;
                }

                if (oper == "mem-read") {
                    LOG_DEBUG("Process memory read");

                    auto ptr_exp = exp.list[1];
                    auto type_exp = exp.list[2];
                    auto* ptr_val = generate_expression(ptr_exp, env);
                    auto* target_type = get_type(type_exp.string, "mem_read");

                    auto* casted_ptr =
                        m_IR_BUILDER->CreateBitCast(ptr_val, target_type->getPointerTo(), "cast_ptr");
                    return m_IR_BUILDER->CreateLoad(target_type, casted_ptr, "load");
                }

                if (oper == "mem-ptr") {
                    LOG_DEBUG("Process get pointer");

                    auto var_name = exp.list[1].string;
                    auto* var_ptr = env->lookup_by_name(var_name);

                    return m_IR_BUILDER->CreateBitCast(
                        var_ptr, m_IR_BUILDER->getInt8Ty()->getPointerTo(), "to_void_ptr");
                }

                if (oper == "mem-deref") {
                    LOG_DEBUG("Process pointer dereference");

                    auto ptr_exp = exp.list[1];
                    auto type_exp = exp.list[2];
                    auto* ptr_val = generate_expression(ptr_exp, env);
                    auto* target_type = get_type(type_exp.string, "mem_deref");

                    auto* casted_ptr =
                        m_IR_BUILDER->CreateBitCast(ptr_val, target_type->getPointerTo(), "cast_ptr");
                    return m_IR_BUILDER->CreateLoad(target_type, casted_ptr, "deref");
                }

                if (oper == "index") {
                    LOG_DEBUG("Process array indexing");

                    if (exp.list.size() != 3) {
                        LOG_CRITICAL("index operation requires 2 arguments");
                    }

                    // First argument must be symbol (array name)
                    if (exp.list[1].type != ExpType::SYMBOL) {
                        LOG_CRITICAL("index: first argument must be array name");
                    }

                    const std::string& array_name = exp.list[1].string;
                    auto array_type_it = m_ARRAY_TYPES.find(array_name);
                    if (array_type_it == m_ARRAY_TYPES.end()) {
                        LOG_CRITICAL("Array '%s' not found", array_name.c_str());
                    }

                    llvm::ArrayType* array_type = array_type_it->second;
                    llvm::Value* array_ptr = env->lookup_by_name(array_name);
                    llvm::Value* index_val = generate_expression(exp.list[2], env);

                    // Validate index type
                    if (!index_val->getType()->isIntegerTy()) {
                        LOG_CRITICAL("Array index must be integer type");
                    }

                    // Create GEP (getelementptr) for array element
                    llvm::Value* zero = m_IR_BUILDER->getInt64(0);
                    std::vector<llvm::Value*> indices = {zero, index_val};
                    llvm::Value* element_ptr =
                        m_IR_BUILDER->CreateInBoundsGEP(array_type, array_ptr, indices, "elementptr");

                    return m_IR_BUILDER->CreateLoad(array_type->getElementType(), element_ptr, "loadarray");
                }

                if (oper == "if") {
                    LOG_DEBUG("Process if-elif-else: %s", exp.list[1].string.c_str());

                    if (exp.list.size() < 4) {
                        LOG_CRITICAL("if requires at least 4 arguments: condition, block, else, else_block",
                                     exp.string.c_str());
                    }

                    auto* merge_block = create_basic_block("if.end");
                    std::vector<llvm::Value*> branch_values;
                    std::vector<llvm::BasicBlock*> branch_blocks;

                    auto* current_block = m_IR_BUILDER->GetInsertBlock();
                    m_IR_BUILDER->SetInsertPoint(current_block);

                    size_t i = 1;
                    llvm::BasicBlock* next_block = nullptr;

                    while (i < exp.list.size()) {
                        if (exp.list[i].type == ExpType::SYMBOL
                            && (exp.list[i].string == "else" || exp.list[i].string == "elif"))
                        {
                            break;
                        }

                        if (i + 1 >= exp.list.size()) {
                            LOG_CRITICAL("if: missing block for condition", exp.string.c_str());
                        }

                        auto* cond = generate_expression(exp.list[i], env);
                        auto* then_block = create_basic_block("if.then", m_ACTIVE_FUNCTION);
                        next_block = create_basic_block("if.next", m_ACTIVE_FUNCTION);

                        m_IR_BUILDER->CreateCondBr(cond, then_block, next_block);

                        m_IR_BUILDER->SetInsertPoint(then_block);
                        auto* then_val = generate_expression(exp.list[i + 1], env);
                        branch_values.push_back(then_val);
                        branch_blocks.push_back(then_block);
                        m_IR_BUILDER->CreateBr(merge_block);

                        m_IR_BUILDER->SetInsertPoint(next_block);
                        current_block = next_block;
                        i += 2;
                    }

                    while (i < exp.list.size()) {
                        if (exp.list[i].type == ExpType::SYMBOL && exp.list[i].string == "elif") {
                            if (i + 2 >= exp.list.size()) {
                                LOG_CRITICAL("elif requires condition and block", exp.string.c_str());
                            }

                            auto* cond = generate_expression(exp.list[i + 1], env);
                            auto* elif_block = create_basic_block("elif.then", m_ACTIVE_FUNCTION);
                            next_block = create_basic_block("elif.next", m_ACTIVE_FUNCTION);

                            m_IR_BUILDER->CreateCondBr(cond, elif_block, next_block);

                            m_IR_BUILDER->SetInsertPoint(elif_block);
                            auto* elif_val = generate_expression(exp.list[i + 2], env);
                            branch_values.push_back(elif_val);
                            branch_blocks.push_back(elif_block);
                            m_IR_BUILDER->CreateBr(merge_block);

                            m_IR_BUILDER->SetInsertPoint(next_block);
                            current_block = next_block;
                            i += 3;
                        } else if (exp.list[i].type == ExpType::SYMBOL && exp.list[i].string == "else") {
                            if (i + 1 >= exp.list.size()) {
                                LOG_CRITICAL("else requires block", exp.string.c_str());
                            }

                            auto* else_block = m_IR_BUILDER->GetInsertBlock();
                            auto* else_val = generate_expression(exp.list[i + 1], env);
                            branch_values.push_back(else_val);
                            branch_blocks.push_back(else_block);
                            m_IR_BUILDER->CreateBr(merge_block);
                            i += 2;
                            break;
                        } else {
                            LOG_CRITICAL("expected elif or else after if conditions", exp.string.c_str());
                        }
                    }

                    m_ACTIVE_FUNCTION->insert(m_ACTIVE_FUNCTION->end(), merge_block);
                    m_IR_BUILDER->SetInsertPoint(merge_block);

                    if (!branch_values.empty()) {
                        auto* first_type = branch_values[0]->getType();
                        for (auto* val : branch_values) {
                            if (val->getType() != first_type) {
                                LOG_CRITICAL("if: all branches must return same type", exp.string.c_str());
                            }
                        }

                        auto* phi = m_IR_BUILDER->CreatePHI(first_type, branch_values.size(), "if_result");
                        for (size_t idx = 0; idx < branch_values.size(); idx++) {
                            phi->addIncoming(branch_values[idx], branch_blocks[idx]);
                        }
                        return phi;
                    }

                    return m_IR_BUILDER->getInt64(0);
                }

                // Loop
                if (oper == "loop") {
                    LOG_DEBUG("Process loop");
                    auto* loop_body = create_basic_block("loop.body", m_ACTIVE_FUNCTION);
                    auto* loop_exit = create_basic_block("loop.exit");

                    m_IR_BUILDER->CreateBr(loop_body);
                    m_IR_BUILDER->SetInsertPoint(loop_body);

                    LoopBlocks const LOOP_BLOCKS = {loop_exit, loop_body};
                    m_LOOP_STACK.push_back(LOOP_BLOCKS);

                    for (size_t i = 1; i < exp.list.size(); i++) {
                        generate_expression(exp.list[i], env);
                    }

                    if (m_IR_BUILDER->GetInsertBlock()->getTerminator() == nullptr) {
                        m_IR_BUILDER->CreateBr(loop_body);
                    }

                    m_ACTIVE_FUNCTION->insert(m_ACTIVE_FUNCTION->end(), loop_exit);
                    m_IR_BUILDER->SetInsertPoint(loop_exit);
                    m_LOOP_STACK.pop_back();

                    return m_IR_BUILDER->getInt64(0);
                }

                // Func
                if (oper == "func") {
                    LOG_DEBUG("Process function: %s", exp.list[1].string.c_str());

                    if (exp.list.size() < 4) {
                        LOG_CRITICAL("Function definition requires at least 3 parts (name, params, body)");
                        return m_IR_BUILDER->getInt64(0);
                    }

                    auto* fn = compile_function(exp, /* name */ exp.list[1].string, env);
                    env->define(exp.list[1].string, fn);
                    return fn;
                }

                // While
                if (oper == "while") {
                    LOG_DEBUG("Process while loop");

                    auto* break_blog = create_basic_block("break");
                    auto* continue_block = create_basic_block("continue");
                    m_LOOP_STACK.push_back({break_blog, continue_block});

                    auto* condition_block = create_basic_block("cond", m_ACTIVE_FUNCTION);
                    m_IR_BUILDER->CreateBr(condition_block);

                    auto* body_block = create_basic_block("body");

                    m_IR_BUILDER->SetInsertPoint(condition_block);
                    auto* condition = generate_expression(exp.list[1], env);
                    m_IR_BUILDER->CreateCondBr(condition, body_block, break_blog);

                    m_ACTIVE_FUNCTION->insert(m_ACTIVE_FUNCTION->end(), body_block);
                    m_IR_BUILDER->SetInsertPoint(body_block);
                    generate_expression(exp.list[2], env);
                    if (m_IR_BUILDER->GetInsertBlock()->getTerminator() == nullptr) {
                        m_IR_BUILDER->CreateBr(continue_block);
                    }

                    m_ACTIVE_FUNCTION->insert(m_ACTIVE_FUNCTION->end(), continue_block);
                    m_IR_BUILDER->SetInsertPoint(continue_block);
                    m_IR_BUILDER->CreateBr(condition_block);

                    m_ACTIVE_FUNCTION->insert(m_ACTIVE_FUNCTION->end(), break_blog);
                    m_IR_BUILDER->SetInsertPoint(break_blog);
                    m_LOOP_STACK.pop_back();

                    return m_IR_BUILDER->getInt64(0);
                }

                if (oper == "for") {
                    LOG_DEBUG("Process for loop");

                    auto init = exp.list[1];
                    auto condition = exp.list[2];
                    auto step = exp.list[3];
                    auto body = exp.list[4];

                    // `for` environment
                    auto for_env = std::make_shared<Environment>(std::map<std::string, llvm::Value*>(), env);

                    // Generate init expression
                    generate_expression(init, for_env);

                    // Create blocks
                    auto* cond_block = create_basic_block("for.cond", m_ACTIVE_FUNCTION);
                    auto* body_block = create_basic_block("for.body");
                    auto* step_block = create_basic_block("for.step");
                    auto* break_blog = create_basic_block("for.break");

                    // Conditions
                    m_IR_BUILDER->CreateBr(cond_block);

                    // Conditions block
                    m_IR_BUILDER->SetInsertPoint(cond_block);
                    auto* cond_value = generate_expression(condition, for_env);
                    m_IR_BUILDER->CreateCondBr(cond_value, body_block, break_blog);

                    // Body block
                    m_ACTIVE_FUNCTION->insert(m_ACTIVE_FUNCTION->end(), body_block);
                    m_IR_BUILDER->SetInsertPoint(body_block);
                    m_LOOP_STACK.push_back({break_blog, step_block});
                    generate_expression(body, for_env);
                    m_LOOP_STACK.pop_back();

                    // Step
                    if (m_IR_BUILDER->GetInsertBlock()->getTerminator() == nullptr) {
                        m_IR_BUILDER->CreateBr(step_block);
                    }

                    // Step block
                    m_ACTIVE_FUNCTION->insert(m_ACTIVE_FUNCTION->end(), step_block);
                    m_IR_BUILDER->SetInsertPoint(step_block);
                    generate_expression(step, for_env);
                    m_IR_BUILDER->CreateBr(cond_block);

                    // Break blog
                    m_ACTIVE_FUNCTION->insert(m_ACTIVE_FUNCTION->end(), break_blog);
                    m_IR_BUILDER->SetInsertPoint(break_blog);

                    return m_IR_BUILDER->getInt64(0);
                }

                if (oper == "break") {
                    LOG_DEBUG("Process break");

                    if (m_LOOP_STACK.empty()) {
                        LOG_CRITICAL("break outside of loop", exp.string.c_str());
                    }

                    auto& loop = m_LOOP_STACK.back();
                    m_IR_BUILDER->CreateBr(loop.break_blog);

                    auto* after = create_basic_block("after_break");
                    m_ACTIVE_FUNCTION->insert(m_ACTIVE_FUNCTION->end(), after);
                    m_IR_BUILDER->SetInsertPoint(after);

                    return m_IR_BUILDER->getInt64(0);
                }

                if (oper == "continue") {
                    LOG_DEBUG("Process continue");

                    if (m_LOOP_STACK.empty()) {
                        LOG_CRITICAL("continue outside of loop", exp.string.c_str());
                    }
                    auto& loop = m_LOOP_STACK.back();
                    m_IR_BUILDER->CreateBr(loop.continue_block);

                    auto* after = create_basic_block("after_continue");
                    m_ACTIVE_FUNCTION->insert(m_ACTIVE_FUNCTION->end(), after);
                    m_IR_BUILDER->SetInsertPoint(after);

                    return m_IR_BUILDER->getInt64(0);
                }

                // if-then-else. Short condition without else-if/elif blocks.
                if (oper == "check") {
                    LOG_DEBUG("Process check (if-then-else)");

                    auto* condition = generate_expression(exp.list[1], env);

                    auto* then_block = create_basic_block("then", m_ACTIVE_FUNCTION);
                    auto* else_block = create_basic_block("else");
                    auto* if_end_block = create_basic_block("ifend");

                    m_IR_BUILDER->CreateCondBr(condition, then_block, else_block);

                    // Then branch
                    m_IR_BUILDER->SetInsertPoint(then_block);
                    auto* then_res = generate_expression(exp.list[2], env);

                    if (m_IR_BUILDER->GetInsertBlock()->getTerminator() == nullptr) {
                        m_IR_BUILDER->CreateBr(if_end_block);
                    }
                    then_block = m_IR_BUILDER->GetInsertBlock();

                    // Else branch
                    m_ACTIVE_FUNCTION->insert(m_ACTIVE_FUNCTION->end(), else_block);
                    m_IR_BUILDER->SetInsertPoint(else_block);
                    auto* else_res = generate_expression(exp.list[3], env);
                    if (m_IR_BUILDER->GetInsertBlock()->getTerminator() == nullptr) {
                        m_IR_BUILDER->CreateBr(if_end_block);
                    }
                    else_block = m_IR_BUILDER->GetInsertBlock();

                    // If-end block
                    m_ACTIVE_FUNCTION->insert(m_ACTIVE_FUNCTION->end(), if_end_block);
                    m_IR_BUILDER->SetInsertPoint(if_end_block);

                    auto* phi = m_IR_BUILDER->CreatePHI(then_res->getType(), 2, "__tmpcheck__");
                    phi->addIncoming(then_res, then_block);
                    phi->addIncoming(else_res, else_block);
                    return phi;
                }

                if (oper == "set") {
                    if (exp.list[1].type == ExpType::LIST && !exp.list[1].list.empty()
                        && exp.list[1].list[0].string == "index")
                    {
                        const Exp& index_exp = exp.list[1];

                        if (index_exp.list.size() != 3) {
                            LOG_CRITICAL("index in set requires 2 arguments");
                        }

                        // First argument must be symbol (array name)
                        if (index_exp.list[1].type != ExpType::SYMBOL) {
                            LOG_CRITICAL("index: first argument must be array name");
                        }

                        const std::string& array_name = index_exp.list[1].string;
                        auto array_type_it = m_ARRAY_TYPES.find(array_name);
                        if (array_type_it == m_ARRAY_TYPES.end()) {
                            LOG_CRITICAL("Array '%s' not found", array_name.c_str());
                        }

                        llvm::ArrayType* array_type = array_type_it->second;
                        llvm::Value* array_ptr = env->lookup_by_name(array_name);
                        llvm::Value* index_val = generate_expression(index_exp.list[2], env);
                        llvm::Value* value = generate_expression(exp.list[2], env);

                        // Validate index type
                        if (!index_val->getType()->isIntegerTy()) {
                            LOG_CRITICAL("Array index must be integer type");
                        }

                        // Create GEP (getelementptr) for array element
                        llvm::Value* zero = m_IR_BUILDER->getInt64(0);
                        std::vector<llvm::Value*> indices = {zero, index_val};
                        llvm::Value* element_ptr =
                            m_IR_BUILDER->CreateInBoundsGEP(array_type, array_ptr, indices, "setptr");

                        // Cast value to element type if needed
                        value = implicit_cast(value, array_type->getElementType(), *m_IR_BUILDER);

                        m_IR_BUILDER->CreateStore(value, element_ptr);
                        return value;
                    }

                    auto var_name = exp.list[1].string;

                    LOG_DEBUG("Process set value to var: %s", var_name.c_str());

                    if (m_CONSTANTS.count(var_name) != 0U) {
                        LOG_CRITICAL("Var name \"%s\" is constant", var_name.c_str());
                        return m_IR_BUILDER->getInt64(0);
                    }

                    auto* value = generate_expression(exp.list[2], env);
                    auto* var_binding = env->lookup_by_name(var_name);

                    // Get actual variable type
                    llvm::Type* var_type = nullptr;
                    if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(var_binding)) {
                        var_type = alloca->getAllocatedType();
                    } else if (auto* global = llvm::dyn_cast<llvm::GlobalVariable>(var_binding)) {
                        var_type = global->getValueType();
                    }

                    // Validate type
                    if (value->getType() != var_type && type_to_string(value->getType()) != type_to_string(var_type)) {
                        // Allow implicit int->frac conversion
                        if (value->getType()->isIntegerTy() && var_type->isDoubleTy()) {
                            value = m_IR_BUILDER->CreateSIToFP(value, var_type, "castset");
                        } else {
                            LOG_CRITICAL("Type mismatch for '%s': cannot assign %s to %s",
                                        var_name.c_str(),
                                        type_to_string(value->getType()).c_str(),
                                        type_to_string(var_type).c_str());
                        }
                    }

                    m_IR_BUILDER->CreateStore(value, var_binding);

                    return value;
                }

                if (oper == "var" || oper == "const") {
                    auto var_name_declaration = exp.list[1];
                    auto var_name = extract_var_name(var_name_declaration);

                    if (m_CONSTANTS.count(var_name) != 0U || m_VARIABLES.count(var_name) != 0U) {
                        LOG_CRITICAL("Var \"%s\" is already defined", var_name.c_str());
                        return m_IR_BUILDER->getInt64(0);
                    }

                    LOG_DEBUG("Process create %s: %s", oper.c_str(), var_name.c_str());

                    auto* init = generate_expression(exp.list[2], env);
                    auto* var_type = extract_var_type(var_name_declaration);

                    if (llvm::isa<llvm::ArrayType>(var_type)) {
                        m_ARRAY_TYPES[var_name] = llvm::cast<llvm::ArrayType>(var_type);
                    }

                    // Validate type
                    if (init->getType() != var_type && type_to_string(var_type) == "!int"
                        || type_to_string(var_type) == "!frac")
                    {
                        // Allow implicit int->frac conversion
                        if (init->getType()->isIntegerTy() && var_type->isDoubleTy()) {
                            init = m_IR_BUILDER->CreateSIToFP(init, var_type, "castinit");
                        } else {
                            LOG_CRITICAL("Type mismatch for '%s': declared as %s but initialized with %s",
                                         var_name.c_str(),
                                         type_to_string(var_type).c_str(),
                                         type_to_string(init->getType()).c_str());
                        }
                    }

                    auto* var_binding = alloc_var(var_name, var_type, env);

                    if (oper == "const") {
                        m_CONSTANTS[var_name] = var_binding;
                    } else {
                        m_VARIABLES[var_name] = var_binding;
                    }

                    return m_IR_BUILDER->CreateStore(init, var_binding);
                }

                if (oper == "scope") {
                    LOG_DEBUG("Process scope");

                    llvm::Value* block_res = nullptr;

                    auto block_env =
                        std::make_shared<Environment>(std::map<std::string, llvm::Value*> {}, env);

                    for (auto i = 1; i < exp.list.size(); i++) {
                        block_res = generate_expression(exp.list[i], block_env);
                    }

                    return block_res;
                }

                if (oper == "fprint") {
                    LOG_DEBUG("Process fprint");
                    auto* printf_function = m_MODULE->getFunction("printf");
                    std::vector<llvm::Value*> args {};

                    for (auto i = 1; i < exp.list.size(); ++i) {
                        args.push_back(generate_expression(exp.list[i], env));
                    }

                    return m_IR_BUILDER->CreateCall(printf_function, args);
                }

                if (oper == "finput") {
                    LOG_DEBUG("Process finput");

                    auto* scanf_fn = m_MODULE->getFunction("scanf");
                    std::vector<llvm::Value*> args;

                    // Automatically convert %s to %[^\n] for string inputs
                    auto format_exp = exp.list[1];
                    std::string format_str = (format_exp.type == ExpType::STRING) ? format_exp.string : "";
                    bool has_string_input = false;

                    // Check for string arguments
                    for (size_t i = 2; i < exp.list.size(); ++i) {
                        std::string var_name = exp.list[i].string;
                        llvm::Value* var_ptr = env->lookup_by_name(var_name);
                        if (var_ptr->getType()->isPointerTy()) {
                            has_string_input = true;
                            break;
                        }
                    }

                    if (has_string_input && format_str.find("%s") != std::string::npos) {
                        // Replace all %s with %[^\n] to read full lines
                        size_t pos = 0;
                        while ((pos = format_str.find("%s", pos)) != std::string::npos) {
                            format_str.replace(pos, 2, "%[^\n]");
                            pos += 6; // Move past the replacement
                        }
                    }

                    // Create format string constant
                    auto* format_const = m_IR_BUILDER->CreateGlobalStringPtr(format_str);
                    args.push_back(format_const);

                    // Process variables and create buffers
                    for (size_t i = 2; i < exp.list.size(); ++i) {
                        std::string var_name = exp.list[i].string;
                        llvm::Value* var_ptr = env->lookup_by_name(var_name);

                        if (var_ptr->getType()->isPointerTy()) {
                            // Allocate 256-byte buffer on stack
                            auto* buffer_type = llvm::ArrayType::get(
                                m_IR_BUILDER->getInt8Ty(), 256);
                            auto* buffer = m_IR_BUILDER->CreateAlloca(
                                buffer_type, nullptr, "input_buffer");
                            auto* buffer_ptr = m_IR_BUILDER->CreateBitCast(
                                buffer, m_IR_BUILDER->getInt8Ty()->getPointerTo());

                            // Store buffer pointer in variable
                            m_IR_BUILDER->CreateStore(buffer_ptr, var_ptr);
                            args.push_back(buffer_ptr);
                        } else {
                            args.push_back(var_ptr);
                        }
                    }

                    auto* scanf_call = m_IR_BUILDER->CreateCall(scanf_fn, args);

                    // Add buffer cleaning after scanf
                    if (has_string_input) {
                        auto* getchar_fn = m_MODULE->getFunction("getchar");
                        if (!getchar_fn) {
                            getchar_fn = llvm::Function::Create(
                                llvm::FunctionType::get(m_IR_BUILDER->getInt64Ty(), false),
                                llvm::Function::ExternalLinkage,
                                "getchar",
                                m_MODULE.get()
                            );
                        }

                        // Create blocks for cleaning loop
                        auto* loop_block = create_basic_block("clean_loop", m_ACTIVE_FUNCTION);
                        auto* end_block = create_basic_block("clean_end", m_ACTIVE_FUNCTION);

                        m_IR_BUILDER->CreateBr(loop_block);
                        m_IR_BUILDER->SetInsertPoint(loop_block);

                        // Read characters until newline or EOF
                        auto* ch = m_IR_BUILDER->CreateCall(getchar_fn, {}, "ch");
                        auto* is_newline = m_IR_BUILDER->CreateICmpEQ(
                            ch, m_IR_BUILDER->getInt64('\n'), "is_newline");
                        auto* is_eof = m_IR_BUILDER->CreateICmpEQ(
                            ch, m_IR_BUILDER->getInt64(-1), "is_eof");
                        auto* should_break = m_IR_BUILDER->CreateOr(
                            is_newline, is_eof, "break_cond");

                        m_IR_BUILDER->CreateCondBr(should_break, end_block, loop_block);
                        m_IR_BUILDER->SetInsertPoint(end_block);
                    }

                    return scanf_call;
                }

                // Function calls

                LOG_DEBUG("Process function call: %s", exp.list[0].string.c_str());

                auto* callable = generate_expression(exp.list[0], env);

                std::vector<llvm::Value*> args {};

                for (auto i = 1; i < exp.list.size(); i++) {
                    args.push_back(generate_expression(exp.list[i], env));
                }

                auto* fn = (llvm::Function*)callable;

                return m_IR_BUILDER->CreateCall(fn, args);
            }

            return m_IR_BUILDER->getInt64(0);
            break;
    }

    return m_IR_BUILDER->getInt64(0);
}

void MorningLanguageLLVM::setup_extern_functions() {
    LOG_TRACE

    // i8* to substitute for char*, void*, etc
    auto* byte_ptr_ty = m_IR_BUILDER->getInt8Ty()->getPointerTo();

    // int printf(const char* format, ...);
    m_MODULE->getOrInsertFunction("printf",
                                  llvm::FunctionType::get(m_IR_BUILDER->getInt64Ty(), byte_ptr_ty, true));

    auto* scanf_type = llvm::FunctionType::get(m_IR_BUILDER->getInt64Ty(),
                                               {m_IR_BUILDER->getInt8Ty()->getPointerTo()},
                                               true    // variadic
    );
    m_MODULE->getOrInsertFunction("scanf", scanf_type);

    m_MODULE->getOrInsertFunction("getchar",
        llvm::FunctionType::get(m_IR_BUILDER->getInt64Ty(), false));
}

auto MorningLanguageLLVM::create_function(const std::string& name, llvm::FunctionType* type, const env& env)
    -> llvm::Function* {
    LOG_TRACE

    if (auto* existing = m_MODULE->getFunction(name)) {
        return existing;
    }

    auto* func = create_function_prototype(name, type, env);
    setup_function_body(func);
    return func;
}

auto MorningLanguageLLVM::create_function_prototype(const std::string& name,
                                                    llvm::FunctionType* type,
                                                    const env& env) -> llvm::Function* {
    LOG_TRACE

    auto* func = llvm::Function::Create(type,
                                        llvm::Function::ExternalLinkage,    // Why? So OS can find main()
                                        name,
                                        m_MODULE.get()    // Module ownership
    );
    verifyFunction(*func);    // Like spell-check for LLVM IR

    env->define(name, func);

    return func;
}

void MorningLanguageLLVM::setup_function_body(llvm::Function* func) {
    LOG_TRACE

    auto* entry_block = create_basic_block("entry", func);
    m_IR_BUILDER->SetInsertPoint(entry_block);    // "Start writing here"
}

auto MorningLanguageLLVM::create_basic_block(const std::string& label, llvm::Function* parent)
    -> llvm::BasicBlock* {
    LOG_TRACE

    return llvm::BasicBlock::Create(*m_CONTEXT, label, parent);
}

void MorningLanguageLLVM::save_module_to_file(const std::string& filename) {
    LOG_TRACE

    std::error_code err_code;
    llvm::raw_fd_ostream out_file(filename, err_code);
    m_MODULE->print(out_file, nullptr);    // Write module contents
}

void MorningLanguageLLVM::initialize_module() {
    LOG_TRACE

    m_CONTEXT = std::make_unique<llvm::LLVMContext>();
    m_MODULE = std::make_unique<llvm::Module>("MorningLangCompilationUnit",    // Module name
                                              *m_CONTEXT    // Context reference
    );
    m_IR_BUILDER = std::make_unique<llvm::IRBuilder<>>(*m_CONTEXT);

    m_VARS_BUILDER = std::make_unique<llvm::IRBuilder<>>(*m_CONTEXT);
}
