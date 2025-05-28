#pragma once

// LLVM Core Headers (Essential Components)
#include <llvm/IR/BasicBlock.h>	                // Code blocks without branches
#include <llvm/IR/Function.h>	                // Function representation
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>	                // IR validity checks
#include <llvm/Support/raw_ostream.h>	        // Output handling

#include "llvm/IR/IRBuilder.h"	                // Intermediate Representation (IR) construction
#include "llvm/IR/LLVMContext.h"	            // Compilation environment isolation
#include "llvm/IR/Module.h"	                    // Code container (like a source file)

// Standard libraries
#include <memory>	                            // Smart pointers
#include <string>	                            // String handling
#include <system_error>                         // Use system errors (providers error code)

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
    MorningLanguageLLVM() { initialize_module(); }

    /**
     * @brief Main compilation process
     * @param program Your source code (currently unused placeholder)
     *
     * Compilation steps:
     * 1. Generate IR structure (like building a skeleton)
     * 2. Print IR to console for debugging
     * 3. Save IR to file for later use
     */
    auto execute(const std::string& /*program*/) -> int {
        generate_ir();
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

    /**
     * @brief Builds the LLVM IR structure
     *
     * Current simple implementation:
     * 1. Create main() function shell
     * 2. Generate return value (hardcoded 42)
     * 3. Insert return instruction
     */
    void generate_ir() {
        // Create function type: i32 main()
        auto* main_type =
            llvm::FunctionType::get(m_IR_BUILDER->getInt32Ty(),	        // Return type = 32-bit integer
                                    /* Parameters */ {},	            // Empty list = no arguments
                                    /* Varargs */ false	            // No "..."
            );

        m_ACTIVE_FUNCTION = create_function("main", main_type);
        llvm::Value* result = generate_expression();
        m_IR_BUILDER->CreateRet(result);	// Insert return instruction
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
    auto generate_expression() -> llvm::Value* {
        // Current simplification: Always returns EXAMPLE_MAGIC_NUMBER
        const int EXAMPLE_MAGIC_NUMBER = 42;

        return m_IR_BUILDER->getInt32(EXAMPLE_MAGIC_NUMBER);	  // Create constant int32
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
    auto create_function(const std::string& name, llvm::FunctionType* type) -> llvm::Function* {
        if (auto* existing = m_MODULE->getFunction(name)) {
            return existing;
        }

        auto* func = create_function_prototype(name, type);
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
    auto create_function_prototype(const std::string& name, llvm::FunctionType* type) -> llvm::Function* {
        auto* func = llvm::Function::Create(type,
                                            llvm::Function::ExternalLinkage,	        // Why? So OS can find main()
                                            name,
                                            m_MODULE.get()	                                // Module ownership
        );
        verifyFunction(*func);	                                                            // Like spell-check for LLVM IR
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
        auto* entry_block = create_basic_block("entry", func);
        m_IR_BUILDER->SetInsertPoint(entry_block);                                      // "Start writing here"
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
        std::error_code err_code;
        llvm::raw_fd_ostream out_file(filename, err_code);
        m_MODULE->print(out_file, nullptr);	                                        // Write module contents
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
        m_CONTEXT = std::make_unique<llvm::LLVMContext>();
        m_MODULE = std::make_unique<llvm::Module>("MorningLangCompilationUnit",	    // Module name
                                                  *m_CONTEXT	                            // Context reference
        );
        m_IR_BUILDER = std::make_unique<llvm::IRBuilder<>>(*m_CONTEXT);
    }
};
