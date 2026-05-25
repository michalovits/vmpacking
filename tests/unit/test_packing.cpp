#include <catch2/catch_test_macros.hpp>
#include <instance_builders.h>
#include <vmp_generaltopology.h>
#include <vmp_host.h>

#include <vmp_packing.h>

#include <memory>
#include <vector>

using namespace vmp;
using vmp::testing::makeGeneralInstance;
using vmp::testing::makeGuest;
using vmp::testing::makeGuests;

TEST_CASE("Packing initialisation", "[packing]")
{
    const auto guests = makeGuests({ { 1, 2 }, { 3, 4 }, { 5 } });

    auto h1 = std::make_shared<Host>(10);
    h1->addGuests(guests.begin(), guests.begin() + 2);

    auto h2 = std::make_shared<Host>(10);
    h2->addGuest(guests[2]);

    const Packing packing({ h1, h2 });

    CHECK(packing.hostCount() == 2);
    CHECK(packing.guestCount() == 3);
}

TEST_CASE("Empty Packing initialisation", "[packing]")
{
    const Packing packing({});

    CHECK(packing.hostCount() == 0);
    CHECK(packing.guestCount() == 0);
}

TEST_CASE("Packing::addHost updates counts", "[packing]")
{
    Packing packing({});

    const auto guests = makeGuests({ { 1 }, { 2 } });

    auto host = std::make_shared<Host>(10);
    host->addGuests(guests.begin(), guests.end());

    packing.addHost(host);

    CHECK(packing.hostCount() == 1);
    CHECK(packing.guestCount() == 2);
}

TEST_CASE("Packing::hosts accessor", "[packing]")
{
    auto host = std::make_shared<Host>(10);
    host->addGuest(makeGuest({ 1 }));

    Packing packing({ host });

    REQUIRE(packing.hosts().size() == 1);
    CHECK(packing.hosts()[0] == host);
}

TEST_CASE("Packing::validateForInstance completeness", "[packing]")
{
    const auto guests = makeGuests({ { 1, 2 }, { 3, 4 } });
    const GeneralInstance instance(10, guests);

    auto host = std::make_shared<Host>(10);
    host->addGuests(guests.begin(), guests.end());

    const Packing packing({ host });

    CHECK(packing.validateForInstance(instance) == PACKING_OKAY);
}

TEST_CASE("Packing::validateForInstance partial case", "[packing]")
{
    const auto guests = makeGuests({ { 1, 2 }, { 3, 4 } });

    SECTION("complete packing")
    {
        const GeneralInstance instance(10, guests);

        auto host = std::make_shared<Host>(10);
        host->addGuests(guests.begin(), guests.end());

        const Packing packing({ host });

        CHECK(packing.validateForInstance(instance) == PACKING_OKAY);
    }
    SECTION("partial packing")
    {
        const GeneralInstance instance(10, guests);

        auto host = std::make_shared<Host>(10);
        host->addGuest(guests[0]);

        const Packing packing({ host });

        CHECK(packing.validateForInstance(instance) == PACKING_PARTIAL);
    }
    SECTION("empty packing")
    {
        const GeneralInstance instance(10, guests);

        const Packing packing({});

        CHECK(packing.validateForInstance(instance) == PACKING_PARTIAL);
    }
}

TEST_CASE("Packing::validateForInstance empty instance", "[packing]")
{
    const auto instance = makeGeneralInstance(10, {});
    const Packing packing({});

    CHECK(packing.validateForInstance(instance) == PACKING_OKAY);
}

TEST_CASE("Packing::validateForInstance empty host", "[packing]")
{
    const auto guests = makeGuests({ { 1 } });
    const GeneralInstance instance(10, guests);

    auto placed = std::make_shared<Host>(10);
    placed->addGuest(guests[0]);
    auto empty = std::make_shared<Host>(10);

    const Packing packing({ placed, empty });

    CHECK(packing.validateForInstance(instance) == PACKING_HOST_EMPTY);
}

TEST_CASE("Packing::validateForInstance overfull host", "[packing]")
{
    const auto guests = makeGuests({ { 1, 2, 3 } });
    const GeneralInstance instance(2, guests);

    auto host = std::make_shared<Host>(2);
    host->addGuest(guests[0]);
    REQUIRE(host->isOverfull());

    const Packing packing({ host });

    CHECK(packing.validateForInstance(instance) == PACKING_HOST_OVERFULL);
}
