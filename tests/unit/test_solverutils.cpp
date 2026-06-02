#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <vmp_guest.h>
#include <vmp_host.h>

#include <vmp_solverutils.h>

#include <memory>
#include <unordered_map>
#include <vector>

using namespace vmp;
using Catch::Approx;

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
    const auto g1 = vmp::Guest({ 1, 2, 3, 4 });
    const auto g2 = vmp::Guest({ 1, 2 });

    auto host = std::make_unique<Host>(10);
    host->addGuest(&g2);

    std::vector<std::unique_ptr<Host>> hosts;
    hosts.push_back(std::move(host));

    // (pagesOnHost = 2 + minDifference = 0) / sqrt(4)
    CHECK(calculateOpportunityAwareEfficiency(g1, *hosts[0], hosts) == Approx(1.0));
}

TEST_CASE("calculateOpportunityAwareEfficiency rewards distance from other hosts", "[solverutils]")
{
    const auto g1 = vmp::Guest({ 1, 2, 3, 4 });
    const auto g2 = vmp::Guest({ 1, 2 });
    const auto g3 = vmp::Guest({ 5, 6 });

    auto host = std::make_unique<Host>(10);
    host->addGuest(&g2);

    auto other = std::make_unique<Host>(10);
    other->addGuest(&g3);

    std::vector<std::unique_ptr<Host>> hosts;
    hosts.push_back(std::move(host));
    hosts.push_back(std::move(other));

    // (pagesOnHost = 2 + minDifference = 2) / sqrt(4)
    CHECK(calculateOpportunityAwareEfficiency(g1, *hosts[0], hosts) == Approx(2.0));
}

TEST_CASE("calculatePageFrequencies over guests", "[solverutils]")
{
    const auto g1 = vmp::Guest({ 1, 2 });
    const auto g2 = vmp::Guest({ 2, 3 });
    const auto g3 = vmp::Guest({ 3 });

    const auto guests = std::vector{ &g1, &g2, &g3 };

    const auto freq = calculatePageFrequencies(guests.begin(), guests.end());

    CHECK(freq.size() == 3);
    CHECK(freq.at(1) == 1);
    CHECK(freq.at(2) == 2);
    CHECK(freq.at(3) == 2);
}

TEST_CASE("calculatePageFrequencies over hosts", "[solverutils]")
{
    const auto g1 = vmp::Guest({ 1, 2 });
    const auto g2 = vmp::Guest({ 2, 3 });
    const auto g3 = vmp::Guest({ 3, 4 });

    auto hostA = std::make_unique<Host>(10);
    hostA->addGuest(&g1);
    hostA->addGuest(&g2);

    auto hostB = std::make_unique<Host>(10);
    hostB->addGuest(&g3);

    std::vector<std::unique_ptr<Host>> hosts;
    hosts.push_back(std::move(hostA));
    hosts.push_back(std::move(hostB));
    const auto freq = calculatePageFrequencies(hosts.begin(), hosts.end());

    CHECK(freq.size() == 4);
    CHECK(freq.at(1) == 1);
    CHECK(freq.at(2) == 2);
    CHECK(freq.at(3) == 2);
    CHECK(freq.at(4) == 1);
}
