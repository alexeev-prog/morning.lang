#include <string>

#include "morningllvm.hpp"

auto main() -> int {
    const std::string PROGRAM = R"(
        // [var ALPHA 42]

        // [scope
        //     [var ALPHA "Hello"]
        //     [fprint "A: %s\n" ALPHA]
        // ]

        [fprint "_VERSION: %d\n\n" _VERSION]

    )";

    MorningLanguageLLVM morning_vm;

    morning_vm.execute(PROGRAM);

    return 0;
}
