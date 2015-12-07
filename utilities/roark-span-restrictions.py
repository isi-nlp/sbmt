#!/usr/bin/env python

# lattice.py is found in the decoder-tools lib directory.  $PYTHONPATH should
# include this directory
import lattice

def edges(latline):
    """
    return all edges of a lattice, descending recursively into blocks
    """
    if hasattr(latline,"lines"):
        for line in latline.lines:
            for e in edges(line):
                yield e
    else:
        yield latline

def vertices(lat):
    """
    return all vertices encountered when traversing edges.
    not unique.
    """
    for e in edges(lat):
        yield e.span[0]
        yield e.span[1]

def spanstr(spn):
    """stringify a span in the form expected by lattice parser"""
    return "[" + str(spn[0]) + "," + str(spn[1]) + "]"

def restriction_vertices(lat,s,i):
    """
    restriction_vertices(lat,"s",0) is S1 vertices
    
    restriction_vertices(lat,"e",1) is E1 vertices

    for multiple edges entering/exiting a vertex, the conservative approach is
    taken.  only if all edges are restrictions will the vertex be a restriction

    another possible approach would be to re-write the lattice so that vertices
    are distinct based on which roark class they belong to
    """
    rm = {}
    for e in edges(lat):
        v = e.span[i]
        if e.properties.has_key("roark-restriction"):
            if e.properties["roark-restriction"].find(s) >= 0:
                try:
                    rm[v] = True and rm[v] 
                except:
                     rm[v] = True
            else:
                rm[v] = False
    for v in rm.keys():
        if rm[v]:
            yield v

def roark_restrictions(lat):
    """
    from a lattice with edges labelled by whether the word on the edge is
    in roarks E1 or S1 set, determine the only spans that a parser should process
    """
    # S1
    rsm = set(restriction_vertices(lat,"s",0))
    
    # E1
    rem = set(restriction_vertices(lat,"e",1))
    
    # spans that correspond to actual edges are the equivalent of length 1
    # spans for vanilla sentences.  we cant let these be omitted
    bas = set([e.span for e in edges(lat)])
    
    vtx = set(vertices(lat))
    
    for v1 in vtx:
        for v2 in vtx:
            if v1 < v2:
                if (v1 not in rsm and v2 not in rem) or (v1,v2) in bas:
                        yield (v1,v2)

def roark_restricted_lattice(lat):
    rs = " ".join(map(spanstr,set(roark_restrictions(lat))))
    if rs:
        lat.properties["span-restrictions"] = rs
    return lat

if __name__ == "__main__":
    import sys
    for lat in lattice.LatticeReader(file(sys.argv[1])):
        print roark_restricted_lattice(lat)
 

        