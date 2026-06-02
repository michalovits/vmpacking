#include <catch2/catch_test_macros.hpp>
#include <vmp_guest.h>
#include <vmp_host.h>

using namespace vmp;

TEST_CASE("Guest initialisation", "[guest]")
{
    const auto g = Guest({ 1, 2, 3 });
    CHECK(g.pages == std::unordered_set<int>{ 1, 2, 3 });
    CHECK(g.uniquePageCount() == 3);
}

TEST_CASE("Empty guest", "[guest]")
{
    const auto g = Guest({});
    CHECK(g.uniquePageCount() == 0);
    CHECK(g.pages.empty());
}

TEST_CASE("Guest::countUniquePagesOn", "[guest]")
{
    const auto onHost = Guest({ 1, 2, 3 });
    Host host(10);
    host.addGuest(&onHost);

    SECTION("no sharing")
    {
        const auto probe = Guest({ 4, 5 });
        CHECK(probe.countUniquePagesOn(host) == 0);
    }
    SECTION("partial sharing")
    {
        const auto probe = Guest({ 2, 3, 4 });
        CHECK(probe.countUniquePagesOn(host) == 2);
    }
    SECTION("full sharing")
    {
        const auto probe = Guest({ 1, 2 });
        CHECK(probe.countUniquePagesOn(host) == 2);
    }
    SECTION("empty")
    {
        const auto probe = Guest({});
        CHECK(probe.countUniquePagesOn(host) == 0);
    }
}
