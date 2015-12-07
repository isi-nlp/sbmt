#! /usr/bin/env python
import argparse
import sys


def main():
  parser = argparse.ArgumentParser(description="turn output of decompose join into something easier for rootprob binary to handle")
  parser.add_argument("--infile", "-i", nargs='?', type=argparse.FileType('r'), default=sys.stdin, help="input file")
  parser.add_argument("--outfile", "-o", nargs='?', type=argparse.FileType('w'), default=sys.stdout, help="output file")



  try:
    args = parser.parse_args()
  except IOError, msg:
    parser.error(str(msg))

  for line in args.infile:
      fields = line.strip().split('\t')
      if len(fields) < 3:
        continue
      args.outfile.write(fields[1]+'\t'+fields[0]+" ### "+fields[2]+'\n')

if __name__ == '__main__':
  main()

