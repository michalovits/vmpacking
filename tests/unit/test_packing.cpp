#include <catch2/catch_test_macros.hpp>

#include <vmp_host.h>
#include <vmp_instance.h>

#include <vmp_packing.h>

#include <memory>
#include <vector>

using namespace vmp;

TEST_CASE("Packing initialisation", "[packing]")
{
    const auto g1 = Guest({ 1, 2 });
    const auto g2 = Guest({ 3, 4 });
    const auto g3 = Guest({ 5 });

    const auto guests = std::vector{ &g1, &g2, &g3 };

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
    const auto packing = vmp::Packing({});

    CHECK(packing.hostCount() == 0);
    CHECK(packing.guestCount() == 0);
}

TEST_CASE("Packing::addHost updates counts", "[packing]")
{
    auto packing = vmp::Packing({});

    const auto g1 = Guest({ 1 });
    const auto g2 = Guest({ 2 });
    const auto guests = std::vector{ &g1, &g2 };

    auto host = std::make_shared<Host>(10);
    host->addGuests(guests.begin(), guests.end());

    packing.addHost(host);

    CHECK(packing.hostCount() == 1);
    CHECK(packing.guestCount() == 2);
}

TEST_CASE("Packing::hosts accessor", "[packing]")
{
    const auto guest = Guest({ 1 });

    auto host = std::make_shared<Host>(10);
    host->addGuest(&guest);

    Packing packing({ host });

    REQUIRE(packing.hosts().size() == 1);
    CHECK(packing.hosts()[0] == host);
}

TEST_CASE("Packing::validateForInstance completeness", "[packing]")
{
    const auto instance = vmp::Instance(10, { Guest({ 1, 2 }), Guest({ 3, 4 }) });

    auto host = std::make_shared<Host>(10);
    host->addGuest(&instance.guests()[0]);
    host->addGuest(&instance.guests()[1]);

    const Packing packing({ host });

    CHECK(packing.validateForInstance(instance) == PACKING_OKAY);
}

TEST_CASE("Packing::validateForInstance partial case", "[packing]")
{
    const auto instance = vmp::Instance(10, { Guest({ 1, 2 }), Guest({ 3, 4 }) });

    SECTION("complete packing")
    {
        auto host = std::make_shared<Host>(10);
        host->addGuest(&instance.guests()[0]);
        host->addGuest(&instance.guests()[1]);

        const Packing packing({ host });

        CHECK(packing.validateForInstance(instance) == PACKING_OKAY);
    }
    SECTION("partial packing")
    {
        auto host = std::make_shared<Host>(10);
        host->addGuest(&instance.guests()[0]);

        const Packing packing({ host });

        CHECK(packing.validateForInstance(instance) == PACKING_PARTIAL);
    }
    SECTION("empty packing")
    {
        const Packing packing({});

        CHECK(packing.validateForInstance(instance) == PACKING_PARTIAL);
    }
}

TEST_CASE("Packing::validateForInstance empty instance", "[packing]")
{
    const auto instance = vmp::Instance(10, {});
    const auto packing = Packing({});

    CHECK(packing.validateForInstance(instance) == PACKING_OKAY);
}

TEST_CASE("Packing::validateForInstance empty host", "[packing]")
{
    const auto guest = Guest({ 1 });

    const auto instance = vmp::Instance(10, { guest });

    auto empty = std::make_shared<Host>(10);
    auto placed = std::make_shared<Host>(10);
    placed->addGuest(&guest);

    const Packing packing({ placed, empty });

    CHECK(packing.validateForInstance(instance) == PACKING_HOST_EMPTY);
}

TEST_CASE("Packing::validateForInstance overfull host", "[packing]")
{
    const auto g1 = Guest({ 1 });
    const auto g2 = Guest({ 2 });
    const auto g3 = Guest({ 3 });

    const auto instance = vmp::Instance(2, { g1, g2, g3 });

    auto host = std::make_shared<Host>(2);
    host->addGuest(&g1);
    host->addGuest(&g2);
    host->addGuest(&g3);

    REQUIRE(host->isOverfull());

    const Packing packing({ host });

    CHECK(packing.validateForInstance(instance) == PACKING_HOST_OVERFULL);
}
