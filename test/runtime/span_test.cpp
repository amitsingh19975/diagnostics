#include <iostream>
#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <type_traits>
#include "diagnostics/span.hpp"

using namespace dark;

TEST_CASE("Span", "[span:marker_relative]") {
    SECTION("Construction") {
        {
            auto s = MarkerRelSpan();
            REQUIRE(s.is_marker_relative());
            REQUIRE(s.empty());
            REQUIRE(s.start() == 0);
            REQUIRE(s.end() == 0);
            REQUIRE(s.size() == 0);
            REQUIRE(std::numeric_limits<std::decay_t<decltype(s.start())>>::is_signed);
            REQUIRE(std::numeric_limits<std::decay_t<decltype(s.end())>>::is_signed);
        }
        {
            auto s = MarkerRelSpan(2, 10);
            REQUIRE(s.is_marker_relative());
            REQUIRE(!s.empty());
            REQUIRE(s.start() == 2);
            REQUIRE(s.end() == 10);
            REQUIRE(s.size() == 8);
        }
        {
            auto s = MarkerRelSpan(-2, -10); // start > end
            REQUIRE(s.is_marker_relative());
            REQUIRE(s.empty());
            REQUIRE(s.start() == 0);
            REQUIRE(s.end() == 0);
            REQUIRE(s.size() == 0);
        }
        {
            auto s = MarkerRelSpan(-10, -2);
            REQUIRE(s.is_marker_relative());
            REQUIRE(!s.empty());
            REQUIRE(s.start() == -10);
            REQUIRE(s.end() == -2);
            REQUIRE(s.size() == 8);
        }
        {
            auto s = MarkerRelSpan(2);
            REQUIRE(s.is_marker_relative());
            REQUIRE(!s.empty());
            REQUIRE(s.start() == 2);
            REQUIRE(s.end() == 3);
            REQUIRE(s.size() == 1);
        }
        {
            auto s = MarkerRelSpan::from_usize(2, 8);
            REQUIRE(s.is_marker_relative());
            REQUIRE(!s.empty());
            REQUIRE(s.start() == 2);
            REQUIRE(s.end() == 10);
            REQUIRE(s.size() == 8);
        }
        {
            auto s = MarkerRelSpan::from_usize(-2, 8);
            REQUIRE(s.is_marker_relative());
            REQUIRE(!s.empty());
            REQUIRE(s.start() == -2);
            REQUIRE(s.end() == 6);
            REQUIRE(s.size() == 8);
        }
    }

    SECTION("Resolve") {
        {
            auto s = MarkerRelSpan(-10, -2);
            REQUIRE(s.is_marker_relative());
            REQUIRE(!s.empty());
            REQUIRE(s.start() == -10);
            REQUIRE(s.end() == -2);
            REQUIRE(s.size() == 8);

            auto r = s.resolve(20);
            REQUIRE(r.is_absolute());
            REQUIRE(!r.empty());
            REQUIRE(r.start() == 10);
            REQUIRE(r.end() == 18);
            REQUIRE(r.size() == 8);
        }
        {
            // Absolute span cannot be negative.
            auto s = MarkerRelSpan(-2);
            REQUIRE(s.is_marker_relative());
            REQUIRE(!s.empty());
            REQUIRE(s.start() == -2);
            REQUIRE(s.end() == -1);
            REQUIRE(s.size() == 1);

            auto r = s.resolve(0);
            REQUIRE(r.is_absolute());
            REQUIRE(r.empty());
            REQUIRE(r.start() == 0);
            REQUIRE(r.end() == 0);
            REQUIRE(r.size() == 0);
        }
    }

    SECTION("Shift") {
        {
            auto s = MarkerRelSpan(-2);
            REQUIRE(s.is_marker_relative());
            REQUIRE(!s.empty());
            REQUIRE(s.start() == -2);
            REQUIRE(s.end() == -1);
            REQUIRE(s.size() == 1);

            auto r = s.shift(10);
            REQUIRE(r.is_marker_relative());
            REQUIRE(!r.empty());
            REQUIRE(r.start() == 8);
            REQUIRE(r.end() == 9);
            REQUIRE(r.size() == 1);
        }
        {
            auto s = MarkerRelSpan(-2);
            REQUIRE(s.is_marker_relative());
            REQUIRE(!s.empty());
            REQUIRE(s.start() == -2);
            REQUIRE(s.end() == -1);
            REQUIRE(s.size() == 1);

            auto r = s.shift(-1);
            REQUIRE(r.is_marker_relative());
            REQUIRE(!r.empty());
            REQUIRE(r.start() == -3);
            REQUIRE(r.end() == -2);
            REQUIRE(r.size() == 1);
        }
    }
}


TEST_CASE("Span", "[span:loc_relative]") {
    SECTION("Construction") {
        {
            auto s = LocRelSpan();
            REQUIRE(s.is_loc_relative());
            REQUIRE(s.empty());
            REQUIRE(s.start() == 0);
            REQUIRE(s.end() == 0);
            REQUIRE(s.size() == 0);
            REQUIRE(!std::numeric_limits<std::decay_t<decltype(s.start())>>::is_signed);
            REQUIRE(!std::numeric_limits<std::decay_t<decltype(s.end())>>::is_signed);
        }
        {
            auto s = LocRelSpan(2, 10);
            REQUIRE(s.is_loc_relative());
            REQUIRE(!s.empty());
            REQUIRE(s.start() == 2);
            REQUIRE(s.end() == 10);
            REQUIRE(s.size() == 8);
        }
        {
            auto s = LocRelSpan(2);
            REQUIRE(s.is_loc_relative());
            REQUIRE(!s.empty());
            REQUIRE(s.start() == 2);
            REQUIRE(s.end() == 3);
            REQUIRE(s.size() == 1);
        }
        {
            auto s = LocRelSpan::from_usize(2, 8);
            REQUIRE(s.is_loc_relative());
            REQUIRE(!s.empty());
            REQUIRE(s.start() == 2);
            REQUIRE(s.end() == 10);
            REQUIRE(s.size() == 8);
        }
    }

    SECTION("Resolve") {
        {
            auto s = LocRelSpan(0, 2);
            REQUIRE(s.is_loc_relative());
            REQUIRE(!s.empty());
            REQUIRE(s.start() == 0);
            REQUIRE(s.end()   == 2);
            REQUIRE(s.size()  == 2);

            auto r = s.resolve(20);
            REQUIRE(r.is_absolute());
            REQUIRE(!r.empty());
            REQUIRE(r.start() == 20);
            REQUIRE(r.end()   == 22);
            REQUIRE(r.size()  == 2);
        }
    }
}

TEST_CASE("Span", "[span:absolute]") {
    SECTION("Construction") {
        {
            auto s = Span();
            REQUIRE(s.is_absolute());
            REQUIRE(s.empty());
            REQUIRE(s.start() == 0);
            REQUIRE(s.end() == 0);
            REQUIRE(s.size() == 0);
            REQUIRE(!std::numeric_limits<std::decay_t<decltype(s.start())>>::is_signed);
            REQUIRE(!std::numeric_limits<std::decay_t<decltype(s.end())>>::is_signed);
        }
        {
            auto s = Span(2, 10);
            REQUIRE(s.is_absolute());
            REQUIRE(!s.empty());
            REQUIRE(s.start() == 2);
            REQUIRE(s.end() == 10);
            REQUIRE(s.size() == 8);
        }
        {
            auto s = Span(2);
            REQUIRE(s.is_absolute());
            REQUIRE(!s.empty());
            REQUIRE(s.start() == 2);
            REQUIRE(s.end() == 3);
            REQUIRE(s.size() == 1);
        }
        {
            auto s = Span::from_usize(2, 8);
            REQUIRE(s.is_absolute());
            REQUIRE(!s.empty());
            REQUIRE(s.start() == 2);
            REQUIRE(s.end() == 10);
            REQUIRE(s.size() == 8);
        }
    }

    SECTION("Shift") {
        {
            auto s = Span(20);
            REQUIRE(s.is_absolute());
            REQUIRE(!s.empty());
            REQUIRE(s.start() == 20);
            REQUIRE(s.end() == 21);
            REQUIRE(s.size() == 1);

            auto r = s.shift(10);
            REQUIRE(r.is_absolute());
            REQUIRE(!r.empty());
            REQUIRE(r.start() == 30);
            REQUIRE(r.end() == 31);
            REQUIRE(r.size() == 1);
        }
        {
            auto s = Span(2);
            REQUIRE(s.is_absolute());
            REQUIRE(!s.empty());
            REQUIRE(s.start() == 2);
            REQUIRE(s.end() == 3);
            REQUIRE(s.size() == 1);

            auto r = s.shift(-1);
            REQUIRE(r.is_absolute());
            REQUIRE(!r.empty());
            REQUIRE(r.start() == 1);
            REQUIRE(r.end() == 2);
            REQUIRE(r.size() == 1);
        }
    }

    SECTION("Testing 'intersects' methos") {
        {
            // Case 1: Intersection
            //          [------------)
            // [----------)
            auto lhs = Span(0, 10);
            auto rhs = Span(8, 15);
            REQUIRE(lhs.intersects(rhs) == true);
        }
        {
            // Case 2: Intersection
            // [------------)
            //         [----------)
            //
            auto lhs = Span(8, 15);
            auto rhs = Span(0, 10);
            REQUIRE(lhs.intersects(rhs) == true);
        }
        {
            // Case 3: No Intersection
            // [------------)
            //              [----------)
            //
            auto lhs = Span(0, 10);
            auto rhs = Span(10, 15);
            REQUIRE(lhs.intersects(rhs) == false);
        }
        {
            // Case 4: No Intersection
            // [------------)
            //                  [----------)
            //
            auto lhs = Span(0, 10);
            auto rhs = Span(12, 15);
            REQUIRE(lhs.intersects(rhs) == false);
        }
        {
            // Case 5: Intersection
            // [------------)
            //    [------)
            //
            auto lhs = Span(0, 10);
            auto rhs = Span(5, 8);
            REQUIRE(lhs.intersects(rhs) == true);
        }
    }

    SECTION("Testing 'get_intersections' methos") {
        {
            // Case 1: Intersection
            // [-----------)
            //        [---------)
            //       |
            //       V
            // [-----)[----)[-----)

            auto lhs = Span(0, 10);
            auto rhs = Span(8, 15);
            auto res = lhs.get_intersections(rhs);

            REQUIRE(res.size() == 3);
            REQUIRE(res[0] == Span(0, 8));
            REQUIRE(res[1] == Span(8, 10));
            REQUIRE(res[2] == Span(10, 15));
        }
        {
            // Case 2: Intersection
            //        [---------)
            // [-----------)
            //       |
            //       V
            // [-----)[----)[-----)

            auto lhs = Span(0, 10);
            auto rhs = Span(8, 15);
            auto res = lhs.get_intersections(rhs);

            REQUIRE(res.size() == 3);
            REQUIRE(res[0] == Span(0, 8));
            REQUIRE(res[1] == Span(8, 10));
            REQUIRE(res[2] == Span(10, 15));
        }
        {
            // Case 3: Intersection
            //    [----)
            // [-----------)
            //       |
            //       V
            // [-----)[----)[-----)

            auto lhs = Span(0, 10);
            auto rhs = Span(5, 8);
            auto res = lhs.get_intersections(rhs);

            REQUIRE(res.size() == 3);
            REQUIRE(res[0] == Span(0, 5));
            REQUIRE(res[1] == Span(5, 8));
            REQUIRE(res[2] == Span(8, 10));
        }
        {
            // Case 4: No Intersection
            //                 [---------)
            // [-----------)
            //       |
            //       V
            // [-----------)   [---------)
            //

            auto lhs = Span(0, 10);
            auto rhs = Span(11, 15);
            auto res = lhs.get_intersections(rhs);

            REQUIRE(res.size() == 2);
            REQUIRE(res[0] == Span(0, 10));
            REQUIRE(res[1] == Span(11, 15));
        }
    }

    SECTION("Testing 'force_merge' and 'merge' methos") {
        {
            // Case 1: Intersection
            // [-----------)
            //        [---------)
            //       |
            //       V
            // [----------------)
            auto lhs = Span(0, 10);
            auto rhs = Span(8, 15);

            REQUIRE(lhs.force_merge(rhs) == Span(0, 15));
            REQUIRE(lhs.merge(rhs) == Span(0, 15));
        }
        {
            // Case 2: No Intersection
            // [-----------)
            //                [---------)
            //       |
            //       V
            // [------------------------)
            auto lhs = Span(0, 10);
            auto rhs = Span(12, 15);

            REQUIRE(lhs.force_merge(rhs) == Span(0, 15));
            REQUIRE(lhs.merge(rhs).has_value() == false);
        }
    }
}
