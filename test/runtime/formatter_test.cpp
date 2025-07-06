#include <catch2/catch_test_macros.hpp>
#include <ostream>
#include <print>
#include <string_view>
#include "diagnostics/core/format.hpp"

using namespace dark::core;

TEST_CASE("Formatter", "[formatter]") {
    auto f = BasicFormatter("Formatter is {} format {} that has {} number of args.", "storing", "args", 3);
    REQUIRE(f.number_of_args() == 3);
    REQUIRE(f.format().to_borrowed() == "Formatter is storing format args that has 3 number of args.");
}

struct CustomFirstArg {
    std::string_view arg;

    auto to_string() const noexcept -> std::string_view { return arg; }
};

struct CustomSecondArg {
    std::string_view arg;
};

auto to_string(CustomSecondArg const& c) noexcept -> std::string_view { return c.arg; }

struct CustomThirdArg {
    int arg;
};

template <>
struct std::formatter<CustomThirdArg> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(CustomThirdArg const& c, auto& ctx) const {
        return std::format_to(ctx.out(), "{}", c.arg);
    }
};

struct CustomOStreamArg {
    int arg;

    friend std::ostream& operator<<(std::ostream& os, CustomOStreamArg const& c) {
        return os << c.arg;
    }
};

TEST_CASE("Formatter", "[formatter:custom_formatter]") {
    SECTION("By overriding 'to_formatter_arg'") {
        auto f = BasicFormatter("Formatter is {} format {} that has {} number of args.", CustomFirstArg{"storing"}, CustomSecondArg{"args"}, CustomThirdArg{3});
        REQUIRE(f.number_of_args() == 3);
        REQUIRE(f.format().to_borrowed() == "Formatter is storing format args that has 3 number of args.");
    }

    SECTION("By overriding 'operator<<'") {
        auto f = BasicFormatter("Formatter with {} number of args.", CustomOStreamArg{3});
        REQUIRE(f.number_of_args() == 1);
        REQUIRE(f.format().to_borrowed() == "Formatter with 3 number of args.");
    }
}
