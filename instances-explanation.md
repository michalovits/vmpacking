# VM Packing Problem

The VM Packing problem is the overlap variant of Bin Packing.
In the VM Packing nomenclature, guests (items) are allocated to hosts (bins).
Each guest is a set of memory pages and we try to place them on as few hosts as possible.
Hosts have the same fixed memory capacity, and the memory footprint of a set of guests is the number of distinct pages in their union.
That is, a page appearing in multiple guests on the same host counts as though it appeared in only one.

## General Instance

An instance of this problem is simply a positive integer capacity and a set of guests, each being a set of pages.

```
type PageId  = Int
type GuestId = Int

Guest {
    gid   : GuestId
    pages : [PageId]
}

General = {
    capacity : Nat
    guests   : [Guest]
}
```

## Tree Instance

A **tree instance** is a model of a general instance that can be easier to work with.
It has the same guests and capacity, except we've pre-treated it by putting guests in a tree structure.
Each node of the tree holds some pages that are shared by all of its descendants.
Guests appear as leaves which contain the residual pages that aren't in the union of their ancestors' pages.

The intent is that you can model the instance using knowledge of the workloads within each guest VM.
Each inner node would map to the pages in some resident objects -- binaries, libraries, data, etc. -- and the guests that use them would be placed under that subtree.
As you might imagine, the model does not guarantee that the inner nodes capture all of the sharing that goes on, so it is inherently lossy.
Some leaves may need to have shared pages that don't appear in a common ancestor.

```
# Guests are leaves holding their residual pages.
# Each inner node holds the pages shared by all of its descendants.

Tree = Leaf { gid : GuestId, residual : [PageId] }
     | Node { shared : [PageId], children : [Tree] }

TreeInstance = {
    capacity : Nat
    tree     : Tree
}
```

## Cluster-Tree Instance

A **cluster-tree instance** takes a similar approach, but generalises the tree to a DAG.
Each guest is a sink node.
All guests that we can reach from a given node share the pages of said node.
In order for algorithms to operate on this structure, the nodes are grouped into an overlaid tree of clusters.
The DAG's nodes are partitioned into clusters, and the invariant is that the nodes in a given cluster can only have outgoing edges to nodes in its child clusters.

The idea is that you could model each cluster as a set of in-memory objects, and descendant guests may contain pages for some of those objects.
This is more fine-grained than the tree model, where the guests of a subtree share all the pages of the subtree root.
Of course this also means you could construct vacuous cluster-trees, where the descendants of a cluster don't actually have any nodes in common.

```
type NodeId    = Int
type ClusterId = Int

# A guest's pages are the union of the pages of every node that reaches it.
# Nodes are partitioned into clusters forming a tree.
# The invariant is that an edge of the DAG may only link a node to another in a child cluster.

CNode = CLeaf  { gid : GuestId, residual : [PageId] }  # a guest (a sink)
      | CInner { shared : [PageId] }

Cluster = {
    parent   : ClusterId
    children : [ClusterId]
    nodes    : [NodeId]
}

ClusterTree = {
    capacity    : Nat
    root        : ClusterId
    edges       : [(NodeId, NodeId)]  # (src, dst) where dst belongs to a child cluster
    for_cluster : Map ClusterId Cluster
    for_node    : Map NodeId CNode
}
```
