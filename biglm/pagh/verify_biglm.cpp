#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdio>

#include "cmph.hpp"
#include "biglm.hpp"
#include "quantizer.hpp"
#include "arpa.hpp"

#include <boost/algorithm/string_regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>

using namespace std;
using namespace boost;
using namespace biglm;

int main(int argc, char *argv[]) {
  using namespace boost::program_options;

  options_description optdes("Allowed options");
  optdes.add_options()
    ("help,h", "display help message")
    ("input-file", value<string>(), "Input language model (ARPA format)")
    ("output-file,o", value<string>(), "Output file (biglm format)");
  
  positional_options_description poptdes;
  poptdes.add("input-file", 1);
  poptdes.add("output-file", 1);

  variables_map vm;
  store(command_line_parser(argc, argv).options(optdes).positional(poptdes).run(), vm);
  notify(vm);

  // validate combination of options
  int error = 0;

  if (!vm.count("input-file") || !vm.count("output-file"))
    error = 1;

  if (vm.count("help") || error) {
    cout << "verify_biglm <input-file> <check-file>\n\n";
    cout << optdes << "\n";
    return 0;
  }

  ifstream in(vm["input-file"].as<string>().c_str());
  if (in.fail()) {
    cerr << "couldn't open language model\n";
    exit(1);
  }

  lm lm(vm["output-file"].as<string>().c_str());
  
  int o;

  arpa_reader r(in, 1);
  arpa_line l;

  ngram_type ngram;

  while ((o = r.next_order())) {
    cerr << "reading " << o << "-grams" << endl;
    ngram.resize(o);

    for (int i=0; i<r.n_ngrams(o); i++) {
      l = r.next_ngram();
      for (int j=0; j<o; j++)
	ngram[j] = lm.lookup_word(l.words[j]);

      if (lm.lookup_prob(ngram) <= IMPOSSIBLE) {
	cerr << "missing n-gram: ";
	for (int j=0; j<o; j++)
	  cerr << l.words[j] << " ";
	cerr << endl;
      }
    }
  }
}
