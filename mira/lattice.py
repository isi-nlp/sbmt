#!/usr/bin/env python

import json, collections

import sys
sys.path.append('/home/nlg-01/chiangd/hiero-mira/lib')
import svector

class Edge(object):
    def __init__(self, i, j, w, v={}):
        self.i = i
        self.j = j
        self.w = w
        self.v = svector.Vector(v)

    def __str__(self):
        return json.dumps(self.to_json(), ensure_ascii=False)

    def to_json(self):
        j_edge = {'i':self.i, 'j':self.j, 'w':self.w}
        if len(self.v) > 0:
            j_edge['v'] = dict(self.v.iteritems())
        return j_edge

class Span(object):
    def __init__(self, i, j, x, f, e, v={}):
        self.i = i
        self.j = j
        self.x = x
        self.f = f
        self.e = e
        self.v = svector.Vector(v)

    def __str__(self):
        return json.dumps(self.to_json(), ensure_ascii=False)

    def to_json(self):
        j_span = {'i':self.i, 'j':self.j, 'x':self.x, 'f':self.f, 'e':self.e}
        if len(self.v) > 0:
            j_span['v'] = dict(self.v.iteritems())
        return j_span

def get_words(words):
    if type(words) in [str, unicode]:
        words = words.split()
    return [word.encode('utf8') for word in words]

def get_vector(d):
    return dict((k.encode('utf8'), v) for (k,v) in d.iteritems())

class Lattice(object):
    def __init__(self):
        self.id = None
        self.edges = []
        self.spans = []
        self.n = 0

    @staticmethod
    def from_json(j_lat):
        if type(j_lat) is str:
            j_lat = json.loads(j_lat)

        lat = Lattice()
        lat.id = j_lat['id']

        n = 0
        for j_edge in j_lat['edges']:
            i, j = j_edge['i'], j_edge['j']
            n = max(n, j+1)
            w = j_edge['w'].encode('utf8')
            v = get_vector(j_edge.get('v', {}))
            lat.edges.append(Edge(i, j, w, v))
        for j_span in j_lat.get('spans',[]):
            i, j = j_span['i'], j_span['j']
            x = j_span['x'].encode('utf8')
            f = get_words(j_span['f'])
            e = get_words(j_span['e'])
            v = get_vector(j_span.get('v', {}))
            lat.spans.append(Span(i, j, x, f, e, v))
        lat.n = n

        # Make a list of words from the "canonical path."
        # This is needed by some models.
        lat.words = [None] * (n-1)
        for e in lat.edges:
            if e.i+1 == e.j:
                lat.words[e.i] = e.w

        return lat

    @staticmethod
    def from_words(ws, id=None):
        if type(ws) is str:
            ws = ws.split()
        lat = Lattice()
        lat.id = id
        lat.words = ws
        lat.n = len(ws)+1
        lat.edges = [Edge(i,i+1,ws[i]) for i in xrange(len(ws))]
        return lat

    def to_sbmt(self, tostring=None):
        """doesn't handle spans right now"""
        s = []
        s.append('lattice id="%d" {' % self.id)
        for edge in self.edges:
            w = edge.w
            if tostring:
                w = tostring(w)
            w = w.replace('\\','\\\\').replace('"','\\"')
            s.append('[%d,%d] "%s" ;' % (edge.i, edge.j, w))
        if len(self.edges) == 0:
            # SBMT doesn't like empty lattices
            s.append('[0,1] "*EMPTY*" ;')
            sys.stderr.write("warning: empty lattice, inserting dummy token\n")
        s.append('}')
        return '\n'.join(s)

    def to_json(self):
        j_lat = {'id': self.id}
        j_lat['edges'] = [edge.to_json() for edge in self.edges]
        if self.spans:
            j_lat['spans'] = [span.to_json() for span in self.spans]
        return j_lat

    def __str__(self):
        return json.dumps(self.to_json(), ensure_ascii=False)

    def compute_distance(self):
        # all pairs shortest paths
        # for heuristics that depend on distance
        edge_index = collections.defaultdict(list)
        for edge in self.edges:
            edge_index[edge.j].append(edge)

        distance = {}
        for i in xrange(self.n):
            distance[i,i] = 0
        for l in xrange(1, self.n):
            for i in xrange(self.n-l):
                j = i + l
                distance[i, j] = min(distance[i, edge.i] + 1 for edge in edge_index[j])

        return distance

if __name__ == "__main__":
    import fileinput, argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('inputs', metavar='FILE', nargs='*', help='input files')
    parser.add_argument('--input-words', dest='input', action='store_const', const='words')
    parser.add_argument('--input-json', dest='input', action='store_const', const='json')
    parser.add_argument('--output-json', dest='output', action='store_const', const='json')
    parser.add_argument('--output-sbmt', dest='output', action='store_const', const='sbmt')
    parser.add_argument('--rewrite-ids', dest='rewrite_ids', action='store_true')
    parser.set_defaults(input='words', output='json')

    args = parser.parse_args()
    
    for li, line in enumerate(fileinput.input(args.inputs)):
        if args.input == 'words':
            lat = Lattice.from_words(line, id=li)
        elif args.input == 'json':
            lat = Lattice.from_json(line)

        if args.rewrite_ids:
            lat.id = li

        if args.output == 'json':
            print json.dumps(lat.to_json(), ensure_ascii=False)
        elif args.output == 'sbmt':
            print lat.to_sbmt()

