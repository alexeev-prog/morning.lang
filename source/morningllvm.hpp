#pragma once

// LLVM Core Headers (Essential Components)
#include "llvm/IR/IRBuilder.h"    // Intermediate Representation (IR) construction
#include "llvm/IR/LLVMContext.h"  // Compilation environment isolation
#include "llvm/IR/Module.h"       // Code container (like a source file)
#include <llvm/IR/BasicBlock.h>   // Code blocks without branches
#include <llvm/IR/Function.h>     // Function representation
#include <llvm/Support/raw_ostream.h> // Output handling
#include <llvm/IR/Verifier.h>     // IR validity checks
#include <memory>                 // Smart pointers
#include <string>                 // String handling

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
    MorningLanguageLLVM() { initializeModule(); }

    /**
     * @brief Main compilation process
     * @param program Your source code (currently unused placeholder)
     * 
     * Compilation steps:
     * 1. Generate IR structure (like building a skeleton)
     * 2. Print IR to console for debugging
     * 3. Save IR to file for later use
     */
    void execute(const std::string& program) {
        generateIR();
        module->print(llvm::outs(), nullptr); // Like cout for LLVM
        saveModuleToFile("./out.ll");
    }

private:
    /// @activeFunction: The function we're currently building (like a work-in-progress)
    llvm::Function* activeFunction;

    /**
     * @brief context: LLVM's isolated workspace
     * 
     * Why needed? Prevents mixups if multiple compilations happen at once.
     * Contains:
     * - All type definitions (like int32)
     * - Constant values (like numbers)
     * - Special rules for the target CPU
     */
    std::unique_ptr<llvm::LLVMContext> context;

    /**
     * @brief module: Container for all generated code
     * 
     * Imagine this as a source file that holds:
     * - Functions (your code)
     * - Global variables (world-accessible data)
     * - Metadata (debugging info, optimization hints)
     */
    std::unique_ptr<llvm::Module> module;

    /**
     * @brief irBuilder: Tool for writing LLVM instructions
     * 
     * How it works:
     * 1. You tell it where to write (current position)
     * 2. You call methods like CreateAdd/CreateRet
     * 3. It writes instructions in SSA form (every value unique)
     */
    std::unique_ptr<llvm::IRBuilder<>> irBuilder;

    /**
     * @brief Builds the LLVM IR structure
     * 
     * Current simple implementation:
     * 1. Create main() function shell
     * 2. Generate return value (hardcoded 42)
     * 3. Insert return instruction
     */
    void generateIR() {
        // Create function type: i32 main()
        auto* mainType = llvm::FunctionType::get(
            irBuilder->getInt32Ty(), // Return type = 32-bit integer
            /* Parameters */ {},     // Empty list = no arguments
            /* Varargs */ false      // No "..."
        );
        
        activeFunction = createFunction("main", mainType);
        llvm::Value* result = generateExpression();
        irBuilder->CreateRet(result); // Insert return instruction
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
    llvm::Value* generateExpression() {
        // Current simplification: Always returns 42
        return irBuilder->getInt32(42); // Create constant int32
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
    llvm::Function* createFunction(const std::string& name,
                                    llvm::FunctionType* type) {
        if (auto* existing = module->getFunction(name))
            return existing;

        auto* func = createFunctionPrototype(name, type);
        setupFunctionBody(func);
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
    llvm::Function* createFunctionPrototype(const std::string& name,
                                            llvm::FunctionType* type) {
        auto* func = llvm::Function::Create(
            type,
            llvm::Function::ExternalLinkage, // Why? So OS can find main()
            name,
            module.get() // Module ownership
        );
        verifyFunction(*func); // Like spell-check for LLVM IR
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
    void setupFunctionBody(llvm::Function* func) {
        auto* entryBlock = createBasicBlock("entry", func);
        irBuilder->SetInsertPoint(entryBlock); // "Start writing here"
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
    llvm::BasicBlock* createBasicBlock(const std::string& label,
                                       llvm::Function* parent = nullptr) {
        return llvm::BasicBlock::Create(*context, label, parent);
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
    void saveModuleToFile(const std::string& filename) {
        std::error_code ec;
        llvm::raw_fd_ostream outFile(filename, ec);
        module->print(outFile, nullptr); // Write module contents
    }

    /**
     * @brief Initializes LLVM components
     * 
     * Order matters:
     * 1. Context first (needed by everyone)
     * 2. Module (needs context)
     * 3. Builder (needs context)
     */
    void initializeModule() {
        context = std::make_unique<llvm::LLVMContext>();
        module = std::make_unique<llvm::Module>(
            "MorningLangCompilationUnit", // Module name
            *context                      // Context reference
        );
        irBuilder = std::make_unique<llvm::IRBuilder<>>(*context);
    }
};