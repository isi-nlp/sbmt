#!/usr/bin/env python
import os,sys
import chinese_utils
import BitVector
from collections import defaultdict

import greedy_reverse_seg 

# split bylines.
# construct lattice for each sentence.

class recursivedefaultdict(defaultdict):
    def __init__(self):
        self.default_factory = type(self) 

def get_env_var(name):
    if os.environ.get(name) == None:
        sys.stderr.write("No %s env variable defined!\n" % name)
        exit(1)
    else:
        return os.environ[name]

# just returns the dir name, no prefix.
def get_dirs(root):
    for path,subdirs,files in os.walk(root):
        for d in subdirs:
            yield d
        break

                             
#def construct_wordseg_lattice(corpus_in, lattice_out, sent_id):
def construct_wordseg_lattice(sent,dict,sentid):
    # reverse word segment the corupus_in
    sent_reverse_seg = greedy_reverse_seg.seg(sent,dict)
    # merge the lattices.
    segs = []
    segs.append(sent.rstrip())
    segs.append(sent_reverse_seg.rstrip())
    return construct_lattice_from_muliseg(segs, sentid)

def compute_char_belong_to(f):
    f_unicode=unicode(f, "utf-8")
    fwords=f_unicode.split()
    w_index=0
    ret=[]
    for w in fwords:
        chars=chinese_utils.segment_into_chars_rs(w)
        for c in chars:
            ret.append(w_index)
        w_index=w_index+1

    return ret


# compile f into  a list of pairs of indexes to chars, each pair correspondsing the 
# boundaries (half close and half open) of the word located by the list index.
def compile_word_boundaries(f):
    f_unicode=unicode(f,"utf-8")
    fwords=f_unicode.split()
    ret=[]
    char_index=0
    for w in fwords:
        chars=chinese_utils.segment_into_chars_rs(w)
        start=char_index
        for c in chars:
            char_index=char_index+1
        ret.append([start, char_index])

    return ret

def construct_bv(seg):
    useg=unicode(seg, 'utf-8')
    chars=chinese_utils.segment_into_chars_rs(useg)
    bv=BitVector.BitVector(size=len(chars)+1)
    for i in range(len(bv)): bv[i]=1
    bnds=compile_word_boundaries(seg)
    for b in bnds:
        bv[b[0]]=0
        bv[b[1]]=0

    return bv


def remap(multisegs):

    useg=unicode(multisegs[0], 'utf-8')
    chars=chinese_utils.segment_into_chars_rs(useg)

    # construct bit vector from each segmentation. with 0 representing a boundary.
    bvs=[]
    for seg in multisegs:
        bv=construct_bv(seg)
        bvs.append(bv)

    # & all the bvs, getting a new bv. This new bv represents the new segmentation.
    for i in range(1, len(bvs)):
        bvs[0] = bvs[0] & bvs[i]

    # returns a re-mapped (re-segmented) sentence.
    new_words=[]
    start=0
    for i in range(len(bvs[0])):
        if bvs[0][i]==0:
            if int(i)>start:
                new_words.append(''.join(chars[int(start):int(i)]))
                start=i

    unewseg=u' '.join(new_words)
    return unewseg.encode('utf-8')


def construct_lattice_from_muliseg(array, sent_id):
    newseg = remap(array)
    char_belong_to_newseg=compute_char_belong_to(newseg)


    out = "lattice id=\"%s\" {\n" % sent_id

    already_printed={}
    for f in array:
        word_bound=compile_word_boundaries(f)
        words=f.split()
        q=0
        for b in word_bound:
            windexes=[]
            for p in range(b[0], b[1]):
                windexes.append(char_belong_to_newseg[p])
            s="%s %s %s" % (windexes[0], windexes[-1], words[q])
            if not already_printed.has_key(s):
                w=words[q].replace("\\", "\\\\")
                w=w.replace("\"", "\\\"")
                out += "  [%s,%s]  \"%s\" ;\n" % (windexes[0], windexes[-1]+1, w)
                already_printed[s]=1
            q=q+1
    out += "};\n\n"
    return out

if __name__ == "__main__":
    bindir=os.path.dirname(sys.argv[0])
    words=os.path.join(bindir, 'chinese_wordlist')
    dict = greedy_reverse_seg.read_dict(words)
    id = 1
    for line in sys.stdin:
        #print '|'.join(x for x in unicode(line,'utf-8').split())
        # remove non-ascii unicode whitespace
        line = ' '.join(x for x in unicode(line,'utf-8').split()).encode('utf-8')
        print construct_wordseg_lattice(line,dict,id)
        id += 1
    


