import json
import random
from dataclasses import dataclass
from itertools import combinations
from typing import Any

node_ids = [100]


@dataclass
class Node:
    id: int
    pages: set[int]
    ancestor_pages: set[int]
    parent_ids: list[int]


def make_node(parent_nodes: list[Node], target_page_count: int, instance_page_count: int) -> Node:
    node_id = node_ids[-1] + 1
    node_ids.append(node_id)

    if not parent_nodes:
        selected_pages = set(random.sample(range(0, instance_page_count), target_page_count))
        return Node(node_id, selected_pages, set(), [])

    max_selected_pages = set()
    max_selected_pages_ancestors = set()
    max_selected_pages_parent_ids = []

    for parents_count in range(len(parent_nodes) + 1, 1, -1):
        for parents in combinations(parent_nodes, parents_count):
            parent_ids = [parent.id for parent in parents]

            parents_pages = set().union(*(parent.pages | parent.ancestor_pages for parent in parents))
            available_pages = [page for page in range(0, instance_page_count) if page not in parents_pages]

            if target_page_count > len(available_pages) and target_page_count >= len(max_selected_pages):
                max_selected_pages = set(available_pages)
                max_selected_pages_ancestors = parents_pages
                max_selected_pages_parent_ids = parent_ids
                continue

            selected_pages = set(random.sample(available_pages, target_page_count))

            if len(available_pages) >= target_page_count:
                return Node(node_id, selected_pages, parents_pages, parent_ids)

    return Node(node_id, max_selected_pages, max_selected_pages_ancestors, max_selected_pages_parent_ids)


def make_cluster_subtree(
        guest_count: int,
        cluster_node_count: int,
        max_cluster_degree: int,
        max_node_page_count: int,
        instance_page_count: int,
        parent_nodes: list[Node] = [],
) -> (dict[str, Any], int):
    max_cluster_degree = min(guest_count, max_cluster_degree)
    assert guest_count != 0

    if guest_count == 1:
        node = make_node(parent_nodes, random.randint(0, max_node_page_count), instance_page_count)
        guest_pages = list(node.pages | node.ancestor_pages)
        return (
            {
                "cluster_children": [],
                "nodes": [{
                    "node_id": node.id,
                    "node_pages": list(node.pages),
                    "node_parents": node.parent_ids,
                    "guest_pages": guest_pages,
                }]
            },
            len(guest_pages))

    nodes = [make_node(parent_nodes, random.randint(0, max_node_page_count), instance_page_count) for _ in
             range(cluster_node_count)]

    cluster_guest_counts = [guest_count // max_cluster_degree + (1 if i < guest_count % max_cluster_degree else 0)
                            for i in range(max_cluster_degree)]

    clusters = [
        make_cluster_subtree(
            cluster_guest_count,
            min(cluster_guest_count, cluster_node_count),
            min(cluster_guest_count, max_cluster_degree),
            max_node_page_count,
            instance_page_count,
            nodes
        ) for cluster_guest_count in cluster_guest_counts
    ]

    return ({
                "nodes": [{
                    "node_id": node.id,
                    "node_parents": node.parent_ids,
                    "node_pages": list(node.pages),
                } for node in nodes],
                "cluster_children": [cluster[0] for cluster in clusters]
            },
            max(*(cluster[1] for cluster in clusters)))


def main():
    random.seed(10)
    cluster_tree, capacity = make_cluster_subtree(guest_count=40, cluster_node_count=2, instance_page_count=40,
                                                  max_node_page_count=5, max_cluster_degree=9)

    cluster_tree["capacity"] = capacity
    print(json.dumps(cluster_tree, indent=2))


if __name__ == "__main__":
    main()
