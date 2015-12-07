#!/usr/bin/env python

# morphtable.py
# Steve DeNeefe <sdeneefe@isi.edu>

# Copyright (c) 2007-2008 University of Southern California. All rights
# reserved. Do not redistribute without permission from the author. Not
# for commercial use.

import sys
import re
import itertools

class Error(Exception):
    """Base class for exceptions in this module."""
    pass

class NotImplementedError(Error):
    """Error thrown by methods that aren't implemented in the base class."""
    pass

class MorphTable:
    """Table of Morphological Equivalents"""
    def __init__(self, morphlines):
        self.morphlines = morphlines

        #self.ara_lookup = {}
        #for m in self.morphlines:
        #    self.ara_lookup[m.arabic] = m
        
        # TODO: reverselookup by english string??

    def show(self):
        for i, m in itertools.izip(itertools.count(1), self.morphlines):
            print "%d. %s" % (i, m)

    def iter(self):
        for m in self.morphlines:
            yield m

            
def list_quoted_words(str):
    #OLD SCHOOL: return str.strip.split()
    qwords = []
    for qmatch in re.finditer('"(.*?)"', str):
        qwords.append(qmatch.group(1))
    if len(qwords) > 0:
        return qwords
    else:
        return []


class ArabicAnalysis:
    def __init__(self, prefixes=None, baseword=None, suffixes=None):
        self.prefixes = prefixes
        self.baseword = baseword
        self.suffixes = suffixes

        
class MorphEquivBase:
    """Arabic morphological affix and its English equivalent(s)"""
    def analyze_arabic(self, aword, beginning_of_sent, used = None):
        raise NotImplementedError('this method is not implemented')

    def apply_morph(self, elinks, flinks, fwords, ewords, used_array):
        raise NotImplementedError('this method is not implemented')


class NormTableStruct:
    def __init__(self, pline):
        tokens = pline.split()
        self.origWord = tokens[0]
        self.origCount = tokens[1]
        self.splitWord = tokens[2]
        self.splitCount = tokens[3]
        
        
class DeleteSentInitPrefix(MorphEquivBase):
    """Always strip sentence-initial prefix"""
    def __init__(self, args):
        self.prefix = args[0].strip()
        
    def __str__(self):
        return "strip all sentence initial prefixes of %s" % (self.prefix)
        
    def analyze_arabic(self, aword, beginning_of_sent, used = None):
        if beginning_of_sent and (used is None or self.prefix not in used):
            (emT, pref, new_word) = aword.partition(self.prefix)
            if new_word != None and len(new_word) > 0 and emT == None or len(emT) == 0:
                if used:
                    used.add(pref)
                return ArabicAnalysis(baseword=new_word)
        return None
    
    def apply_morph(self, elinks, flinks, fwords, ewords, used_array):
        # TODO:  will always examine first entry in fwords (could be a prefix), not the first actual word
        fanalysis = self.analyze_arabic(fwords[0], True, used_array[0])
        if fanalysis and fanalysis.baseword:
            sys.stderr.write("LOG: removed %s, %s -> %s\n" % (self.prefix, fwords[0], fanalysis.baseword))
            
            new_f = [fanalysis.baseword]
            new_f.extend(fwords[1:])
            return None, new_f
        return None, None
    
        
class TableNormalize(MorphEquivBase):
    """Normalize certain words (sentence-initial or full sentence) based on a lexicon table"""
    def __init__(self, sent_init_only, args):
        self.sent_init_only = sent_init_only
        self.normTableFN = args[0].strip()
        self.normTable = dict()
        normTableFile = open(self.normTableFN)
        for wline in normTableFile.readlines():
            ns = NormTableStruct(wline)
            self.normTable[ns.origWord] = ns
            
        normTableFile.close()
        
    def __str__(self):
        if self.sent_init_only:
            return "normalize sentence initial word based on file %s" % (self.normTableFN)
        else:
            return "normalize words in sentence based on file %s" % (self.normTableFN)
        
    def analyze_arabic(self, aword, beginning_of_sent, used = None):
        if (beginning_of_sent or not self.sent_init_only) and aword in self.normTable:
            #sys.stderr.write("aword=%s, orig_word=%s, split_word=%s\n" % (aword, self.normTable[aword].origWord, self.normTable[aword].splitWord))
            return ArabicAnalysis(baseword=self.normTable[aword].splitWord)
        return None
    
    def apply_morph(self, elinks, flinks, fwords, ewords, used_array):
        changes = False
        new_f = []
        fanalysis = self.analyze_arabic(fwords[0], True, used_array[0])
        if fanalysis and fanalysis.baseword:
            sys.stderr.write("LOG: normalized %s -> %s\n" % (fwords[0], fanalysis.baseword))
            changes = True
            new_f.append(fanalysis.baseword)
        else:
            new_f.append(fwords[0])
        
        if self.sent_init_only:
            if changes:
                new_f.extend(fwords[1:])
                return None, new_f
            else:
                return None, None
            
        for fw, u in izip(fwords[1:], used_array[1:]):
            fanalysis = self.analyze_arabic(fw, False, u)
            if fanalysis and fanalysis.baseword:
                sys.stderr.write("LOG: normalized %s -> %s\n" % (fw, fanalysis.baseword))
                changes = True
                new_f.append(fanalysis.baseword)
            else:
                new_f.append(fw)
            
        if changes:
            return None, new_f
        else:
            return None, None
      

class MorphEquivNaive(MorphEquivBase):
    """Do naive analysis of Arabic morphological prefixes with English equivalent(s)"""
    def __init__(self, args):
        arabic, eng, eng2 = args
        self.arabic = arabic.strip()
        self.english = list_quoted_words(eng)
        self.english2 = list_quoted_words(eng2)
        self.english_optional_anywhere = False
        self.english_optional_begin_sent = False
        if re.search('\*e\*', eng2):
            self.english_optional_anywhere = True
        if re.search('\*e-initial\*', eng2):
            self.english_optional_begin_sent = True
            
    def __str__(self):
        str = "naive: %s -> %s" % (self.arabic, self.english)

        if len(self.english2) > 0:
            str += " and sometimes %s" % (self.english2)
        if self.english_optional_anywhere:
            str += " or missing"
        if self.english_optional_begin_sent:
            str += " or missing at beginning of sentence"
        return str

    def matching_eng(self, ewords, at_sent_beginning):
        # compare ewords (list) to possible english used by the arabic affix
        # returns if matches and # of words used
        if len(ewords) == 0:
            return False, 0
        
        words_used = 0
        for e in self.english + self.english2:
            if e.startswith(ewords[0]):
                e_compare = e.split()
                ln = len(e_compare)
                if len(ewords) < ln:
                    # cannot be the same if its shorter
                    break
                if e_compare == ewords[:ln]:
                    return True, ln
                #match = True
                #for pos in range(ln):
                #    if e_compare[pos] != ewords[pos]:
                #        match = False
                #        break
                #if match:
                #    return True, ln
        if self.english_optional_anywhere or (self.english_optional_begin_sent and at_sent_beginning):
            return True, 0
        return False, 0

    def analyze_arabic(self, aword, beginning_of_sent, used = None):
        # returns split version if applicable, or empty array
        prefixes = []
        curr_word = aword

        if self.english_optional_begin_sent and not beginning_of_sent and len(self.english) == 0 and len(self.english2) == 0:
            return None
        if aword.startswith(self.arabic) and (used is None or self.arabic not in used):
            (emT, pref, new_word) = aword.partition(self.arabic)
            if new_word == None or len(new_word) == 0:
                # this "prefix" is the whole word, don't do anything
                return None

            #if self.delete_optional and ((self.english_optional_begin_sent and beginning_of_sent) or self.english_optional_anywhere)
            prefixes.append(self.arabic)
            if used:
                used.add(self.arabic)
            if emT != None and len(emT) > 0:
                sys.stderr.write("internal error -- unexpected result: emT value '%s' should be empty for word %s and prefix %s" % (emT, aword, pref))
            if pref != self.arabic:
                sys.stderr.write("internal error -- unexpected result: pref value '%s' should be '%s' for word %s" % (pref, self.arabic, aword))

            return ArabicAnalysis(prefixes=prefixes, baseword=new_word)

        return None
    
    def analyze_english(self, ewords, beginning_of_sent):
        # determines if arabic prefix corresponds
        # to a sequence of english words, and yields every english word
        # position after consuming the english words that correspond to the
        # arabic prefix

        curr_word_pos = 0
        at_sent_beginning = beginning_of_sent
        (matches, words_used) = self.matching_eng(ewords[curr_word_pos:], at_sent_beginning)
        at_sent_beginning = False
        if matches:
            curr_word_pos += words_used
            yield curr_word_pos

        return

    def apply_morph(self, elinks, flinks, fwords, ewords, used_array):
        # compute new alignments (and new f words, too)
        new_align = []
        new_f = []
        new_align_offset = 0
        sent_init = True
        used_offset = 0
        for fi in range(len(fwords)):
            fw = fwords[fi]
            u = used_array[fi + used_offset]
            fanalysis = self.analyze_arabic(fw, sent_init, u)
            sent_init = False
            
            # if there are any possible affixes
            if fanalysis:
                rel_ep = flinks[fi]
                rel_ewds = [ewords[ei] for ei in rel_ep]
                # TODO:  should I include unaligned ewords, too?
                
                lastp = 0
                ap = 0  # TODO:  check to see if there are enough prefixes
                # don't check to see if un-affixed arabic matches rest or english (TODO: do this?)
                for ep in self.analyze_english(rel_ewds, fi == 0):
                    #if debugOutput:
                    #    new_f.append('*' + fanalysis[ap])
                    #else:
                    new_f.append(fanalysis.prefixes[ap])
                    used_array.insert(fi + used_offset, None)  # insert None into the used array in this case
                    used_offset += 1

                    if lastp < ep:
                        #new_align.extend([[ei, fi + new_align_offset + ap, 1] for ei in rel_ep[lastp:ep]])
                        new_align.extend([(ei, fi + new_align_offset + ap) for ei in rel_ep[lastp:ep]])

                    lastp = ep
                    ap += 1

                new_f.append(''.join(fanalysis.prefixes[ap:].append(fanalysis.baseword)))
                #new_align.extend([[ei, fi + new_align_offset + ap, 0] for ei in rel_ep[lastp:]])
                new_align.extend([(ei, fi + new_align_offset + ap) for ei in rel_ep[lastp:]])
                new_align_offset += ap
                             
            else:
                # len(fanalysis) == 0  (TODO:  assert this?)
                new_f.append(fw)
                #new_align.extend([[ei, fi + new_align_offset, 0] for ei in flinks[fi]])
                new_align.extend([(ei, fi + new_align_offset) for ei in flinks[fi]])
            
        return new_align, new_f
    

class MorphEquivBasic(MorphEquivBase):
    """Do naive analysis of Arabic morphological prefixes with English equivalent(s)"""
    def __init__(self, args):
        arabic, eng, eng2 = args
        self.arabic = arabic.strip()
        self.english = list_quoted_words(eng)
        self.english2 = list_quoted_words(eng2)
        self.english_optional_anywhere = False
        self.english_optional_begin_sent = False
        if re.search('\*e\*', eng2):
            self.english_optional_anywhere = True
        if re.search('\*e-initial\*', eng2):
            self.english_optional_begin_sent = True
            
    def __str__(self):
        str = "basic: %s -> %s" % (self.arabic, self.english)

        if len(self.english2) > 0:
            str += " and sometimes %s" % (self.english2)
        if self.english_optional_anywhere:
            str += " or missing"
        if self.english_optional_begin_sent:
            str += " or missing at beginning of sentence"
        return str

    def matching_eng(self, ewords, at_sent_beginning):
        # compare ewords (list) to possible english used by the arabic affix
        # returns if matches and # of words used
        if len(ewords) == 0:
            return False, 0
        
        words_used = 0
        for e in self.english + self.english2:
            if e.startswith(ewords[0]):
                e_compare = e.split()
                ln = len(e_compare)
                if len(ewords) < ln:
                    # cannot be the same if its shorter
                    break
                if e_compare == ewords[:ln]:
                    return True, ln
                #match = True
                #for pos in range(ln):
                #    if e_compare[pos] != ewords[pos]:
                #        match = False
                #        break
                #if match:
                #    return True, ln
        if self.english_optional_anywhere or (self.english_optional_begin_sent and at_sent_beginning):
            return True, 0
        return False, 0

    def analyze_arabic(self, aword, beginning_of_sent, used = None):
        # returns split version if applicable, or empty array
        prefixes = []
        curr_word = aword

        if self.english_optional_begin_sent and not beginning_of_sent and len(self.english) == 0 and len(self.english2) == 0:
            return None
        if aword.startswith(self.arabic) and (used is None or self.arabic not in used):
            (emT, pref, new_word) = aword.partition(self.arabic)
            if new_word == None or len(new_word) == 0:
                # this "prefix" is the whole word, don't do anything
                return None

            #if self.delete_optional and ((self.english_optional_begin_sent and beginning_of_sent) or self.english_optional_anywhere)
            prefixes.append(self.arabic)
            if used:
                used.add(self.arabic)
            if emT != None and len(emT) > 0:
                sys.stderr.write("internal error -- unexpected result: emT value '%s' should be empty for word %s and prefix %s" % (emT, aword, pref))
            if pref != self.arabic:
                sys.stderr.write("internal error -- unexpected result: pref value '%s' should be '%s' for word %s" % (pref, self.arabic, aword))

            return ArabicAnalysis(prefixes=prefixes, baseword=new_word)

        return None
    
    def analyze_english(self, ewords, beginning_of_sent):
        # determines if arabic prefix corresponds
        # to a sequence of english words, and yields every english word
        # position after consuming the english words that correspond to the
        # arabic prefix

        curr_word_pos = 0
        at_sent_beginning = beginning_of_sent
        (matches, words_used) = self.matching_eng(ewords[curr_word_pos:], at_sent_beginning)
        at_sent_beginning = False
        if matches:
            curr_word_pos += words_used
            yield curr_word_pos

        return

    def apply_morph(self, elinks, flinks, fwords, ewords, used_array):
        # compute new alignments (and new f words, too)
        new_align = []
        new_f = []
        new_align_offset = 0
        sent_init = True
        used_offset = 0
        for fi in range(len(fwords)):
            fw = fwords[fi]
            u = used_array[fi + used_offset]
            fanalysis = self.analyze_arabic(fw, sent_init, u)
            sent_init = False
            
            # if there are any possible affixes
            if fanalysis:
                # follow alignments until we have the complete set of linked words
                print "found prefix on arabic word '%s' (pos %d)" % (fw, fi)

                old_ep = set([])
                rel_ep = set(flinks[fi])
                old_fp = set([])
                total_fp = set([fi])
                while old_ep != rel_ep or old_fp != total_fp:
                    old_fp = total_fp
                    old_ep = rel_ep
                    rel_ep = reduce(lambda x, y: x.union(set(y)), [flinks[fp] for fp in total_fp], set([]))
                    total_fp = reduce(lambda x, y: x.union(set(y)), [elinks[ep] for ep in rel_ep], set([]))

                rel_ep = sorted(rel_ep)  # make back into sorted list
                rel_ewds = [ewords[ei] for ei in rel_ep]
                alignments_to_remove = []
                # TODO:  should I include unaligned ewords, too?
                
                lastp = 0
                ap = 0 # TODO:  check to see if there are enough prefixes
                # don't check to see if un-affixed arabic matches rest or english (TODO: do this?)
                for ep in self.analyze_english(rel_ewds, fi == 0):
                    #if debugOutput:
                    #    new_f.append('*' + fanalysis[ap])
                    #else:

                    new_f.append(fanalysis.prefixes[ap])
                    used_array.insert(fi + used_offset, None)  # insert None into the used array in this case
                    used_offset += 1

                    if lastp < ep:
                        #new_align.extend([[ei, fi + new_align_offset + ap, 1] for ei in rel_ep[lastp:ep]])
                        new_align.extend([(ei, fi + new_align_offset + ap) for ei in rel_ep[lastp:ep]])
                        alignments_to_remove.extend(rel_ep[lastp:ep])
                        
                    lastp = ep
                    ap += 1

                rest = []
                if fanalysis.prefixes:
                    rest.extend(fanalysis.prefixes[ap:])
                if fanalysis.baseword:
                    rest.append(fanalysis.baseword)
                new_f.append(''.join(rest))

                if ap > 0:
                    sys.stderr.write("LOG: split %s -> %s + %s\n" % (fw, new_f[-2], new_f[-1]))  # only shows last prefix
            
                #new_align.extend([[ei, fi + new_align_offset + ap, 0] for ei in rel_ep[lastp:]])
                new_align.extend([(ei, fi + new_align_offset + ap) for ei in rel_ep[lastp:]])
                new_align_offset += ap

                # remove any future elinks or flinks (that have now been split)
                if len(total_fp) > 1:
                    for fp in total_fp:
                        if fp != fi:
                            for ei in alignments_to_remove:
                                if ei in flinks[fp]:
                                    flinks[fp].remove(ei)
                                if fp in elinks[ei]:
                                    elinks[ei].remove(fp)

            else:
                # len(fanalysis) == 0  (TODO:  assert this?)
                new_f.append(fw)
                #new_align.extend([[ei, fi + new_align_offset, 0] for ei in flinks[fi]])
                new_align.extend([(ei, fi + new_align_offset) for ei in flinks[fi]])
            
        return new_align, new_f



class MorphEquivSmarter(MorphEquivBase):
    """Do basic analysis of Arabic morphological prefixes with English equivalent(s)"""
    def __init__(self, args):
        arabic, eng, eng2 = args
        self.arabic = arabic.strip()
        self.english = list_quoted_words(eng)
        self.english2 = list_quoted_words(eng2)
        self.english_optional_anywhere = False
        self.english_optional_begin_sent = False
        if re.search('\*e\*', eng2):
            self.english_optional_anywhere = True
        if re.search('\*e-initial\*', eng2):
            self.english_optional_begin_sent = True
            
    def __str__(self):
        str = "smarter: %s -> %s" % (self.arabic, self.english)

        if len(self.english2) > 0:
            str += " and sometimes %s" % (self.english2)
        if self.english_optional_anywhere:
            str += " or missing"
        if self.english_optional_begin_sent:
            str += " or missing at beginning of sentence"
        return str

    def matching_eng(self, ewords, at_sent_beginning):
        # compare ewords (list) to possible english used by the arabic affix
        # returns if matches and # of words used
        if len(ewords) == 0:
            return False, 0
        
        words_used = 0
        for e in self.english + self.english2:
            if e.startswith(ewords[0]):
                e_compare = e.split()
                ln = len(e_compare)
                if len(ewords) < ln:
                    # cannot be the same if its shorter
                    break
                if e_compare == ewords[:ln]:
                    return True, ln
                #match = True
                #for pos in range(ln):
                #    if e_compare[pos] != ewords[pos]:
                #        match = False
                #        break
                #if match:
                #    return True, ln
        if self.english_optional_anywhere or (self.english_optional_begin_sent and at_sent_beginning):
            return True, 0
        return False, 0

    def analyze_arabic(self, aword, beginning_of_sent, used = None):
        # returns split version if applicable, or empty array
        prefixes = []
        curr_word = aword

        if self.english_optional_begin_sent and not beginning_of_sent and len(self.english) == 0 and len(self.english2) == 0:
            return None
        if aword.startswith(self.arabic) and (used is None or self.arabic not in used):
            (emT, pref, new_word) = aword.partition(self.arabic)
            if new_word == None or len(new_word) == 0:
                # this "prefix" is the whole word, don't do anything
                return None

            #if self.delete_optional and ((self.english_optional_begin_sent and beginning_of_sent) or self.english_optional_anywhere)
            prefixes.append(self.arabic)
            if used:
                used.add(self.arabic)
            if emT != None and len(emT) > 0:
                sys.stderr.write("internal error -- unexpected result: emT value '%s' should be empty for word %s and prefix %s" % (emT, aword, pref))
            if pref != self.arabic:
                sys.stderr.write("internal error -- unexpected result: pref value '%s' should be '%s' for word %s" % (pref, self.arabic, aword))

            return ArabicAnalysis(prefixes=prefixes, baseword=new_word)

        return None

    def internal_analyze_arabic(self, aword, beginning_of_sent, used = None):
        fanalysis = self.analyze_arabic(aword, beginning_of_sent, used)
        if fanalysis:
            return fanalysis.prefixes + fanalysis.baseword
        return []
    
    def analyze_english(self, ewords, beginning_of_sent):
        # determines if arabic prefix corresponds
        # to a sequence of english words, and yields every english word
        # position after consuming the english words that correspond to the
        # arabic prefix

        curr_word_pos = 0
        at_sent_beginning = beginning_of_sent
        (matches, words_used) = self.matching_eng(ewords[curr_word_pos:], at_sent_beginning)
        at_sent_beginning = False
        if matches:
            curr_word_pos += words_used
            yield curr_word_pos

        return

    def apply_morph(self, elinks, flinks, fwords, ewords, used_array):
        # compute new alignments (and new f words, too)
        new_align = []
        new_f = []
        new_align_offset = 0
        used_offset = 0
        for grp in self.fphrase_iter(elinks, flinks, fwords, ewords):
            fw = grp.fwords[0]
            fi = grp.fwordnum[0]
            u = used_array[fi + used_offset]
            sent_init = fi == 0

            fanalysis = self.analyze_arabic(fw, sent_init, u)
            

            # OLD:  (TODO -- remove and re-write rest of function)
            fanalysis = self.internal_analyze_arabic(fw, sent_init, u)
            
            # if there are any possible affixes
            if len(fanalysis) > 1:
                rel_ep = grp.elinks
                rel_ewds = grp.ewords
                # TODO:  should I include unaligned ewords, too?
                
                lastp = 0
                ap = 0
                # don't check to see if un-affixed arabic matches rest or english (TODO: do this?)
                for ep in self.analyze_english(rel_ewds, sent_init):
                    #if debugOutput:
                    #    new_f.append('*' + fanalysis[ap])
                    #else:
                    new_f.append(fanalysis[ap])
                    used_array.insert(fi + used_offset, None)  # insert None into the used array in this case
                    used_offset += 1
                    
                    if lastp < ep:
                        #new_align.extend([[ei, fi + new_align_offset + ap, 1] for ei in rel_ep[lastp:ep]])
                        new_align.extend([(ei, fi + new_align_offset + ap) for ei in rel_ep[lastp:ep]])

                    lastp = ep
                    ap += 1

                new_f.append(''.join(fanalysis[ap:]))
                #new_align.extend([[ei, fi + new_align_offset + ap, 0] for ei in rel_ep[lastp:]])
                new_align.extend([(ei, fi + new_align_offset + ap) for ei in rel_ep[lastp:]])
                new_align_offset += ap
                             
            else:
                # len(fanalysis) == 0  (TODO:  assert this?)
                new_f.append(fw)
                #new_align.extend([[ei, fi + new_align_offset, 0] for ei in flinks[fi]])
                new_align.extend([(ei, fi + new_align_offset) for ei in flinks[fi]])

            sent_init = False
            
        return new_align, new_f




def read(fn):
    # read morphological affix equivalents from file
    mlist = []
    mfile = open(fn)
    for mline in mfile.readlines():
        linetokens = mline.split('#')
        algorithm = linetokens.pop(0).strip()
        if algorithm == "naive_search":
            mlist.append(MorphEquivNaive(linetokens))
        elif algorithm == "basic_search":
            mlist.append(MorphEquivBasic(linetokens))
        elif algorithm == "smarter_search":
            mlist.append(MorphEquivSmarter(linetokens))
        elif algorithm == "delete_sent_init_prefix":
            mlist.append(DeleteSentInitPrefix(linetokens))
        elif algorithm == "table_normalize_sent_init":
            mlist.append(TableNormalize(True, linetokens)) # True = sent init only
        elif algorithm == "table_normalize":
            mlist.append(TableNormalize(False, linetokens)) # False = full sent
        elif algorithm != "none":  # placeholder to ignore for now
            sys.stderr.write("Unknown morph search algorithm '%s'\n" % (algorithm))

    mfile.close()
    
    return MorphTable(mlist)

   
if __name__ == "__main__":
    import optparse

    optparser = optparse.OptionParser()
    (opts, args) = optparser.parse_args()

    # test this
    tableFN = 'morph.table'
    if len(args) > 0:
        tableFN = args[0]
    mt = read(tableFN)
    mt.show()

    # TODO: tests are broken!!!
#     print
#     print '--- test analyze_arabic ---'
#     aword_list = '\xd9\x88\xd9\x85\xd9\x86 \xd8\xa7\xd9\x84\xd9\x85\xd9\x82\xd8\xb1\xd8\xb1 \xd8\xa7\xd9\x86 \xd8\xaa\xd8\xa8\xd8\xaf\xd8\xa7'.split()
#     for aw in aword_list:
#         print "analyzing '%s' as '%s'" % (aw, "', '".join(mt.analyze_arabic(aw)))
#         for m in mt.iter:
#             ana = m.analyze_arabic(remaining_fw, sent_initial)  # TO
#     print
#     print '--- test analyze_english ---'
#     for (aw, ews, bgsnt) in [['\xd9\x88\xd9\x85\xd9\x86', 'is', True], ['\xd8\xa7\xd9\x84\xd9\x85\xd9\x82\xd8\xb1\xd8\xb1', 'is', False], ['\xd9\x88\xd9\x82\xd8\xaf', 'and may', False]]:
#         ewl = ews.split()
#         aa = mt.analyze_arabic(aw)

#         new_alignments = []
#         lastp = 0
#         ap = 0
#         for ep in mt.analyze_english(aa,ewl,bgsnt):
#             new_alignments.append([aa[ap], ' '.join(ewl[lastp:ep])])
#             lastp = ep
#             ap += 1

#         new_alignments.append([' '.join(aa[ap:]), ' '.join(ewl[lastp:])])
#         print "analyzing '%s' <-> '%s' as '%s'" % (aw, ews, "', '".join(["%s' <-> '%s" % (a,e) for (a,e) in new_alignments]))
