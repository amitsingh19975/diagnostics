#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <type_traits>
#include "diagnostics/span.hpp"

using namespace dark;

TEST_CASE("Span", "[span]") {
    SECTION("Construction") {
        {
            auto s = Span();
            REQUIRE(s.empty());
            REQUIRE(s.start() == 0);
            REQUIRE(s.end() == 0);
            REQUIRE(s.size() == 0);
        }
        {
            auto s = Span(2, 10);
            REQUIRE(!s.empty());
            REQUIRE(s.start() == 2);
            REQUIRE(s.end() == 10);
            REQUIRE(s.size() == 8);
        }
        {
            auto s = Span(2);
            REQUIRE(!s.empty());
            REQUIRE(s.start() == 2);
            REQUIRE(s.end() == 3);
            REQUIRE(s.size() == 1);
        }
        {
            auto s = Span::from_size(2, 8);
            REQUIRE(!s.empty());
            REQUIRE(s.start() == 2);
            REQUIRE(s.end() == 10);
            REQUIRE(s.size() == 8);
        }
    }

    SECTION("Shift") {
        {
            auto s = Span(20);
            REQUIRE(!s.empty());
            REQUIRE(s.start() == 20);
            REQUIRE(s.end() == 21);
            REQUIRE(s.size() == 1);

            auto r = s.shift(10);
            REQUIRE(!r.empty());
            REQUIRE(r.start() == 30);
            REQUIRE(r.end() == 31);
            REQUIRE(r.size() == 1);
        }
        {
            auto s = Span(2);
            REQUIRE(!s.empty());
            REQUIRE(s.start() == 2);
            REQUIRE(s.end() == 3);
            REQUIRE(s.size() == 1);

            auto r = s.shift(-1);
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

    SECTION("Testing intersections methos") {
        {
            // Case 1: Intersection
            // [-----------)
            //        [---------)
            //       |
            //       V
            // [-----)[----)[-----)

            auto lhs = Span(0, 10);
            auto rhs = Span(8, 15);
            auto res = lhs.calculate_intersections(rhs);

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
            auto res = lhs.calculate_intersections(rhs);

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
            auto res = lhs.calculate_intersections(rhs);

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
            auto res = lhs.calculate_intersections(rhs);

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
