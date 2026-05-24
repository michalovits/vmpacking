#include <catch2/catch_test_macros.hpp>
#include <instance_builders.h>
#include <vmp_generalinstance.h>

using namespace vmp;
using vmp::testing::makeGeneralInstance;
using vmp::testing::makeGuests;

TEST_CASE("GeneralInstance initialisation", "[generalinstance]")
{
    const auto instance = makeGeneralInstance(8, { { 1, 2 }, { 3, 4 }, { 5 } });

    CHECK(instance.capacity() == 8);
    CHECK(instance.guestCount() == 3);
    CHECK(instance.guests().size() == 3);
}

TEST_CASE("GeneralInstance preserves guest order", "[generalinstance]")
{
    const auto guests = makeGuests({ { 1 }, { 2 }, { 3 } });
    const GeneralInstance instance(10, guests);

    REQUIRE(instance.guests().size() == 3);
    CHECK(instance.guests()[0] == guests[0]);
    CHECK(instance.guests()[1] == guests[1]);
    CHECK(instance.guests()[2] == guests[2]);
}

TEST_CASE("Empty GeneralInstance", "[generalinstance]")
{
    const GeneralInstance instance(5, {});
    CHECK(instance.capacity() == 5);
    CHECK(instance.guestCount() == 0);
    CHECK(instance.guests().empty());
}
