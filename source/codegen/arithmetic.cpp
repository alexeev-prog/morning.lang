#include "arithmetic.hpp"
#include "../utils/cast.hpp"

using namespace llvm;

const std::unordered_map<std::string, std::string>
ArithmeticCodegen::OP_MAPPING = {
    {"__PLUS_OPERAND__", "+"},
    {"__SUB_OPERAND__", "-"},
    {"__MUL_OPERAND__", "*"},
    {"__DIV_OPERAND__", "/"},
    {"__CMPG__", ">"},
    {"__CMPL__", "<"},
    {"__CMPGE__", ">="},
    {"__CMPLE__", "<="},
    {"__CMPEQ__", "=="},
    {"__CMPNE__", "!="}
};

Type* ArithmeticCodegen::get_common_type(
    Value* left,
    Value* right
) {
    Type* left_type = left->getType();
    Type* right_type = right->getType();

    if (left_type->isDoubleTy() || right_type->isDoubleTy()) {
        return Type::getDoubleTy(left_type->getContext());
    }
    return left_type;
}

Value* ArithmeticCodegen::generate_binary_op(
    const std::string& op,
    Value* left,
    Value* right,
    IRBuilder<>& builder
) {
    std::string operation = op;
    if (auto it = OP_MAPPING.find(op); it != OP_MAPPING.end()) {
        operation = it->second;
    }

    Type* common_type = get_common_type(left, right);
    left = implicit_cast(left, common_type, builder);
    right = implicit_cast(right, common_type, builder);

    if (common_type->isDoubleTy()) {
        if (operation == "+") return builder.CreateFAdd(left, right, "fadd_tmp");
        if (operation == "-") return builder.CreateFSub(left, right, "fsub_tmp");
        if (operation == "*") return builder.CreateFMul(left, right, "fmul_tmp");
        if (operation == "/") return builder.CreateFDiv(left, right, "fdiv_tmp");
        if (operation == ">") return builder.CreateFCmpOGT(left, right, "fcmp_tmp");
        if (operation == "<") return builder.CreateFCmpOLT(left, right, "fcmp_tmp");
        if (operation == ">=") return builder.CreateFCmpOGE(left, right, "fcmp_tmp");
        if (operation == "<=") return builder.CreateFCmpOLE(left, right, "fcmp_tmp");
        if (operation == "==") return builder.CreateFCmpOEQ(left, right, "fcmp_tmp");
        if (operation == "!=") return builder.CreateFCmpONE(left, right, "fcmp_tmp");
    }
    else {
        if (operation == "+") return builder.CreateAdd(left, right, "add_tmp");
        if (operation == "-") return builder.CreateSub(left, right, "sub_tmp");
        if (operation == "*") return builder.CreateMul(left, right, "mul_tmp");
        if (operation == "/") return builder.CreateSDiv(left, right, "div_tmp");
        if (operation == ">") return builder.CreateICmpSGT(left, right, "icmp_tmp");
        if (operation == "<") return builder.CreateICmpSLT(left, right, "icmp_tmp");
        if (operation == ">=") return builder.CreateICmpSGE(left, right, "icmp_tmp");
        if (operation == "<=") return builder.CreateICmpSLE(left, right, "icmp_tmp");
        if (operation == "==") return builder.CreateICmpEQ(left, right, "icmp_tmp");
        if (operation == "!=") return builder.CreateICmpNE(left, right, "icmp_tmp");
    }

    return nullptr;
}
