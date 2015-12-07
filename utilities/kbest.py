import sys

sys.path.append('/home/nlg-02/pust/decode-tools-trunk/x86_64/lib')

import forest
import mmap

fname = sys.argv[1]
wname = sys.argv[2]

ffile = open(fname,"r+")
fmap = mmap.mmap(ffile.fileno(),0)
wfile = open(wname,"r")

ignore = raw_input("load forest. [enter] to continue")

w = forest.read_features(wfile.readline())
f = forest.read_forest(fmap)
fmap.close()

ignore = raw_input("forest loaded.  [enter] to continue")

klist = forest.kbest(f,w)

try:
    for k in klist:
        print k
except IOError:
    pass

