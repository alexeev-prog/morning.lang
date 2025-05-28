#include <catch2/catch_test_macros.hpp>

#include "morningllvm.hpp"

TEST_CASE("Check base", "[BASIC]") {
	const std::string PROGRAM = R"(
        42
    )";

    MorningLanguageLLVM morning_vm;

	int status = morning_vm.execute(PROGRAM);
	REQUIRE(status == 0);
}