#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#include <boost/lexical_cast.hpp>

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
  cerr << "reading test data from " << argv[1] << endl;
  int wordcount = 0;
  while (!getline(infile, line).eof()) {
    istringstream tokenizer(line);
    string word;
    vector<VocabIndex> words;
    words.push_back(vocab.getIndex(Vocab_SentStart));
    while (tokenizer >> word) {
      words.push_back(vocab.addWord(word.c_str()));
      wordcount++;
    }
    words.push_back(vocab.getIndex(Vocab_SentEnd));
    sentences.push_back(words);
  }
  cerr << wordcount << " words\n";
  infile.close();

  File filterfile(argv[1], "r");
  Ngram *filterlm = make_filter(filterfile, vocab, order);
  filterfile.close();

  vector<FilterNgram *> lms;
  vector<double> lambda;

  for (int i=2; i<argc; i+=2) {
    lambda.push_back(boost::lexical_cast<double>(argv[i+1]));
    cerr << "loading " << argv[i] << " with weight " << lambda[lambda.size()-1] << endl;

    FilterNgram *lm = new FilterNgram(vocab, order);
    File lmfile(argv[i], "r");
    lm->read(lmfile, *filterlm);
    lmfile.close();

    lms.push_back(lm);
  }

  int oov=0;

  double ll = 0.0;
  vector<double> ps;

  for (int j=0; j<sentences.size(); j++) {
    ps.resize(sentences[j].size()-1); // -1 for <s>
    for (int k=0; k<ps.size(); k++)
      ps[k] = 0.0;

    for (int i=0; i<lms.size(); i++) {
      FilterNgram *lm = lms[i];

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
	//cerr << vocab.getWord(words[k]) << " = " << lp << endl;

	ps[k] += pow(10.0,lp)*lambda[i];
      }
    }

    double lp = 0.0;
    for (int k=0; k<ps.size(); k++)
      lp += log10(ps[k]);
    cout << lp << endl;

    ll += lp;
  }

  if (oov)
    cerr << "OOVs = " << oov << endl;
  cerr << "log-likelihood = " << ll << "\t";
  cerr << "perplexity = " << pow(10.0,-ll/(wordcount+sentences.size())) << endl;

}
