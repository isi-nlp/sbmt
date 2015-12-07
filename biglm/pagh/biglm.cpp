#include <iostream>
#include <vector>

#if 0
#include <unistd.h>
#include <sys/mman.h>
#endif

#ifdef _WIN32
#include <io.h>
#include <stdio.h>
typedef __int64 offset_type;
#else
#include <stdio.h>
#include <unistd.h>
typedef off_t offset_type;
#endif

static offset_type tellpos(FILE* file)
{
#ifdef _WIN32
	return _telli64(_fileno(file)); 
#else
	return ftello(file);
#endif
}

static void seekpos(FILE *file, offset_type len)
{
#ifdef _WIN32
	_lseeki64(_fileno(file),len,SEEK_CUR);
#else
    fseeko(file,len,SEEK_CUR);
#endif
}

using namespace std;

#include "biglm.hpp"

// BigLM format:

// 1 integer                   for |V|
// |V| null-terminated strings for vocab
// 1 mph                       for vocab

// 1 integer                   for n
// n-1 mphs                    for n-grams

// 1 length+hash_state         for checksum
// 2 count+quantizers          for prob and bow quantizers
// 3 integers                  for widths of checksum, prob, bow
// n length+bitset             for values

namespace biglm {

void load_quantizers(FILE *fp, vector<quantizer> &pqs, vector<quantizer> &bqs) {
  int n;
  fread(&n, sizeof(n), 1, fp);
  pqs.resize(n);
  for (vector<quantizer>::iterator it=pqs.begin(); it != pqs.end(); ++it)
    *it = quantizer(fp);

  fread(&n, sizeof(n), 1, fp);
  bqs.resize(n);
  for (vector<quantizer>::iterator it=bqs.begin(); it != bqs.end(); ++it)
    *it = quantizer(fp);
}

int lm::debug = 0;

lm::lm(const string &filename, bool a_override_unk, weight_type a_p_unk) {
  FILE *fp = fopen(filename.c_str(), "rb");
  if (fp == NULL) {
    throw runtime_error("biglm: file not found");
  }
  read_mph(fp);
  read_values(fp,filename);
  
  override_unk = a_override_unk;
  p_unk = a_p_unk;
}

lm::~lm() {
  delete vocab_mph;
  for (vector<cmph::mph<ngram_type> *>::iterator it=ngram_mph.begin(); it != ngram_mph.end(); ++it)
    delete *it;
  delete ngram_hash;
  for (size_type i=0; i<values.size(); i++) {
    delete values[i];
  }
}

void lm::read_mph(FILE *fp) {
  size_type vocab_size;

  fread(&vocab_size, sizeof(vocab_size), 1, fp);
  // guarantees that the capacity of vocab is vocab_size
  vector<string>(vocab_size).swap(vocab);

  for (size_type i=0; i<vocab_size; i++) {
    string s;
    int c;
    while ((c = fgetc(fp)) != '\0')
      s.push_back(c);
    vocab[i] = s;
  }

  cerr << "vocabulary = " << vocab_size << endl;

  vocab_mph = new cmph::mph<string>(fp);

  fread(&order, sizeof(order), 1, fp);

  for (int o=2; o<=order; o++) {
    ngram_mph.push_back(new cmph::mph<ngram_type>(fp));
    cerr << o << "-grams = " << ngram_mph[o-2]->size() << endl;
  }

  unk = vocab_mph->search("<unk>");
}

void lm::read_values(FILE *fp, std::string const& filename) {
  // read checksum hash function
  size_type len;
  fread(&len, sizeof(len), 1, fp);
  
  std::vector<char> hashbuf(len);
  fread(&hashbuf[0], 1, len, fp);
  
  ngram_hash = new cmph::hash_state<ngram_type>(string(&hashbuf[0], len));
  
  load_quantizers(fp, prob_quantizer, bow_quantizer);
  
  // read bit widths
  fread(&checksum_bits, sizeof(checksum_bits), 1, fp);
  fread(&prob_bits, sizeof(prob_bits), 1, fp);
  fread(&bow_bits, sizeof(bow_bits), 1, fp);
  
  cerr << "checksum bits = " << checksum_bits << endl;
  cerr << "prob bits = " << prob_bits << endl;
  cerr << "bow bits = " << bow_bits << endl;

  if (sizeof(size_type) > sizeof(off_t))
    cerr << "warning: maybe you should recompile with -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64\n";
  
  for (int o=1; o<=order; o++) {
    fread(&len, sizeof(len), 1, fp);
    offset_type offset = tellpos(fp);
	int residue = offset % boost::iostreams::mapped_file_source::alignment();
    
    cerr << "mmapping " << len << " bytes at offset " << offset << endl;
    
    boost::iostreams::mapped_file_source mfile(filename,len+residue, offset-residue);
    //char *buf = reinterpret_cast<char *>(mmap(NULL, len+residue, PROT_READ, MAP_PRIVATE, fileno(fp), offset-residue));
    //save_start.push_back(buf);
    //save_length.push_back(len+residue);
    save_maps.push_back(mfile);
    char const* buf = mfile.data();
    buf += residue;

    values.push_back(new sliceable_bitset(reinterpret_cast<block_type *>(const_cast<char*>(buf)), n_keys(o)*value_bits(o)));
    
    seekpos(fp, len);
  }
}

word_type lm::lookup_word(const string &s) const {
  word_type w = vocab_mph->search(s);
  if (w >= 0 && w < vocab.size() && vocab[w] == s)
    return w;
  else
    return unk;
}

int lm::lookup_qprob(const word_type *ngram, int o) const {
  size_type h;
  size_type cur; 
  if (o == 1) {
    h = ngram[0];
    cur = h*value_bits(o);
  } else {
    // trick: we know that <unk> is only in the LM as a unigram
    for (int i=0; i<o; i++)
      if (ngram[i] == unk)
	return -1;

    if (debug) {
      cerr << "lookup prob ";
      for (int i=0; i<o; i++)
	cerr << vocab[ngram[i]] << " ";
      cerr << "\n";
    }
      
    h = ngram_mph[o-2]->search(reinterpret_cast<const char *>(ngram), o*sizeof(word_type));

    if (h < 0 || h >= ngram_mph[o-2]->size())
      return -1;

    cur = h*value_bits(o);
    
    if (debug)
      cerr << "h = " << h <<", cur = " << cur << endl;
    
    block_type ngram_checksum = ngram_hash->hash(reinterpret_cast<const char *>(ngram), o*sizeof(word_type)) & lsbits(checksum_bits); 
    block_type found_checksum = values[o-1]->get_slice(cur, cur+checksum_bits);
    cur += checksum_bits;
    
    if (debug) {
      cerr << hex;
      cerr << "ngram_checksum = " << ngram_checksum << endl;
      cerr << "found_checksum = " << found_checksum << endl;
      cerr << dec;
    }

    if (ngram_checksum != found_checksum) {
      if (debug) {
	cerr << "found value would have been " << values[o-1]->get_slice(cur, cur+prob_bits) << endl;
      }
      return -1;
    }
  }
 
  return values[o-1]->get_slice(cur, cur+prob_bits);
}

int lm::lookup_qbow(const word_type *ngram, int o) const {
  size_type h;
  size_type cur;

  if (o == 0)
    return -1;
  else if (o == 1) {
    h = ngram[0];
    cur = h*value_bits(o)+prob_bits;
  } else {
    // trick: we know that <unk> is only in the LM as a unigram
    for (int i=0; i<o; i++)
      if (ngram[i] == unk)
	return -1;

    if (debug) {
      cerr << "lookup bow ";
      for (int i=0; i<o; i++)
	cerr << vocab[ngram[i]] << " ";
      cerr << "\n";
    }

    h = ngram_mph[o-2]->search(reinterpret_cast<const char *>(ngram), o*sizeof(word_type));
    
    if (h < 0 || h >= ngram_mph[o-2]->size())
      return -1;

    cur = h*value_bits(o);
    if (debug) {
      cerr << "h = " << h << endl;
    }
    
    block_type ngram_checksum = ngram_hash->hash(reinterpret_cast<const char *>(ngram), o*sizeof(word_type)) & lsbits(checksum_bits);
    block_type found_checksum = values[o-1]->get_slice(cur, cur+checksum_bits);
    cur += checksum_bits+prob_bits;
    
    if (debug) {
      cerr << hex;
      cerr << "ngram_checksum = " << ngram_checksum << endl;
      cerr << "found_checksum = " << found_checksum << endl;
      cerr << dec;
    }
    
    if (ngram_checksum != found_checksum) {
      if (debug) {
	cerr << "found value would have been " << values[o-1]->get_slice(cur, cur+bow_bits) << endl;
      }
      return -1;
    }
  }

  return values[o-1]->get_slice(cur, cur+bow_bits);
}

  
  /*Checks whether a sub-sequence of an ngram is in the LM*/
  bool lm::check_ngram(const word_type *ngram, int start, int length) const{
    
    cmph::cmph_uint32 h;
    size_type tmpCur; 
    h = ngram_mph[length-2]->search(reinterpret_cast<const char *>(&ngram[start]), length*sizeof(word_type));
    if (h < 0 || h >= ngram_mph[length-2]->size())
      return false;
    tmpCur = h*value_bits(length);
    block_type ngram_checksum = ngram_hash->hash(reinterpret_cast<const char *>(&ngram[start]), length*sizeof(word_type)) & lsbits(checksum_bits); 
    block_type found_checksum = values[length-1]->get_slice(tmpCur, tmpCur+checksum_bits);
    
    if (ngram_checksum != found_checksum)
      return false;
    
    return true;
    
  }

weight_type lm::lookup_ngram(const word_type *ngram, int n) const {
  int o = n;
  weight_type bow = 0.0;
  
  while (o > 0) {
    if (o == 1 && ngram[n-1] == unk && override_unk)
      return bow+p_unk;

    int qp = lookup_qprob(&ngram[n-o], o);
    if (qp >= 0) {
      return bow+prob_quantizer[o-1].decode(qp);
    }
    int qb = lookup_qbow(&ngram[n-o], o-1);
    if (qb >= 0) {
      bow += bow_quantizer[o-2].decode(qb);
    }
    o--;
  }
  return IMPOSSIBLE;
}

weight_type lm::lookup_ngrams(const word_type *ngrams, int n, int start, int stop) const {
  // lookup the probabilities of the words ngrams[start], ..., ngrams[stop]-1 inclusive
  // in the buffer at ngrams of size n
  weight_type p = 0.0;
  for (int j=start+1; j<=stop; j++) {
    int i = j-get_order();
    if (i < 0) i = 0;
    p += lookup_ngram(ngrams+i, j-i);
  }
  return p;
}
  
weight_type lm::lookup_ngram_checked(const word_type *ngram, int n) const {
  int o = n;
  weight_type bow = 0.0;

  int nextProb = n;
  int nextBow = n;
  bool failedChk = false;
  bool bowChked = false;
  
  while (o > 0) {
    if (o == 1 && ngram[n-1] == unk && override_unk)
      return bow+p_unk;

    if (o <= nextProb) {
      int qp = lookup_qprob(&ngram[n-o], o);
      if (qp >= 0) {
	
	/*Redundancy check for lower order i-grams*/
	failedChk = false;

	for (int i=o-1; i>1; i--){
	  
	  if(!bowChked){
	    if (!check_ngram(ngram, 0, i)){
	      failedChk = true;
	      nextBow = o-2;
	    }
	    
	    /*Starting at position j*/
	    //for (int j=0; j<o-i+1; j++){
	    for (int j=o-i-1; j>0; j--){
	      if(!check_ngram(ngram, j, i)){
		failedChk = true;
		nextBow = o-2-j;
		nextProb = o-1-j;
		break;
	      }
	    }
	  }

	  if (!check_ngram(ngram, o-i, i)){
	    failedChk = true;
	    nextProb = i-1;
	  }
	  
	  if (failedChk)
	    break;
	}
	
	if (!failedChk){
	  if (0&&debug) {
	    cerr << "    prob(";
	    for (int i=n-o; i<n; i++)
	      cerr << " " << lookup_id(ngram[i]);
	    cerr << " ) = " << qp << " = " << prob_quantizer[o-1].decode(qp) << endl;
	  }
	  return bow+prob_quantizer[o-1].decode(qp);
	}
      }
    }
    
    
    if (o-1 <= nextBow){
      int qb = lookup_qbow(&ngram[n-o], o-1);
      if (qb >= 0) {
	
	/*Redundancy check for lower order i-grams*/
	failedChk = false;
	if (!bowChked){
	  for (int i=o-2; i>1; i--){
	    
	    if (!check_ngram(ngram, 0, i))
	      failedChk = true;
	    
	    /*Starting at position j*/
	    for (int j=o-1-i; j>0; j--){
	      if(!check_ngram(ngram, j, i)){
		failedChk = true;
		nextBow = o-2-j;
		nextProb = o-1-j;
		break;
	      }
	    }
	    
	    if (failedChk)
	      break;
	  }
	}
	
        if (!failedChk) {
          if (0&&debug) {
            cerr << "    bow(";
            for (int i=n-o; i<n-1; i++)
              cerr << " " << lookup_id(ngram[i]);
            cerr << " ) = " << qb << " = " << bow_quantizer[o-2].decode(qb) << endl;
          }
          bow += bow_quantizer[o-2].decode(qb);
          bowChked = true;
        }
      }
    }
    o--;
    
  }
  return IMPOSSIBLE;
}

}
