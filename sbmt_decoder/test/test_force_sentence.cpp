/*# include <boost/test/auto_unit_test.hpp>
# include <boost/shared_ptr.hpp>

# include <iostream>
# include <fstream>
# include <string>

# include <sbmt/grammar/grammar_in_memory.hpp>
# include <sbmt/grammar/brf_file_reader.hpp>
# include <sbmt/search/filter_bank.hpp>
# include <sbmt/search/force_sentence_filter.hpp>
# include <sbmt/search/span_filter.hpp>
# include <sbmt/search/parse_order.hpp>
# include <sbmt/edge/sentence_info.hpp>
# include <sbmt/edge/null_info.hpp>
# include <sbmt/chart/force_sentence_cell.hpp>
*/
//using namespace std ;
//using namespace sbmt ;
//using namespace boost ;

//FIXME: too many compile errors
# if 0

template <class IT, class ItrT>
void children_recurse(edge<IT> const& e, ItrT& input)
{
    if (is_lexical(e.root()) or e.root().type() == tag_token) {
        *input = e; 
        ++input; 
    } else {
        children_recurse(e.first_children().representative(), input);
        if (e.is_binary())
            children_recurse(e.second_children().representative(), input);
    }
}

template <class IT>
std::vector< edge<IT> > children(edge<IT> const& e)
{
    assert(is_nonterminal(e.root()));
    std::vector< edge<IT> > retval;
    std::back_insert_iterator< std::vector< edge<IT> > > itr(retval);
    children_recurse(e.first_children().representative(),itr);
    if(e.is_binary())
        children_recurse(e.second_children().representative(),itr);
    return retval;
}

template<class IT, class GT>
std::string english_sentence(edge<IT> const& e, GT& gram) ;

template <class IT, class GT>
std::string english_sent_recurse( indexed_syntax_rule::tree_node const& node
                                , std::vector< edge<IT> > const& children_vec
                                , GT& gram )
{
    indexed_token_factory &tf = gram.dict();
    if(node.indexed()) return english_sentence(children_vec[node.index()], gram);
    if(node.lexical()) return tf.label(node.get_token()) ;
    else {
        std::string retval ;
        indexed_syntax_rule::lhs_children_iterator itr = node.children_begin() ;
        indexed_syntax_rule::lhs_children_iterator end = node.children_end() ;
        if (itr != end) {
            retval += english_sent_recurse(*itr,children_vec,gram) ;
            ++itr ;
        } for(; itr != end; ++itr) {
            retval += " " + english_sent_recurse(*itr,children_vec,gram) ;
        }
        return retval;
    }
}

template<class IT, class GT>
std::string english_sentence(edge<IT> const& e, GT& gram)
{
    indexed_token_factory &tf = gram.dict();
    if(is_lexical(e.root())) return tf.label(e.root()) ;
    std::vector< edge<IT> > children_vec = children(e) ;
    indexed_syntax_rule syntax = gram.get_syntax(e.syntax_id(gram)) ;
    return english_sent_recurse(*syntax.lhs_root(),children_vec,gram) ;
}

void grammar_from_stream(istream& gram_str,grammar_in_mem& gram)
{
    brf_stream_reader reader(gram_str) ;
    score_combiner sc ;
    gram.load(reader,sc) ;
}

template <class ET,class GT> 
basic_chart<ET,force_sentence_chart_edges_policy> 
chart_from_sent( indexed_sentence s
                   , concrete_edge_factory<ET,GT> &ef )
{
    edge_equivalence_pool<ET> epool ;
    basic_chart<ET,force_sentence_chart_edges_policy> chart(s.size()) ;
    indexed_sentence::iterator i = s.begin(), e = s.end() ;
    span_t total_span(0,s.size()) ;
    shift_generator sg(total_span,1) ;
    shift_generator::iterator si = sg.begin() ;
    for (;i != e; ++i, ++si) {
        chart.insert_edge(epool, ef.create_edge(*i,*si)) ;
    }
    
    return chart ;
}

void force_decode (string testfilename)
{
    typedef edge<sentence_info> edge_t ;
    //typedef edge<null_info>     edge_t ;
    typedef grammar_in_mem      gram_t ;
    typedef force_grammar<gram_t> fgram_t;
    typedef basic_chart<edge_t, force_sentence_chart_edges_policy> chart_t ;
    
    typedef filter_bank<edge_t,fgram_t,chart_t>            filter_bank_t ;   
    typedef force_sentence_factory<edge_t,fgram_t,chart_t> filter_factory_t ;
    //typedef exhaustive_span_factory<edge_t,gram_t,chart_t> filter_factory_t ;
    
    gram_t gram ;
    ifstream testfile(testfilename.c_str()) ;
    string input ;
    getline(testfile,input) ;
    string output ;
    getline(testfile,output) ;
    
    grammar_from_stream(testfile,gram) ;
    indexed_sentence in = foreign_sentence(input, gram.dict()) ;
    indexed_sentence out = native_sentence(output, gram.dict()) ;

    edge_factory<info_factory<sentence_info> > ef;
    concrete_edge_factory<edge_t,gram_t> efact(ef);
    
    edge_equivalence_pool<edge_t> epool ;
    
    chart_t chart = chart_from_sent<edge_t,gram_t>(in,efact) ;
    
    span_t target(0,in.size()) ;
        
    force_grammar<gram_t> fgram(gram,out);

    //predicate_edge_filter<edge_t> efilt = histogram_predicate<edge_t>(100) ;
    //shared_ptr<filter_factory_t> 
    //    ffact(new filter_factory_t(efilt,target)) ;
    
    filter_bank_t fbank(new filter_factory_t(out,target), fgram, efact, epool, chart, target) ;
    parse_cky cky;
    cky(fbank, target) ;
    
    indexed_token S = gram.dict().toplevel_tag() ;
    
    stringstream failure;
    failure << print(chart,gram.dict()) << endl ;
    failure << "input:" << input << "  output:" << output ;
    BOOST_REQUIRE_MESSAGE(
        chart.edges(S,target).begin() !=
        chart.edges(S,target).end()
      , failure.str()
    ) ;
    chart_t::edge_iterator itr = chart.edges(S,target).begin() ,
                           end = chart.edges(S,target).end() ;
                           
    for (; itr != end; ++itr) {
        edge_t e = itr->representative() ;
        
        BOOST_CHECK_MESSAGE(
            english_sentence(e,gram) == output
          , failure.str()
        ) ;
    }

}

BOOST_AUTO_TEST_CASE(test_force_decode)
{
    force_decode(SBMT_TEST_DIR "/force.brf") ;
    force_decode(SBMT_TEST_DIR "/forceswap.brf");
}

# endif 
