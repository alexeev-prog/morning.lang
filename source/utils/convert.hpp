#include <llvm/IR/Type.h>
#include "../logger.hpp"

inline auto type_to_string(llvm::Type* type) -> std::string {
        std::string type_str;

        llvm::raw_string_ostream rso(type_str);

        type->print(rso);

        auto value = rso.str();

        if (value == "i64") {
            return "!int64";
        }
        if (value == "i32") {
            return "!int64";
        }
        if (value == "i16") {
            return "!int64";
        }
        if (value == "i8") {
            return "!int64";
        }
        if (value == "ptr") {
            return "!str";
        }
        if (value == "double") {
            return "!frac";
        }
        if (type_str == "i1") {
            return "!bool";
        }
        if (type_str == "void") {
            return "!none";
        }

        // TODO
        // if (type_str == "i8*") {
        //     return "!str";
        // }

        LOG_WARN("In codegen process type_to_string function get unknown type: %s", value.c_str());

        return value;
    }
