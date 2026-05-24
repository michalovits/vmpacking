#include <catch2/catch_test_macros.hpp>
#include <instance_builders.h>
#include <vmp_guest.h>
#include <vmp_host.h>

using namespace vmp;
using vmp::testing::makeGuest;
using vmp::testing::makeGuests;

TEST_CASE("Host initialisation", "[host]")
{
    const Host host(5);
    CHECK(host.getCapacity() == 5);
    CHECK(host.getGuestCount() == 0);
    CHECK(host.getUniquePageCount() == 0);
    CHECK(host.getGuests().empty());
    CHECK(host.getPageFrequencies().empty());
    CHECK_FALSE(host.isOverfull());
}

TEST_CASE("Host::addGuest with unique pages", "[host]")
{
    Host host(10);
    const auto g = makeGuest({ 1, 2, 3 });

    host.addGuest(g);

    CHECK(host.hasGuest(g));
    CHECK(host.getGuestCount() == 1);
    CHECK(host.getUniquePageCount() == 3);
    CHECK(host.getPageFrequency(1) == 1);
    CHECK(host.getPageFrequency(2) == 1);
    CHECK(host.getPageFrequency(3) == 1);
    CHECK(host.getPageFrequency(99) == 0);
}

TEST_CASE("Host::addGuest with shared pages", "[host]")
{
    Host host(10);
    host.addGuest(makeGuest({ 1, 2 }));
    host.addGuest(makeGuest({ 2, 3 }));

    CHECK(host.getUniquePageCount() == 3);
    CHECK(host.getPageFrequency(1) == 1);
    CHECK(host.getPageFrequency(2) == 2);
    CHECK(host.getPageFrequency(3) == 1);
}

TEST_CASE("Host::addGuest makes overfull", "[host]")
{
    Host host(2);
    CHECK_FALSE(host.addGuest(makeGuest({ 1, 2 })));
    CHECK(host.addGuest(makeGuest({ 3 })));
    CHECK(host.isOverfull());
}

TEST_CASE("Host::addGuest -> ::removeGuest", "[host]")
{
    Host host(10);
    const auto g1 = makeGuest({ 1, 2 });
    const auto g2 = makeGuest({ 2, 3 });
    host.addGuest(g1);
    host.addGuest(g2);

    host.removeGuest(g1);
    host.addGuest(g1);
    host.removeGuest(g1);

    CHECK_FALSE(host.hasGuest(g1));
    CHECK(host.hasGuest(g2));
    CHECK(host.getGuestCount() == 1);
    CHECK(host.getUniquePageCount() == 2);
    CHECK(host.getPageFrequency(1) == 0);
    CHECK(host.getPageFrequency(2) == 1);
    CHECK(host.getPageFrequency(3) == 1);
}

TEST_CASE("Host page frequencies erase entries", "[host]")
{
    Host host(10);
    const auto g = makeGuest({ 1 });
    host.addGuest(g);
    host.removeGuest(g);

    CHECK_FALSE(host.getPageFrequencies().contains(1));
    CHECK(host.getUniquePageCount() == 0);
}

TEST_CASE("Host::clearGuests", "[host]")
{
    Host host(10);
    host.addGuest(makeGuest({ 1, 2 }));
    host.addGuest(makeGuest({ 3, 4 }));

    host.clearGuests();

    CHECK(host.getGuestCount() == 0);
    CHECK(host.getGuests().empty());
    CHECK(host.getUniquePageCount() == 0);
    CHECK(host.getPageFrequencies().empty());
}

TEST_CASE("Host::accommodatesGuest", "[host]")
{
    Host host(3);
    host.addGuest(makeGuest({ 1, 2 }));

    SECTION("under because shared")
    {
        const Guest probe({ 1, 2 });
        CHECK(host.accommodatesGuest(probe));
    }
    SECTION("exact")
    {
        const Guest probe({ 3 });
        CHECK(host.accommodatesGuest(probe));
    }
    SECTION("over")
    {
        const Guest probe({ 3, 4 });
        CHECK_FALSE(host.accommodatesGuest(probe));
    }
}

TEST_CASE("Host::accommodatesGuests ranged", "[host]")
{
    Host host(4);
    host.addGuest(makeGuest({ 1 }));

    const auto fits = makeGuests({ { 1, 2 }, { 2, 3 } });
    CHECK(host.accommodatesGuests(fits.begin(), fits.end()));

    const auto overflows = makeGuests({ { 5 }, { 6 }, { 7 }, { 8 } });
    CHECK_FALSE(host.accommodatesGuests(overflows.begin(), overflows.end()));
}

TEST_CASE("Host::countPagesWithGuest", "[host]")
{
    Host host(10);
    host.addGuest(makeGuest({ 1, 2, 3 }));

    SECTION("no sharing")
    {
        const Guest probe({ 4, 5 });
        CHECK(host.countPagesWithGuest(probe) == 5);
    }
    SECTION("partial sharing")
    {
        const Guest probe({ 2, 4 });
        CHECK(host.countPagesWithGuest(probe) == 4);
    }
    SECTION("full sharing")
    {
        const Guest probe({ 1, 2 });
        CHECK(host.countPagesWithGuest(probe) == 3);
    }
}

TEST_CASE("Host::countPagesNotOn", "[host]")
{
    Host host(10);
    host.addGuest(makeGuest({ 1, 2, 3 }));

    const Guest probe({ 2, 4 });
    CHECK(host.countPagesNotOn(probe) == 2);
}

TEST_CASE("Host::countPagesWithGuests ranged", "[host]")
{
    Host host(10);
    host.addGuest(makeGuest({ 1, 2 }));

    const auto probes = makeGuests({ { 2, 3 }, { 3, 4 } });
    CHECK(host.countPagesWithGuests(probes.begin(), probes.end()) == 4);
}

TEST_CASE("Host::isOverfull", "[host]")
{
    Host host(2);
    host.addGuest(makeGuest({ 1, 2 }));
    REQUIRE_FALSE(host.isOverfull());

    host.addGuest(makeGuest({ 3 }));
    CHECK(host.isOverfull());
}

TEST_CASE("Host::addGuests ranged", "[host]")
{
    Host host(10);
    const auto guests = makeGuests({ { 1 }, { 2 }, { 3 } });

    host.addGuests(guests.begin(), guests.end());

    CHECK(host.getGuestCount() == 3);
    CHECK(host.getUniquePageCount() == 3);
}
