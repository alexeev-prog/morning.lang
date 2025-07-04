#pragma once

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

/**
 * @brief Implicit cast
 *
 * @param value value
 * @param target_type target type
 * @param builder IR builder
 * @return llvm::Value*
 **/
inline auto implicit_cast(
    llvm::Value* value,
    llvm::Type* target_type,
    llvm::IRBuilder<>& builder
) -> llvm::Value* {
    if (value->getType() == target_type) {
        return value;
    }

    // Handle integer to fractional conversion
    if (value->getType()->isIntegerTy() && target_type->isDoubleTy()) {
        return builder.CreateSIToFP(value, target_type, "cast_int_to_double");
    }

    // Handle string to pointer conversion
    if (value->getType()->isPointerTy() && target_type->isPointerTy()) {
        return builder.CreatePointerCast(value, target_type, "cast_ptr");
    }

    if (value->getType()->isIntegerTy() && target_type->isIntegerTy()) {
        unsigned value_bits = value->getType()->getIntegerBitWidth();
        unsigned target_bits = target_type->getIntegerBitWidth();

        if (value_bits < target_bits) {
            return builder.CreateZExt(value, target_type, "zext_cast");
        } else if (value_bits > target_bits) {
            return builder.CreateTrunc(value, target_type, "trunc_cast");
        }
        return value;
    }

    return value;
}
