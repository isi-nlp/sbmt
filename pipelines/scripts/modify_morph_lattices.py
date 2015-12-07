#!/usr/bin/env python

import sys
import itertools

import monitor
import re
import morphtable
import toposort

import lattice

class Error(Exception):
    """Base class for exceptions in this module."""
    pass

def make_identity(fw):    
    has_ascii_non_number = False
    for let in fw:
        if ord(let) >= 128:
            return None
        if not let.isdigit():
            has_ascii_non_number = True
    
    if has_ascii_non_number:
        return not re.match('-?[0-9.]+%?$', fw)
    else:
        return None

def escape(fw):
    #return re.sub(r'"', r'\"', re.sub(r'\\',r'\\\\', fw))
    return fw.replace('\\',r'\\').replace(r'"',r'\"')

def fix_edges(edges):
    # turn all "fake" (negative) verticies monotonically increasing numbers...
    # TODO:  this could probably be made more efficient by doing a topo sort
    #   directly, rather than using this (helpful) library

    verticies = set([])
    pairs = []
    for e in edges:
        verticies.add(e.span[0])
        verticies.add(e.span[1])
        pairs.append(e.span)
        
    sorted_verticies = toposort.topological_sort(verticies, pairs)
    if sorted_verticies == None:
        sys.stderr.write("cannot process lattice -- could there be a loop?\n")
        return [lattice.Vertex(id=0, label='error_in_lattice_toposort')]
    
    vertex_transform = {}
    for i in xrange(len(verticies)):
        vertex_transform[sorted_verticies[i]] = i

    return [lattice.Edge(span=(vertex_transform[e.span[0]],vertex_transform[e.span[1]]),label=e.label,properties=e.properties) for e in edges]

        
class LatticeAndSplitTask(object):
    """Task metadata holder for lattice-ifying input and affix split"""
    def __init__(self, opts):
        self.opts = opts
        self.linesExamined = 0
        self.linesSkipped = 0
        self.morphTable = None
        #self.f_infile = None
        self.f_outfile = sys.stdout
        
        #self.f_infile = open(self.infile)
        #self.f_outfile = open(self.outfile, "w")
            
        if opts.morphtable:
            self.morphTable = morphtable.read(opts.morphtable)


    def plaintext_to_lattice(self, infile):
        for fline in infile:
            edges = [lattice.Edge(span=(0,1), label='<foreign-sentence>')]
            vid = 1
            for fw in fline.split():
                edges.append(lattice.Edge(span=(vid, vid+1), label=fw, properties={'tok':'10^-1'}))
                vid += 1
            yield lattice.Lattice(lines=edges)  # TODO:  sentence id?


    def split_and_print(self, latticeIter):
        for lat in latticeIter:
            #print "DEBUG: " + str(lat)
            self.linesExamined += 1
            if self.linesExamined % 100 == 0:
                sys.stderr.write("%d sentences, %d skipped, %s per second\n" % (self.linesExamined, self.linesSkipped, float(self.linesExamined)/monitor.cpu()))
            edges = []
            initial_start = 0
            next_fake_id = -1  # when splitting words, we add fake id (which are negative) -- to be fixed later
            for fed in sorted(lat.lines, lambda x, y: cmp(x.span, y.span)):
                # only supports simple lattices right now
                # TODO:  change to support blocks?

                # add basic edge to edges list (ids will change later)
                edges.append(fed)

                sent_initial = fed.span[0] == initial_start
                if fed.span == (0,1) and fed.label == '<foreign-sentence>':
                    # foreign sentence is handled specially
                    initial_start = 1
                    continue
                    
                fw = fed.label
                remaining_fw = fw

                remaining_fw_start_pos, remaining_fw_end_pos = fed.span

                if opts.add_identity_arcs and make_identity(fw):
                    edges.append(lattice.Edge(span=(remaining_fw_start_pos, remaining_fw_end_pos), label=fw, properties={'identity':'10^-1','target':'NNP("' + fw + '")'}))

                if self.morphTable:
                    used = set([]) # to avoid double use of affixes
                    for m in self.morphTable.iter():
                        fanalysis = m.analyze_arabic(remaining_fw, sent_initial, used)
                        if fanalysis:
                            if fanalysis.prefixes and len(fanalysis.prefixes) > 0:
                                sent_initial = False  # only keep the sentence initial flag if there are no prefixes...?
                                for pr in fanalysis.prefixes:
                                    edges.append(lattice.Edge(span=(remaining_fw_start_pos, next_fake_id), label=pr, properties={'s_morph':'10^-1'}))
                                    remaining_fw_start_pos = next_fake_id
                                    next_fake_id -= 1  # fake id's "increase" in negative space
                                
                            if fanalysis.suffixes and len(fanalysis.suffixes) > 0:
                                fanalysis.suffixes.reverse()  # we want to walk backward over this list
                                for sf in fanalysis.suffixes:
                                    edges.append(lattice.Edge(span=(next_fake_id, remaining_fw_end_pos), label=sf, properties={'s_morph':'10^-1'}))
                                    remaining_fw_end_pos = next_fake_id
                                    next_fake_id -= 1  # fake id's "increase" in negative space

                            # place the baseword itself there
                            if fanalysis.baseword:                                
                                edges.append(lattice.Edge(span=(remaining_fw_start_pos, remaining_fw_end_pos), label=fanalysis.baseword, properties={'s_morph':'10^-1'}))
                                remaining_fw = fanalysis.baseword
                            else:
                                sys.stderr.write("Analysis of word '%s' missing baseword using morph transform %s" % (remaining_fw, str(m)))

            self.f_outfile.write(str(lattice.Lattice(lines=fix_edges(edges),properties=lat.properties)) + ";\n")
            
            #except Error, inst:
            #    print "Error encountered splitting affixes:", inst
            #    self.linesSkipped += 1
            #    if self.f_outfile:
            #        self.f_outfile.write(????)
            #    continue

        sys.stderr.write("%d sentences, %d skipped, %s per second\n" % (self.linesExamined, self.linesSkipped, float(self.linesExamined)/monitor.cpu()))
    
    def done(self):
        # nothing to do here (for now)
        a=0
        #if self.f_infile:
        #    self.f_infile.close()
        #if self.f_outfile and self.f_outfile != sys.stdout:
        #    self.f_outfile.close()

   
if __name__ == "__main__":
    import optparse

    optparser = optparse.OptionParser("usage: %prog [options] < input.f-lattice > input.f-lattice")
    optparser.add_option('-f', '--flat-input', dest='plaintext_input', action="store_true", help='input is flat plaintext, not a lattice')
    optparser.add_option('-i', '--identity-arcs', dest='add_identity_arcs', action="store_true", help='arcs for identity rules should be added to the input (lattice or plaintext')
    optparser.add_option('-m', '--morphological-affix-table', dest="morphtable", nargs=1, default='', help='table to specify Arabic affixes and their English equivalents (without this, lattice is created with no morphological splitting)')
    
    (opts, args) = optparser.parse_args()
    
#    if len(args) > 0:
#        if len(args) == 1:
#            # assume this is the input file
#             inputf = args[0]
#         else:
#            optparser.error("unexpected input, can specify input file as argument (or it reads STDIN)")

    task = LatticeAndSplitTask(opts)
    
    latticeIter = None
    if opts.plaintext_input:
        latticeIter = task.plaintext_to_lattice(sys.stdin)
    else:
        latticeIter = lattice.LatticeReader(sys.stdin)

    task.split_and_print(latticeIter)
    task.done()
    
