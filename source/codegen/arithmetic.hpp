#pragma once

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <string>
#include <unordered_map>

class ArithmeticCodegen {
public:
    static llvm::Value* generate_binary_op(
        const std::string& op,
        llvm::Value* left,
        llvm::Value* right,
        llvm::IRBuilder<>& builder
    );

private:
    static const std::unordered_map<std::string, std::string> OP_MAPPING;

    static llvm::Type* get_common_type(
        llvm::Value* left,
        llvm::Value* right
    );
};
