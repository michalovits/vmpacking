import json
import random


def generate_guest_pages(page_count: int, guest_count: int) -> dict[int, set[int]]:
    guest_pages = {i: set() for i in range(guest_count)}

    for page in range(page_count):
        guests_with_page = random.sample(range(guest_count), random.randint(1, guest_count))
        for guest in guests_with_page:
            guest_pages[guest].add(page)

    return guest_pages


def main():
    random.seed(10)

    guest_pages = generate_guest_pages(page_count=40, guest_count=40)
    capacity = max(*(len(pages) for pages in guest_pages.values()))

    print(json.dumps({
        "guests": [list(pages) for pages in guest_pages.values()],
        "capacity": capacity,
    }, indent=2))


if __name__ == "__main__":
    main()
