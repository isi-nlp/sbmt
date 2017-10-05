#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <deque>
//#include <stdlib>
#include <time.h>
#include <vector>
#include <algorithm>  // for partition
#include <functional> // for greater_equal
#include <numeric>    // for accumulate

#include <boost/unordered_map.hpp> 
#include <boost/algorithm/string.hpp>

#include "maybe_omp.h"
#ifdef EIGEN_USE_MKL_ALL
#include <mkl.h>
#endif

#include "util.h"

extern double drand48();

using namespace Eigen;
using namespace std;
using namespace boost::random;

namespace nplm
{

void splitBySpace(const std::string &line, std::vector<std::string> &items)
{
    string copy(line);
    boost::trim_if(copy, boost::is_any_of(" \t"));
    if (copy == "")
    {
	items.clear();
	return;
    }
    boost::split(items, copy, boost::is_any_of(" \t"), boost::token_compress_on);
}

void readWeightsFile(ifstream &TRAININ, vector<float> &weights) {
  string line;
  while (getline(TRAININ, line) && line != "")
  {
    vector<string> items;
    splitBySpace(line, items);
    if (items.size() != 1)
    {
        cerr << "Error: weights file should have only one weight per line" << endl;
        exit(-1);
    }
    weights.push_back(boost::lexical_cast<float>(items[0]));
  }
}

void readWordsFile(ifstream &TRAININ, vector<string> &word_list)
{
  string line;
  while (getline(TRAININ, line) && line != "")
  {
    vector<string> words;
    splitBySpace(line, words);
    if (words.size() != 1)
    {
        cerr << "Error: vocabulary file must have only one word per line" << endl;
        exit(-1);
    }
    word_list.push_back(words[0]);
  }
}

void readWordsFile(const string &file, vector<string> &word_list)
{
  cerr << "Reading word list from: " << file<< endl;

  ifstream TRAININ;
  TRAININ.open(file.c_str());
  if (! TRAININ)
  {
    cerr << "Error: can't read word list from file " << file<< endl;
    exit(-1);
  }

  readWordsFile(TRAININ, word_list);
  TRAININ.close();
}

void writeWordsFile(const vector<string> &words, ofstream &file)
{
    for (int i=0; i<words.size(); i++)
    {
	file << words[i] << endl;
    }
}

void writeWordsFile(const vector<string> &words, const string &filename)
{
    ofstream OUT;
    OUT.open(filename.c_str());
    if (! OUT)
    {
      cerr << "Error: can't write to file " << filename << endl;
      exit(-1);
    }
    writeWordsFile(words, OUT);
    OUT.close();
}


// Read a data file of unknown size into a flat vector<int>.
// If this takes too much memory, we should create a vector of minibatches.
void readDataFile(const string &filename, int &ngram_size, vector<int> &data, int minibatch_size)
{
  cerr << "Reading minibatches from file " << filename << ": ";

  ifstream DATAIN(filename.c_str());
  if (!DATAIN)
  {
    cerr << "Error: can't read data from file " << filename<< endl;
    exit(-1);
  }

  vector<int> data_vector;

  string line;
  long long int n_lines = 0;
  while (getline(DATAIN, line))
  {
    vector<string> ngram;
    splitBySpace(line, ngram);

    if (ngram_size == 0)
        ngram_size = ngram.size();

    if (ngram.size() != ngram_size)
    {
        cerr << "Error: expected " << ngram_size << " fields in instance, found " << ngram.size() << endl;
	exit(-1);
    }

    for (int i=0;i<ngram_size;i++)
        data.push_back(boost::lexical_cast<int>(ngram[i]));

    n_lines++;
    if (minibatch_size && n_lines % (minibatch_size * 10000) == 0)
      cerr << n_lines/minibatch_size << "...";
  }
  cerr << "done." << endl;
  DATAIN.close();
}

double logadd(double x, double y)
{
    if (x > y)
        return x + log1p(std::exp(y-x));
    else
        return y + log1p(std::exp(x-y));
}

//std::vector<double> linf(std::vector<double> &v, double delta)
void linf(std::vector<double> &v, double delta)
{
    ///mu is a list of floats.
    ///Repeatedly subtract an infinitesimal amount from the maximum
    ///element of v until a total of delta has been subtracted.
    ///Or: sort v in descending order. Then find the unique rho such that
    ///      v[rho] <= (sum(v[:rho]) - delta) / rho <= v[rho-1],
    ///which turns out to be equivalent to finding the maximum rho
    ///satisfying the second inequality, or finding the minimum rho
    ///satisfying the first inequality.
    ///Use a variant of quickselect to do this in O(n) time, following:
    ///Duchi et al., 2008. Efficient projections onto the l1-ball for
    ///learning in high dimensions.

    vector <double> mu;

    int lo = 0;
    int hi = v.size();
    double s = 0.0; // always equals sum(mu[:lo])

    for (int k = 0; k < v.size(); k++)
        mu.push_back(abs(v[k]));
    
    while (lo < hi)
    {
        int k = rand() % (hi - lo) + lo;
        double pivot = mu[k];

	// Partition mu[lo:hi] so that
	//   pivot is in the middle,
	//   everything >= pivot is to the left,
	//   everything <  pivot is to the right.
	// Set rho_new to be the first element < pivot.
	std::swap(mu[lo], mu[k]);
	std::vector<double>::iterator rho_new_it = std::partition(mu.begin()+lo+1, mu.begin()+hi, std::bind2nd(std::greater_equal<double>(), pivot));
	std::swap(mu[lo], *(rho_new_it-1));

	int rho_new = rho_new_it-mu.begin();
	double s_new = s + std::accumulate(mu.begin()+lo, mu.begin()+rho_new, 0.0);
	double xi_new = (s_new - delta) / rho_new;

        if (xi_new <= pivot)
        {
            s = s_new;
            lo = rho_new;
        }
        else
        {
            hi = rho_new-1;
        }
    }

    int rho = lo;
    double xi = std::max(0.0, (s - delta) / rho);
    for (int i = 0; i < v.size(); i++)
    {
      if      (v[i] > xi)  v[i] =  xi;
      else if (v[i] < -xi) v[i] = -xi;
    }

    return;
}

#ifdef USE_CHRONO
void Timer::start(int i)
{
    m_start[i] = clock_type::now();
}

void Timer::stop(int i)
{
    m_total[i] += clock_type::now() - m_start[i];
}

void Timer::reset(int i) { m_total[i] = duration_type(); }

double Timer::get(int i) const
{
    return boost::chrono::duration<double>(m_total[i]).count();
}

Timer timer(20);
#endif

int setup_threads(int n_threads)
{
    #ifdef _OPENMP
    if (n_threads)
        omp_set_num_threads(n_threads);
    n_threads = omp_get_max_threads();
    if (n_threads > 1)
        cerr << "Using " << n_threads << " threads" << endl;

    Eigen::initParallel();
    Eigen::setNbThreads(n_threads);

    #ifdef MKL_SINGLE
    // Set the threading layer to match the compiler.
    // This lets MKL automatically go single-threaded in parallel regions.
    #ifdef __INTEL_COMPILER
    mkl_set_threading_layer(MKL_THREADING_INTEL);
    #elif defined __GNUC__
    mkl_set_threading_layer(MKL_THREADING_GNU);
    #endif
    mkl_set_num_threads(n_threads);
    #endif
    #endif

    return n_threads;
}

} // namespace nplm
