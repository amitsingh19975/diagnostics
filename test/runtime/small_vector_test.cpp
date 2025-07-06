#include "diagnostics/core/small_vec.hpp"
#include <catch2/catch_test_macros.hpp>
#include <print>

using namespace dark::core;

TEST_CASE("Small Vector", "[small_vec:construction]") {
    SECTION("Default dynamic or static construction") {
        {
            auto temp = SmallVec<int, 0>();
            REQUIRE(temp.is_small() == false);
            REQUIRE(temp.empty());
            REQUIRE(temp.size() == 0);
        }

        {
            auto temp = SmallVec<int, 10>();
            REQUIRE(temp.is_small() == true);
            REQUIRE(temp.empty());
            REQUIRE(temp.size() == 0);
        }
    }

    SECTION("Initializer list") {
        {
            auto temp = SmallVec<int, 0>{1, 2, 3, 4, 5};
            REQUIRE(temp.is_small() == false);
            REQUIRE(!temp.empty());
            REQUIRE(temp.size() == 5);
            REQUIRE(temp[0] == 1);
            REQUIRE(temp[1] == 2);
            REQUIRE(temp[2] == 3);
            REQUIRE(temp[3] == 4);
            REQUIRE(temp[4] == 5);
        }

        {
            auto temp = SmallVec<int, 10>{1, 2, 3, 4, 5};
            REQUIRE(temp.is_small());
            REQUIRE(!temp.empty());
            REQUIRE(temp.size() == 5);
            REQUIRE(temp[0] == 1);
            REQUIRE(temp[1] == 2);
            REQUIRE(temp[2] == 3);
            REQUIRE(temp[3] == 4);
            REQUIRE(temp[4] == 5);
        }
    }
}

TEST_CASE("Small Vector", "[small_vec:iterator]") {
    {
        auto temp = SmallVec<int, 0>{1, 2, 3, 4, 5};
        for (auto i = 0zu; auto const& el: temp) {
            REQUIRE(i + 1 == el);
            ++i;
        }
    }

    {
        auto temp = SmallVec<int, 0>{1, 2, 3, 4, 5};

        auto i = 5;
        for (auto it = temp.rbegin(); it != temp.rend(); ++it) {
            REQUIRE(i == *it);
            --i;
        }
    }

    {
        auto temp = SmallVec<int, 10>{1, 2, 3, 4, 5};
        for (auto i = 0zu; auto const& el: temp) {
            REQUIRE(i + 1 == el);
            ++i;
        }
    }

    {
        auto const temp = SmallVec<int, 10>{1, 2, 3, 4, 5};

        auto i = 5;
        for (auto it = temp.rbegin(); it != temp.rend(); ++it) {
            REQUIRE(i == *it);
            --i;
        }
    }
}

TEST_CASE("Small Vector", "[small_vec:modfying]") {
    SECTION("Pushing at the back of the vector") {
        {
            auto temp = SmallVec<int, 5>{1, 2, 3, 4, 5};
            REQUIRE(temp.is_small());
            REQUIRE(!temp.empty());
            REQUIRE(temp.size() == 5);

            temp.push_back(6);
            REQUIRE(temp.is_small() == false);
            REQUIRE(temp.size() == 6);
        }

        {
            auto temp = SmallVec<int, 5>{1, 2, 3, 4};
            REQUIRE(temp.is_small());
            REQUIRE(!temp.empty());
            REQUIRE(temp.size() == 4);

            temp.push_back(5);
            REQUIRE(temp.is_small());
            REQUIRE(temp.size() == 5);

            temp.push_back(6);
            REQUIRE(temp.is_small() == false);
            REQUIRE(temp.size() == 6);

            for (auto i = 0zu; i < temp.size(); ++i) {
                REQUIRE(i + 1 == temp[i]);
            }
        }
    }

    SECTION("Popping from the back of the vector") {
        {
            auto temp = SmallVec<int, 4>{1, 2, 3, 4, 5};
            REQUIRE(temp.is_small() == false);
            REQUIRE(!temp.empty());
            REQUIRE(temp.size() == 5);

            auto v = temp.back();
            temp.pop_back();
            REQUIRE(temp.is_small() == false);
            REQUIRE(temp.size() == 4);
            REQUIRE(v == 5);
        }

        {
            auto temp = SmallVec<int, 5>{1, 2, 3, 4, 5};
            REQUIRE(temp.is_small());
            REQUIRE(!temp.empty());
            REQUIRE(temp.size() == 5);

            auto v = temp.back();
            temp.pop_back();
            REQUIRE(temp.is_small());
            REQUIRE(temp.size() == 4);
            REQUIRE(v == 5);
        }
    }

    SECTION("Removing a range") {
        {
            auto temp = SmallVec<int, 5>{1, 2, 3, 4, 5};
            REQUIRE(temp.is_small());
            REQUIRE(!temp.empty());
            REQUIRE(temp.size() == 5);

            temp.erase(temp.begin() + 1, temp.begin() + 4);
            REQUIRE(temp.size() == 2);
            REQUIRE(temp[0] == 1);
            REQUIRE(temp[1] == 5);
        }

        {
            auto temp = SmallVec<int, 0>{1, 2, 3, 4, 5};
            REQUIRE(temp.is_small() == false);
            REQUIRE(!temp.empty());
            REQUIRE(temp.size() == 5);

            temp.erase(temp.begin() + 1, temp.begin() + 4);
            REQUIRE(temp.size() == 2);
            REQUIRE(temp[0] == 1);
            REQUIRE(temp[1] == 5);
        }
    }

    SECTION("Extending the vector") {
        {
            auto a = SmallVec<int, 0>{1, 2, 3};
            auto b = SmallVec<int, 0>{4, 5};

            auto sz = a.size();
            a.resize(sz + b.size());
            std::copy(b.begin(), b.end(), a.begin() + sz);
            REQUIRE(a.size() == 5);
            REQUIRE(a[0] == 1);
            REQUIRE(a[1] == 2);
            REQUIRE(a[2] == 3);
            REQUIRE(a[3] == 4);
            REQUIRE(a[4] == 5);
        }
        {
            auto a = SmallVec<int, 0>{1, 2, 3};
            auto b = SmallVec<int, 2>{4, 5};

            auto sz = a.size();
            a.resize(sz + b.size());
            std::copy(b.begin(), b.end(), a.begin() + sz);
            REQUIRE(a.size() == 5);
            REQUIRE(a[0] == 1);
            REQUIRE(a[1] == 2);
            REQUIRE(a[2] == 3);
            REQUIRE(a[3] == 4);
            REQUIRE(a[4] == 5);
        }
        {
            auto a = SmallVec<int, 5>{1, 2, 3};
            auto b = SmallVec<int, 0>{4, 5};

            auto sz = a.size();
            a.resize(sz + b.size());
            std::copy(b.begin(), b.end(), a.begin() + sz);
            REQUIRE(a.is_small());
            REQUIRE(a.size() == 5);
            REQUIRE(a[0] == 1);
            REQUIRE(a[1] == 2);
            REQUIRE(a[2] == 3);
            REQUIRE(a[3] == 4);
            REQUIRE(a[4] == 5);
        }
        {
            auto a = SmallVec<int, 3>{1, 2, 3};
            auto b = SmallVec<int, 2>{4, 5};

            auto sz = a.size();
            a.resize(sz + b.size());
            std::copy(b.begin(), b.end(), a.begin() + sz);
            REQUIRE(a.is_small() == false);
            REQUIRE(a.size() == 5);
            REQUIRE(a[0] == 1);
            REQUIRE(a[1] == 2);
            REQUIRE(a[2] == 3);
            REQUIRE(a[3] == 4);
            REQUIRE(a[4] == 5);
        }
    }

    SECTION("Inserting the vector") {
        {
            auto a = SmallVec<int, 0>{1, 2, 3};

            a.insert(a.begin(), 6);
            REQUIRE(a.size() == 4);
            REQUIRE(a.is_small() == false);
            REQUIRE(a[0] == 6);
            REQUIRE(a[1] == 1);
            REQUIRE(a[2] == 2);
            REQUIRE(a[3] == 3);
        }
        {
            auto a = SmallVec<int, 0>{1, 2, 3};

            a.insert(a.begin() + 3, 6);
            REQUIRE(a.size() == 4);
            REQUIRE(a.is_small() == false);
            REQUIRE(a[0] == 1);
            REQUIRE(a[1] == 2);
            REQUIRE(a[2] == 3);
            REQUIRE(a[3] == 6);
        }
        {
            auto a = SmallVec<int, 0>{1, 2, 3};

            a.insert(a.begin() + 1, 6);
            REQUIRE(a.size() == 4);
            REQUIRE(a.is_small() == false);
            REQUIRE(a[0] == 1);
            REQUIRE(a[1] == 6);
            REQUIRE(a[2] == 2);
            REQUIRE(a[3] == 3);
        }
        {
            auto a = SmallVec<int, 4>{1, 2, 3};

            a.insert(a.begin() + 0, 6);
            REQUIRE(a.size() == 4);
            REQUIRE(a.is_small());
            REQUIRE(a[0] == 6);
            REQUIRE(a[1] == 1);
            REQUIRE(a[2] == 2);
            REQUIRE(a[3] == 3);
        }
        {
            auto a = SmallVec<int, 4>{1, 2, 3};

            a.insert(a.begin() + 3, 6);
            REQUIRE(a.size() == 4);
            REQUIRE(a.is_small());
            REQUIRE(a[0] == 1);
            REQUIRE(a[1] == 2);
            REQUIRE(a[2] == 3);
            REQUIRE(a[3] == 6);
        }
        {
            auto a = SmallVec<int, 4>{1, 2, 3};

            a.insert(a.begin() + 1, 6);
            REQUIRE(a.size() == 4);
            REQUIRE(a.is_small());
            REQUIRE(a[0] == 1);
            REQUIRE(a[1] == 6);
            REQUIRE(a[2] == 2);
            REQUIRE(a[3] == 3);
        }
        {
            auto a = SmallVec<int, 3>{1, 2, 3};

            a.insert(a.begin() + 0, 6);
            REQUIRE(a.size() == 4);
            REQUIRE(a.is_small() == false);
            REQUIRE(a[0] == 6);
            REQUIRE(a[1] == 1);
            REQUIRE(a[2] == 2);
            REQUIRE(a[3] == 3);
        }
        {
            auto a = SmallVec<int, 3>{1, 2, 3};

            a.insert(a.begin() + 3, 6);
            REQUIRE(a.size() == 4);
            REQUIRE(a.is_small() == false);
            REQUIRE(a[0] == 1);
            REQUIRE(a[1] == 2);
            REQUIRE(a[2] == 3);
            REQUIRE(a[3] == 6);
        }
        {
            auto a = SmallVec<int, 3>{1, 2, 3};

            a.insert(a.begin() + 1, 6);
            REQUIRE(a.size() == 4);
            REQUIRE(a.is_small() == false);
            REQUIRE(a[0] == 1);
            REQUIRE(a[1] == 6);
            REQUIRE(a[2] == 2);
            REQUIRE(a[3] == 3);
        }
        {
            auto a = SmallVec<int, 3>{1, 3};

            a.insert(a.begin() + 1, 2);
            a.insert(a.begin() + 1, 6);
            REQUIRE(a.size() == 4);
            REQUIRE(a.is_small() == false);
            REQUIRE(a[0] == 1);
            REQUIRE(a[1] == 6);
            REQUIRE(a[2] == 2);
            REQUIRE(a[3] == 3);
        }
    }
}
