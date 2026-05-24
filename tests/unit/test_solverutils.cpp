#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <instance_builders.h>
#include <vmp_guest.h>
#include <vmp_host.h>

#include <vmp_solverutils.h>

#include <memory>
#include <unordered_map>
#include <vector>

using namespace vmp;
using Catch::Approx;
using vmp::testing::makeGuest;
using vmp::testing::makeGuests;

TEST_CASE("calculateRelSize weights pages by frequency", "[solverutils]")
{
    const Guest guest({ 1, 2, 3, 4 });
    const std::unordered_map<int, int> pageFreq{ { 1, 1 }, { 2, 2 }, { 3, 4 }, { 4, 4 } };

    // 1/1 + 1/2 + 1/4 + 1/4
    CHECK(calculateRelSize(guest, pageFreq) == Approx(2.0));
}

TEST_CASE("calculateRelSize treats unseen pages as size one", "[solverutils]")
{
    const Guest guest({ 1, 2, 3 });

    CHECK(calculateRelSize(guest, {}) == Approx(3.0));
}

TEST_CASE("calculateSizeRelRatio divides unique pages by relative size", "[solverutils]")
{
    const Guest guest({ 1, 2, 3, 4 });
    const std::unordered_map<int, int> pageFreq{ { 1, 1 }, { 2, 2 }, { 3, 4 }, { 4, 4 } };

    // 4 / 2.0
    CHECK(calculateSizeRelRatio(guest, pageFreq) == Approx(2.0));
}

TEST_CASE("calculateSizeRelRatio is one when no pages are shared", "[solverutils]")
{
    const Guest guest({ 1, 2, 3 });

    CHECK(calculateSizeRelRatio(guest, {}) == Approx(1.0));
}

TEST_CASE("calculateOpportunityAwareEfficiency with a single host", "[solverutils]")
{
    const auto guest = makeGuest({ 1, 2, 3, 4 });

    auto host = std::make_shared<Host>(10);
    host->addGuest(makeGuest({ 1, 2 }));

    // (pagesOnHost = 2 + minDifference = 0) / sqrt(4)
    CHECK(calculateOpportunityAwareEfficiency(*guest, host, { host }) == Approx(1.0));
}

TEST_CASE("calculateOpportunityAwareEfficiency rewards distance from other hosts", "[solverutils]")
{
    const auto guest = makeGuest({ 1, 2, 3, 4 });

    auto host = std::make_shared<Host>(10);
    host->addGuest(makeGuest({ 1, 2 }));

    auto other = std::make_shared<Host>(10);
    other->addGuest(makeGuest({ 2, 5, 6 }));

    // (pagesOnHost = 2 + minDifference = 2) / sqrt(4)
    CHECK(calculateOpportunityAwareEfficiency(*guest, host, { host, other }) == Approx(2.0));
}

TEST_CASE("calculatePageFrequencies over guests", "[solverutils]")
{
    const auto guests = makeGuests({ { 1, 2 }, { 2, 3 }, { 3 } });

    const auto freq = calculatePageFrequencies(guests.begin(), guests.end());

    CHECK(freq.size() == 3);
    CHECK(freq.at(1) == 1);
    CHECK(freq.at(2) == 2);
    CHECK(freq.at(3) == 2);
}

TEST_CASE("calculatePageFrequencies over hosts", "[solverutils]")
{
    auto hostA = std::make_shared<Host>(10);
    hostA->addGuest(makeGuest({ 1, 2 }));
    hostA->addGuest(makeGuest({ 2, 3 }));

    auto hostB = std::make_shared<Host>(10);
    hostB->addGuest(makeGuest({ 3, 4 }));

    const auto hosts = std::vector<std::shared_ptr<const Host>>{ hostA, hostB };
    const auto freq = calculatePageFrequencies(hosts.begin(), hosts.end());

    CHECK(freq.size() == 4);
    CHECK(freq.at(1) == 1);
    CHECK(freq.at(2) == 2);
    CHECK(freq.at(3) == 2);
    CHECK(freq.at(4) == 1);
}
