#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#include "Vocab.h"
#include "Ngram.h"
#include "FilterNgram.hpp"

using namespace std;

const int order = 5;
const double lfloor = -99.0;

int main (int argc, char *argv[]) {
  Vocab vocab;
  vocab.unkIsWord() = 1;

  vector<vector<VocabIndex> > sentences;
  vector<vector<double> > probs;

  ifstream infile(argv[1]);
  string line;
  cerr << "reading lines from " << argv[1] << endl;
  while (!getline(infile, line).eof()) {
    istringstream tokenizer(line);
    string word;
    vector<VocabIndex> words;
    words.push_back(vocab.getIndex(Vocab_SentStart));
    while (tokenizer >> word) {
      words.push_back(vocab.addWord(word.c_str()));
    }
    words.push_back(vocab.getIndex(Vocab_SentEnd));
    sentences.push_back(words);
  }
  infile.close();

  File filterfile(argv[1], "r");
  Ngram *filterlm = make_filter(filterfile, vocab, order);
  filterfile.close();

  for (int i=2; i<argc; i++) {
    cerr << "scoring using " << argv[i] << endl;

    FilterNgram *lm = new FilterNgram(vocab, order);
    File lmfile(argv[i], "r");
    lm->read(lmfile, *filterlm);
    lmfile.close();

    vector<double> ps;
    
    int oov=0;
    for (int j=0; j<sentences.size(); j++) {
      double ll = 0.0;

      // prepare sentence for lookup
      vector<VocabIndex> words(sentences[j]);

      for (int k=1; k<words.size(); k++) { // skip <s>
	VocabIndex none = Vocab_None;
	// if no unigram prob, assume OOV
	if (!lm->findProb(words[k], &none))
	  words[k] = vocab.unkIndex();
      }

      reverse(words.begin(), words.end());
      words.push_back(Vocab_None);

      for (int k=0; k<words.size()-2; k++) { // -1 for Vocab_None, -1 for <s>
	double lp = lm->wordProb(words[k], &words[k+1]);
	if (lp <= lfloor) {
	  lp = lfloor;
	  oov++;
	}
	ll += lp;
      }
      ps.push_back(ll);
    }
    if (oov)
      cerr << "OOVs = " << oov << endl;
    probs.push_back(ps);
    //delete lm;
  }

  for (int j=0; j<sentences.size(); j++) {
    for (int i=0; i<probs.size(); i++)
      cout << probs[i][j] << " ";
    cout << endl;
  }

}
