#include <catch2/catch_test_macros.hpp>
#include <vmp_instance.h>

#include <unordered_set>

using namespace vmp;

TEST_CASE("Instance initialisation", "[instance]")
{
    const auto g1 = Guest({ 1, 2 });
    const auto g2 = Guest({ 3, 4 });
    const auto g3 = Guest({ 5 });

    const auto instance = vmp::Instance(8, { g1, g2, g3 });

    CHECK(instance.capacity() == 8);
    CHECK(instance.guestCount() == 3);
    CHECK(instance.guests().size() == 3);
}

TEST_CASE("Instance preserves guest order", "[instance]")
{
    const auto g1 = Guest({ 1 });
    const auto g2 = Guest({ 2 });
    const auto g3 = Guest({ 3 });

    const auto instance = vmp::Instance(10, { g1, g2, g3 });

    REQUIRE(instance.guests().size() == 3);
    CHECK(instance.guests()[0]->pages == std::unordered_set{ 1 });
    CHECK(instance.guests()[1]->pages == std::unordered_set{ 2 });
    CHECK(instance.guests()[2]->pages == std::unordered_set{ 3 });
}

TEST_CASE("Empty Instance", "[instance]")
{
    const auto instance = vmp::Instance(5, {});
    CHECK(instance.capacity() == 5);
    CHECK(instance.guestCount() == 0);
    CHECK(instance.guests().empty());
}
