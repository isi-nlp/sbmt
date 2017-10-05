#!/usr/bin/env python

import re, math, sys

def token_spans(tokenized_vector, original_string):
    offset = 0
    ret = []
    for tok in tokenized_vector:
        choices = [tok]
        
        if tok[0] == '@':
            for t in choices[:]:
                choices.append(t[1:])
        if tok[-1] == '@':
            for t in choices[:]:
                choices.append(t[:-1])
        s = -1
        e = -1
        for c in choices:
            x = original_string.find(c)
            if x >= 0:
                if s == -1:
                    s = x
                    e = x + len(c)
                elif s == x:
                     e = max(e, x + len(c))
                elif x < s:
                    s = x
                    e = x + len(c)
        if s == -1:
            print >> sys.stderr, ret, choices, original_string
            raise Exception("non matching in token_spans")
        ret.append((tok,offset + s, offset + e))
        offset = offset + e
        original_string = original_string[e:]
        #print >> sys.stderr, ret, choices, original_string
    return ret

if __name__ == "__main__":
    untok = "@wow@ this isn't an un-tokenized string, see what you make of that."
    tok = "@wow@ this is @n@ @'t an un-@ tokenized string @, see what you make of that ."
    print token_spans(tok.split(), untok)
