#include <string>

#include "morningllvm.hpp"

auto main() -> int {
    const std::string PROGRAM = R"(
[var x (+ 100 1)]

// [condition (== x 101)]
//     [set x 202]
//     [set x 303]]

[fprint "X: %d\n" x]
[fprint "X is 101? %d\n\n" (== x 101)]
    )";

    MorningLanguageLLVM morning_vm;

    morning_vm.execute(PROGRAM);

    return 0;
}
