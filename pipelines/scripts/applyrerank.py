#! /usr/bin/env python
#PBS -l walltime=2:00:00
#PBS -N applyrerank
#PBS -q isi


import argparse
import sys
import codecs
from collections import defaultdict as dd
import re
from rerankutil import list_to_dict
from rerankutil import parse_nbest




def main():
  parser = argparse.ArgumentParser(description="given a set of reranking weights (which specify features), original weights, and an n-best list, do reranking and output the one-best")
  parser.add_argument("--infile", "-i", nargs='?', type=argparse.FileType('r'), default=sys.stdin, help="input file (nbest.sort)")
  parser.add_argument("--weightfile", "-w", nargs='?', type=argparse.FileType('r'), help="weight file as produced by mira")
  parser.add_argument("--rerankweightfile", "-k", nargs='?', type=argparse.FileType('r'), help="reranked weight file, as produced by runrerank")
  parser.add_argument("--bestfile", "-b", nargs='?', type=argparse.FileType('w'), default=sys.stdout, help="output (1-best) file")
                      
  hypkey = "hyp" # where the sentence text is
  sentkey = "sent" # where the sentence id is
  

  try:
    args = parser.parse_args()
  except IOError, msg:
    parser.error(str(msg))

  reader = codecs.getreader('utf8')
  writer = codecs.getwriter('utf8')
  infile = reader(args.infile)
  weightfile = reader(args.weightfile)
  rerankweightfile = reader(args.rerankweightfile)
  bestfile = writer(args.bestfile)

  # get all original feature weights
  weights = list_to_dict(re.findall(r"[^:,]+", weightfile.readline().strip()))
  # get all features not in core model
  rerankfeats = rerankweightfile.readline().strip().split()[:-1]
  # get weights used to choose onebest
  rerankweights = map(float, rerankweightfile.readline().strip().split())
  modelweight = rerankweights[-1]

  # weights for inner model score
  modelweights = dict()
  for wname, wval in weights.iteritems():
    if wname not in rerankfeats:
      modelweights[wname] = float(wval)

  currsent = 0
  currbest = None
  currbestscore = float("inf")*-1

  for line in infile:
    prefeats = parse_nbest(line.strip())
    feats = dd(lambda: "0")
    feats.update(prefeats)
    sent = int(feats[sentkey])-1
    if sent != currsent:
      bestfile.write(currbest)
      currsent = sent
      currbest = None
      currbestscore = float("inf")*-1
    hyp = feats[hypkey].lstrip("{").rstrip("}")+"\n"
    modelscore = 0.0
    for fname, fval in feats.iteritems():
      if fname in modelweights:
        modelscore += -(float(fval))*modelweights[fname]
    rerankmodelscore = modelscore*modelweight
    for wname, wval in zip(rerankfeats, rerankweights[:-1]):
      rerankmodelscore += -(float(feats[wname]))*wval
    if rerankmodelscore > currbestscore:
      currbest = hyp
      currbestscore = rerankmodelscore
  bestfile.write(currbest)

if __name__ == '__main__':
  main()
