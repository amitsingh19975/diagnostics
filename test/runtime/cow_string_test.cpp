#include <catch2/catch_test_macros.hpp>
#include <string_view>
#include "diagnostics/core/cow_string.hpp"

using namespace dark::core;

TEST_CASE("Cow String", "[cow_string]") {
    SECTION("Borrowed String") {
        {
            auto sv = std::string_view("Borrowed String");
            auto cs = CowString(sv);
            REQUIRE(cs.is_borrowed());
            REQUIRE(!cs.is_owned());
            REQUIRE(sv == cs);
        }

        {
            auto cs = CowString("Borrowed String");
            REQUIRE(cs.is_borrowed());
            REQUIRE(!cs.is_owned());
            REQUIRE("Borrowed String" == cs);
        }

        {
            auto s = "Borrowed String";
            auto cs = CowString(s, CowString::BorrowedTag{});
            REQUIRE(cs.is_borrowed());
            REQUIRE(!cs.is_owned());
            REQUIRE(s == cs);
        }
    }

    SECTION("Owned String") {
        {
            auto s = "Owned String";
            auto cs = CowString(s);
            REQUIRE(!cs.is_borrowed());
            REQUIRE(cs.is_owned());
            REQUIRE(s == cs);
        }

        {
            auto s = std::string("Owned String");
            auto cs = CowString(s);
            REQUIRE(!cs.is_borrowed());
            REQUIRE(cs.is_owned());
            REQUIRE(s == cs);
        }

        {
            auto sv = std::string_view("Borrowed String");
            auto cs = CowString(sv, CowString::OwnedTag{});
            REQUIRE(!cs.is_borrowed());
            REQUIRE(cs.is_owned());
            REQUIRE(sv == cs);
        }
    }
}
