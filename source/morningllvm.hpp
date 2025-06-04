#pragma once

// LLVM Core Headers (Essential Components)
#include <llvm/IR/BasicBlock.h>	                // Code blocks without branches
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>	                // Function representation
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>	                // IR validity checks
#include <llvm/Support/Alignment.h>
#include <llvm/Support/raw_ostream.h>	        // Output handling
#include "llvm/IR/Constants.h"

#include "llvm/IR/IRBuilder.h"	                // Intermediate Representation (IR) construction
#include "llvm/IR/LLVMContext.h"	            // Compilation environment isolation
#include "llvm/IR/Module.h"	                    // Code container (like a source file)

#include "parser/MorningLangGrammar.h"          // Grammatic Parser
#include "env.h"
#include "tracelogger.hpp"

// Standard libraries
#include <memory>	                            // Smart pointers
#include <string>	                            // String handling
#include <system_error>                         // Use system errors (providers error code)
#include <vector>                               // Use vectors
#include <regex>                                // Use regex expressions

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
    MorningLanguageLLVM() : m_PARSER(std::make_unique<syntax::MorningLangGrammar>()) {
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
        m_MODULE->print(llvm::outs(), nullptr);	   // Like cout for LLVM

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

    std::unique_ptr<syntax::MorningLangGrammar> m_PARSER;

    std::shared_ptr<Environment> m_GLOBAL_ENV;

    void setup_global_environment() {
        LOG_TRACE

        std::map<std::string, llvm::Value*> global_object{
            {"_VERSION", m_IR_BUILDER->getInt64(101)},
        };

        std::map<std::string, llvm::Value*> global_rec{};

        for (auto& entry : global_object) {
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
            llvm::FunctionType::get(m_IR_BUILDER->getInt64Ty(),	        // Return type = 32-bit integer
                                    /* Parameters */ {},	            // Empty list = no arguments
                                    /* Varargs */ false	            // No "..."
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
    auto create_global_variable(const std::string& name, llvm::Constant* init_value, bool is_mutable = false) -> llvm::GlobalVariable* {
        LOG_TRACE

        m_MODULE->getOrInsertGlobal(name, init_value->getType());

        auto *variable = m_MODULE->getNamedGlobal(name);

        variable->setAlignment(llvm::MaybeAlign(4));
        variable->setConstant(!is_mutable);
        variable->setInitializer(init_value);

        return variable;
    }

    /**
     * @brief Generates value from expression (currently simple number)
     * @return LLVM value representing computed result
     *
     * LLVM uses SSA (Static Single Assignment):
     * - Every value is created once
     * - Values are immutable (can't change)
     * - %1 = add i32 %0, 1 (typical SSA form)
     */
    auto generate_expression(const Exp& exp, env env) -> llvm::Value* {
        LOG_TRACE

        switch (exp.type) {
            case ExpType::NUMBER: return m_IR_BUILDER->getInt64(exp.number);
            case ExpType::FRACTIONAL: return llvm::ConstantFP::get(m_IR_BUILDER->getDoubleTy(), exp.fractional);
            case ExpType::STRING: {
                auto str = replace_regex_in_string(exp.string);

                return m_IR_BUILDER->CreateGlobalStringPtr(str);
            } case ExpType::SYMBOL:
                if (exp.string == "true" || exp.string == "false") {
                    return m_IR_BUILDER->getInt64(static_cast<uint64_t>(exp.string == "true"));
                } else {
                    auto var_name = exp.string;
                    auto value = env->lookup_by_name(var_name);

                    // TODO: local variables

                    if (auto global_var = llvm::dyn_cast<llvm::GlobalVariable>(value)) {
                        return m_IR_BUILDER->CreateLoad(global_var->getInitializer()->getType(), global_var, var_name.c_str());
                    }

                    return m_MODULE->getNamedGlobal(exp.string)->getInitializer();
                }
            case ExpType::LIST:
                auto tag = exp.list[0];

                if (tag.type == ExpType::SYMBOL) {
                    auto oper = tag.string;

                    if (oper == "var") {
                        auto var_name = exp.list[1].string;
                        auto *init = generate_expression(exp.list[2], env);

                        return create_global_variable(var_name, static_cast<llvm::Constant*>(init))->getInitializer();
                    }

                    if (oper == "mutvar") {
                        auto var_name = exp.list[1].string;
                        auto *init = generate_expression(exp.list[2], env);

                        return create_global_variable(var_name, static_cast<llvm::Constant*>(init), /*is_mutable=*/true)->getInitializer();
                    }

                    if (oper == "scope") {
                        llvm::Value* block_res;

                        for (auto i = 1; i < exp.list.size(); i++) {
                            block_res = generate_expression(exp.list[i], env);
                        }

                        return block_res;
                    }

                    if (oper == "fprint") {
                        auto *printf_function = m_MODULE->getFunction("printf");
                        std::vector<llvm::Value*> args{};

                        for (auto i = 1; i < exp.list.size(); ++i) {
                            args.push_back(generate_expression(exp.list[i], env));
                        }

                        return m_IR_BUILDER->CreateCall(printf_function, args);
                    }
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
        auto *byte_ptr_ty = m_IR_BUILDER->getInt8Ty()->getPointerTo();

        // int printf(const char* format, ...);
        m_MODULE->getOrInsertFunction("printf",
            llvm::FunctionType::get(
                m_IR_BUILDER->getInt64Ty(),
                byte_ptr_ty,
                true
            )
        );
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
    auto create_function(const std::string& name, llvm::FunctionType* type, env env) -> llvm::Function* {
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
    auto create_function_prototype(const std::string& name, llvm::FunctionType* type, env env) -> llvm::Function* {
        LOG_TRACE

        auto* func = llvm::Function::Create(type,
                                            llvm::Function::ExternalLinkage,	    // Why? So OS can find main()
                                            name,
                                            m_MODULE.get()	                            // Module ownership
        );
        verifyFunction(*func);	                                                        // Like spell-check for LLVM IR

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
        m_IR_BUILDER->SetInsertPoint(entry_block);                                  // "Start writing here"
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
        m_MODULE->print(out_file, nullptr);	   // Write module contents
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
        m_MODULE = std::make_unique<llvm::Module>("MorningLangCompilationUnit",	    // Module name
                                                  *m_CONTEXT	                            // Context reference
        );
        m_IR_BUILDER = std::make_unique<llvm::IRBuilder<>>(*m_CONTEXT);
    }
};
