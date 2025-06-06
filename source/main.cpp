#include <string>

#include "morningllvm.hpp"

auto main() -> int {
    const std::string PROGRAM = R"(
[var a 10]

[while (> a 0)
    [scope
        [set a (- a 1)]
        [fprint "%d " a]]]

[fprint "\nA: %d\n\n" a]
    )";

    MorningLanguageLLVM morning_vm;

    morning_vm.execute(PROGRAM);

    return 0;
}
