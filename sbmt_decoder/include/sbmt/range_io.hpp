#ifndef SBMT__RANGE_IO_HPP
#define SBMT__RANGE_IO_HPP

#include <sbmt/range.hpp>
#include <sbmt/printer.hpp>
#include <graehl/shared/word_spacer.hpp>
#include <vector>
namespace sbmt {


template <class OS,class Itr,class TF>
void print(OS& os, itr_pair_range<Itr> const& v, TF const& tf)
{
  Itr itr = v.begin(), end = v.end();
  graehl::word_spacer sep;
  os << '[';
  for(; itr != end; ++itr) {
    os << sep;
    print(os,*itr,tf);
  }
  os << ']';
}

template <class OS,class X,class TF>
void print(OS& os, std::vector<X> const& v, TF const& tf)
{
/*  typename std::vector<X>::const_iterator itr = v.begin(), end = v.end();
  graehl::word_spacer sep;
  for(; itr != end; ++itr) os << sep << print(*itr,tf);
*/
  print(os,make_itr_pair_range(v),tf);
}

} //ns



#endif
