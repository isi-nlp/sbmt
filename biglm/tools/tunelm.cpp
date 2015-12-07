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

  if (argc < 3) {
    cerr << "usage: tunelm <heldout> <lm>+" << endl;
    exit(1);
  }

  ifstream infile(argv[1]);
  if (infile.fail()) {
    cerr << "couldn't open heldout file " << argv[1] << endl;
    exit(1);
  }
  string line;
  cerr << "reading held-out data from " << argv[1] << endl;
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

  vector<double> lambda;
  for (int i=2; i<argc; i++) {
    lambda.push_back(1.0);
    cerr << "scoring using " << argv[i] << endl;

    FilterNgram *lm = new FilterNgram(vocab, order);
    File lmfile(argv[i], "r");
    lm->read(lmfile, *filterlm);
    lmfile.close();

    vector<double> ps;
    double ll = 0.0;
    
    int oov=0;
    for (int j=0; j<sentences.size(); j++) {

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
	ll += lp;
	ps.push_back(pow(10.0,lp));
      }
    }
    if (oov)
      cerr << "OOVs = " << oov << endl;
    cerr << "log-likelihood = " << ll << "\t";
    cerr << "perplexity = " << pow(10.0,-ll/(wordcount+sentences.size())) << endl;
    probs.push_back(ps);
    //delete lm;
  }

  if (lambda.size() == 1)
    exit(0);

  for (int i=0; i<lambda.size(); i++)
    lambda[i] /= lambda.size();

  const int n = probs[0].size();
  cerr << "reestimating interpolation weights" << endl;

  int iter=0;
  double prev_ll;
  while (1) {
    vector<double> count(lambda.size());
    double ll = 0.0;

    // expectation
    for (int i=0; i<n; i++) {
      double sum = 0.0;
      for (int j=0; j<lambda.size(); j++) 
	sum += lambda[j] * probs[j][i];
      for (int j=0; j<lambda.size(); j++) 
	count[j] += lambda[j] * probs[j][i] / sum;
      ll += log10(sum);
    }
    cerr << "log-likelihood = " << ll << "\t";
    cerr << "perplexity = " << pow(10.0,-ll/(wordcount+sentences.size())) << endl;
    // maximization
    cerr << "weights = ";
    for (int j=0; j<lambda.size(); j++) {
      lambda[j] = count[j] / n;
      cerr << lambda[j] << " ";
    }
    cerr << endl;

    if (iter > 0 && ll < prev_ll)
      cerr << "warning: log-likelihood decreased (this shouldn't happen)" << endl;

    if (iter > 0 && fabs(ll-prev_ll) < 0.01 || iter > 100)
      break;

    prev_ll = ll;

    iter++;
  }

}
