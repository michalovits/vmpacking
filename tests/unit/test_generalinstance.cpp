#include <catch2/catch_test_macros.hpp>
#include <instance_builders.h>
#include <vmp_generalinstance.h>

using namespace vmp;
using vmp::testing::makeGeneralInstance;
using vmp::testing::makeGuests;

TEST_CASE("GeneralInstance initialisation", "[generalinstance]")
{
    const auto instance = makeGeneralInstance(8, { { 1, 2 }, { 3, 4 }, { 5 } });

    CHECK(instance.getCapacity() == 8);
    CHECK(instance.getGuestCount() == 3);
    CHECK(instance.getGuests().size() == 3);
}

TEST_CASE("GeneralInstance preserves guest order", "[generalinstance]")
{
    const auto guests = makeGuests({ { 1 }, { 2 }, { 3 } });
    const GeneralInstance instance(10, guests);

    REQUIRE(instance.getGuests().size() == 3);
    CHECK(instance.getGuests()[0] == guests[0]);
    CHECK(instance.getGuests()[1] == guests[1]);
    CHECK(instance.getGuests()[2] == guests[2]);
}

TEST_CASE("Empty GeneralInstance", "[generalinstance]")
{
    const GeneralInstance instance(5, {});
    CHECK(instance.getCapacity() == 5);
    CHECK(instance.getGuestCount() == 0);
    CHECK(instance.getGuests().empty());
}
