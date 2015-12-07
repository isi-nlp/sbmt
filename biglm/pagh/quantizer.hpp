#ifndef QUANTIZE_HPP
#define QUANTIZE_HPP

#include <iostream>
#include <vector>
#include <cstdio>

#include "types.hpp"

namespace biglm {

class quantizer {
  std::vector<weight_type> means;
  std::vector<weight_type> boundaries;
public:
  quantizer() {}
  quantizer(FILE *fp);
  quantizer(std::string s);
  void dump(FILE *fp) const;
  weight_type decode(unsigned int q) const;
  unsigned int encode(weight_type p) const;
  int size() const { return means.size(); }
};

}

#endif
