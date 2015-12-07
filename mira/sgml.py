#!/nfshomes/dchiang/pkg/python/bin/python2.4

# sgml.py
# David Chiang <chiang@isi.edu>

# Copyright (c) 2004-2006 University of Maryland. All rights
# reserved. Do not redistribute without permission from the
# author. Not for commercial use.

'''Decoder interface:

Dataset.process() expects a function, which in turn takes a Sentence as input
and produces a Sentence or list of Sentences as output.

The input Sentence will be marked with the <seg> tag it was found in
the input file with.

The output Sentences should be marked with <seg> tags if they are to
be marked as such in the output file.
'''

import sys, sgmllib, xml.sax.saxutils, log
import sym

def attrs_to_str(d):
    if len(d) == 0:
        return ""
    l = [""]+["%s=%s" % (name, xml.sax.saxutils.quoteattr(value)) for (name, value) in d]
    return " ".join(l)

def attrs_to_dict(a):
    d = {}
    for (name, value) in a:
	if d.has_key(name.lower()):
	    raise ValueError, "duplicate attribute names"
	d[name.lower()] = value
    return d

def strip_newlines(s):
    return " ".join(s.split())

class Sentence(object):
    def __init__(self, fwords=None, fmeta=None):
        if fwords is not None:
            self.fwords = list(fwords)
        else:
            self.fwords = []
        if fmeta is not None:
            self.fmeta = fmeta
        else:
            self.fmeta = []

    def copy(self):
        return Sentence(self.fwords, list(self.fmeta))

    def mark(self, tag, attrs):
        self.fmeta.append((tag, attrs, 0, len(self.fwords)))

    def getmark(self):
        if len(self.fmeta) > 0:
            (tag, attrs, i, j) = self.fmeta[-1]
            if i == 0 and j == len(self.fwords):
                return (tag, attrs)
            else:
                return None
        else:
            return None

    def unmark(self):
        mark = self.getmark()
        if mark is not None:
            self.fmeta = self.fmeta[:-1]
        return mark

    def __cmp__(self, other):
        return cmp((self.fwords, self.fmeta), (other.fwords, other.fmeta))

    def __str__(self):
        def cmp_spans((tag1,attr1,i1,j1),(tag2,attr2,i2,j2)):
            if i1==i2<=j1==j2:
                return 0
            elif i2<=i1<=j1<=j2:
                return -1
            elif i1<=i2<=j2<=j1:
                return 1
            else:
                return cmp((i1,j1),(i2,j2)) # don't care
        # this guarantees that equal spans will come out nested
        # we want the later spans to be outer
        # this relies on stable sort
        open = [[] for i in xrange(len(self.fwords)+1)]
        # there seems to be a bug still with empty spans
        empty = [[] for i in xrange(len(self.fwords)+1)]
        close = [[] for j in xrange(len(self.fwords)+1)]
        for (tag,attrs,i,j) in sorted(self.fmeta, cmp=cmp_spans):
            if i == j:
                # do we want these to nest?
                empty[i].append("<%s%s></%s>" % (tag, attrs_to_str(attrs), tag))
            else:
                open[i].append("<%s%s>" % (tag, attrs_to_str(attrs)))
                close[j].append("</%s>" % tag)

        result = []
        if len(empty[0]) > 0:
            result.extend(empty[0])
        for i in xrange(len(self.fwords)):
            if i > 0:
                result.append(" ")
            result.extend(reversed(open[i]))
            result.append(xml.sax.saxutils.escape(self.fwords[i]))
            result.extend(close[i+1])
            if len(empty[i+1]) > 0:
                result.extend(empty[i+1])

        return "".join(result)

    def __add__(self, other):
        if type(other) in (list, tuple):
            return Sentence(self.fwords + list(other), self.fmeta)
        else:
            othermeta = [(tag, attrs, i+len(self.fwords), j+len(self.fwords)) for (tag, attrs, i, j) in other.fmeta]
            return Sentence(self.fwords + other.fwords, self.fmeta+othermeta)

def read_raw(f):
    """Read a raw file into a list of Sentences."""
    if type(f) is str:
        f = file(f, "r")
    for (li,line) in enumerate(f):
        sent = process_sgml_line(line)
        mark = sent.getmark()
        if mark:
            tag, attrs = mark
            attrs = attrs_to_dict(attrs)
            if False and tag == "seg" and "id" in attrs:
                sent.id = attrs["id"]
            else:
                sent.id = str(li)
        else:
            sent.id = str(li)
        yield sent

def process_sgml_line(line):
    p = DatasetParser(None)
    p.pos = 0
    p.words = []
    p.meta = []
    p.feed(line)
    p.close()
    sent = Sentence(p.words, p.meta)
    return sent

class DatasetParser(sgmllib.SGMLParser):
    def __init__(self, set):
        sgmllib.SGMLParser.__init__(self)
	self.words = None
        self.mystack = []

    def handle_starttag(self, tag, method, attrs):
        thing = method(attrs)
        self.mystack.append(thing)

    def handle_endtag(self, tag, method):
        thing = self.mystack.pop()
        method(thing)

    def unknown_starttag(self, tag, attrs):
        thing = self.start(tag, attrs)
        self.mystack.append(thing)

    def unknown_endtag(self, tag):
        thing = self.mystack.pop()
        self.end(tag, thing)

    """# Special case for start and end of sentence
    def start_s(self, attrs):
        if self.words is not None:
            self.pos += 1
            self.words.append('<s>')
        return None

    def end_s(self, thing):
        if self.words is not None:
            self.pos += 1
            self.words.append('</s>')"""

    def start(self, tag, attrs):
        if self.words is not None:
            return (tag, attrs, self.pos, None)
        else:
            return None

    def end(self, tag, thing):
        if self.words is not None:
            (tag, attrs, i, j) = thing
            self.meta.append((tag, attrs, i, self.pos))

    def handle_data(self, s):
        if self.words is not None:
            words = s.split()
            self.pos += len(words)
	    self.words.extend(words)

