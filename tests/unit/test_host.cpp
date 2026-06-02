#include <catch2/catch_test_macros.hpp>

#include <vmp_guest.h>
#include <vmp_host.h>

#include <vector>

using namespace vmp;

TEST_CASE("Host initialisation", "[host]")
{
    const Host host(5);
    CHECK(host.capacity() == 5);
    CHECK(host.guestCount() == 0);
    CHECK(host.uniquePageCount() == 0);
    CHECK(host.guests().empty());
    CHECK(host.pageFrequencies().empty());
    CHECK_FALSE(host.isOverfull());
}

TEST_CASE("Host::addGuest with unique pages", "[host]")
{
    Host host(10);
    const auto g = Guest({ 1, 2, 3 });

    host.addGuest(&g);

    CHECK(host.hasGuest(&g));
    CHECK(host.guestCount() == 1);
    CHECK(host.uniquePageCount() == 3);
    CHECK(host.pageFrequency(1) == 1);
    CHECK(host.pageFrequency(2) == 1);
    CHECK(host.pageFrequency(3) == 1);
    CHECK(host.pageFrequency(99) == 0);
}

TEST_CASE("Host::addGuest with shared pages", "[host]")
{
    const auto g1 = Guest({ 1, 2 });
    const auto g2 = Guest({ 2, 3 });

    Host host(10);
    host.addGuest(&g1);
    host.addGuest(&g2);

    CHECK(host.uniquePageCount() == 3);
    CHECK(host.pageFrequency(1) == 1);
    CHECK(host.pageFrequency(2) == 2);
    CHECK(host.pageFrequency(3) == 1);
}

TEST_CASE("Host::addGuest makes overfull", "[host]")
{
    const auto g1 = Guest({ 1, 2 });
    const auto g2 = Guest({ 3 });
    Host host(2);

    CHECK_FALSE(host.addGuest(&g1));
    CHECK(host.addGuest(&g2));
    CHECK(host.isOverfull());
}

TEST_CASE("Host::addGuest -> ::removeGuest", "[host]")
{
    Host host(10);
    const auto g1 = vmp::Guest({ 1, 2 });
    const auto g2 = vmp::Guest({ 2, 3 });
    host.addGuest(&g1);
    host.addGuest(&g2);

    host.removeGuest(&g1);
    host.addGuest(&g1);
    host.removeGuest(&g1);

    CHECK_FALSE(host.hasGuest(&g1));
    CHECK(host.hasGuest(&g2));
    CHECK(host.guestCount() == 1);
    CHECK(host.uniquePageCount() == 2);
    CHECK(host.pageFrequency(1) == 0);
    CHECK(host.pageFrequency(2) == 1);
    CHECK(host.pageFrequency(3) == 1);
}

TEST_CASE("Host page frequencies erase entries", "[host]")
{
    Host host(10);
    const auto g = vmp::Guest({ 1 });
    host.addGuest(&g);
    host.removeGuest(&g);

    CHECK_FALSE(host.pageFrequencies().contains(1));
    CHECK(host.uniquePageCount() == 0);
}

TEST_CASE("Host::clearGuests", "[host]")
{
    const auto g1 = Guest({ 1, 2 });
    const auto g2 = Guest({ 3, 4 });

    Host host(10);
    host.addGuest(&g1);
    host.addGuest(&g2);

    host.clearGuests();

    CHECK(host.guestCount() == 0);
    CHECK(host.guests().empty());
    CHECK(host.uniquePageCount() == 0);
    CHECK(host.pageFrequencies().empty());
}

TEST_CASE("Host::accommodatesGuest", "[host]")
{
    const auto g1 = Guest({ 1, 2 });

    Host host(3);
    host.addGuest(&g1);

    SECTION("under because shared")
    {
        const auto probe = Guest({ 1, 2 });
        CHECK(host.accommodatesGuest(probe));
    }
    SECTION("exact")
    {
        const auto probe = Guest({ 3 });
        CHECK(host.accommodatesGuest(probe));
    }
    SECTION("over")
    {
        const auto probe = Guest({ 3, 4 });
        CHECK_FALSE(host.accommodatesGuest(probe));
    }
}

TEST_CASE("Host::accommodatesGuests ranged", "[host]")
{
    const auto g1 = Guest({ 1, 2 });
    const auto g2 = Guest({ 2, 3 });

    Host host(4);
    host.addGuest(&g1);

    const auto fits = std::vector{ &g1, &g2 };
    CHECK(host.accommodatesGuests(fits.begin(), fits.end()));

    const auto of1 = Guest({ 5 });
    const auto of2 = Guest({ 6 });
    const auto of3 = Guest({ 7 });
    const auto of4 = Guest({ 8 });
    const auto overflows = std::vector{ &of1, &of2, &of3, &of4 };
    CHECK_FALSE(host.accommodatesGuests(overflows.begin(), overflows.end()));
}

TEST_CASE("Host::countPagesWithGuest", "[host]")
{
    const auto g = Guest({ 1, 2, 3 });

    Host host(10);
    host.addGuest(&g);

    SECTION("no sharing")
    {
        const auto probe = Guest({ 4, 5 });
        CHECK(host.countPagesWithGuest(probe) == 5);
    }
    SECTION("partial sharing")
    {
        const auto probe = Guest({ 2, 4 });
        CHECK(host.countPagesWithGuest(probe) == 4);
    }
    SECTION("full sharing")
    {
        const auto probe = Guest({ 1, 2 });
        CHECK(host.countPagesWithGuest(probe) == 3);
    }
}

TEST_CASE("Host::countPagesNotOn", "[host]")
{
    const auto g = Guest({ 1, 2, 3 });

    Host host(10);
    host.addGuest(&g);

    const auto probe = Guest({ 2, 4 });
    CHECK(host.countPagesNotOn(probe) == 2);
}

TEST_CASE("Host::countPagesWithGuests ranged", "[host]")
{
    const auto g = Guest({ 1, 2 });

    Host host(10);
    host.addGuest(&g);

    const auto probe1 = Guest({ 2, 3 });
    const auto probe2 = Guest({ 3, 4 });
    const auto probes = std::vector{ &probe1, &probe2 };
    CHECK(host.countPagesWithGuests(probes.begin(), probes.end()) == 4);
}

TEST_CASE("Host::isOverfull", "[host]")
{
    const auto g1 = Guest({ 1, 2 });

    Host host(2);
    host.addGuest(&g1);
    REQUIRE_FALSE(host.isOverfull());

    const auto g2 = Guest({ 3 });
    host.addGuest(&g2);
    CHECK(host.isOverfull());
}

TEST_CASE("Host::addGuests ranged", "[host]")
{
    const auto g1 = Guest({ 1 });
    const auto g2 = Guest({ 2 });
    const auto g3 = Guest({ 3 });
    const auto guests = std::vector{ &g1, &g2, &g3 };

    Host host(10);
    host.addGuests(guests.begin(), guests.end());

    CHECK(host.guestCount() == 3);
    CHECK(host.uniquePageCount() == 3);
}
