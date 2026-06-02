import json
import random
from typing import Any


def generate_guest_pages(page_count: int, guest_count: int) -> dict[int, set[int]]:
    guest_pages = {i: set() for i in range(guest_count)}

    for page in range(page_count):
        guests_with_page = random.sample(range(guest_count), random.randint(1, guest_count))
        for guest in guests_with_page:
            guest_pages[guest].add(page)

    return guest_pages


def make_subtree(guest_unplaced_pages: dict[int, set[int]], guest_ancestor_pages: dict[int, set[int]],
                 min_node_degree: int, max_node_degree: int) -> dict[str, Any]:
    if len(guest_unplaced_pages) == 1:
        guest = next(iter(guest_unplaced_pages))
        return {
            "pages": list(guest_unplaced_pages[guest] - guest_ancestor_pages[guest]),
            "guest_pages": list(guest_ancestor_pages[guest] | guest_unplaced_pages[guest])
        }

    shared_pages = set.intersection(*guest_unplaced_pages.values()) if guest_unplaced_pages else set()
    for guest in guest_ancestor_pages:
        guest_ancestor_pages[guest].update(shared_pages)
        guest_unplaced_pages[guest].difference_update(shared_pages)

    st_count = min(len(guest_unplaced_pages), random.randint(min_node_degree, max_node_degree))
    sts = [(dict(), dict()) for _ in range(st_count)]

    guests = list(guest_unplaced_pages.keys())
    random.shuffle(guests)

    for i, guest in enumerate(guests):
        st_selected = i % st_count
        sts[st_selected][0][guest] = guest_unplaced_pages[guest]
        sts[st_selected][1][guest] = guest_ancestor_pages.get(guest, set()).copy()

    st_nodes = []
    for st_guest_unplaced_pages, st_guest_ancestor_pages in sts:
        if not st_guest_unplaced_pages:
            continue
        st_nodes.append(make_subtree(st_guest_unplaced_pages, st_guest_ancestor_pages,
                                     min_node_degree, max_node_degree))

    return {
        "pages": list(shared_pages),
        "children": st_nodes
    }


def main():
    random.seed(10)

    guest_pages = generate_guest_pages(page_count=40, guest_count=40)
    capacity = max(*(len(pages) for pages in guest_pages.values()))

    tree = make_subtree(guest_pages, {guest: set() for guest in guest_pages.keys()},
                        min_node_degree=2, max_node_degree=4)

    tree["capacity"] = capacity

    print(json.dumps(tree, indent=2))


if __name__ == "__main__":
    main()
