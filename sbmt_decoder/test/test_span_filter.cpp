
#include <sbmt/search/span_filter.hpp>
#include <sbmt/search/sorted_cube.hpp>
#include <boost/test/auto_unit_test.hpp>

#include <sbmt/edge/null_info.hpp>
#include <sbmt/grammar/grammar_in_memory.hpp>

template class
sbmt::exhaustive_span_filter< sbmt::edge<sbmt::null_info>
                            , sbmt::grammar_in_mem
                            , sbmt::basic_chart< sbmt::edge<sbmt::null_info> > 
                            >;

//FIXME: compile errors
#if 0
template class
sbmt::sorted_cube< sbmt::edge_equivalence<sbmt::edge<sbmt::null_info> >
                 , sbmt::edge_equivalence<sbmt::edge<sbmt::null_info> >
                 , sbmt::grammar_in_mem::rule_type 
                 >;
#endif 
