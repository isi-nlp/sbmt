#!/usr/bin/env python

# need to set PYTHONPATH to point to this file and _lattice.so
# need to set LD_LIBRARY_PATH to point to your Boost library directory and to libgusc.so

#import gusc
import _lattice

def LatticeReader(f):
    """Iterate through the lattices contained in a file.
    
    Input: a file object.
    Output: an iterator over the lattices (type Lattice) contained in the file.
    """
    while True:
        lat = _lattice.AST()
        try:
            lat.read(f)
            yield Lattice(ast=lat)
        except EOFError:
            break

def _add_properties(c, d):
    for (k,v) in d.iteritems():
        c.add_property(k,v)

class Block(object):
    """Abstract syntax tree for a nested block.

    Attributes:
      properties  a dictionary mapping from keys to values.
      lines       a list of Blocks or Edges.
    """
    def __init__(self, lines=None, properties=None, ast=None):
        self.lines = lines if lines is not None else []
        self.properties = properties if properties is not None else {}
        
        if ast is not None:
            self.properties = dict(ast.properties())
            self.lines = [Block(ast=line) if line.is_block() else Edge(ast=line) for line in ast.lines()]

    def ast(self, past=None):
        if past is None:
            ast = _lattice.AST()
        else:
            ast = past.new_block()
        _add_properties(ast, self.properties)
        for line in self.lines:
            line.ast(ast)
        return ast

class Lattice(Block):
    """Abstract syntax tree for a lattice.

    Attributes:
      properties  a dictionary mapping from keys to values.
      lines       a list of Blocks or Edges.
      vertexes    a dictionary mapping from Vertex ids to Vertexes.
    """
    def __init__(self, lines=None, properties=None, vertexes=None, ast=None):
        self.vertexes = vertexes if vertexes is not None else {}

        if ast is not None:
            self.vertexes = dict((vertex.id, Vertex(ast=vertex)) for vertex in ast.vertex_infos())
            
        Block.__init__(self, lines=lines, properties=properties, ast=ast)

    def ast(self):
        ast = Block.ast(self)
        for vertex in self.vertexes.itervalues():
            vertex.ast(ast)
        return ast

    def __str__(self):
        """Returns a string representation in ISI sbmt format."""
        return str(self.ast())

class Vertex(object):
    """Abstract syntax tree for a vertex.

    Attributes:
      id         numeric id
      label      string label
      properties a dictionary mapping from keys to values.
      """
    def __init__(self, id=None, label="", properties=None, ast=None):
        self.id = id
        self.label = label
        self.properties = properties if properties is not None else {}
        
        if ast is not None:
            self.id = ast.id
            self.label = ast.label
            self.properties = dict(ast.properties())

    def ast(self, past):
        ast = past.new_vertex_info(self.id, self.label)
        _add_properties(ast, self.properties)
        return ast

class Edge(object):
    """Abstract syntax tree for an edge.

    Attributes:
      span       pair of vertex ids indicating start and stop
      label      string label
      properties a dictionary mapping from keys to values.
      """
    
    def __init__(self, span=None, label="", properties=None, ast=None):
        self.span = span
        self.label = label
        self.properties = properties if properties is not None else {}
        
        if ast is not None:
            self.span = tuple(ast.span)
            self.label = ast.label
            self.properties = dict(ast.properties())

    def ast(self, past):
        ast = past.new_edge(self.span[0], self.span[1], self.label)
        _add_properties(ast, self.properties)
        return ast

if __name__ == "__main__":
    import sys
    for lat in LatticeReader(file(sys.argv[1])):
        print lat

