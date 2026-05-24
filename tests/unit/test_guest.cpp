#include <catch2/catch_test_macros.hpp>
#include <instance_builders.h>
#include <vmp_guest.h>
#include <vmp_host.h>

using namespace vmp;
using vmp::testing::makeGuest;

TEST_CASE("Guest initialisation", "[guest]")
{
    const Guest g({ 1, 2, 3 });
    CHECK(g.pages == std::unordered_set<int>{ 1, 2, 3 });
    CHECK(g.uniquePageCount() == 3);
}

TEST_CASE("Empty guest", "[guest]")
{
    const Guest g({});
    CHECK(g.uniquePageCount() == 0);
    CHECK(g.pages.empty());
}

TEST_CASE("Guest::countUniquePagesOn", "[guest]")
{
    const auto onHost = makeGuest({ 1, 2, 3 });
    Host host(10);
    host.addGuest(onHost);

    SECTION("no sharing")
    {
        const Guest probe({ 4, 5 });
        CHECK(probe.countUniquePagesOn(host) == 0);
    }
    SECTION("partial sharing")
    {
        const Guest probe({ 2, 3, 4 });
        CHECK(probe.countUniquePagesOn(host) == 2);
    }
    SECTION("full sharing")
    {
        const Guest probe({ 1, 2 });
        CHECK(probe.countUniquePagesOn(host) == 2);
    }
    SECTION("empty")
    {
        const Guest probe({});
        CHECK(probe.countUniquePagesOn(host) == 0);
    }
}
