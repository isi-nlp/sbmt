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
# include <sbmt/forest/implicit_xrs_forest.hpp> // mugu
# include <sbmt/logging.hpp>

namespace sbmt { namespace lazy {

template <class EQ, class GRAM, class ECS>
void emit_hyps(EQ eq, GRAM const& gram, ECS const& ecs,bool onlyunary=false)
{
    if (eq.representative().has_syntax_id(gram)) {
    if ((not onlyunary) or (eq.representative().child_count() == 1 and is_nonterminal(eq.representative().get_child(0).root()))) {
        xforest xf(make_equiv_as_xforest_no_component(eq,gram,ecs));
        //std::cout << "mugu:\n";
        BOOST_FOREACH(xhyp xh,xf) {
            std::cout << xf.id() << ' ' << xh.rule().id() << ' ' << xh.scores()[0] << ' ' << xh.score();
            indexed_syntax_rule::lhs_preorder_iterator
                litr = xh.rule().lhs_begin(),
                lend = xh.rule().lhs_end();
            for (; litr != lend; ++litr) {
                if (litr->indexed()) {
                    std::cout << ' ' << xh.child(litr->index()).id();
                }
            }
            std::cout << '\n';
        }
    }
    }
}

template <class CCONS, class GRAM, class ECS>
void emit_cell(CCONS const& cc, GRAM const& gram, ECS const& ecs,bool onlyunary=false)
{   
    //std::cout << "emit:";
    typedef typename CCONS::const_reference reference;
    BOOST_FOREACH(reference rng, cc) {
        typedef typename CCONS::value_type::value_type equiv_type;
        BOOST_FOREACH(equiv_type eq, rng) {
            emit_hyps(eq,gram,ecs,onlyunary);
        }
    }
}

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
    , lodm(make_lex_distance_outside_map(gram,make_lex_distance_inside_map(gram)))
    , lrdm( make_left_right_distance_map(
              skip_lattice(to_dag(lat,gram.dict()), gram.dict())
            , gram.dict()
            )
          )
    {
        /*
        lex_distance_map ldmap = make_lex_distance_inside_map(gram);
        std::cerr << token_label(gram.dict());
        lex_distance_outside_map lomap = make_lex_distance_outside_map(gram,ldmap);

        std::cerr << token_label(gram.dict());
        BOOST_FOREACH(lex_distance_outside_map::value_type& od, lodm) {
            BOOST_FOREACH(lex_outside_distances::value_type& vt, od.second.get<0>()) {
                std::cerr << vt.first << "=[ ";
                BOOST_FOREACH(int d, vt.second) {
                    std::cerr << d << ' ';
                }
                std::cerr << "] ";
            }
            std::cerr << "<- " << od.first << " -> ";
            BOOST_FOREACH(lex_outside_distances::value_type& vt, od.second.get<1>()) {
                std::cerr << vt.first << "=[ ";
                BOOST_FOREACH(int d, vt.second) {
                    std::cerr << d << ' ';
                }
                std::cerr << "] ";
            }
            std::cerr << "\n";
        }
        */
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
    lex_distance_outside_map lodm;
    left_right_distance_map lrdm;

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
                filter_cell(generator,filter,ccons,*gram,*ef);
            } else {
                cube_factory_type csf(*gram,left_map,*ef);
                typename boost::result_of<merge_splits(cube_factory_type,chart_type,cky_generator::partition_range)>::type
                    generator = merge_gen(csf,chrt,parts);
                filter_cell(generator,filter,ccons,*gram,*ef);
            }
        }
        //emit_cell(ccons,*gram,*ef);
        chrt.put_cell(ccons);
        //unary_processor<grammar_in_mem,unary_rulemap,typename chart_type::data_type>
        //    ups(chrt[span],*gram,unary_map,*ef);
        //single_ply_unary_filter_cell(ups,unary_filter,ccons,*gram,*ef,span);
        //emit_cell(ccons,*gram,*ef,true);
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
            //emit_hyps(*ptr,*gram,*ef);
            return *ptr;
        } else {
	  throw std::runtime_error("didn't build any toplevel items");
	  //throw sbmt::empty_chart();        
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
    ef.finish(span);
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

template <class CellProcessor, class Chart, class Edge, class Grammar>
void cky_mt( span_t target_span
           , cky_generator const& ckygen
           , CellProcessor const& proc
           , Chart& chart
           , concrete_edge_factory<Edge,Grammar>& ef
           , size_t numthreads )
{
    thread_pool pool(numthreads);
    for (span_index_t len = 1; len <= ckygen.shift_sizes(target_span); ++len) {
        edge_stats length_edge_stats = ef.stats();
        io::time_space_change length_spent;

        BOOST_FOREACH(span_t span, ckygen.shifts(target_span,len))
        {
            // this condition prevents recursive block decoding from processing
            // a compound span as the leaf in a parent block
            if (boost::empty(chart[span])) {
                pool.add(
                    boost::bind(
                        cky_work<CellProcessor,Chart,Edge,Grammar>
                      , boost::cref(ckygen)
                      , span
                      , boost::cref(proc)
                      , boost::ref(chart)
                      , boost::ref(ef)
                    )
                );
            }
        }
        pool.wait();
        SBMT_TERSE_STREAM(
          cky_domain
        , "length " << len << " spans processed. "<< (ef.stats() - length_edge_stats)
               << " in " << length_spent
        )
        ;
    }
    pool.join();
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

    if (numthreads > 1) {
        cky_mt( blockspan
              , span_transform_generator(sset,ckygen)
              , proc
              , chart
              , ef
              , numthreads );
    } else {
        cky_st( blockspan
              , span_transform_generator(sset,ckygen)
              , proc
              , chart
              , ef );
    }

    remove_virtuals(chart,blockspan);
}
/*
template <class Edge, class Grammar>
std::string partialetree( Grammar& gram
                        , concrete_edge_factory<Edge,Grammar>& ef
                        , edge_equivalence<Edge> const& eq)
{
    if (is_native_tag(eq.representative().root())) {
        derivation_interpreter<Edge,Grammar> interp(gram,ef);
        std::stringstream sstr;
        interp.print_english_tree(sstr,eq);
        return sstr.str();
    }

    size_t ngram_id = gram.prop_constructors.property_map("ngram")["lm_string"];
    Edge const& e = eq.representative();
    typename Grammar::rule_type rule = gram.rule(e.rule_id());
    std::string retval;
    if (not gram.rule_has_property(rule,ngram_id)) return retval;
    indexed_lm_string const&
        lmstr = gram.template rule_property<indexed_lm_string>(rule,ngram_id);
    std::vector< edge_equivalence<Edge> > ntchildren;
    for (size_t x = 0; x != e.child_count(); ++x) {
        if (not is_lexical(e.get_child(x).root())) {
            ntchildren.push_back(e.get_children(x));
        }
    }
    bool first = true;
    BOOST_FOREACH(indexed_lm_token tok, lmstr) {
        if (tok.is_token()) {
            if (not first) retval += " ";
            retval += gram.dict().label(tok.get_token());
            first = false;
        } else {
            std::string r = partialetree(gram,ef,ntchildren.at(tok.get_index()));
            if (r != "") {
                if (not first) retval += " ";
                first = false;
                retval += r;
            }
        }
    }
    return retval;
}
*/

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

    left_right_distance_map
        lrdm = make_left_right_distance_map(
                 skip_lattice(
                   to_dag(ltree,proc.gram->dict())
                 , proc.gram->dict()
                 )
               , proc.gram->dict()
               );
    /*
    std::cerr << token_label(proc.gram->dict());
    BOOST_FOREACH(left_right_distance_map::value_type& od, lrdm)
    {
        BOOST_FOREACH(lex_outside_distances::value_type& vt, od.second.get<0>()) {
            std::cerr << vt.first << "=[ ";
            BOOST_FOREACH(int d, vt.second) {
                std::cerr << d << ' ';
            }
            std::cerr << "] ";
        }
        std::cerr << "<- " << od.first << " -> ";
        BOOST_FOREACH(lex_outside_distances::value_type& vt, od.second.get<1>()) {
            std::cerr << vt.first << "=[ ";
            BOOST_FOREACH(int d, vt.second) {
                std::cerr << d << ' ';
            }
            std::cerr << "] ";
        }
        std::cerr << "\n";
    }
    */
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
            ef.finish(total_span);
            cell_construct<Edge> ccons(total_span);
            BOOST_FOREACH(typename chart_cell<chart_t>::type cell,chrt[total_span]) {
                BOOST_FOREACH(typename chart_edge_equivalence<chart_t>::type eq, cell){
                    ccons.insert(eq);
                }
            }
            ccons.insert(teq);
            chrt.put_cell(ccons);

            /*
            std::map<std::string,std::string>::const_iterator
                bd = ltree.feature_map().find("binarized-derivation");
            if (bd != ltree.feature_map().end()) {
                //std::cerr << "++ found binarized-derivation\n";
                deriv_note_ptr dptr = parse_binarized_derivation(bd->second);
                deriv_note_ptr dparent;
                edge_equivalence<Edge> eq;
                std::vector< edge_equivalence<Edge> > children;
                typedef typename find_parse_error_return<chart_t,Grammar>::type retvec_t;
                retvec_t retvec = find_parse_error(dptr,chrt,*proc.gram);
                bool failure = false;
                int error_idx = 0;
                BOOST_FOREACH(typename retvec_t::value_type r,retvec) {

                    boost::tie(dparent,dptr,eq,children) = r;
                    if (eq.size() == 0) {
                        typedef boost::tuple<int,int,int,int> cell_pos_t;
                        std::vector<cell_pos_t> input_positions;
                        BOOST_FOREACH(edge_equivalence<Edge> ceq, children) {
                            indexed_token root = ceq.representative().root();
                            span_t span = ceq.representative().span();
                            int count = 0;
                            int pos = -1;
                            int counts = 0;
                            int poss = -1;
                            BOOST_FOREACH(typename chart_cell<chart_t>::type cell, chrt[span]) {
                                if (pos == -1 and cell_root(cell) == root) {
                                    pos = count;
                                    BOOST_FOREACH(edge_equivalence<Edge> eqs, cell) {
                                        if (eqs == ceq and poss == -1) {
                                            poss = counts;
                                        }
                                        ++counts;
                                    }
                                }

                                ++count;
                            }
                            input_positions.push_back(boost::make_tuple(pos,count,poss,counts));
                        }

                        failure = true;
                        bool application_found = false;
                        indexed_token root;
                        if (dptr->syn) root = proc.gram->get_syntax(dptr->synid).lhs_root()->get_token();
                        else root = proc.gram->dict().virtual_tag(dptr->tok);
                        size_t total, total_worse, total_inside_worse;
                        total = total_worse = total_inside_worse = 0;
                        BOOST_FOREACH(typename chart_cell<chart_t>::type cell,chrt[dptr->span]) {
                            BOOST_FOREACH(typename chart_edge_equivalence<chart_t>::type eq, cell){
                                ++total;
                                if (not application_found and root == eq.representative().root()) {
                                    if (dptr->syn) {
                                        BOOST_FOREACH(typename chart_edge<chart_t>::type e, eq) {
                                            if (e.syntax_id(*proc.gram) == dptr->synid) application_found = true;
                                        }
                                    } else application_found = true;
                                }
                                if (dptr->total <= eq.representative().score()) ++total_worse;
                                if (dptr->inside <= eq.representative().inside_score()) ++total_inside_worse;
                            }
                        }
                        std::string afoundstr = "";
                        if (application_found) {
                            afoundstr = " rule application found, but not with correct children.";
                        }

                        if (dptr->syn) {
                            std::string rule = boost::lexical_cast<std::string>(dptr->synid);


                            SBMT_INFO_MSG(
                              parse_error_domain
                            , "sent=%s error=%s: failed to apply rule %s over span %s.%s cost worse than %s of %s. inside cost worse than %s"
                            , ltree.id
                            % error_idx
                            % rule
                            % dptr->span
                            % afoundstr
                            % total_worse
                            % total
                            % total_inside_worse
                            );
                        } else {
                            std::string rule = boost::lexical_cast<std::string>(dparent->synid);
                            std::string vrule = dptr->tok;
                            SBMT_INFO_MSG(
                              parse_error_domain
                            , "sent=%s error=%s: failed to apply rule %s over span %s (at virtual rule %s over span %s).%s cost worse than %s of %s. inside cost worse than %s"
                            , ltree.id
                            % error_idx
                            % rule
                            % dparent->span
                            % vrule
                            % dptr->span
                            % afoundstr
                            % total_worse
                            % total
                            % total_inside_worse
                            );
                        }
                        int x = 0;
                        BOOST_FOREACH(cell_pos_t inp, input_positions) {
                            int pos, total, cellpos, celltotal;
                            boost::tie(pos,total,cellpos,celltotal) = inp;
                            SBMT_INFO_MSG(
                              parse_error_domain
                            , "sent=%s error=%s child %s in precube cell %s of %s, %s of %s in that cell"
                            , ltree.id
                            % error_idx
                            % x
                            % (pos+1)
                            % total
                            % (cellpos+1)
                            % celltotal
                            );
                            ++x;
                        }
                        BOOST_FOREACH(typename chart_cell<chart_t>::type cell,chrt[dptr->span]) {
                            BOOST_FOREACH(typename chart_edge_equivalence<chart_t>::type eq, cell) {
                                std::string lmstring;
                                std::string etreestring = partialetree(*proc.gram,ef,eq);
                                BOOST_FOREACH(indexed_lm_token tok,binsent(*proc.gram,eq)) {
                                    lmstring += proc.gram->dict().label(tok.get_token()) + " ";
                                }
                                std::stringstream scores;
                                scores << logmath::neglog10_scale;
                                derivation_interpreter<typename chart_edge<chart_t>::type,Grammar> interp(*proc.gram,ef);
                                interp.print_all_scores(scores,eq);
                                SBMT_INFO_MSG(
                                  parse_error_domain
                                , "sent=%s error=%s root=%s cost-diff=%s cost=%s hyp={{{ %s}}} etree={{{ %s }}} state=%s %s"
                                , ltree.id
                                % error_idx
                                % proc.gram->dict().label(eq.representative().root())
                                % (eq.representative().score()/dptr->total).neglog10()
                                % eq.representative().score().neglog10()
                                % lmstring
                                % etreestring
                                % ef.hash_string(*proc.gram,eq.representative())
                                % scores.str()
                                );
                            }
                        }
                        ++error_idx;
                    }
                }
                if (not failure) {
                    SBMT_INFO_MSG(
                      parse_error_domain
                    , "sent=%s: binarized-derivation found in forest"
                    , ltree.id
                    );
                }
            }
            */
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

} } // namespace sbmt::lazy

# endif //     SBMT__SEARCH__LAZY__CKY_HPP
