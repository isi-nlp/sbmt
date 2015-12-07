#include <sbmt/search/filter_bank.hpp>
#include <sbmt/search/filter_predicates.hpp>
#include <sbmt/search/span_filter.hpp>
#include <sbmt/search/separate_bins_filter.hpp>
#include <sbmt/search/parse_order.hpp>

#include <sbmt/edge/null_info.hpp>
#include <sbmt/grammar/grammar_in_memory.hpp>
#include <sbmt/grammar/brf_archive_io.hpp>
#include <sbmt/sentence.hpp>

#include <boost/shared_ptr.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

using namespace sbmt;
using namespace std;
using namespace boost;

template<class IT, class GT>
string english_tree(edge<IT> const& e, GT& gram);

int main(int argc, char** argv)
{
    typedef edge<null_info>                             edge_type;
    typedef grammar_in_mem                              gram_type;
    typedef concrete_edge_factory<edge_type,gram_typ>   edge_factory_type;
    typedef edge_equivalence_pool< edge<null_info> >    equiv_pool_type;
    typedef edge_factory_type::edge_type                edge_type;
    typedef basic_chart<edge_type>                      chart_type;
    typedef filter_bank<edge_type,gram_type,chart_type> filter_bank_type;
    
    edge_factory_type ef;
    equiv_pool_type epool;
    chart_type chart;
    
    ifstream sentence_file(argv[1]);
    string fss;
    getline(sentence_file,fss);
    fat_sentence s = foreign_sentence(fss);
    
    gram_type gram;
    ifstream gramfile(argv[2]);
    ifstream wt_file(argv[3]);
    brf_archive_reader reader(gramfile);
    string wt_string;
    getline(wt_file,wt_string);
    score_combiner sc(wt_string);
    
    cout << "loading grammar..."<< flush;
    gram.load(reader,sc);
    cout << "done"<< endl;
    
    fat_sentence::iterator itr = s.begin();
    fat_sentence::iterator end = s.end();
   
    int i=0;
    /// initializing the chart with a sentence.
    for(; itr != end; ++itr,++i) {
        edge_type e = ef.create_edge(gram,itr->label(),span_t(i,i+1));
        chart.insert_edge(epool,e);
    }
    span_t target_span = span_t(0,i);
    
    typedef span_filter_factory< edge_type
                               , gram_type
                               , chart_type > spanfilt_factory_t;
                               
    typedef exhaustive_span_factory< edge_type
                                   , gram_type
                                   , chart_type > exhaustive_fact_t;
                               
    typedef shared_ptr<spanfilt_factory_t> spanfilt_factory_ptr;
    
    // predicate_edge_filter.  edge_filter_predicate !?!  did i actually
    // come up with these names ?
    //
    // okay, an edge-filter-predicate is a class that decides whether to
    // keep an edge or not, based on the best and worst edges it sees in a 
    // queue.  the class gets called like:
    //    bool_type p.keep_edge( edge_queue const& equeue
    //                         , edge const& edge_to_insert )
    //    bool      p.pop_top(edge_queue const& equeue)
    // it doesnt actually do anything to the queue. this way the classes can
    // be intersected or unioned.
    //
    // a predicate-edge-filter is an object that holds onto a queue, and one
    // of these predicates, and keeps or rejects edges based on what the
    // predicate object says to do.
    // first it asks the predicate whether to keep the edge.  then, if it
    // keeps the edge, it asks whether to remove the worst edges via p.pop_top()
    edge_filter_predicate<edge_type> p = histogram_predicate<edge_type>(400);
    predicate_edge_filter<edge_type> f(p);
    
    // as ive said before, initializing span-filters can get really tedious
    // as they get more complicated.  really, the user should be able to pass
    // in some structured string expression to a master registry of all filter
    // types, so that complicated expressions dont have to be written in c++.
    // this one is simple though.
    spanfilt_factory_ptr filt_fact(new exhaustive_fact_t(f, target_span));
        
    filter_bank_type filter_bank( filt_fact
                                , gram
                                , ef
                                , epool
                                , chart
                                , target_span ); 
    
    parse_cky(filter_bank,target_span,&cout);
    
    indexed_token TOP = gram.dict().toplevel_tag();

    edge_type best = chart.edges(TOP,target_span).begin()->representative();
    
    cout << english_tree(best,gram);
}

////////////////////////////////////////////////////////////////////////////////
//
// before jon's forest manipulation functions, i had to print my one-best out
// sort of the hard way.  im using my old methods here because at least they 
// give you some examples of manipulating the methods in edges and syntax 
// trees.
//
////////////////////////////////////////////////////////////////////////////////

// children() and children_recurse()
// this method starts at a syntax edge, and recursively descends into its 
// representative childeren until it reaches other syntax edges.  those edges
// are stuffed into a vector 
// 
//
// so if you have a syntac edge e with associated syntax rule
// A(x0:B x1:C) -> x1 "de" x0
// and you call 
//    vector< edge<null_info> > ve = children(e);
// then the first item in ve[0].root() == C and ve[1].root() == B
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
vector< edge<IT> > children(edge<IT> const& e)
{
    assert(is_nonterminal(e.root()));
    vector< edge<IT> > retval;
    back_insert_iterator< vector< edge<IT> > > itr(retval);
    children_recurse(e.first_children().representative(),itr);
    if(e.is_binary())
        children_recurse(e.second_children().representative(),itr);
    return retval;
}

template <class IT, class GT>
string english_tree_recurse( indexed_syntax_rule::tree_node const& node
                           , vector< edge<IT> > const& children_vec
                           , GT& gram )
{
    indexed_token_factory& tf = gram.dict();
    if(node.indexed()) return english_tree(children_vec[node.index()], gram);
    if(node.lexical()) return "\"" + tf.label(node.get_token()) + "\"";
    else {
        string retval = tf.label(node.get_token()) + "(";
        indexed_syntax_rule::lhs_children_iterator itr = node.children_begin();
        indexed_syntax_rule::lhs_children_iterator end = node.children_end();
        if (itr != end) {
            retval += english_tree_recurse(*itr,children_vec,gram);
            ++itr;
        } for(; itr != end; ++itr) {
            retval += "," + english_tree_recurse(*itr,children_vec,gram);
        }
        retval += ")";
        return retval;
    }
}

template<class IT, class GT>
string english_tree(edge<IT> const& e, GT& gram)
{
    indexed_token_factory& tf = gram.dict();
    if(is_lexical(e.root())) return "\"" + tf.label(e.root()) + "\"";
    vector< edge<IT> > children_vec = children(e);
    cout << "syntax rule id: "<< e.syntax_id(gram);     
    indexed_syntax_rule syntax = gram.get_syntax(e.syntax_id(gram));
    return english_tree_recurse(*syntax.lhs_root(),children_vec,gram);
}
