#pragma once

// LLVM Core Headers (Essential Components)
#include <llvm/IR/BasicBlock.h>                     // Code blocks without branches
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>                       // Function representation
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>                       // IR validity checks
#include <llvm/Support/Alignment.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>               // Output handling

#include "env.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"                      // Intermediate Representation (IR) construction
#include "llvm/IR/LLVMContext.h"                    // Compilation environment isolation
#include "llvm/IR/Module.h"                         // Code container (like a source file)
#include "parser/MorningLangGrammar.h"              // Grammatic Parser
#include "tracelogger.hpp"

// Standard libraries
#include <cstdint>
#include <map>
#include <memory>                                   // Smart pointers
#include <regex>                                    // Use regex expressions
#include <string>                                   // String handling
#include <system_error>                             // Use system errors (providers error code)
#include <utility>
#include <vector>                                   // Use vectors

/**
 * @brief Macros for generating binary operation
 *
 **/
#define GEN_BINARY_OP(Op, varName) \
    do { \
        auto oper1 = generate_expression(exp.list[1], env); \
        auto oper2 = generate_expression(exp.list[2], env); \
        return m_IR_BUILDER->Op(oper1, oper2, varName); \
    } while (false)

using env = std::shared_ptr<Environment>;

/**
 * @brief Replace regex in string
 *
 * @param str string for replacing text in string by regex templates
 * @return std::string
 **/
inline auto replace_regex_in_string(const std::string& str) -> std::string {
    LOG_TRACE

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
inline auto extract_var_name(const Exp& exp) -> std::string {
    LOG_TRACE

    return exp.type == ExpType::LIST ? exp.list[0].string : exp.string;
}

/**
 * @brief Check if function has a return type
 *
 * @param fn_exp function exp
 * @return true
 * @return false
 **/
inline auto has_return_type(const Exp& fn_exp) -> bool {
    LOG_TRACE

    return fn_exp.list[3].type == ExpType::SYMBOL && fn_exp.list[3].string == "->";
}

/**
 * @brief Check tag in list
 *
 * @param exp expression
 * @param tag tag
 * @return true
 * @return false
 **/
inline auto is_tagged_list(const Exp& exp, const std::string& tag) -> bool {
    LOG_TRACE

    return exp.type == ExpType::LIST && exp.list[0].type == ExpType::SYMBOL && exp.list[0].string == tag;
}

/**
 * @brief Check is a variable declaration
 *
 * @param exp expression
 * @return true
 * @return false
 **/
inline auto is_variable(const Exp& exp) -> bool {
    LOG_TRACE

    return is_tagged_list(exp, "var");
}

/**
 * @brief Check is a function declaration
 *
 * @param exp expression
 * @return true
 * @return false
 **/
inline auto is_function(const Exp& exp) -> bool {
    LOG_TRACE

    return is_tagged_list(exp, "function");
}

/**
 * @brief Check is a newobj declaration
 *
 * @param exp expression
 * @return true
 * @return false
 **/
inline auto is_newobj(const Exp& exp) -> bool {
    LOG_TRACE

    return is_tagged_list(exp, "newobj");
}

/**
 * @brief Class Info struct
 *
 * This struct have a class object, parent class object, fields and methods
 *
 **/
struct ClassInfo {
    llvm::StructType* cls {};
    llvm::StructType* parent {};
    std::map<std::string, llvm::Type*> fields_map;
    std::map<std::string, llvm::Function*> methods_map;
};

/**
 * @class MorningLanguageLLVM
 * @brief Converts MorningLang code to LLVM IR (Intermediate Representation)
 *
 * Think of this as a translator that takes your code and converts it
 * into LLVM's assembly-like format, which can then be compiled to
 * machine code for any CPU.
 */
class MorningLanguageLLVM {
  public:
    /**
     * @brief Sets up the LLVM "workspace"
     *
     * When creating a compiler, we need:
     * 1. A workspace (Context) to keep track of types and numbers
     * 2. A notebook (Module) to write our code in
     * 3. A pen (IRBuilder) to write instructions
     */
    MorningLanguageLLVM()
        : m_PARSER(std::make_unique<syntax::MorningLangGrammar>()) {
        LOG_TRACE

        initialize_module();
        setup_extern_functions();
        setup_global_environment();
    }

    /**
     * @brief Main compilation process
     * @param program Your source code (currently unused placeholder)
     *
     * Compilation steps:
     * 1. Generate IR structure (like building a skeleton)
     * 2. Print IR to console for debugging
     * 3. Save IR to file for later use
     */
    auto execute(const std::string& program) -> int {
        LOG_TRACE

        auto ast = m_PARSER->parse("[scope " + program + "]");

        generate_ir(ast);

        llvm::verifyModule(*m_MODULE, &llvm::errs());
        m_MODULE->print(llvm::outs(), nullptr);    // Like cout for LLVM

        save_module_to_file("./out.ll");

        return 0;
    }

  private:
    /// @activeFunction: The function we're currently building (like a work-in-progress)
    llvm::Function* m_ACTIVE_FUNCTION {};

    /**
     * @brief context: LLVM's isolated workspace
     *
     * Why needed? Prevents mixups if multiple compilations happen at once.
     * Contains:
     * - All type definitions (like int32)
     * - Constant values (like numbers)
     * - Special rules for the target CPU
     */
    std::unique_ptr<llvm::LLVMContext> m_CONTEXT;

    /**
     * @brief module: Container for all generated code
     *
     * Imagine this as a source file that holds:
     * - Functions (your code)
     * - Global variables (world-accessible data)
     * - Metadata (debugging info, optimization hints)
     */
    std::unique_ptr<llvm::Module> m_MODULE;

    /**
     * @brief irBuilder: Tool for writing LLVM instructions
     *
     * How it works:
     * 1. You tell it where to write (current position)
     * 2. You call methods like CreateAdd/CreateRet
     * 3. It writes instructions in SSA form (every value unique)
     */
    std::unique_ptr<llvm::IRBuilder<>> m_IR_BUILDER;

    /**
     * @brief Morning language parser
     *
     * This is Morning Language Syntax Parser
     **/
    std::unique_ptr<syntax::MorningLangGrammar> m_PARSER;

    /**
     * @brief Global Environment
     *
     * This is global program environment
     **/
    std::shared_ptr<Environment> m_GLOBAL_ENV;

    /**
     * @brief IR Builder for vars
     *
     * Tool for writing LLVM Vars instructions
     **/
    std::unique_ptr<llvm::IRBuilder<>> m_VARS_BUILDER;

    llvm::StructType* m_CURRENT_CLASS = nullptr;

    std::map<std::string, ClassInfo> m_CLASS_MAP;

    /**
     * @brief Set the up global environment
     **/
    void setup_global_environment() {
        LOG_TRACE

        std::map<std::string, llvm::Value*> const GLOBAL_OBJECT {
            {"_VERSION", m_IR_BUILDER->getInt64(101)},
        };

        std::map<std::string, llvm::Value*> global_rec {};

        for (const auto& entry : GLOBAL_OBJECT) {
            global_rec[entry.first] = create_global_variable(entry.first, (llvm::Constant*)entry.second);
        }

        m_GLOBAL_ENV = std::make_shared<Environment>(global_rec, nullptr);
    }

    /**
     * @brief Builds the LLVM IR structure
     *
     * Current simple implementation:
     * 1. Create main() function shell
     * 2. Generate return value (hardcoded 42)
     * 3. Insert return instruction
     */
    void generate_ir(const Exp& ast) {
        LOG_TRACE

        // Create function type: i32 main()
        auto* main_type =
            llvm::FunctionType::get(m_IR_BUILDER->getInt64Ty(),    // Return type = 32-bit integer
                                    /* Parameters */ {},    // Empty list = no arguments
                                    /* Varargs */ false    // No "..."
            );

        m_ACTIVE_FUNCTION = create_function("main", main_type, m_GLOBAL_ENV);

        create_global_variable("_MORNING_VERSION", m_IR_BUILDER->getInt64(1));

        generate_expression(ast, m_GLOBAL_ENV);

        m_IR_BUILDER->CreateRet(m_IR_BUILDER->getInt64(0));
    }

    /**
     * @brief Create a global variable object
     *
     * Global variables allocated outside of any function, stored in the binary file mutable, but can have
     * only constant initializers.
     *
     * @param name variable name
     * @param init_value initializer of variable
     * @return llvm::GlobalVariable*
     **/
    auto create_global_variable(const std::string& name, llvm::Constant* init_value, bool is_mutable = false)
        -> llvm::GlobalVariable* {
        LOG_TRACE

        m_MODULE->getOrInsertGlobal(name, init_value->getType());

        auto* variable = m_MODULE->getNamedGlobal(name);

        variable->setAlignment(llvm::MaybeAlign(4));
        variable->setConstant(!is_mutable);
        variable->setInitializer(init_value);

        return variable;
    }

    /**
     * @brief Get the type by name
     *
     * `!int/!int64` - 64bit integer
     * `!int32` - 32bit integer
     * `!int16` - 16bit integer
     * `!int8` - 8bit integer
     * `!str` - string
     * `!frac` - fractional (double) number
     * `!bool` - basic integer
     * `!none` - void, none
     * Otherwise: 64bit integer
     *
     * @param type_string
     * @return llvm::Type*
     **/
    auto get_type(const std::string& type_string) -> llvm::Type* {
        LOG_TRACE

        if (type_string == "!int" || type_string == "!int64") {
            return m_IR_BUILDER->getInt64Ty();
        }

        if (type_string == "!int32") {
            return m_IR_BUILDER->getInt32Ty();
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

        if (type_string == "!frac") {
            return m_IR_BUILDER->getDoubleTy();
        }

        if (type_string == "!bool") {
            return m_IR_BUILDER->getInt64Ty();
        }

        if (type_string == "!none") {
            return m_IR_BUILDER->getVoidTy();
        }

        return m_CLASS_MAP[type_string].cls->getPointerTo();
        // return m_IR_BUILDER->getInt64Ty();
    }

    /**
     * @brief Extract var type
     *
     * @param exp exp token
     * @return llvm::Type* var type
     **/
    auto extract_var_type(const Exp& exp) -> llvm::Type* {
        LOG_TRACE

        return exp.type == ExpType::LIST ? get_type(exp.list[1].string) : m_IR_BUILDER->getInt64Ty();
    }

    /**
     * @brief Extract function type
     *
     * @param fn_exp function exp
     * @return llvm::FunctionType*
     **/
    auto extract_function_type(const Exp& fn_exp) -> llvm::FunctionType* {
        LOG_TRACE

        auto params = fn_exp.list[2];

        auto* return_type =
            has_return_type(fn_exp) ? get_type(fn_exp.list[4].string) : m_IR_BUILDER->getInt64Ty();

        std::vector<llvm::Type*> param_types {};

        for (auto& param : params.list) {
            auto param_name = extract_var_name(param);
            auto* param_type = extract_var_type(param);
            param_types.push_back(param_name == "this" ? (llvm::Type*)m_CURRENT_CLASS->getPointerTo()
                                                       : param_type);
        }

        return llvm::FunctionType::get(return_type, param_types, /* varargs */ false);
    }

    /**
     * @brief Allocate a variable
     *
     * @param name variable name
     * @param var_type variable type
     * @param env variables environment
     * @return llvm::Value* allocated var
     **/
    auto alloc_var(const std::string& name, llvm::Type* var_type, const env& env) -> llvm::Value* {
        LOG_TRACE

        m_VARS_BUILDER->SetInsertPoint(&m_ACTIVE_FUNCTION->getEntryBlock());

        auto* allocated_var = m_VARS_BUILDER->CreateAlloca(var_type, nullptr, name);

        env->define(name, allocated_var);

        return allocated_var;
    }

    /**
     * @brief Build function environment and compile function block
     *
     * @param fn_exp function exp
     * @param fn_name function name
     * @param env parent environment
     * @return llvm::Value*
     **/
    auto compile_function(const Exp& fn_exp, std::string fn_name, const env& env) -> llvm::Value* {
        LOG_TRACE

        auto params = fn_exp.list[2];
        auto body = has_return_type(fn_exp) ? fn_exp.list[5] : fn_exp.list[3];

        auto* prev_fn = m_ACTIVE_FUNCTION;
        auto* prev_block = m_IR_BUILDER->GetInsertBlock();

        // auto orig_name = fn_name;

        if (m_CURRENT_CLASS != nullptr) {
            fn_name = std::string(m_CURRENT_CLASS->getName().data()) + "_" + fn_name;
        }

        auto* new_fn = create_function(fn_name, extract_function_type(fn_exp), env);
        m_ACTIVE_FUNCTION = new_fn;

        auto idx = 0;

        auto fn_env = std::make_shared<Environment>(std::map<std::string, llvm::Value*> {}, env);

        for (auto& arg : m_ACTIVE_FUNCTION->args()) {
            auto param = params.list[idx++];
            auto arg_name = extract_var_name(param);

            arg.setName(arg_name);

            auto* arg_binding = alloc_var(arg_name, arg.getType(), fn_env);
            m_IR_BUILDER->CreateStore(&arg, arg_binding);
        }

        m_IR_BUILDER->CreateRet(generate_expression(body, fn_env));

        m_IR_BUILDER->SetInsertPoint(prev_block);
        m_ACTIVE_FUNCTION = prev_fn;

        return new_fn;
    }

    /**
     * @brief Inherit class
     * @todo Make class inheritance
     * @deprecated Its not done
     *
     * @param llvm::StructType* child class struct
     * @param llvm::StructType* parent class struct
     *
     **/
    void inherit_class(llvm::StructType* /*cls*/, llvm::StructType* /*parent*/) {
        LOG_TRACE

        // TODO: MAKE inheritance
    }

    /**
     * @brief Get the class by name object
     *
     * @param name
     * @return llvm::StructType*
     **/
    auto get_class_by_name(const std::string& name) -> llvm::StructType* {
        LOG_TRACE

        return llvm::StructType::getTypeByName(*m_CONTEXT, name);
    }

    /**
     * @brief Build class info
     *
     * @param cls class struct
     * @param cls_exp class expression
     * @param env environment
     **/
    void build_class_info(llvm::StructType* cls, const Exp& cls_exp, const env& env) {
        LOG_TRACE

        auto class_name = cls_exp.list[1].string;
        auto* class_info = &m_CLASS_MAP[class_name];

        auto body = cls_exp.list[1];

        for (auto i = 1; i < body.list.size(); i++) {
            auto exp = body.list[i];

            if (is_variable(exp)) {
                auto var_name_decl = exp.list[1];

                auto field_name = extract_var_name(var_name_decl);
                auto* field_type = extract_var_type(var_name_decl);

                class_info->fields_map[field_name] = field_type;
            } else if (is_function(exp)) {
                auto method_name = exp.list[1].string;

                auto fn_name = class_name + "_" + method_name;

                class_info->methods_map[method_name] =
                    create_function_prototype(fn_name, extract_function_type(exp), env);
            }
        }

        build_class_body(cls);
    }

    /**
     * @brief Build class body
     *
     * @param cls class struct
     **/
    void build_class_body(llvm::StructType* cls) {
        LOG_TRACE

        std::string const CLASS_NAME(cls->getName().data());

        auto* class_info = &m_CLASS_MAP[CLASS_NAME];

        auto cls_fields = std::vector<llvm::Type*> {};

        for (const auto& field_info : class_info->fields_map) {
            cls_fields.push_back(field_info.second);
        }

        cls->setBody(cls_fields, false);

        // TODO: vTable
    }

    /**
     * @brief Generates value from expression (currently simple number)
     * @todo Rewrite this method (split) in future versions
     *
     * @param exp expression
     * @param env environment
     *
     * @return LLVM value representing computed result
     *
     * LLVM uses SSA (Static Single Assignment):
     * - Every value is created once
     * - Values are immutable (can't change)
     * - %1 = add i32 %0, 1 (typical SSA form)
     */
    auto generate_expression(const Exp& exp, const env& env) -> llvm::Value* {
        LOG_TRACE

        switch (exp.type) {
            case ExpType::NUMBER:
                return m_IR_BUILDER->getInt64(exp.number);
            case ExpType::FRACTIONAL:
                return llvm::ConstantFP::get(m_IR_BUILDER->getDoubleTy(), exp.fractional);
            case ExpType::STRING: {
                auto str = replace_regex_in_string(exp.string);

                return m_IR_BUILDER->CreateGlobalStringPtr(str);
            }
            case ExpType::SYMBOL:
                if (exp.string == "true" || exp.string == "false") {
                    return m_IR_BUILDER->getInt64(static_cast<uint64_t>(exp.string == "true"));
                } else {
                    auto var_name = exp.string;
                    auto* value = env->lookup_by_name(var_name);

                    if (auto* local_var = llvm::dyn_cast<llvm::AllocaInst>(value)) {
                        return m_IR_BUILDER->CreateLoad(
                            local_var->getAllocatedType(), local_var, var_name.c_str());
                    }

                    if (auto* global_var = llvm::dyn_cast<llvm::GlobalVariable>(value)) {
                        return m_IR_BUILDER->CreateLoad(
                            global_var->getInitializer()->getType(), global_var, var_name.c_str());
                    }

                    return value;
                }
                return m_MODULE->getNamedGlobal(exp.string)->getInitializer();
            case ExpType::LIST:
                auto tag = exp.list[0];

                if (tag.type == ExpType::SYMBOL) {
                    auto oper = tag.string;

                    if (oper == "+" || oper == "__PLUS_OPERAND__") {
                        GEN_BINARY_OP(CreateAdd, "__tmpadd__");
                    } else if (oper == "-" || oper == "__SUB_OPERAND__") {
                        GEN_BINARY_OP(CreateSub, "__tmpsub__");
                    } else if (oper == "*" || oper == "__MUL_OPERAND__") {
                        GEN_BINARY_OP(CreateMul, "__tmpmul__");
                    } else if (oper == "/" || oper == "__DIV_OPERAND__") {
                        GEN_BINARY_OP(CreateSDiv, "__tmpdiv__");
                    } else if (oper == ">" || oper == "__CMPG__") {
                        GEN_BINARY_OP(CreateICmpUGT, "__tmpcmp__");
                    } else if (oper == "==" || oper == "__CMPEQ__") {
                        GEN_BINARY_OP(CreateICmpEQ, "__tmpcmp__");
                    } else if (oper == "<" || oper == "__CMPL__") {
                        GEN_BINARY_OP(CreateICmpULT, "__tmpcmp__");
                    } else if (oper == "!=" || oper == "__CMPNE__") {
                        GEN_BINARY_OP(CreateICmpNE, "__tmpcmp__");
                    } else if (oper == ">=" || oper == "__CMPGE__") {
                        GEN_BINARY_OP(CreateICmpUGE, "__tmpcmp__");
                    } else if (oper == "<=" || oper == "__CMPLE__") {
                        GEN_BINARY_OP(CreateICmpULE, "__tmpcmp__");
                    }

                    // Class definition
                    if (oper == "class") {
                        auto name = exp.list[1].string;
                        auto* parent =
                            exp.list[2].string == "self" ? nullptr : get_class_by_name(exp.list[2].string);

                        m_CURRENT_CLASS = llvm::StructType::create(*m_CONTEXT, name);

                        if (parent != nullptr) {
                            inherit_class(m_CURRENT_CLASS, parent);
                        } else {
                            m_CLASS_MAP[name] = {m_CURRENT_CLASS, parent, {}, {}};
                        }

                        build_class_info(m_CURRENT_CLASS, exp, env);

                        generate_expression(exp.list[3], env);

                        m_CURRENT_CLASS = nullptr;

                        return m_IR_BUILDER->getInt64(0);
                    }

                    // Function definition
                    if (oper == "func") {
                        return compile_function(exp, /* name */ exp.list[1].string, env);
                    }

                    // Typical while
                    if (oper == "while") {
                        auto* condition_block = create_basic_block("cond", m_ACTIVE_FUNCTION);
                        m_IR_BUILDER->CreateBr(condition_block);

                        auto* body_block = create_basic_block("body");
                        auto* loop_end_block = create_basic_block("loopend");

                        m_IR_BUILDER->SetInsertPoint(condition_block);
                        auto* condition = generate_expression(exp.list[1], env);

                        m_IR_BUILDER->CreateCondBr(condition, body_block, loop_end_block);

                        m_ACTIVE_FUNCTION->insert(m_ACTIVE_FUNCTION->end(), body_block);
                        m_IR_BUILDER->SetInsertPoint(body_block);
                        generate_expression(exp.list[2], env);
                        m_IR_BUILDER->CreateBr(condition_block);

                        m_ACTIVE_FUNCTION->insert(m_ACTIVE_FUNCTION->end(), loop_end_block);
                        m_IR_BUILDER->SetInsertPoint(loop_end_block);

                        return m_IR_BUILDER->getInt64(0);
                    }

                    // if-then-else. Short condition without else-if blocks.
                    if (oper == "check") {
                        auto* condition = generate_expression(exp.list[1], env);

                        auto* then_block = create_basic_block("then", m_ACTIVE_FUNCTION);

                        auto* else_block = create_basic_block("else");
                        auto* if_end_block = create_basic_block("ifend");

                        // Condition branch
                        m_IR_BUILDER->CreateCondBr(condition, then_block, else_block);

                        // Then branch
                        m_IR_BUILDER->SetInsertPoint(then_block);
                        auto* then_res = generate_expression(exp.list[2], env);
                        m_IR_BUILDER->CreateBr(if_end_block);

                        then_block = m_IR_BUILDER->GetInsertBlock();

                        // Else branch
                        m_ACTIVE_FUNCTION->insert(m_ACTIVE_FUNCTION->end(), else_block);
                        m_IR_BUILDER->SetInsertPoint(else_block);
                        auto* else_res = generate_expression(exp.list[3], env);
                        m_IR_BUILDER->CreateBr(if_end_block);
                        else_block = m_IR_BUILDER->GetInsertBlock();

                        // If-end block
                        m_ACTIVE_FUNCTION->insert(m_ACTIVE_FUNCTION->end(), if_end_block);
                        m_IR_BUILDER->SetInsertPoint(if_end_block);

                        // Result of the check expressions is phi
                        auto* phi = m_IR_BUILDER->CreatePHI(then_res->getType(), 2, "__tmpcheck__");

                        phi->addIncoming(then_res, then_block);
                        phi->addIncoming(else_res, else_block);

                        return phi;
                    }

                    // Set variable value
                    if (oper == "set") {
                        auto* value = generate_expression(exp.list[2], env);
                        auto var_name = exp.list[1].string;

                        auto* var_binding = env->lookup_by_name(var_name);

                        m_IR_BUILDER->CreateStore(value, var_binding);

                        return value;
                    }

                    // Define variable
                    if (oper == "var") {
                        if (m_CURRENT_CLASS != nullptr) {
                            return m_IR_BUILDER->getInt64(0);
                        }

                        auto var_name_declaration = exp.list[1];
                        auto var_name = extract_var_name(var_name_declaration);

                        auto* init = generate_expression(exp.list[2], env);

                        auto* var_type = extract_var_type(var_name_declaration);

                        auto* var_binding = alloc_var(var_name, var_type, env);

                        return m_IR_BUILDER->CreateStore(init, var_binding);
                    }

                    // Define scope block
                    if (oper == "scope") {
                        llvm::Value* block_res = nullptr;
                        auto block_env =
                            std::make_shared<Environment>(std::map<std::string, llvm::Value*> {}, env);
                        ;

                        for (auto i = 1; i < exp.list.size(); i++) {
                            block_res = generate_expression(exp.list[i], block_env);
                        }

                        return block_res;
                    }

                    // Define
                    if (oper == "fprint") {
                        auto* printf_function = m_MODULE->getFunction("printf");
                        std::vector<llvm::Value*> args {};

                        for (auto i = 1; i < exp.list.size(); ++i) {
                            args.push_back(generate_expression(exp.list[i], env));
                        }

                        return m_IR_BUILDER->CreateCall(printf_function, args);
                    }

                    // Function calls
                    auto* callable = generate_expression(exp.list[0], env);

                    std::vector<llvm::Value*> args {};

                    for (auto i = 1; i < exp.list.size(); i++) {
                        args.push_back(generate_expression(exp.list[i], env));
                    }

                    auto* function = (llvm::Function*)callable;

                    return m_IR_BUILDER->CreateCall(function, args);
                }

                return m_IR_BUILDER->getInt64(0);
                break;
        }

        return m_IR_BUILDER->getInt64(0);
    }

    /**
     * @brief Set the up extern functions
     *
     * Define external functions (from libc++)
     **/
    void setup_extern_functions() {
        LOG_TRACE

        // i8* to substitute for char*, void*, etc
        auto* byte_ptr_ty = m_IR_BUILDER->getInt8Ty()->getPointerTo();

        // int printf(const char* format, ...);
        m_MODULE->getOrInsertFunction("printf",
                                      llvm::FunctionType::get(m_IR_BUILDER->getInt64Ty(), byte_ptr_ty, true));
    }

    /**
     * @brief Creates function with proper setup
     * @param name Function name ("main")
     * @param type Function signature (return + params)
     *
     * Three-step process:
     * 1. Check if function exists
     * 2. Create prototype if needed
     * 3. Prepare first code block
     */
    auto create_function(const std::string& name, llvm::FunctionType* type, const env& env)
        -> llvm::Function* {
        LOG_TRACE

        if (auto* existing = m_MODULE->getFunction(name)) {
            return existing;
        }

        auto* func = create_function_prototype(name, type, env);
        setup_function_body(func);
        return func;
    }

    /**
     * @brief Creates function declaration (prototype)
     * @param name Visible function name
     * @param type Function signature
     *
     * Important choices:
     * - ExternalLinkage: Makes function visible to linker
     * - Verification: Checks for common errors
     */
    auto create_function_prototype(const std::string& name, llvm::FunctionType* type, const env& env)
        -> llvm::Function* {
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

    /**
     * @brief Prepares function body structure
     * @param func Function to receive body
     *
     * Every LLVM function needs:
     * 1. At least one basic block (container for code)
     * 2. A starting point (entry block)
     * 3. Terminator instruction (like return)
     */
    void setup_function_body(llvm::Function* func) {
        LOG_TRACE

        auto* entry_block = create_basic_block("entry", func);
        m_IR_BUILDER->SetInsertPoint(entry_block);    // "Start writing here"
    }

    /**
     * @brief Creates labeled code block
     * @param label Block name (for debugging)
     * @param parent Parent function (optional)
     *
     * Basic Blocks (BBs):
     * - Linear code sequences
     * - Must end with terminator (ret, jump)
     * - Labels help debuggers understand code
     */
    auto create_basic_block(const std::string& label, llvm::Function* parent = nullptr) -> llvm::BasicBlock* {
        LOG_TRACE

        return llvm::BasicBlock::Create(*m_CONTEXT, label, parent);
    }

    /**
     * @brief Saves generated IR to text file
     * @param filename Output path (.ll extension)
     *
     * LLVM assembly format:
     * - Human-readable
     * - Can be modified and recompiled
     * - Contains target information (CPU features)
     */
    void save_module_to_file(const std::string& filename) {
        LOG_TRACE

        std::error_code err_code;
        llvm::raw_fd_ostream out_file(filename, err_code);
        m_MODULE->print(out_file, nullptr);    // Write module contents
    }

    /**
     * @brief Initializes LLVM components
     *
     * Order matters:
     * 1. Context first (needed by everyone)
     * 2. Module (needs context)
     * 3. Builder (needs context)
     */
    void initialize_module() {
        LOG_TRACE

        m_CONTEXT = std::make_unique<llvm::LLVMContext>();
        m_MODULE = std::make_unique<llvm::Module>("MorningLangCompilationUnit",    // Module name
                                                  *m_CONTEXT    // Context reference
        );
        m_IR_BUILDER = std::make_unique<llvm::IRBuilder<>>(*m_CONTEXT);

        m_VARS_BUILDER = std::make_unique<llvm::IRBuilder<>>(*m_CONTEXT);
    }
};
