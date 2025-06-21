#include "morningllvm.hpp"
#include "logger.hpp"

namespace {
    /**
    * @brief Replace regex in string
    *
    * @param str string for replacing text in string by regex templates
    * @return std::string
    **/
    static auto replace_regex_in_string(const std::string& str) -> std::string {
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
    static auto extract_var_name(const Exp& exp) -> std::string {
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
            case ExpType::STRING:
                return "\"" + exp.string + "\"";
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

        if (exp.type != ExpType::NUMBER && exp.type != ExpType::SYMBOL) {
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
}

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
    auto* main_type =
        llvm::FunctionType::get(m_IR_BUILDER->getInt64Ty(),    // Return type = 32-bit integer
                                /* Parameters */ {},    // Empty list = no arguments
                                /* Varargs */ false    // No "..."
    );

    m_ACTIVE_FUNCTION = create_function("main", main_type, m_GLOBAL_ENV);

    generate_expression(ast, m_GLOBAL_ENV);

    m_IR_BUILDER->CreateRet(m_IR_BUILDER->getInt64(0));
}

auto MorningLanguageLLVM::create_global_variable(const std::string& name, llvm::Constant* init_value, bool is_mutable)
    -> llvm::GlobalVariable* {
    LOG_TRACE

    m_MODULE->getOrInsertGlobal(name, init_value->getType());

    auto* variable = m_MODULE->getNamedGlobal(name);

    variable->setAlignment(llvm::MaybeAlign(4));
    variable->setConstant(!is_mutable);
    variable->setInitializer(init_value);

    return variable;
}

auto MorningLanguageLLVM::get_type(const std::string& type_string) -> llvm::Type* {
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

    return m_IR_BUILDER->getInt64Ty();
}

auto MorningLanguageLLVM::extract_var_type(const Exp& exp) -> llvm::Type* {
    return exp.type == ExpType::LIST ? get_type(exp.list[1].string) : m_IR_BUILDER->getInt64Ty();
}

auto MorningLanguageLLVM::extract_function_type(const Exp& fn_exp) -> llvm::FunctionType* {
    auto params = fn_exp.list[2];

    auto* return_type =
        has_return_type(fn_exp) ? get_type(fn_exp.list[4].string) : m_IR_BUILDER->getInt64Ty();

    std::vector<llvm::Type*> param_types {};

    for (auto& param : params.list) {
        auto* param_type = extract_var_type(param);
        param_types.push_back(param_type);
    }

    return llvm::FunctionType::get(return_type, param_types, /* varargs */ false);
}

auto MorningLanguageLLVM::alloc_var(const std::string& name, llvm::Type* var_type, const env& env) -> llvm::Value* {
    LOG_TRACE

    m_VARS_BUILDER->SetInsertPoint(&m_ACTIVE_FUNCTION->getEntryBlock());

    auto* allocated_var = m_VARS_BUILDER->CreateAlloca(var_type, nullptr, name);

    env->define(name, allocated_var);

    return allocated_var;
}

auto MorningLanguageLLVM::compile_function(const Exp& fn_exp, const std::string& fn_name, const env& env) -> llvm::Value* {
    auto params = fn_exp.list[2];
    auto body = has_return_type(fn_exp) ? fn_exp.list[5] : fn_exp.list[3];

    auto* prev_fn = m_ACTIVE_FUNCTION;
    auto* prev_block = m_IR_BUILDER->GetInsertBlock();

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

auto MorningLanguageLLVM::generate_expression(const Exp& exp, const env& env) -> llvm::Value* {
    LOG_TRACE

    add_expression_to_traceback_stack(exp);

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

                if (oper == "if") {
                    LOG_DEBUG("Process if-elif-else: %s",exp.list[1].string.c_str());

                    if (exp.list.size() < 4) {
                        LOG_CRITICAL(
                            "if requires at least 4 arguments: condition, block, else, else_block",
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
                            LOG_CRITICAL("expected elif or else after if conditions",
                                         exp.string.c_str());
                        }
                    }

                    m_ACTIVE_FUNCTION->insert(m_ACTIVE_FUNCTION->end(), merge_block);
                    m_IR_BUILDER->SetInsertPoint(merge_block);

                    if (!branch_values.empty()) {
                        auto* first_type = branch_values[0]->getType();
                        for (auto* val : branch_values) {
                            if (val->getType() != first_type) {
                                LOG_CRITICAL("if: all branches must return same type",
                                             exp.string.c_str());
                            }
                        }

                        auto* phi =
                            m_IR_BUILDER->CreatePHI(first_type, branch_values.size(), "if_result");
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

                    LoopBlocks const loop_blocks = {loop_exit, loop_body};
                    m_LOOP_STACK.push_back(loop_blocks);

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
                    return compile_function(exp, /* name */ exp.list[1].string, env);
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
                    auto for_env =
                        std::make_shared<Environment>(std::map<std::string, llvm::Value*>(), env);

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

                    if ((then_block->getTerminator() == nullptr)
                        && (else_block->getTerminator() == nullptr))
                    {
                        auto* phi = m_IR_BUILDER->CreatePHI(then_res->getType(), 2, "__tmpcheck__");
                        phi->addIncoming(then_res, then_block);
                        phi->addIncoming(else_res, else_block);
                        return phi;
                    }

                    return m_IR_BUILDER->getInt64(0);
                }

                if (oper == "set") {
                    LOG_DEBUG("Process set value to var: %s", exp.list[1].string.c_str());

                    auto var_name = exp.list[1].string;

                    if (m_CONSTANTS.count(var_name) != 0U) {
                        LOG_CRITICAL("Var name \"%s\" is constant", var_name.c_str());
                        return m_IR_BUILDER->getInt64(0);
                    }

                    auto* value = generate_expression(exp.list[2], env);

                    auto* var_binding = env->lookup_by_name(var_name);

                    m_IR_BUILDER->CreateStore(value, var_binding);

                    return value;
                }

                if (oper == "var" || oper == "const") {
                    auto var_name_declaration = exp.list[1];
                    auto var_name = extract_var_name(var_name_declaration);

                    LOG_DEBUG("Process create %s: %s", oper.c_str(), exp.list[1].string.c_str());

                    auto* init = generate_expression(exp.list[2], env);

                    auto* var_type = extract_var_type(var_name_declaration);

                    auto* var_binding = alloc_var(var_name, var_type, env);

                    if (oper == "const") {
                        m_CONSTANTS[var_name] = var_binding;
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

                    // Format string
                    args.push_back(generate_expression(exp.list[1], env));

                    // Variable references
                    for (size_t i = 2; i < exp.list.size(); ++i) {
                        std::string const VAR_NAME = exp.list[i].string;
                        llvm::Value* var_ptr = env->lookup_by_name(VAR_NAME);
                        args.push_back(var_ptr);
                    }

                    return m_IR_BUILDER->CreateCall(scanf_fn, args);
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

    auto* scanf_type = llvm::FunctionType::get(m_IR_BUILDER->getInt32Ty(),
                                               {m_IR_BUILDER->getInt8Ty()->getPointerTo()},
                                               true    // variadic
    );
    m_MODULE->getOrInsertFunction("scanf", scanf_type);
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

auto MorningLanguageLLVM::create_function_prototype(const std::string& name, llvm::FunctionType* type, const env& env)
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

void MorningLanguageLLVM::setup_function_body(llvm::Function* func) {
    LOG_TRACE

    auto* entry_block = create_basic_block("entry", func);
    m_IR_BUILDER->SetInsertPoint(entry_block);    // "Start writing here"
}

auto MorningLanguageLLVM::create_basic_block(const std::string& label, llvm::Function* parent) -> llvm::BasicBlock* {
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
