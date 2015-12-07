#include "Vocab.h"
#include "Ngram.h"
#include "NgramStats.h"

const LogP LogP_PseudoZero = -99.0;
#define DEBUG_READ_STATS 1

#include "FilterNgram.hpp"

FilterNgram::FilterNgram(Vocab &vocab, unsigned order) : Ngram(vocab, order) {}

Ngram *make_filter(File &file, Vocab &vocab, int order) {
  NgramStats intStats(vocab, order);
  intStats.countFile(file);
  Ngram *lm = new Ngram(vocab, order);
  lm->estimate(intStats);
  return lm;
}

// lifted out of NgramLM.cc
Boolean
FilterNgram::read(File &file, Ngram &filter)
{
  const Boolean limitVocab = 1;

    char *line;
    unsigned maxOrder = 0;	/* maximal n-gram order in this model */
    unsigned numNgrams[maxNgramOrder + 1];
				/* the number of n-grams for each order */
    unsigned numRead[maxNgramOrder + 1];
				/* Number of n-grams actually read */
    unsigned numOOVs = 0;	/* Numer of n-gram skipped due to OOVs */
    int state = -1 ;		/* section of file being read:
				 * -1 - pre-header, 0 - header,
				 * 1 - unigrams, 2 - bigrams, ... */
    Boolean warnedAboutUnk = false; /* at most one warning about <unk> */

    for (int i = 0; i <= maxNgramOrder; i++) {
	numNgrams[i] = 0;
	numRead[i] = 0;
    }

    clear();

    /*
     * The ARPA format implicitly assumes a zero-gram backoff weight of 0.
     * This has to be properly represented in the BOW trie so various
     * recursive operations work correctly.
     */
    VocabIndex nullContext[1];
    nullContext[0] = Vocab_None;
    *insertBOW(nullContext) = LogP_Zero;

    while (line = file.getline()) {
	
	Boolean backslash = (line[0] == '\\');

	switch (state) {

	case -1: 	/* looking for start of header */
	    if (backslash && strncmp(line, "\\data\\", 6) == 0) {
		state = 0;
		continue;
	    }
	    /*
	     * Everything before "\\data\\" is ignored
	     */
	    continue;

	case 0:		/* ngram header */
	    unsigned thisOrder;
	    int nNgrams;

	    if (backslash && sscanf(line, "\\%d-grams", &state) == 1) {
		/*
		 * start reading n-grams
		 */
		if (state < 1 || state > maxOrder) {
		    file.position() << "invalid ngram order " << state << "\n";
		    return false;
		}

	        if (debug(DEBUG_READ_STATS)) {
		    dout() << (state <= order ? "reading " : "skipping ")
			   << numNgrams[state] << " "
			   << state << "-grams\n";
		}

		continue;
	    } else if (sscanf(line, "ngram %d=%d", &thisOrder, &nNgrams) == 2) {
		/*
		 * scanned a line of the form
		 *	ngram <N>=<howmany>
		 * now perform various sanity checks
		 */
		if (thisOrder <= 0 || thisOrder > maxNgramOrder) {
		    file.position() << "ngram order " << thisOrder
				    << " out of range\n";
		    return false;
		}
		if (nNgrams < 0) {
		    file.position() << "ngram number " << nNgrams
				    << " out of range\n";
		    return false;
		}
		if (thisOrder > maxOrder) {
		    maxOrder = thisOrder;
		}
		numNgrams[thisOrder] = nNgrams;
		continue;
	    } else {
		file.position() << "unexpected input\n";
		return false;
	    }

	default:	/* reading n-grams, where n == state */

	    if (backslash) {
		if (numOOVs > 0) {
		    if (debug(DEBUG_READ_STATS)) {
			dout() << "discarded " << numOOVs
			       << " OOV " << state << "-grams\n";
		    }
		    numOOVs = 0;
		}
	    }

	    if (backslash && sscanf(line, "\\%d-grams", &state) == 1) {
		if (state < 1 || state > maxOrder) {
		    file.position() << "invalid ngram order " << state << "\n";
		    return false;
		}

	        if (debug(DEBUG_READ_STATS)) {
		    dout() << (state <= order ? "reading " : "skipping ")
			   << numNgrams[state] << " "
			   << state << "-grams\n";
		}

		/*
		 * start reading more n-grams
		 */
		continue;
	    } else if (backslash && strncmp(line, "\\end\\", 5) == 0) {
		/*
		 * Check that the total number of ngrams read matches
		 * that found in the header
		 */
		for (int i = 0; i <= maxOrder && i <= order; i++) {
		    if (numNgrams[i] != numRead[i]) {
			file.position() << "warning: " << numRead[i] << " "
			                << i << "-grams read, expected "
			                << numNgrams[i] << "\n";
		    }
		}

		return true;
	    } else if (state > order) {
		/*
		 * Save time and memory by skipping ngrams outside
		 * the order range of this model
		 */
		continue;
	    } else {
		VocabString words[1+ maxNgramOrder + 1 + 1];
				/* result of parsing an n-gram line
				 * the first and last elements are actually
				 * numerical parameters, but so what? */
		VocabIndex wids[maxNgramOrder + 1];
				/* ngram translated to word indices */
		LogP prob, bow; /* probability and back-off-weight */

		/*
		 * Parse a line of the form
		 *	<prob>	<w1> <w2> ...	<bow>
		 */
		unsigned howmany = Vocab::parseWords(line, words, state + 3);

		if (howmany < state + 1 || howmany > state + 2) {
		    file.position() << "ngram line has " << howmany
				    << " fields (" << state + 2
				    << " expected)\n";
		    return false;
		}

		/*
		 * Parse prob
		 */
		if (!parseLogP(words[0], prob)) {
		    file.position() << "bad prob \"" << words[0] << "\"\n";
		    return false;
		} else if (prob > LogP_One || prob != prob) {
		    file.position() << "warning: questionable prob \""
				    << words[0] << "\"\n";
		} else if (prob == LogP_PseudoZero) {
		    /*
		     * convert pseudo-zeros back into real zeros
		     */
		    prob = LogP_Zero;
		}

		/* 
		 * Parse bow, if any
		 */
		if (howmany == state + 2) {
		    /*
		     * Parsing floats strings is the most time-consuming
		     * part of reading in backoff models.  We therefore
		     * try to avoid parsing bows where they are useless,
		     * i.e., for contexts that are longer than what this
		     * model uses.  We also do a quick sanity check to
		     * warn about non-zero bows in that position.
		     */
		    if (state == maxOrder) {
			if (words[state + 1][0] != '0') {
			    file.position() << "ignoring non-zero bow \""
					    << words[state + 1]
					    << "\" for maximal ngram\n";
			}
		    } else if (state == order) {
			/*
			 * save time and memory by skipping bows that will
			 * never be used as a result of higher-order ngram
			 * skipping
			 */
			;
		    } else if (!parseLogP(words[state + 1], bow)) {
			file.position() << "bad bow \"" << words[state + 1]
					<< "\"\n";
			return false;
		    } else if (bow == LogP_Inf || bow != bow) {
			file.position() << "warning: questionable bow \""
		                    	<< words[state + 1] << "\"\n";
		    } else if (bow == LogP_PseudoZero) {
			/*
			 * convert pseudo-zeros back into real zeros
			 */
			bow = LogP_Zero;
		    }
		}

		numRead[state] ++;

		/* 
		 * Terminate the words array after the last word,
		 * then translate it to word indices.  We also
		 * reverse the ngram since that's how we'll need it
		 * to index the trie.
		 */
		words[state + 1] = 0;
		if (limitVocab) {
		    /*
		     * Skip N-gram if it contains OOV word.
		     * Do NOT add new words to the vocabulary.
		     */
		    vocab.getIndices(&words[1], wids, maxNgramOrder);

		    Boolean haveOOV = false;
		    for (unsigned j = 0; j < state; j ++) {
			if (wids[j] == Vocab_None) {
			    haveOOV = true;
			    break;
			}
		    }
		    if (haveOOV) {
			numOOVs ++;
			/* skip rest of processing and go to next ngram */
			continue;
		    }
		} else {
		    vocab.addWords(&words[1], wids, maxNgramOrder);
		}
		Vocab::reverse(wids);
		
		/*
		 * Store bow, if any
		 */
		if (howmany == state + 2 && state < order) {
		  if (filter.findBOW(wids))
		    *insertBOW(wids) = bow;
		}

		/*
		 * Save the last word (which is now the first, due to reversal)
		 * then use the first n-1 to index into
		 * the context trie, storing the prob.
		 */
		if (filter.findProb(wids[0], &wids[1])) {
		  BOnode *bonode = contexts.find(&wids[1]);
		  if (!bonode) {
#if 0
		    file.position() << "warning: no bow for prefix of ngram \""
				    << &words[1] << "\"\n";
#endif
		  } else {
		    if (!warnedAboutUnk &&
			wids[0] == vocab.unkIndex() &&
			prob != LogP_Zero &&
		    	!vocab.unkIsWord())
		      {
			file.position() << "warning: non-zero probability for "
			                << vocab.getWord(vocab.unkIndex())
			                << " in closed-vocabulary LM\n";
			warnedAboutUnk = true;
		      }
		    
		    /* efficient for: *insertProb(wids[0], &wids[1]) = prob */
		    *bonode->probs.insert(wids[0]) = prob;
		  }
		}

		/*
		 * Hey, we're done with this ngram!
		 */
	    }
	}
    }

    /*
     * we reached a premature EOF
     */
    file.position() << "reached EOF before \\end\\\n";
    return false;
}

