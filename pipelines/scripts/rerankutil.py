#! /usr/bin/env python
import sys
import re

def list_to_dict(l, tuple_size=2, key=0, val=1):
    ''' given a list of items, form a dict out of it '''
    # http://stackoverflow.com/questions/4576115/python-list-to-dictionary
    return dict(zip(l[key::tuple_size], l[val::tuple_size]))

def parse_feat_string(string):
  '''
  given an isi-style string of space separated key=val pairs and key={{{entry with spaces}}} pairs, return
  a dict of those entries. meant to be used by various flavors, i.e. nbest list, rule
  '''
  feats={}
  spos=0
  entryre=re.compile(r"\s*(\S+)=((?:[^\s{}]+)|(?:{{{[^\}]*}}}))\s*")
  for match in entryre.findall(string):
    feats[match[0]]=match[1]
  return feats

def parse_nbest(string):
  '''
  given an isi-style nbest entry headed with NBEST, return the feature dictionary
  '''
  fields=string.split()
  if fields[0] != "NBEST":
    raise Exception("String should start with NBEST but starts with "+fields[0])
  return parse_feat_string(' '.join(fields[1:]))


