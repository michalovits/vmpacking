#include <catch2/catch_test_macros.hpp>
#include <instance_builders.h>
#include <vmp_instance.h>

#include <unordered_set>

using namespace vmp;
using vmp::testing::makeInstance;

TEST_CASE("Instance initialisation", "[instance]")
{
    const auto instance = makeInstance(8, { { 1, 2 }, { 3, 4 }, { 5 } });

    CHECK(instance.capacity() == 8);
    CHECK(instance.guestCount() == 3);
    CHECK(instance.guests().size() == 3);
}

TEST_CASE("Instance preserves guest order", "[instance]")
{
    const auto instance = makeInstance(10, { { 1 }, { 2 }, { 3 } });

    REQUIRE(instance.guests().size() == 3);
    CHECK(instance.guests()[0]->pages == std::unordered_set{ 1 });
    CHECK(instance.guests()[1]->pages == std::unordered_set{ 2 });
    CHECK(instance.guests()[2]->pages == std::unordered_set{ 3 });
}

TEST_CASE("Empty Instance", "[instance]")
{
    const auto instance = makeInstance(5, {});
    CHECK(instance.capacity() == 5);
    CHECK(instance.guestCount() == 0);
    CHECK(instance.guests().empty());
}
