#include "diagnostics/core/cow_string.hpp"
#include "diagnostics/core/formatter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <ostream>
#include <string_view>

using namespace dark::core;

TEST_CASE("Formatter", "[formatter]") {
    auto f = Formatter("Formatter is {} format {} that has {} number of args.", "storing", "args", 3);
    REQUIRE(f.number_of_args() == 3);
    REQUIRE(f.format() == "Formatter is storing format args that has 3 number of args.");
}

struct CustomFirstArg {
    std::string s;
};

struct CustomSecondArg {
    std::string s;
};

struct CustomThirdArg {
    int val;
};

struct CustomOStreamArg {
    int val;
};

auto to_formatter_arg(CustomFirstArg const& arg) -> CowString {
    return arg.s;
}

auto to_formatter_arg(CustomSecondArg const& arg) -> CowString {
    return arg.s;
}

auto to_formatter_arg(CustomThirdArg const& arg) -> int {
    return arg.val;
}

std::ostream& operator<<(std::ostream& os, CustomOStreamArg const& arg) {
    return os << arg.val;
}

TEST_CASE("Formatter", "[formatter:custom_formatter]") {
    SECTION("By overriding 'to_formatter_arg'") {
        auto f = Formatter("Formatter is {} format {} that has {} number of args.", CustomFirstArg{"storing"}, CustomSecondArg{"args"}, CustomThirdArg{3});
        REQUIRE(f.number_of_args() == 3);
        REQUIRE(f.format() == "Formatter is storing format args that has 3 number of args.");
    }

    SECTION("By overriding 'operator<<'") {
        auto f = Formatter("Formatter with {} number of args.", CustomOStreamArg{3});
        REQUIRE(f.number_of_args() == 1);
        REQUIRE(f.format() == "Formatter with 3 number of args.");
    }
}
