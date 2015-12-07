#include <vector>
#include <algorithm>

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>

#include <stdexcept>

#include "quantizer.hpp"

using namespace std;

namespace biglm {

quantizer::quantizer(FILE *fp) {
  int n;
  fread(&n, sizeof(n), 1, fp);
  weight_type x, prev=0;
  for (int i=0; i<n; i++) {
    fread(&x, sizeof(x), 1, fp);
    means.push_back(x);
    if (i > 0)
      boundaries.push_back(0.5*(x+prev));
    prev = x;
  }
}

quantizer::quantizer(string s) {
  istringstream tokenizer(s);
  weight_type x, prev=0;
  bool first = 1;
  while (tokenizer >> x) {
    means.push_back(x);
    if (!first)
      boundaries.push_back(0.5*(x+prev));
    prev = x;
    first = 0;
  }
}

void quantizer::dump(FILE *fp) const {
  int n = means.size();
  fwrite(&n, sizeof(n), 1, fp);
  for (vector<weight_type>::const_iterator it=means.begin(); it != means.end(); ++it)
    fwrite(&*it, sizeof(*it), 1, fp);
}

weight_type quantizer::decode(unsigned int q) const {
  return means[q];
}

unsigned int quantizer::encode(weight_type p) const {
  return lower_bound(boundaries.begin(), boundaries.end(), p)-boundaries.begin();
}

}
