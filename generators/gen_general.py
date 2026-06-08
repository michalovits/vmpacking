"""
Generator for VM Packing general instances.

Think of the guest-page assignment as a bipartite graph. It's defined by two distributions:
- k_dist: how many pages each guest has
- d_dist: how many guests share each page

To generate:
- begin with each guest posting k_i ~ k_dist bids for pages
- pick a page and match it to d_p ~ d_dist guests, weighted by their number of active bids
- repeat until all bids are exhausted
"""

import random
from dataclasses import dataclass, field
from typing import Callable


@dataclass
class Guest:
    gid: int
    pages: list[int] = field(default_factory=list[int])


def gen_general(
    n_guests: int,
    k_dist: Callable[[], int],  # pages per guest (guest size)
    d_dist: Callable[[], int],  # guests per page (guest overlap)
) -> list[Guest]:
    guests = {g: Guest(gid=g) for g in range(n_guests)}

    bids = {g: k_dist() for g in range(n_guests)}
    page = 1

    while bids:
        bidders = list(bids.keys())
        n_matched = min(d_dist(), len(bidders))

        # select the bidder side
        # weighted for each bidder g by (g's active bids) / (all active bids)
        # without weighting, we would end up skewed;
        # the guests with few bids would go away quickly
        # and those with many bids would concentrate in the high pages
        # and so we'd lose the overlap distribution, d_dist -
        # we want to maintain a set of bidders that's roughly the same as the starting guest set!
        matched = sorted(bidders, key=lambda g: random.expovariate(bids[g]))[:n_matched]

        for g in matched:
            guests[g].pages.append(page)

            bids[g] -= 1
            if bids[g] == 0:
                del bids[g]

        page += 1

    return list(guests.values())


if __name__ == "__main__":
    import json

    random.seed(42)

    instance = gen_general(
        n_guests=10,
        k_dist=lambda: random.randint(3, 8),
        d_dist=lambda: random.choices([1, 5], weights=[0.7, 0.3])[0],
    )

    print(
        json.dumps(
            {
                "guests": [g.pages for g in instance],
                "capacity": max(len(g.pages) for g in instance),
            }
        )
    )
