#pragma once

#include <string>
#include <unordered_map>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

class ArithmeticCodegen {
  public:
    /**
     * @brief Generate binary operation
     *
     * @param op operand
     * @param left left side
     * @param right right side
     * @param builder IR builder
     * @return llvm::Value*
     **/
    static auto generate_binary_op(const std::string& op,
                                   llvm::Value* left,
                                   llvm::Value* right,
                                   llvm::IRBuilder<>& builder) -> llvm::Value*;

  private:
    static const std::unordered_map<std::string, std::string> m_OP_MAPPING;

    /**
     * @brief Get the common type object
     *
     * @param left left side
     * @param right right side
     * @return llvm::Type*
     **/
    static auto get_common_type(llvm::Value* left, llvm::Value* right) -> llvm::Type*;
};
