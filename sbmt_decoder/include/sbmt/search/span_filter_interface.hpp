#ifndef   SBMT_SEARCH_SPAN_FILTER_INTERFACE_HPP
#define   SBMT_SEARCH_SPAN_FILTER_INTERFACE_HPP



#include  <sbmt/search/concrete_edge_factory.hpp>
#include  <sbmt/search/edge_filter.hpp>
#include  <sbmt/search/filter_predicates.hpp>
#include  <sbmt/search/logging.hpp>
#include  <sbmt/edge/edge.hpp>
#include  <sbmt/edge/edge_equivalence.hpp>
#include  <sbmt/span.hpp>
#include  <iostream>
#include  <sbmt/grammar/sorted_rhs_map.hpp>
#include  <sbmt/hash/concrete_iterator.hpp>
#include  <sbmt/chart/chart.hpp>

namespace sbmt {

SBMT_REGISTER_CHILD_LOGGING_DOMAIN(span_filters,search);
/**
\page example1 A basic parsing example using the sbmt library

To do a basic parse we need 
- a grammar
- a sentence
- a way of combining spans of the sentence
- a way of outputing the final parse

\section grammar the grammar
the only grammars we support are modified CNF form.  And at some
point that grammar needs to look like the output of itg-binarizer.  you will
also need a way of combining the scores in that grammar.

\code
   ostream gramfile("mygrammar.brf");
   brf_stream_reader reader(gramfile);
   score_combiner sc("scr1:0.5,scr2:2.0");
   grammar_in_mem gram;
   gram.load(reader,sc);
\endcode

\section edge_factory the edge_factory
to combine spans of words of a sentence into a parse, we need an edge_factory.
edge_factory is actually a templated family of classes that produce edges.
how you handle the templating determines the types of state and score information
that is present when combining spans (which is simply called the edge_info type).

For a bare bones, translation-model-only, no ngram, no pre-computed outside 
cost, type of edge combining strategy, we use null_info as the info type.
We will also need a chart to collect equivalent edges together.

\code
    typedef info_factory<null_info>         info_factory_t;
    typedef edge_factory<edge<null_info> >  edge_factory_t;
    typedef edge_factory_type::edge_type    edge_t;
    typedef basic_chart<edge_type>          chart_t;
    typedef grammar_in_mem                  gram_t;
    
    edge_factory_t ef;
    chart_t        chart;
\endcode

Due to the templated nature of most of the sbmt library, names can get really 
long.  The "typedef" and "typename" will go a long way toward cleaning up the 
mess.  Also, when compiling, g++ can often deduce when you have declared a 
typedef, which can make compiler errors slightly less painful to read.

\section tokens tokens
We also need a sentence to decode

\code
   fat_sentence s = foreign_sentence("THESE 7PEOPLE INCLUDE COMINGFROM "
                                     "FRANCE AND RUSSIA p-DE ASTRO- -NAUTS .");
\endcode

Throughout the library you may see classes that are prefixed indexed_* or fat_*.
This usually refers to the type of token object it contains.  For instance,
fat_sentence is just a typedef to sentence<fat_token>.  Tokens are essentially 
words that have some type information attached to them that identify themselves
as 
 - foreign words 
 - native words
 - non-terminals
 - virtual non-terminals (created during the binarization process on syntax 
   rules)
 - top-level non-terminal (a rule that represents end of parsing, no rule can 
   have a top-level as its constituent)
   
fat_token is completely self contained.  indexed_token stores its string in a
dictionary to avoid redundency.  it cant print itself out without help of the
dictionary, but it has efficient type and equality checking.  indexed_token is
used almost exclusively by the library; fat_token is mostly for test code, to 
avoid the hassle of using a dictionary.

\section initialization initialize the chart
To start a parse we have to convert our sentence into edges.

\code
    int i=0;
    fat_sentence::iterator itr = s.begin(),
                           end = s.end();
    for(; itr != end; ++itr,++i) {
        edge_type e = ef.create_edge(gram,itr->label(),span_t(i,i+1));
        chart.insert_edge(ef,e);
    }
\endcode

A couple of notes.  The chart will keep track of which edges are equivalent,
and store them in an edge_equivalence object. So when you go to pull edges out
of the chart you will actually be pulling out an edge_equivalence.  
edge_equivalence objects are created by the edge_factory, and their memory is 
tied to the lifetime of the edge_factory, which is why ef is passed to the
chart during insert_edge.  This behaviour is becoming problematic for 
multi-threading.  It will be changed but its not clear how yet.  Either an 
edge_equivalence will essentially be a shared pointer, or its lifetime will be
tied to a different object.  stay tuned.

When a foreign word is promoted to an edge via create_edge, no attention is 
paid to whether the span is length 1.  It just happens to be so here.  This 
should make it easy to write an initialization routine that initializes the 
chart with a lattice.  all that is needed is a new function parameter to pass 
the lattice edge's score.

The grammar is passed to create_edge to turn the foreign word into an 
indexed_token that matches one seen in the grammar.

\section parsing exhaustive parsing
If we just want to exhaustive parse we can do so with a few functions.  
Unfortunately, these functions only exist in test-code.  Thats because 
equivalent code is found in the filter_bank class.  Still, people wishing to try
A* and other parse-orderings should know how to combine edges into a parse 
forest.

An exhaustive parse might start out like this:
\code
    template<class EdgeT, class GramT, class ChartT>
    void lame_cyk_fill_chart( ChartT& chart
                            , GramT& gram
                            , edge_factory<EdgeT>& ef
                            , span_t max_span )
{
    shift_generator sg(max_span,1);
    for (shift_generator::iterator s = sg.begin(); s != sg.end(); ++s) {
        apply_unary_rules(chart,gram,ef,*s);
    }
    
    for (int size = 2; size <= length(max_span); ++size) {
        shift_generator ssg(max_span,size);
        shift_generator::iterator s  = ssg.begin(),
                                  se = ssg.end();
        for (; s != se; ++s) {
            partitions_generator pg(*s);
            partitions_generator::iterator pitr = pg.begin(),
                                           pend = pg.end();
            
            for (; pitr != pend; ++pitr) {
                span_t span_left  = pitr->first;
                span_t span_right = pitr->second;
                apply_binary_rules(chart,gram,ef,span_left,span_right);
            }
        }
    }
}
\endcode

shift_generator and partitions_generator just do common operations on spans
you need for a cyk parse.  shift_generator(span,size) generates all the sub-
spans in span of a given size, from left to right.  partitions_generator 
generates all the splits of a span from smallest on the left to largest on the 
left.

apply_unary_rules looks like this:

\code
template <class EdgeT, class GramT, class ChartT>
void apply_unary_rules( ChartT& chart
                      , GramT const& gram
                      , edge_factory<EdgeT>& ef
                      , span_t s )
{ 
    typedef typename chart_traits<ChartT>::edge_range    edge_range;
    typedef typename chart_traits<ChartT>::edge_iterator edge_iterator;
    typedef typename chart_traits<ChartT>::cell_range    cell_range;
    typedef typename chart_traits<ChartT>::cell_iterator cell_iterator;
    typedef typename GramT::rule_range                   rule_range;
    typedef typename GramT::rule_iterator                rule_iterator;
    typedef typename chart_traits<ChartT>::edge_type     edge_type;
    
    cell_range range = chart.cells(s);
    
    cell_iterator itr = range.begin();
    cell_iterator end = range.end();
    
    for (; itr != end; ++itr) {
        rule_range r = gram.unary_rules(itr->root()); 
        edge_range er = itr->edges();
        
        for (edge_iterator e_itr = er.begin(); e_itr != er.end(); ++e_itr) {
            rule_iterator rule_itr = range.begin();
            rule_iterator rule_end = range.end();
    
            for (; rule_itr != rule_end; ++rule_itr) {
                edge_type e = ef.create_edge(gram, *rule_itr, eq);
                chart.insert_edge(ef,e);
            }
        }
    }
}
\endcode

A note on terminology.  In the chart, a cell is a set of edges that have the 
same span and are rooted with the same token.  In other words, they all will 
have the same rules apply to them.  

Inside a cell will be a collection of edge_equivalence objects.  When doing a
translation-model only decode, each cell will have just one edge_equivalence.  
But an edge_info like ngram_info<3> (trigram) will split the edge_equivalence,
hence the middle loop.

You can imagine the apply_binary_rules written similarly, except more loops or
more nested function calls.  but at the center there is a call:

\code
    edge_type e = ef.create_edge(gram, rule, eq1, eq2);
    chart.insert_edge(ef,e);
\endcode

**/

/**

\page filter_example using filters (pruning)
 the \ref example1 "first example" illustrated an exhaustive cyk parse, but a 
 real cyk parse requires some form of pruning of the edges that are being built 
 to keep the runtime reasonable.  The sbmt library has a flexible pruning system 
 revolving around the edge_queue, edge_filter, span_filter_interface, and 
 filter_bank objects.
 
 \section filter_bank the filter bank
 A filter_bank can be thought of as encapsulating the steps of the cyk algorithm,
 with the following additions:
 - you drive the parse order using its interface (useful for parallelizing cyk)
 - you can inject edge filtering into the edge combining steps.
 
 An example of a parsing function (after initialization) might look like this:
 \code
 template <class filter_bank_type>
 void parse_cyk( filter_bank_type &filter_bank, span_t target_span )
 {
     unsigned chart_size=target_span.size();
    
     for (span_index_t len=1, max=length(target_span); len <= max; ++len) {          
         shift_generator sg(target_span,len);
         shift_generator::iterator sitr = sg.begin();
                                   send = sg.end();
         for (; sitr != send; ++sitr) {
             partitions_generator pg(*sitr);
             partitions_generator::iterator pitr = pg.begin(),
                                            pend = pg.end();
             for (;pitr != pend; ++pitr) {     
                 filter_bank.apply_rules(pitr->first, pitr->second.right());
             }
             filter_bank.finalize(*sitr);
         }
     }
     filter_bank.finalize(target_span);
 }
 \endcode
 
 A few such functions already exist.  One is a vanilla one like the above, but
 with some logging.  Another one handles falling back to increasingly tighter
 filters, using the first function in an inner try block.  There is also work 
 being done on one that puts the middle for loop work into multiple threads 
 (though this requires stronger thread-safety guarantees from filter_bank).
 
 filter_bank::apply_rules(spn, r) will attempt to combine all edges with 
 left constituent span spn, and rule coverage ending at r.  the signature is
 generic enough to support quasi-binary rules in the future.
 
 when edges in the filter_bank are combined, they are not immediately placed in
 the chart.  They are held in temporary storage.  Depending on the filtration
 occuring in the filter_bank, edges may or may not be fully pruned. 
 This delay aids both 
 quasi-binary parsing, and parallelization.  To place items in the chart, you 
 call filter_bank::finalize(span).  Attempting to combine spans that are not
 finalized results in an error. 
 
 A filter_bank is initialized with a reference to a chart, an edge_factory,
 and a span_filter_factory for creating the filtrations that apply to each span.
 
 \section span_filter_interface span filters
 Span filters are a collection of objects conforming to the 
 span_filter_interface.  The basic role of a span_filter is to
 - apply a given rule to all edge_equivalences in the chart at a given span
 - decide which resulting edges to keep (some filters like 
   cube_heap_span_filter may decide which applications to even perform)
 - return the resulting filtered edges
 
 Many span filters are designed to be nested, taking other span_filter_interface
 objects as constructor arguments, which allows arbitrarily complex filtrations
 to occur.
 
 Since span filters have to be created for each span in a filter_bank, and each
 can have a different constructor signature, each span filter implementation 
 should provide a span_filter_factory implementation to allow for uniform 
 creation of span filter objects.
 
 Examples of span filters include:
 - exhaustive_span_filter -- creates all edges and then filters out which 
   ones to keep using an edge_filter functor.
 - cube_heap_span_filter -- heapify the pre-productions based on an estimated 
   score, then use a fuzzy edge_filter to decide which to keep, and when to stop
   making edges out of the heap of pre-productions.
 - separate_bins_filter -- apply a different span filter to virtual and 
   non-virtual productions
   
 Other span filters that could be created include ones that:
 - apply tighter or 
  looser beams, depending on the span being generated
 - keep different 
  percentages of edges depending on the non-terminal token and a prior table of
 nonterminals
 - stop all syntax rules from applying past a certain
  span length.
 - only allow edges to be built which can create a given translation 
   (forced decoding)

 an example:
 the following creates a span filter factory that filters tag and virtual edges
 separately, each getting cube pruned with a beam of 0.125, a histogram of 4, 
 and a fuzz of 0.9: 
 \code    
    typedef span_filter_factory< edge_type
                               , gram_type
                               , chart_type > sff_t;
                               
    typedef cube_heap_factory< edge_type
                             , gram_type
                             , chart_type > cube_heap_f;
                             
    typedef separate_bins_factory< edge_type
                                 , gram_type
                                 , chart_type > join_f;
     
    typedef shared_ptr<sff_t> sff_ptr;
    
    edge_filter_predicate<edge_type,fuzzy_bool> fuzzy_p =
    intersect_predicates( fuzzy_ratio_predicate<edge_type>( score_t(0.125)
                                                          , score_t(0.9) )
                        , fuzzy_histogram_predicate<edge_type>( 4
                                                              , score_t(0.9) ) 
                        );
    
    predicate_edge_filter<edge_type,fuzzy_bool> fuzzy_f(fuzzy_p);
    
    sff_ptr virt_fact(new cube_heap_f(fuzzy_f, target_span));
                                                      
    sff_ptr tag_fact(new cube_heap_f(fuzzy_f, target_span));
    
    sff_ptr join_fact(new join_f(tag_fact, virt_fact, target_span));
                                        
    
 \endcode
 As you can see, flexibility comes with a cost of verbosity.  The end goal is
 to create a markup language to ease building these objects at runtime.
 
 \section edge_filter edge filters, predicate functions, and queues
 Span filters typically defer all pruning decisions to edge_filter objects. 
 Also, span filters do the work of combining edges within a span, whereas 
 edge filters simply decide which built edges to keep and which to discard.
 As a result, even though span_filters may not be appropriate for more best-first
 styles of parsing, edge-filters may.  
 
 Edge filters in existence include predicate_edge_filter (as in above example),
 and cell_edge_filter (which limits the edges produced based on their root token).
 
 **/

////////////////////////////////////////////////////////////////////////////////

template <class EdgeT, class GramT, class ChartT>
class span_filter_interface
{
public:
    typedef typename GramT::rule_type rule_type;
    typedef concrete_forward_iterator<rule_type const> rule_iterator;
    typedef boost::tuple<rule_iterator,rule_iterator> rule_range;

    typedef EdgeT                                     edge_type;
    typedef edge_equivalence<EdgeT>                   edge_equiv_type;
    typedef ChartT                                    chart_type;
    typedef GramT                                     grammar_type;
    typedef typename chart_traits<ChartT>::edge_range edge_range;
    
    span_filter_interface( span_t const& target_span 
                         , grammar_type& g
                         , concrete_edge_factory<EdgeT,GramT>& ef
                         , chart_type& chart )
    : target_span(target_span)
    , gram(g)
    , ef(ef)
    , chart(chart) {}
    
    virtual void apply_rules( rule_range const& rr
                            , edge_range const& er1
                            , edge_range const& er2 ) = 0;
                         
    virtual void finalize() = 0;
    virtual bool is_finalized() const = 0;
    
    virtual edge_equiv_type const& top() const = 0;
    virtual void pop() = 0;
    virtual bool empty() const = 0;
    
    virtual ~span_filter_interface() {}
    edge_equivalence_pool<EdgeT>& edge_equiv_pool() { return epool; }
protected:
    span_t                              target_span;
    grammar_type&                       gram;
    concrete_edge_factory<EdgeT,GramT>& ef;
    edge_equivalence_pool<EdgeT>        epool;
    chart_type&                         chart;
};

////////////////////////////////////////////////////////////////////////////////

template <class EdgeT, class GramT, class ChartT>
class span_filter_factory
{
public:
    typedef span_filter_interface<EdgeT,GramT,ChartT>  filter_type;
    typedef filter_type* result_type;
    typedef boost::shared_ptr<filter_type> shared_p;
    
    span_filter_factory(span_t const& total_span)
    : total_span(total_span){}

    virtual void print_settings(std::ostream &o) const = 0;
    
    ////////////////////////////////////////////////////////////////////////////
    ///
    /// should return true if there is a point to decoding further 
    /// (i.e. if ANY of the parameters have adjusted so that we might 
    /// hope to not run out of memory.  
    /// obviously the first try (retry_i=0) goes on regardless of the 
    /// returned value; however, it will be called.
    ///
    ////////////////////////////////////////////////////////////////////////////
    virtual bool adjust_for_retry(unsigned retry_i)=0;  
    
    virtual result_type create( span_t const& target_span
                              , GramT& gram
                              , concrete_edge_factory<EdgeT,GramT>& ecs
                              , ChartT& chart ) = 0;

    shared_p create_shared(
        span_t const& target_span
        , GramT& gram
        , concrete_edge_factory<EdgeT,GramT>& ecs
        , ChartT& chart )
    {
        return shared_p(create(target_span,gram,ecs,chart));
    }
    
    virtual edge_filter<EdgeT> unary_filter(span_t const& target_span) = 0;
    
    virtual ~span_filter_factory(){}
    
protected:
    span_t total_span;
};

////////////////////////////////////////////////////////////////////////////////

template <class E, class G, class C>
std::ostream& 
operator << (std::ostream& os, span_filter_factory<E,G,C> const& f)
{
    f.print_settings(os);
    return os;
}
    
} // namespace sbmt

#endif // SBMT_SEARCH_SPAN_FILTER_INTERFACE_HPP
