# if ! defined(SBMT__SEARCH__LAZY__CKY_HPP)
# define       SBMT__SEARCH__LAZY__CKY_HPP

# include <sbmt/search/lazy/rules.hpp>
# include <sbmt/search/lazy/right_left_split.hpp>
# include <sbmt/search/lazy/filter.hpp>
# include <sbmt/search/edge_filter.hpp>
# include <sbmt/search/filter_predicates.hpp>
# include <sbmt/search/lazy/chart.hpp>
# include <sbmt/search/edge_filter.hpp>
# include <sbmt/search/concrete_edge_factory.hpp>
# include <boost/utility/result_of.hpp>
# include <boost/tuple/tuple.hpp>
# include <sbmt/search/logging.hpp>
# include <sbmt/hash/thread_pool.hpp>
# include <boost/bind.hpp>
# include <boost/tuple/tuple_io.hpp>
# include <graehl/shared/time_space_report.hpp>
# include <graehl/shared/reserved_memory.hpp>
# include <sbmt/search/lazy/unary.hpp>
# include <sbmt/search/lattice_reader.hpp>
# include <sbmt/search/lazy/find_parse_error.hpp>
# include <sbmt/search/parse_error.hpp>


namespace mugu {

template <class Edge>
struct first_experiment_cell_proc {
    typedef grammar_in_mem grammar_type;
    typedef Edge edge_type;
    typedef chart<edge_type> chart_type;
    typedef left_right_split_factory<grammar_type, rulemap, edge_type>
            split_factory_type;
    typedef exhaustive_cube_split_factory<grammar_type, rulemap, edge_type>
            cube_factory_type;
    typedef edge_filter<edge_type,boost::logic::tribool> edge_filter_type;
    typedef edge_filter<edge_type,bool> unary_edge_filter_type;

    first_experiment_cell_proc( grammar_type& gram
                              , concrete_edge_factory<edge_type,grammar_type>& ef
                              , lattice_tree const& lat
                              , edge_filter_type filter_prototype
                              , unary_edge_filter_type unary_filter_prototype
                              , size_t lookahead
                              )
    : left_map(make_rulemap(gram,ef,rhs_left))
    , right_map(make_rulemap(gram,ef,rhs_right))
    , unary_map(make_unary_rulemap(gram,ef))
    , filter_prototype(filter_prototype)
    , unary_filter_prototype(unary_filter_prototype)
    , lookahead(lookahead)
    , intro(ef,gram,lat)
    , gram(&gram)
    , ef(&ef)
    {
        SBMT_TERSE_MSG(cky_domain,"binary-filter: %s", filter_prototype);
        SBMT_TERSE_MSG(cky_domain,"unary-filter: %s", unary_filter_prototype);
    }

    rulemap left_map;
    rulemap right_map;
    unary_rulemap unary_map;
    merge_splits merge_gen;
    edge_filter_type filter_prototype;
    unary_edge_filter_type unary_filter_prototype;
    size_t lookahead;
    intro_cells<edge_type> intro;
    grammar_type* gram;
    concrete_edge_factory<edge_type,grammar_type>* ef;

    void operator()(span_t const& span, cky_generator::partition_range const& parts, chart_type& chrt) const
    {

        edge_filter_type filter = filter_prototype;
        unary_edge_filter_type unary_filter = unary_filter_prototype;
        cell_construct<edge_type> ccons = intro(span);
        if (!boost::empty(parts)) {
            if (lookahead) {
                split_factory_type sf( *gram
                                     , left_map
                                     , right_map
                                     , *ef
                                     , lookahead
                                     );
                typename boost::result_of<merge_splits(split_factory_type,chart_type,cky_generator::partition_range)>::type
                    generator = merge_gen(sf,chrt,parts);
                filter_cell(generator,filter,ccons);
            } else {
                cube_factory_type csf(*gram,left_map,*ef);
                typename boost::result_of<merge_splits(cube_factory_type,chart_type,cky_generator::partition_range)>::type
                    generator = merge_gen(csf,chrt,parts);
                filter_cell(generator,filter,ccons);
            }
        }
        chrt.put_cell(ccons);
        unary_processor<grammar_in_mem,unary_rulemap,typename chart_type::data_type>
            ups(chrt[span],*gram,unary_map,*ef);
        single_ply_unary_filter_cell(ups,unary_filter,ccons,*gram,*ef,span);
        chrt.put_cell(ccons);
    }


    bool adjust_for_retry(size_t i)
    {
        bool ret = false;
        if (filter_prototype.adjust_for_retry(i)) ret = true;
        if (unary_filter_prototype.adjust_for_retry(i)) ret = true;
        return ret;
    }
};

template <class Edge>
struct exhaustive_top_addition {
    typedef Edge edge_type;
    typedef chart<edge_type> chart_type;
    typedef left_right_split_factory<grammar_in_mem, rulemap, edge_type>
            split_factory_type;

    exhaustive_top_addition( grammar_in_mem const& gram
                           , concrete_edge_factory<edge_type,grammar_in_mem>& ef
                           )
    : left_map(make_rulemap(gram,ef,rhs_left,true))
    , right_map(make_rulemap(gram,ef,rhs_right,true))
    , gram(&gram)
    , ef(&ef)
    , sf(gram,left_map,right_map,ef,1)
    {}

    rulemap left_map;
    rulemap right_map;
    grammar_in_mem const* gram;
    concrete_edge_factory<edge_type,grammar_in_mem>* ef;
    split_factory_type sf;
    merge_splits merge_gen;

    typedef edge_equivalence<edge_type> equiv_type;

    template <class Partitions>
    std::auto_ptr<equiv_type>
    binary_top(Partitions const& parts, chart_type const& chrt) const
    {
        typename boost::result_of<
                   merge_splits(split_factory_type,chart_type,Partitions)
                 >::type
            generator = merge_gen(sf,chrt,parts);
        if (generator) {
            edge_equivalence<edge_type> eq(generator());
            while (generator) {
                eq.insert_edge(generator());
            }
            return std::auto_ptr<equiv_type>(new equiv_type(eq));
        } else {
            return std::auto_ptr<equiv_type>(NULL);
        }
    }

    std::auto_ptr<equiv_type>
    unary_top(std::auto_ptr<equiv_type> ptr, chart_type const& chrt) const
    {
        BOOST_FOREACH(typename chart_cell<chart_type>::type cell,chrt[chrt.span()]) {
            BOOST_FOREACH( grammar_in_mem::rule_type rule
                         , gram->toplevel_unary_rules(cell[0].representative().root()) ) {
                BOOST_FOREACH(typename chart_edge_equivalence<chart_type>::type eq, cell) {
                    gusc::any_generator<edge_type>
                        gen = ef->create_edge(*gram,rule,eq);

                    while (gen) {
                        if (ptr.get()) {
                            ptr->insert_edge(gen());
                        } else {
                            ptr.reset(new equiv_type(gen()));
                        }
                    }
                }
            }
        }
        return ptr;
    }

    template <class Partitions>
    edge_equivalence<edge_type> operator()(Partitions const& parts, chart_type& chrt) const
    {
        std::auto_ptr<equiv_type> ptr = binary_top(parts,chrt);
        ptr = unary_top(ptr,chrt);
        if (ptr.get()) {
            return *ptr;
        } else {
            throw std::runtime_error("didn't build any toplevel items");
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

template <class CellProcessor, class Chart, class Edge, class Grammar>
void cky_work( cky_generator const& gen
             , span_t const& span
             , CellProcessor const& proc
             , Chart& chart
             , concrete_edge_factory<Edge,Grammar>& ef )
{
    io::time_space_change span_spent;
    proc(span,gen.partitions(span),chart);
    SBMT_INFO_STREAM(
      cky_domain
    , span << " processed in " << span_spent
    )
    ;
}

template <class CellProcessor, class Chart, class Edge, class Grammar>
void cky_st( span_t target_span
           , cky_generator const& ckygen
           , CellProcessor const& proc
           , Chart& chart
           , concrete_edge_factory<Edge,Grammar>& ef )
{
    for (span_index_t len = 1; len <= ckygen.shift_sizes(target_span); ++len) {
        edge_stats length_edge_stats = ef.stats();
        io::time_space_change length_spent;

        BOOST_FOREACH(span_t span, ckygen.shifts(target_span,len))
        {
            // this condition prevents recursive block decoding from processing
            // a compound span as the leaf in a parent block
            if (boost::empty(chart[span])) {
                cky_work(ckygen,span,proc,chart,ef);
            }
        }
    }
}

template <class Chart>
void remove_virtuals(Chart& chart, span_t const& span)
{
    typedef typename chart_edge<Chart>::type edge_type;
    cell_construct<edge_type> cc(span);
    BOOST_FOREACH(typename chart_cell<Chart>::type c, chart[span]) {
        if (c[0].representative().root().type() != virtual_tag_token) {
            BOOST_FOREACH(typename chart_edge_equivalence<Chart>::type eq, c) {
                cc.insert(eq);
            }
        }
    }
    chart.put_cell(cc);
}

template <class CellProcessor, class Chart, class Edge, class Grammar>
void block_cky_( lattice_tree::node const& node
               , cky_generator const& ckygen
               , CellProcessor const& proc
               , Chart& chart
               , concrete_edge_factory<Edge,Grammar>& ef
               , size_t numthreads )
{
    assert(node.is_internal());
    std::set<span_index_t> sset;
    span_t blockspan = node.span();
    lattice_tree::children_iterator itr = node.children_begin(),
                                    end = node.children_end();

    for (; itr != end; ++itr) {
        if (itr->is_internal()) {
            block_cky_(*itr,ckygen,proc,chart,ef,numthreads);
        }
        span_t s = itr->span();
        sset.insert(s.left());
        sset.insert(s.right());
    }

   cky_st( blockspan
              , span_transform_generator(sset,ckygen)
              , proc
              , chart
              , ef );
    }

    remove_virtuals(chart,blockspan);
}

template <class Edge, class Grammar>
std::vector<indexed_lm_token> binsent(Grammar const& gram, edge_equivalence<Edge> const& eq)
{
    size_t ngram_id = gram.prop_constructors.property_map("ngram")["lm_string"];
    Edge const& e = eq.representative();
    typename Grammar::rule_type rule = gram.rule(e.rule_id());
    std::vector<indexed_lm_token> retval;
    if (not gram.rule_has_property(rule,ngram_id)) return retval;
    indexed_lm_string const&
        lmstr = gram.template rule_property<indexed_lm_string>(rule,ngram_id);
    std::vector< edge_equivalence<Edge> > ntchildren;
    for (size_t x = 0; x != e.child_count(); ++x) {
        if (not is_lexical(e.get_child(x).root())) {
            ntchildren.push_back(e.get_children(x));
        }
    }
    BOOST_FOREACH(indexed_lm_token tok, lmstr) {
        if (tok.is_token()) {
            retval.push_back(tok);
        } else {
            BOOST_FOREACH(indexed_lm_token ttok, binsent(gram,ntchildren.at(tok.get_index()))) {
                assert(ttok.is_token());
                retval.push_back(ttok);
            }
        }
    }
    return retval;
}

template <class CellProcessor, class TopProcessor, class Edge, class Grammar>
boost::tuple<edge_equivalence<Edge>,bool>
block_cky( lattice_tree const& ltree
         , cky_generator const& ckygen
         , CellProcessor& proc
         , TopProcessor& tops
         , concrete_edge_factory<Edge,Grammar>& ef
         , size_t numthreads
         , unsigned max_retries=7
         , std::size_t reserve_bytes=50*1024*1024 )
{
    graehl::time_space_change total_spent;
    span_t total_span = ltree.root().span();
    edge_stats total_stats = ef.stats();

    for (size_t retry_i = 0; ; ) {
        try {
            graehl::reserved_memory spare(reserve_bytes);
            typedef chart<Edge> chart_t;
            chart<Edge> chrt(total_span);

            block_cky_(ltree.root(),ckygen,proc,chrt,ef,numthreads);

            SBMT_TERSE_MSG(
              cky_domain
            , "In total, %s %s", (ef.stats() - total_stats) % total_spent
            );

            typename Edge::edge_equiv_type teq = tops(ckygen.partitions(total_span),chrt);

            cell_construct<Edge> ccons(total_span);
            BOOST_FOREACH(typename chart_cell<chart_t>::type cell,chrt[total_span]) {
                BOOST_FOREACH(typename chart_edge_equivalence<chart_t>::type eq, cell){
                    ccons.insert(eq);
                }
            }
            ccons.insert(teq);
            chrt.put_cell(ccons);

            return boost::make_tuple(
                     teq
                   , retry_i > 0
                   );
        } catch (std::bad_alloc const& e) {
            ++retry_i;
            if (retry_i == max_retries) {
                SBMT_ERROR_MSG(
                  cky_domain
                , "GIVING UP: out of memory still after %s retries (ERROR: %s)"
                , retry_i % e.what()
                );
                throw e;
            } else if (proc.adjust_for_retry(retry_i) == false) {
                SBMT_ERROR_MSG(
                  cky_domain
                , "GIVING UP: %s"
                , "no more span filter factory adjustments are available."
                );
                throw e;
            } else {
                SBMT_WARNING_MSG(
                  cky_domain
                , "RETRYING: ran out of memory on retry #%s"
                  " - trying again with tighter beams. "
                  "(CAUGHT exception: %s)"
                , retry_i % e.what()
                );
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

} // namespace mugu

# endif //     SBMT__SEARCH__LAZY__CKY_HPP
