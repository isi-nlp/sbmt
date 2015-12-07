#!/bin/env python

# rs: respect space.
# sentence is in utf8
def segment_into_chars_rs(uline):
    l=[]
    for w in uline.split():
       if all(ord(ww)<128 for ww in w):
           #l.insert(0,w)
           l.append(w)
           #s=w.encode("utf-8");
       else:
           for c in w:
               #s=c.encode("utf-8");
               #l.insert(0,c)
               l.append(c)
    return l
