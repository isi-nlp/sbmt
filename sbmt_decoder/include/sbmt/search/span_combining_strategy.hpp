#ifndef   SBMT_STRATEGIES_SPAN_COMBINING_STRATEGY_HPP
#define   SBMT_STRATEGIES_SPAN_COMBINING_STRATEGY_HPP
#if 0
#include <sbmt/search/concrete_edge_factory.hpp>
#include <sbmt/span.hpp>

namespace sbmt {
    
////////////////////////////////////////////////////////////////////////////////
///
///  a span combining strategy basically handles the specifics of how
///  a new chart span is filled by specifying two (or one) previously filled 
///  chart spans.
///
///  a span combining strategy is an algorithm type.  it should not be 
///  considered some heavy thing.  the pieces it works with it holds by
///  reference, namely: your grammar, your edge factory, and your chart.
///
///  \todo: would it be useful if the strategy raised exceptions if it was used
///  in a way where a span is combined after being used as an argument of a
///  combination?
///
////////////////////////////////////////////////////////////////////////////////
template <class DerivedT>
class span_combining_strategy 
{
public:
    typedef typename DerivedT::grammar_type grammar_type;
    typedef typename DerivedT::edge_type    edge_type;
    typedef typename DerivedT::chart_type   chart_type;
    
    span_combining_strategy( grammar_type& g ,
                           , edge_creation_strategy<edge_type,grammar_type> ecs
                           , chart_type& );
    
    void apply_unary_rules(span_t s)
    {
        derived().apply_unary_rules(s);
    }
    
    void apply_binary_rules(span_t left_span, span_t right_span)
    {
        derived().apply_binary_rules(left_span, right_span);
    }
    
    /// same as apply_unary_rules (some folks just like functional notation)
    void operator()(span_t s)
    { return this->apply_unary_rules(s); }
    
    /// same as apply_binary_rules (some folks just like functional notation)
    void operator()(span_t left_span, span_t right_span)
    { return this->apply_binary_rules(left_span, right_span); }

protected:    
    grammar_type& get_grammar() { return gram.ref(); }
    ecs_type&     get_edge_creator() { return ecs.ref(); }
    chart_type&   get_chart() { return chart.ref(); }
private:
    DerivedT & derived() { return static_cast<DerivedT&>(*this); }
    typedef edge_creation_strategy<edge_type,grammar_type> ecs_type;
    boost::reference_wrapper<grammar_type> gram;
    boost::reference_wrapper<ecs_type>     ecs;
    boost::reference_wrapper<chart_type>   chart;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt
#endif
#endif // SBMT_STRATEGIES_SPAN_COMBINING_STRATEGY_HPP
