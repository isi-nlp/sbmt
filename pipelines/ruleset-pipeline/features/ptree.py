# tree.py
# David Chiang <chiang@isi.edu>

# Copyright (c) 2004-2006 University of Maryland. All rights
# reserved. Do not redistribute without permission from the
# author. Not for commercial use.

import re
_tokenizer = re.compile(r"\(|\)|[^()\s]+")

class Node(object):
    "Tree node"
    def __init__(self, label, children=None):
        self.label = label

        # doubly-linked tree
        self.children = children or []
        self.parent = None

        # which child of my parent I am
        self.order = None

        # how many leaves are under me
        self.length = 0

        if children:
            for i in range(len(self.children)):
                self.children[i].parent = self
                self.children[i].order = i
                self.length += self.children[i].length

        if len(self.children) == 0:
            self.length = 1

    def __str__(self):
        if len(self.children) != 0:
            s = ["(", str(self.label)]
            for child in self.children:
                s.extend([" ", str(child)])
            s.append(")")
            return "".join(s)
        else:
            return self.label.replace("(", "-LRB-").replace(")", "-RRB-")

    def descendant(self, addr):
        if len(addr) == 0:
            return self
        else:
            return self.children[addr[0]].descendant(addr[1:])

    def insert_child(self, i, child):
        """Insert child at position i"""
        child.parent = self
        self.children[i:i] = [child]
        for j in range(i,len(self.children)):
            self.children[j].order = j
        if len(self.children) > 1:
            self.length += child.length
        else:
            self.length = child.length # because self.label changes into nonterminal

    def delete_child(self, i):
        """Delete child at position i. The child becomes a root node."""
        self.children[i].parent = None
        self.children[i].order = 0
        self.length -= self.children[i].length
        if i != -1:
            self.children[i:i+1] = []
        else:
            self.children[-1:] = []
        for j in range(i,len(self.children)):
            self.children[j].order = j

    def detach(self, clean=False):
        """Delete self from parent, becoming a root node. If clean is True,
           then any ancestors which are left childless are also deleted."""
        parent = self.parent
        parent.delete_child(self.order)
        if clean and len(parent.children) == 0:
            parent.detach(clean=True)
        
    def frontier(self):
        if len(self.children) > 0:
            for child in self.children:
                for leaf in child.frontier():
                    yield leaf
        else:
            yield self

    def preorder(self):
        yield self
        for child in self.children:
            for desc in child.preorder():
                yield desc

    def postorder(self):
        for child in self.children:
            for desc in child.postorder():
                yield desc
        yield self

    def is_terminal(self):
        return len(self.children) == 0

    def is_preterminal(self):
        return len(self.children) == 1 and self.children[0].is_terminal()

    def is_dominated_by(self, node):
        return self is node or (self.parent and self.parent.is_dominated_by(node))
    
    def dominates(self, *nodes):
        result = True
        for node in nodes:
            result = result and node.is_dominated_by(self)
        return result

    @staticmethod
    def from_str(s):
        tree, _ = Node._scan_tree(_tokenizer.findall(s), 0)
        return tree

    @staticmethod
    def _scan_tree(tokens, pos):
        try:
            if tokens[pos] == "(":
                if tokens[pos+1] == "(":
                    label = ""
                    pos += 1
                else:
                    label = tokens[pos+1]
                    pos += 2
                children = []
                child, pos = Node._scan_tree(tokens, pos)
                while child != None:
                    children.append(child)
                    (child, pos) = Node._scan_tree(tokens, pos)
                if tokens[pos] == ")":
                    return Node(label, children), pos+1
                else:
                    return None, pos
            elif tokens[pos] == ")":
                return (None, pos)
            else:
                label = tokens[pos]
                label = label.replace("-LRB-", "(")
                label = label.replace("-RRB-", ")")
                return Node(label), pos+1
        except IndexError:
            return None, pos


class TreeIndex(object):
    def __init__(self, root):
        self.root = root

        self.spans = {}
        i = 0
        for node in root.postorder():
            if node.is_terminal():
                self.spans[node] = (i,i+1)
                i += 1
            else:
                self.spans[node] = (self.spans[node.children[0]][0], self.spans[node.children[-1]][1])

    def _containing_node_helper(self, node, (i,j)):
        for child in node.children:
            ci, cj = self.spans[child]
            if ci <= i and j <= cj:
                return self._containing_node_helper(child, (i,j))

        return node

    def containing_node(self, (i,j)):
        return self._containing_node_helper(self.root, (i,j))

    def outside_nodes(self, outer, (i,j)):
        """Return minimal list of descendants of outer that exactly cover outer minus (i,j)"""
        nodes = []
        
        for child in node.children:
            ci, cj = self.spans[child]
            if cj <= i or j <= ci:
                nodes.append(child)
            elif not (i <= ci and ci <= j):
                nodes.extend(self.outside_nodes(child, (i,j)))

        return nodes
        
    def inside_nodes(self, (i,j)):
        """Return minimal list of nodes that exactly cover (i,j)"""
        nodes = []
        for child in node.children:
            ci, cj = self.spans[child]
            if i <= ci and cj <= j:
                nodes.append(child)
            elif not (cj <= i or j <= ci):
                nodes.extend(self.inside_nodes((i,j)))
        return nodes
