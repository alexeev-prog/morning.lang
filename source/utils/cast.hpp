#pragma once

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

inline auto implicit_cast(
    llvm::Value* value,
    llvm::Type* target_type,
    llvm::IRBuilder<>& builder
) -> llvm::Value* {
    if (value->getType() == target_type) {
        return value;
    }

    if (value->getType()->isIntegerTy() && target_type->isDoubleTy()) {
        return builder.CreateSIToFP(value, target_type, "cast_int_to_double");
    }

    return value;
}
