#! /usr/bin/env python
#SBATCH -p isi --time=12:00:00
#PBS -l walltime=5:00:00
#PBS -N rerank
#PBS -q isi

import argparse
import sys
import codecs
from collections import defaultdict as dd
import re
import tempfile
import numpy as np
import bleu
import itertools
import os
import pipes
import shutil
from rerankutil import list_to_dict
from rerankutil import parse_nbest

# turn off NIST normalization
def normalize(words):
    return words

bleu.normalize = normalize

# set eff ref len to match scoreTranslation behavior
bleu.eff_ref_len = "closest"


def main():
  scriptdir = os.path.dirname(os.path.abspath(__file__))
  parser = argparse.ArgumentParser(description="do full reranking and weight generation given data in its native pipeline form. Nbest should be amended with external features and sign should usually be positive")
  parser.add_argument("--infile", "-i", nargs='?', type=argparse.FileType('r'), default=sys.stdin, help="input file (nbest.sort)")
  parser.add_argument("--feats", "-f", nargs='+', default=[], help="set of feature names to explicitly tune. recommended values are text-length derivation-size lm1 lm2 as well as any new features")
  parser.add_argument("--weightfile", "-w", nargs='?', type=argparse.FileType('r'), help="weight file. comma separated, name:weight")
  parser.add_argument("--reference", "-r", nargs='+', type=argparse.FileType('r'), default=[], help="reference files")
  parser.add_argument("--randoms", type=int, default=0, help="number of random start points")
  parser.add_argument("--defaultweight", "-d", default="0", help="default start value for unseen features") # deliberately a string
  parser.add_argument("--outfile", "-o", nargs='?', type=argparse.FileType('w'), default=sys.stderr, help="output (weights) file")
  parser.add_argument("--bestfile", "-b", nargs='?', type=argparse.FileType('w'), default=sys.stdout, help="output (1-best) file")
  parser.add_argument("--mert", default=os.path.join(scriptdir, "..", "bin", "mert"), help="path to mert")
                      
  bleun = 4 # bleu to 4gram
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
  outfile = writer(args.outfile)
  bestfile = writer(args.bestfile)


  # set up temp dir and temp files
  workdir = tempfile.mkdtemp(prefix="rerank")
  # sys.stderr.write("Writing to "+workdir+"\n")
  startweights, startweightsname = tempfile.mkstemp(prefix="startweights", dir=workdir, text=True)
  startweights = os.fdopen(startweights, 'w')
  hypfile, hypfilename = tempfile.mkstemp(prefix="hyps", dir=workdir, text=True)
  hypfile = writer(os.fdopen(hypfile, 'w'))
  tunefile, tunefilename = tempfile.mkstemp(prefix="tune", dir=workdir, text=True)
  tunefile = os.fdopen(tunefile, 'w')
  mertweightfile, mertweightfilename = tempfile.mkstemp(prefix="mert", dir=workdir, text=True)
  mertweightfile = os.fdopen(mertweightfile, 'w')

  # get all feature weights
  weights = dd(lambda: args.defaultweight)
  weights.update(list_to_dict(re.findall(r"[^:,]+", weightfile.readline().strip())))
  # write tuned weights and new weights to temp file
  for feat in args.feats:
    startweights.write(weights[feat]+" ")
  # model score start
  startweights.write("1\n")

  # weight vector for model score
  modelweights = dict()
  for wname, wval in weights.iteritems():
    if wname not in args.feats:
      modelweights[wname] = float(wval)

  # write extra random weights to temp file
  for point in range(args.randoms):
    startweights.write(' '.join(map(str, np.random.rand(len(args.feats)+1)))+"\n")
  startweights.close()

  # cook references for comps
  cookedrefs = []
  for lines in itertools.izip(*(args.reference)):
        cookedrefs.append(bleu.cook_refs([line.split() for line in lines], n=bleun))

  
  for line in infile:
    prefeats = parse_nbest(line.strip())
    feats = dd(lambda: "0")
    feats.update(prefeats)
    hyp = feats[hypkey].lstrip("{").rstrip("}")
    sent = int(feats[sentkey])-1

    # write hyp to temp file
    hypfile.write(hyp+"\n")

    # write id, components, features to tuning file

    tunefile.write("%d ||| " % sent)

    # convert hyp to components using bleu stuff
    cook = bleu.cook_test(hyp.split(), cookedrefs[sent], n=bleun)
    for k in range(bleun):
      tunefile.write("%d " % cook["correct"][k])
      tunefile.write("%d " % cook["guess"][k])
    tunefile.write("%d ||| " % cook["reflen"])

    # pull out tuned features
    for feat in args.feats:
      tunefile.write(str(-(float(feats[feat])))+" ")
    # form model feature from untuned features
    modelscore = 0.0
    for fname, fval in feats.iteritems():
      if fname in modelweights:
        modelscore += -(float(fval))*modelweights[fname]
    tunefile.write("%f\n" % modelscore)
  hypfile.close()
  tunefile.close()

  # sys.stderr.write("Built mert file\n")
  # run mert on temp files
  # stuff about pipes: http://pymotw.com/2/pipes/
  mertpipe = pipes.Template()
  mertpipe.prepend("%s %s %s" % (args.mert, startweightsname, tunefilename), ".-")
  mertfile = mertpipe.open(None, 'r')
  # write learned weights
  mertweightfile.write(mertfile.read())
  mertweightfile.close()

  # read weights back in and keep the best one; write it out along with tuned feature names
  mertweightfile = open(mertweightfilename, 'r')
  bestweights = None
  bestweightscore = 0.0
  for line in mertweightfile:
    wstr, score = line.strip().split('|||')
    if float(score) > bestweightscore:
      bestweights = map(float, wstr.strip().split())
      bestweightscore = float(score)
  mertweightfile.close()
  outfile.write(' '.join(args.feats)+" MODEL\n")
  outfile.write(' '.join(map(str, bestweights))+"\n")
  outfile.write(str(bestweightscore)+"\n")
  outfile.close()
  # sys.stderr.write("Ran mert\n")
  tunefile = open(tunefilename, 'r')
  hypfile = reader(open(hypfilename, 'r'))
  currsent = "0"
  currbest = None
  currbestscore = float("inf")*-1
  # read feats back in
  # determine and write 1-best

  for linenum, (line, hyp) in enumerate(itertools.izip(tunefile, hypfile)):
    sid, _, feats = line.strip().split('|||')
    if sid.strip() != currsent:
      bestfile.write(currbest)
      currsent = sid.strip()
      currbest = None
      currbestscore = float("inf")*-1
    feats = map(float, feats.strip().split())
    score = np.dot(bestweights, feats)
    if score > currbestscore:
#      print "new best: "+str(score)+">"+str(currbestscore)
      currbest = hyp
      currbestscore = score
  bestfile.write(currbest)
  # clean up
  shutil.rmtree(workdir)

if __name__ == '__main__':
  main()
