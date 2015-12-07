#!/usr/bin/env python
import sys
import chinese_utils

# the maximum word length, in number of chars.
MAX_WORD_LEN=10

def strip_newline(s):
    return " ".join(s.split())

# l: list of characters (N.B. can contain tokens like 2002). 
# dict: the word list
# both in utf8 encoding.
def word_segment(l,dict):
    backtrace=[]
    for i in range(len(l)): 
        backtrace.append(0)

    i=0 # beginning position of the current word.
    while i < len(l):
        ss=-1.0
        for j in range(1,MAX_WORD_LEN+1): 
            if i+j >= len(l): k=len(l)
            else: k=i+j
            l2=l[i:k]
            l2.reverse()
            uword=''.join(l2)
            word = uword.encode("utf-8")
            if (word in dict):
                if  j > ss:
                    ss = k-i
            elif j==1: # this is the default case when no words are in the dict
                ss = 1
            backtrace[k-1]=i-1
        i=i+ss

    m=len(l)-1
    o=[]
    while m>=0:
        if backtrace[m]==-1: 
            w=''.join(l[m::-1])
        else:
            w=''.join(l[m:backtrace[m]:-1])
        o.append(w)
        m=backtrace[m]

    return o



# read the word list.
def read_dict(filename):
    dict = {}
    dict_file=open(filename, 'r')
    sys.stderr.write("loading word list ...\n")
    line=dict_file.readline()
    while line:
        line=strip_newline(line)
        fields=line.split()
        dict[fields[0]] = 1 #fields[1]
        line=dict_file.readline()
    dict_file.close()
    return dict


def seg(line,dict):
    ret = ""
    uline=unicode(line,"utf-8")
    l=chinese_utils.segment_into_chars_rs(uline)
    l.reverse()
    output=word_segment(l,dict)
    for i in range(len(output)): 
        ret += "%s " % output[i].encode("utf-8")

    return ret

if __name__ == "__main__":
    try:
        import psyco
        psyco.full()
    except ImportError:
        pass

    dict = read_dict(sys.argv[1])

    for line in sys.stdin:
        if line != "":
            sys.stdout.write(seg(line,dict))
            sys.stdout.write("\n")


