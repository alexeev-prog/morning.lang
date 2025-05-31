#include <string>

#include "morningllvm.hpp"

auto main() -> int {
    const std::string PROGRAM = R"(

        (fprint "Value: %d\n" 418)

    )";

    MorningLanguageLLVM morning_vm;

    morning_vm.execute(PROGRAM);

    return 0;
}
