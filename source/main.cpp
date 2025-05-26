#include "morningllvm.hpp"

int main() {
    std::string program = R"(
        42
    )";

    MorningLanguageLLVM vm;

    vm.execute(program);

    return 0;
}
