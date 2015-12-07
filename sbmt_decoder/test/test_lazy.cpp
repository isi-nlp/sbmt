/*
# include <sbmt/search/lazy.hpp>

# include <vector>
# include <sbmt/grammar/grammar_in_memory.hpp>
# include <sbmt/edge/edge.hpp>
# include <sbmt/edge/composite_info.hpp>

template class
sbmt::lazy::cube_processor< 
  sbmt::grammar_in_mem
, std::vector<sbmt::grammar_in_mem::rule_type>
, std::vector<sbmt::edge_equivalence<sbmt::edge<sbmt::composite_info> > >
>;

# include <boost/range.hpp>
# include <boost/multi_index_container.hpp>
# include <boost/multi_index/hashed_index.hpp>

struct rhs_map_hash {
    template <class Range>
    size_t operator()(Range const& range) const
    {
        typedef typename boost::range_value<Range>::type value_type;
        typedef typename boost::range_value<Range>::type const& reference;
        size_t ret = 0;
        BOOST_FOREACH(reference v, range) {
            boost::hash_combine(ret,v);
        }
        return ret;
    }
    size_t operator()(std::vector<sbmt::grammar_in_mem::rule_type> const& rules) const
    {
        size_t ret = 0;
        for (size_t x = 0; x != gram->rule_rhs_size(rules[0]); ++x) {
            boost::hash_combine(ret,gram->rule_rhs(rules[0],x));
        }
        return ret;
    }
    sbmt::grammar_in_mem const* gram;
};

struct rhs_map_equal {
    
    bool operator()( std::vector<sbmt::grammar_in_mem::rule_type> const& rules1
                   , std::vector<sbmt::grammar_in_mem::rule_type> const& rules2 ) const
    {
        size_t x = 0;
        size_t xs = gram->rule_rhs_size(rules1[0]);
        
        if (xs != gram->rule_rhs_size(rules2[0])) return false;
        
        for (; x != xs; ++x) {
            if (gram->rule_rhs(rules1[0],x) != gram->rule_rhs(rules2[0],x)) return false;
        }
        return true;
    }
    template <class Range>
    bool operator()(std::vector<sbmt::grammar_in_mem::rule_type> const& rules, Range const& range) const
    {
        typename boost::range_iterator<Range>::type
            ri = boost::begin(range),
            re = boost::end(range);
        
        size_t x = 0;
        size_t xs = gram->rule_rhs_size(rules[0]);
        for (; ri != re and x != xs; ++ri,++x) {
            if (*ri != gram->rule_rhs(rules[0],x)) return false;
        }
        
        return (x == xs) and (ri == re);
    }
    
    template <class Range>
    bool operator()(Range const& range, std::vector<sbmt::grammar_in_mem::rule_type> const& rules) const
    {
        return this->operator()(rules,range);
    }
    
    sbmt::grammar_in_mem const* gram;
};

typedef
boost::multi_index_container<
  std::vector<sbmt::grammar_in_mem::rule_type>
, boost::multi_index::indexed_by<
    boost::multi_index::hashed_unique<
      boost::multi_index::identity< std::vector<sbmt::grammar_in_mem::rule_type> >
    , rhs_map_hash
    , rhs_map_equal
    >
  >
> rule_map_type;

template class
sbmt::lazy::split_processor<
  sbmt::grammar_in_mem
, rule_map_type
, std::vector< std::vector< sbmt::edge_equivalence< sbmt::edge<sbmt::composite_info> > > >
>
;

template class
sbmt::lazy::left_right_split_processor<
  sbmt::grammar_in_mem
, sbmt::lazy::rulemap
, sbmt::lazy::chart< sbmt::edge<sbmt::composite_info> >::cell
>; 

typedef 
    sbmt::lazy::left_right_split_processor<
      sbmt::grammar_in_mem
    , sbmt::lazy::rulemap
    , sbmt::lazy::chart_cell< sbmt::lazy::chart< sbmt::edge<sbmt::composite_info> > >::type
    >  split_processor_t;

template class
sbmt::lazy::chart< sbmt::edge<sbmt::composite_info> >;

typedef sbmt::lazy::chart< sbmt::edge<sbmt::composite_info> > chart_t;
chart_t chart(sbmt::span_t(0,10));

typedef sbmt::lazy::left_right_split_factory<
          sbmt::grammar_in_mem
        , sbmt::lazy::rulemap
        , sbmt::edge<sbmt::composite_info>
        > split_factory_t;

sbmt::grammar_in_mem gram;
sbmt::concrete_edge_factory<sbmt::edge<sbmt::composite_info>,sbmt::grammar_in_mem> ef;

sbmt::lazy::rulemap left_rmap(sbmt::lazy::make_rulemap(gram,ef,sbmt::lazy::rhs_left));

sbmt::lazy::merge_splits merger;

split_factory_t sf(gram,left_rmap,left_rmap,ef);

typedef boost::result_of<sbmt::lazy::merge_splits(split_factory_t,chart_t,sbmt::cky_generator::partition_range)>::type
span_processor_t;

span_processor_t sproc = merger(sf,chart,sbmt::full_cky_generator().partitions(sbmt::span_t(0,10)));

# include <sbmt/search/edge_filter.hpp>

template 
sbmt::lazy::cell_construct< sbmt::edge<sbmt::composite_info> >&
sbmt::lazy::filter_cell<
  gusc::any_generator< sbmt::edge<sbmt::composite_info> >
, sbmt::edge_filter< sbmt::edge<sbmt::composite_info>, bool >
, sbmt::lazy::cell_construct< sbmt::edge<sbmt::composite_info> >
>
( gusc::any_generator< sbmt::edge<sbmt::composite_info> >&
, sbmt::edge_filter< sbmt::edge<sbmt::composite_info>, bool >
, sbmt::lazy::cell_construct< sbmt::edge<sbmt::composite_info> >&
)
;

template
sbmt::lazy::chart< sbmt::edge<sbmt::composite_info> > 
sbmt::lazy::make_chart( concrete_edge_factory<sbmt::edge<sbmt::composite_info>,sbmt::grammar_in_mem>& ef
                      , sbmt::grammar_in_mem& gram
                      , sbmt::lattice_tree const& lat );

template 
void sbmt::lazy::cky( span_t target_span
                    , cky_generator const& ckygen
                    , sbmt::lazy::first_experiment_cell_proc const& proc
                    , sbmt::lazy::chart< sbmt::edge<sbmt::composite_info> >& chart );
*/
