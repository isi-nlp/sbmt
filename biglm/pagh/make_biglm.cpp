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

regex means_re("\\s*(prob|bow)(\\d+)\\s+(.*)");

int debug = 0;

int brz_b = 175;
double brz_graphsize = 2.9; // should be >= 2.6

void load_text(istream &file, vector<quantizer> &pqs, vector<quantizer> &bqs) {
  string line;
  smatch what;
  while (!getline(file, line).eof()) {
    if (regex_match(line, what, means_re)) {
      string type = what[1].str();
      int o = lexical_cast<int>(what[2].str());
      if (type == "prob") {
	if (pqs.size() < o)
	  pqs.resize(o);
	pqs[o-1] = quantizer(what[3].str());
      } else {
	if (bqs.size() < o)
	  bqs.resize(o);
	bqs[o-1] = quantizer(what[3].str());
      }      
    } else {
      throw runtime_error("bad line in means file");
    }
  }
}

void dump_quantizers(FILE *fp, const vector<quantizer> &pqs, const vector<quantizer> &bqs) {
  int n;
  n = pqs.size();
  fwrite(&n, sizeof(n), 1, fp);
  for (vector<quantizer>::const_iterator it=pqs.begin(); it != pqs.end(); ++it)
    it->dump(fp);

  n = bqs.size();
  fwrite(&n, sizeof(n), 1, fp);
  for (vector<quantizer>::const_iterator it=bqs.begin(); it != bqs.end(); ++it)
    it->dump(fp);
}

// slurp n-grams straight out of ARPA file into MPH creator
class ngram_io_adapter : public cmph::custom_io_adapter<ngram_type> {
  arpa_reader& reader;
  cmph::mph<string> &vocab_mph;
public:
  ngram_io_adapter(arpa_reader& r, size_type nkeys, cmph::mph<string> &h)
    : cmph::custom_io_adapter<ngram_type>(nkeys),
      reader(r),
      vocab_mph(h)
  { }

  ngram_type read() {
    arpa_line l = reader.next_ngram();
    int order = l.words.size();
    ngram_type ngram(order);
    for (int j=0; j<order; j++)
      ngram[j] = vocab_mph.search(l.words[j]);
    return ngram;
  }

  void rewind() { reader.rewind_ngrams(); }
};

void dump_mph_from_arpa(istream &file, const char *filename) {
  cmph::mph<string> *vocab_mph = NULL;
  FILE *fp = fopen(filename, "wb");
  int order;
  arpa_reader r(file, 0);
  arpa_line l;
  vector<string> vocab;
  size_type vocab_size;

  order = r.n_orders();

  int o;
  while ((o = r.next_order())) {
    if (o == 1) {
      cerr << "building perfect hash for vocab (" << r.n_ngrams(o) << " keys)...";

      for (int i=0; i<r.n_ngrams(o); i++) {
	arpa_line l = r.next_ngram();
	vocab.push_back(l.words[0]);
      }

      cmph::vector_io_adapter<string> source(vocab);
      vocab_mph = new cmph::mph<string>(source, cmph::algo::BMZ);

      // dump vocab
      vocab_size = vocab.size();
      fwrite(&vocab_size, sizeof(vocab_size), 1, fp);
      
      vector<string> newvocab(vocab_size);
      for (vector<string>::iterator it=vocab.begin(); it != vocab.end(); ++it)
	newvocab[vocab_mph->search(*it)] = *it;
      for (vector<string>::iterator it=newvocab.begin(); it != newvocab.end(); ++it)
	fwrite(it->c_str(), 1, it->size()+1, fp);

      vocab_mph->dump(fp);

      fwrite(&order, sizeof(order), 1, fp);

      cerr << "done" << endl;
    } else {
      cerr << "building perfect hash for " << o << "-grams (" << r.n_ngrams(o) << " keys)...";

      ngram_io_adapter source(r, r.n_ngrams(o), *vocab_mph);
      cmph::mph<ngram_type>::dump_brz(source, fp, brz_b, brz_graphsize);
      cerr << "done" << endl;
    }
  }
  fclose(fp);
  delete vocab_mph;
}

void dump_values_from_arpa(istream &file, const char *filename, int checksum_bits, const vector<quantizer> &pqs, const vector<quantizer> &bqs) {
  FILE *fp = fopen(filename, "rb+");
  lm lm;

  lm.read_mph(fp);

  // reread the arpa file, now armed with the mphs

  int o;

  // dump checksum hash function
  cmph::hash_state<ngram_type> ngram_hash(cmph::hash_algo::JENKINS, 100000);
  string s = ngram_hash.dump();
  size_type len = s.size();
  fwrite(&len, sizeof(len), 1, fp);
  fwrite(s.data(), 1, s.size(), fp);

  // dump quantizers
  dump_quantizers(fp, pqs, bqs);

  arpa_reader r(file, 1);
  arpa_line l;

  if (lm.order != r.n_orders())
    throw runtime_error("mismatch in order");

  lm.checksum_bits = checksum_bits;

  // currently we use the same bit width for all backoff levels
  // though in principle we could let it vary

  int max_quanta = 0;
  for (o=0; o<lm.order; o++) {
    if (o >= pqs.size())
      throw runtime_error("quantizer order is less than LM order");
    if (pqs[o].size() > max_quanta)
      max_quanta = pqs[o].size();
  }
  cerr << max_quanta << " quanta for probabilities" << endl;
  lm.prob_bits = 1;
  while (1<<lm.prob_bits < max_quanta)
    lm.prob_bits++;
  cerr << "using " << lm.prob_bits << " bits for probabilities" << endl;

  max_quanta = 0;
  for (o=0; o<lm.order-1; o++) {
    if (o >= bqs.size())
      throw runtime_error("quantizer order is less than LM order");
    if (bqs[o].size() > max_quanta)
      max_quanta = bqs[o].size();
  }
  cerr << max_quanta << " quanta for backoff weights" << endl;
  lm.bow_bits = 1;
  while (1<<lm.bow_bits < max_quanta)
    lm.bow_bits++;
  cerr << "using " << lm.bow_bits << " bits for backoff weights" << endl;

  cerr << "using " << lm.checksum_bits << " bits for checksums" << endl;

  fwrite(&lm.checksum_bits, sizeof(lm.checksum_bits), 1, fp);
  fwrite(&lm.prob_bits, sizeof(lm.prob_bits), 1, fp);
  fwrite(&lm.bow_bits, sizeof(lm.bow_bits), 1, fp);

  ngram_type ngram;

  while ((o = r.next_order())) {
    cerr << "reading " << o << "-grams" << endl;
    ngram.resize(o);

    sliceable_bitset values(lm.n_keys(o)*lm.value_bits(o));

    cerr << "allocated " << values.size_bytes() << " bytes at " << hex << values.data() << dec << endl;

    for (int i=0; i<r.n_ngrams(o); i++) {
      l = r.next_ngram();
      for (int j=0; j<o; j++)
	ngram[j] = lm.vocab_mph->search(l.words[j]);

      // Pack checksum and values into buffer
      size_type cur;
      size_type h;
      if (o == 1) {
	h = ngram[0];
	lm.vocab[h] = l.words[0];
	cur = h*lm.value_bits(o);
      } else {
        h = lm.ngram_mph[o-2]->search(ngram);

	cur = h*lm.value_bits(o);

	if (debug) {
	  for (int j=0; j<o; j++)
	    cerr << l.words[j] << " ";
	  cerr << "=> " << h << endl;
	}

	if (values.get_slice(cur, cur+lm.checksum_bits) != 0) {
	  cerr << "warning: collision in bucket " << h << endl;
	}
	values.set_slice(cur, cur+lm.checksum_bits, ngram_hash.hash(ngram)&lsbits(lm.checksum_bits));
	cur += lm.checksum_bits;
      }
      values.set_slice(cur, cur+lm.prob_bits, pqs[o-1].encode(l.prob));
      cur += lm.prob_bits;
      if (o < lm.order) {
	values.set_slice(cur, cur+lm.bow_bits, bqs[o-1].encode(l.has_bow?l.bow:0.0));
      }
    }

    size_type size = values.size_bytes();
    fwrite(&size, sizeof(size), 1, fp);
    cerr << "dumping " << size << " bytes at " << hex << values.data() << dec << endl;
    fwrite(values.data(), 1, size, fp);
  }

  fclose(fp);
}

int main(int argc, char *argv[]) {
  using namespace boost::program_options;

  options_description optdes("Allowed options");
  optdes.add_options()
    ("help,h", "display help message")
    ("mph-file,m", value<string>(), "Use vocabulary and perfect hash functions from file")
    ("mph-only", "Only generate vocabulary and perfect hash functions")
    ("input-file", value<string>(), "Input language model (ARPA format)")
    ("quantizer-file,q", value<string>(), "File containing interval means")
    ("output-file,o", value<string>(), "Output file (biglm format)")
    ("checksum-bits,k", value<int>()->default_value(8), "bits for checksum (higher is more accurate)")
    ("b", value<int>()->default_value(175), "b parameter to BRZ")
    ("graphsize", value<double>()->default_value(2.9), "graph size parameter to BRZ (>= 2.6)")
    ("debug,d", "debug mode");
  
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
  if (vm.count("mph-only") && vm.count("mph-file"))
    error = 1;
  if (!vm.count("mph-only") && !vm.count("quantizer-file"))
    error = 1;

  if (vm.count("help") || error) {
    cout << "make_biglm <input-file> <output-file>\n\n";
    cout << optdes << "\n";
    return 0;
  }

  if (vm.count("debug")) {
    debug = 1;
  }

  brz_b = vm["b"].as<int>();
  brz_graphsize = vm["graphsize"].as<double>();

  ifstream in(vm["input-file"].as<string>().c_str());
  if (in.fail()) {
    cerr << "couldn't open language model\n";
    exit(1);
  }
  
  if (vm.count("mph-file")) {
    // just copy the mphs from mph-file
    ifstream ifile(vm["mph-file"].as<string>().c_str());
    ofstream ofile(vm["output-file"].as<string>().c_str());
    ofile << ifile.rdbuf();
    ifile.close();
    ofile.close();
  } else {
    // create mphs from input-file
    dump_mph_from_arpa(in, vm["output-file"].as<string>().c_str());
  }

  if (!vm.count("mph-only")) {
    // read quantizer-file
    ifstream qfile(vm["quantizer-file"].as<string>().c_str());
    vector<quantizer> pqs, bqs;
    load_text(qfile, pqs, bqs);
    qfile.close();

    // finish output-file
    in.seekg(0);
    dump_values_from_arpa(in, vm["output-file"].as<string>().c_str(), vm["checksum-bits"].as<int>(), pqs, bqs);
  }
}
