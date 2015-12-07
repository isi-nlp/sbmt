import getopt, sys, itertools

if __name__ == "__main__":
    opts, args = getopt.getopt(sys.argv[1:], "sn:")
    opts = dict(opts)
    n = int(opts['-n'])
    for line in sys.stdin:
        words = ['<s>']+line.split()+['</s>']
        if '-s' in opts and len(words) >= n:
            print " ".join(words[:n])
        else:
            for i in xrange(len(words)-n+1):
                print " ".join(words[i:i+n])
