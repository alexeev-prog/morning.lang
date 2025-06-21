#pragma once

// LLVM Core Headers (Essential Components)
#include <llvm/IR/BasicBlock.h>    ///< Represents basic blocks of code without branches
#include <llvm/IR/DerivedTypes.h>  ///< Contains derived LLVM type definitions
#include <llvm/IR/Function.h>      ///< Provides function representation in LLVM IR
#include <llvm/IR/GlobalVariable.h>///< Handles global variable declarations
#include <llvm/IR/Instructions.h>  ///< Contains instruction definitions
#include <llvm/IR/Value.h>         ///< Fundamental value representation in LLVM
#include <llvm/IR/Verifier.h>      ///< Tools for IR validity checks
#include <llvm/Support/Alignment.h>///< Alignment utilities
#include <llvm/Support/Casting.h>  ///< Type casting support
#include <llvm/Support/raw_ostream.h> ///< Output stream handling

#include "env.h"                   ///< Environment header
#include "llvm/IR/IRBuilder.h"     ///< IR construction utilities
#include "llvm/IR/LLVMContext.h"   ///< Context for compilation environment isolation
#include "llvm/IR/Module.h"        ///< Container for code (similar to source file)
#include "parser/MorningLangGrammar.h" ///< Grammar parser for MorningLang

#include <map>                     ///< Standard map container
#include <memory>                  ///< Smart pointers
#include <string>                  ///< String utilities
#include <vector>                  ///< Vector container

/**
 * @def GEN_BINARY_OP(Op, varName)
 * @brief Macro to generate binary operation IR instructions
 *
 * This macro generates LLVM IR for binary operations by:
 * 1. Evaluating the first operand from expression list index 1
 * 2. Evaluating the second operand from expression list index 2
 * 3. Creating the specified binary operation with the given variable name
 *
 * @param Op The LLVM IRBuilder operation method (e.g., CreateAdd)
 * @param varName Temporary variable name for the result
 */
#define GEN_BINARY_OP(Op, varName) \
    do { \
        auto oper1 = generate_expression(exp.list[1], env); \
        auto oper2 = generate_expression(exp.list[2], env); \
        return m_IR_BUILDER->Op(oper1, oper2, varName); \
    } while (false)

using env = std::shared_ptr<Environment>; ///< Shared pointer to Environment

/**
 * @struct LoopBlocks
 * @brief Structure to manage loop control flow blocks
 *
 * Contains basic blocks for loop break and continue points,
 * enabling proper control flow management during loop generation
 */
struct LoopBlocks {
    llvm::BasicBlock* break_blog;    ///< Block to jump to when breaking from loop
    llvm::BasicBlock* continue_block;///< Block to jump to when continuing loop
};

/**
 * @class MorningLanguageLLVM
 * @brief Converts MorningLang source code to LLVM Intermediate Representation (IR)
 *
 * This class acts as a compiler frontend that:
 * 1. Parses MorningLang source code
 * 2. Generates LLVM IR
 * 3. Manages compilation context and environment
 * 4. Handles control flow structures and variable scoping
 *
 * It creates a complete LLVM Module containing functions, global variables,
 * and executable instructions that can be compiled to machine code.
 */
class MorningLanguageLLVM {
  public:
    /**
     * @brief Constructs and initializes the LLVM compilation environment
     *
     * Performs critical initialization steps:
     * 1. Creates LLVM context and module
     * 2. Initializes IR builders
     * 3. Sets up global environment
     * 4. Registers external functions
     */
    MorningLanguageLLVM();

    /**
     * @brief Executes the full compilation pipeline
     *
     * Processes source code through:
     * 1. Parsing to Abstract Syntax Tree (AST)
     * 2. IR generation from AST
     * 3. Module verification
     * 4. Output to file
     *
     * @param program MorningLang source code string
     * @param output_base Base filename for output files (without extension)
     * @return int Status code (0 = success)
     */
    auto execute(const std::string& program, const std::string& output_base) -> int;

  private:
    llvm::Function* m_ACTIVE_FUNCTION {}; ///< Current function being generated
    std::vector<LoopBlocks> m_LOOP_STACK; ///< Stack for nested loop management
    std::unique_ptr<llvm::LLVMContext> m_CONTEXT; ///< LLVM context for isolation
    std::unique_ptr<llvm::Module> m_MODULE; ///< Container for generated IR
    std::unique_ptr<llvm::IRBuilder<>> m_IR_BUILDER; ///< Builder for IR instructions
    std::unique_ptr<syntax::MorningLangGrammar> m_PARSER; ///< Source code parser
    std::shared_ptr<Environment> m_GLOBAL_ENV; ///< Global symbol environment
    std::unique_ptr<llvm::IRBuilder<>> m_VARS_BUILDER; ///< Builder for variable allocation
    std::map<std::string, llvm::Value*> m_CONSTANTS; ///< Map of constant variables

    /**
     * @brief Configures target triple for generated module
     *
     * Sets architecture-specific information (default: x86_64-unknown-linux-gnu)
     */
    void setup_triple();

    /**
     * @brief Initializes global environment with predefined variables
     *
     * Creates global variables like _VERSION and registers them
     * in the global environment
     */
    void setup_global_environment();

    /**
     * @brief Generates IR from Abstract Syntax Tree
     *
     * Recursively traverses AST to generate corresponding LLVM IR:
     * 1. Creates main function
     * 2. Processes expressions
     * 3. Handles control flow
     *
     * @param ast Root of the Abstract Syntax Tree
     */
    void generate_ir(const Exp& ast);

    /**
     * @brief Creates a global variable in the module
     *
     * @param name Variable name
     * @param init_value Initial constant value
     * @param is_mutable Flag for mutable variables (default=false)
     * @return llvm::GlobalVariable* Pointer to created global variable
     */
    auto create_global_variable(const std::string& name, llvm::Constant* init_value, bool is_mutable = false)
        -> llvm::GlobalVariable*;

    /**
     * @brief Maps type strings to LLVM types
     *
     * @param type_string MorningLang type specifier (e.g., "!int")
     * @return llvm::Type* Corresponding LLVM type
     */
    auto get_type(const std::string& type_string, const std::string& var_name) -> llvm::Type*;

    /**
     * @brief Extracts variable type from declaration expression
     *
     * @param exp Variable declaration expression
     * @return llvm::Type* Resolved LLVM type
     */
    auto extract_var_type(const Exp& exp) -> llvm::Type*;

    /**
     * @brief Derives function type from function expression
     *
     * @param fn_exp Function definition expression
     * @return llvm::FunctionType* Complete function signature type
     */
    auto extract_function_type(const Exp& fn_exp) -> llvm::FunctionType*;

    /**
     * @brief Allocates stack space for a variable
     *
     * @param name Variable name
     * @param var_type LLVM type of variable
     * @param env Current environment
     * @return llvm::Value* Pointer to allocated stack slot
     */
    auto alloc_var(const std::string& name, llvm::Type* var_type, const env& env) -> llvm::Value*;

    /**
     * @brief Compiles function definition to LLVM function
     *
     * @param fn_exp Function expression from AST
     * @param fn_name Name of function
     * @param env Parent environment
     * @return llvm::Value* Pointer to generated function
     */
    auto compile_function(const Exp& fn_exp, const std::string& fn_name, const env& env) -> llvm::Value*;

    /**
     * @brief Generates IR for any expression type
     *
     * Handles all expression types including:
     * - Literals (numbers, strings)
     * - Variables
     * - Binary operations
     * - Control structures (if, loop, while)
     * - Function calls
     * - Variable declarations
     *
     * @param exp Expression to compile
     * @param env Current environment
     * @return llvm::Value* Resulting LLVM value
     */
    auto generate_expression(const Exp& exp, const env& env) -> llvm::Value*;

    /**
     * @brief Registers external function prototypes
     *
     * Adds declarations for:
     * - printf (output)
     * - scanf (input)
     */
    void setup_extern_functions();

    /**
     * @brief Creates complete function with body
     *
     * @param name Function name
     * @param type Function signature type
     * @param env Environment for symbol registration
     * @return llvm::Function* Created function
     */
    auto create_function(const std::string& name, llvm::FunctionType* type, const env& env)
        -> llvm::Function*;

    /**
     * @brief Creates function prototype (declaration only)
     *
     * @param name Function name
     * @param type Function signature type
     * @param env Environment for symbol registration
     * @return llvm::Function* Created function prototype
     */
    auto create_function_prototype(const std::string& name, llvm::FunctionType* type, const env& env)
        -> llvm::Function*;

    /**
     * @brief Sets up function body structure
     *
     * Creates entry block and positions builder for code generation
     *
     * @param func Function to initialize
     */
    void setup_function_body(llvm::Function* func);

    /**
     * @brief Creates labeled basic block
     *
     * @param label Name for the basic block
     * @param parent Function containing the block (nullptr for detached)
     * @return llvm::BasicBlock* Created basic block
     */
    auto create_basic_block(const std::string& label, llvm::Function* parent = nullptr) -> llvm::BasicBlock*;

    /**
     * @brief Saves generated module to file
     *
     * Outputs human-readable LLVM IR to specified file
     *
     * @param filename Output filename (.ll extension recommended)
     */
    void save_module_to_file(const std::string& filename);

    /**
     * @brief Initializes core LLVM components
     *
     * Creates:
     * - LLVM context
     * - Module
     * - IR builders
     */
    void initialize_module();
};
