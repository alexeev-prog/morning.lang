#include <string>

#include "morningllvm.hpp"

auto main() -> int {
    const std::string PROGRAM = R"(
[var [ALPHA !int] 42]

[scope
    [var [ALPHA !string] "Hello"]
    [fprint "ALPHA: %s\n" ALPHA]]

[fprint "ALPHA: %d\n" ALPHA]

[set ALPHA 100]

[fprint "ALPHA: %d\n" ALPHA]

[fprint "_VERSION: %d\n\n" _VERSION]

    )";

    MorningLanguageLLVM morning_vm;

    morning_vm.execute(PROGRAM);

    return 0;
}
