#include "source/morning_llvm.hpp"

int main() {
    std::string program = R"(
        42
    )";

    MorningLanguageLLVM vm;

    vm.exec(program);

    return 0;
}
