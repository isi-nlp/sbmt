#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdio>

#include <boost/program_options.hpp>

#include "cmph.hpp"
#include "biglm.hpp"

using namespace std;
using namespace biglm;

#include <cstdlib>
#include <cctype>

int main(int argc, char *argv[]) {
  using namespace boost::program_options;

  options_description optdes("Allowed options");
  optdes.add_options()
    ("help,h", "display help message")
    ("verbose,v", "verbose output")
    ("map-digits,a", "replace digits with @")
    ("no-startstop,s", "don't add <s> and </s>")
    ("lm", value<string>(), "input language model (nz format)");
  positional_options_description poptdes;
  poptdes.add("lm", 1);

  variables_map vm;
  store(command_line_parser(argc, argv).options(optdes).positional(poptdes).run(), vm);
  notify(vm);

  // validate combination of options
  int error = 0;

  if (!vm.count("lm"))
    error = 1;

  if (vm.count("help") || error) {
    cout << "use_biglm <lm>\n\n";
    cout << optdes << "\n";
    return 0;
  }

  lm lm(vm["lm"].as<string>());

  //lm::debug = 1;

  string line, word;
  vector<string> words;
  vector<word_type> ids;
  while (getline(cin, line)) {
    if (vm.count("map-digits"))
      for (int i=0,e=line.size();i<e;++i)
        if (isdigit(line[i]))
          line[i]='@';

    weight_type p=0.0;
    words.clear();
    if (!vm.count("no-startstop"))
      words.push_back("<s>");
    istringstream tokenizer(line);
    while (tokenizer >> word)
      words.push_back(word);
    if (!vm.count("no-startstop"))
      words.push_back("</s>");

    ids.clear();
    for (vector<string>::iterator it = words.begin(); it != words.end(); ++it)
      ids.push_back(lm.lookup_word(*it));

    int i;
    if (vm.count("no-startstop"))
      i = 0;
    else
      i = 1;
    for (; i<ids.size(); i++) {
      ngram_type ngram;
      for (int j=i-lm.get_order()+1; j<=i; j++)
	if (j >= 0)
	  ngram.push_back(ids[j]);
      weight_type pw = lm.lookup_ngram(ngram);
      if (vm.count("verbose"))
	cerr << words[i] << "\t" << pw << endl;
      p += pw;
    }
    //cerr << "  total = " << p << "\n\n";
    cout << p << endl;
    cout.flush();
  }

}
