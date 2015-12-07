# if ! defined(SBMT__SEARCH__NEGATIVE_BLOCK_FILTER_HPP)
# define       SBMT__SEARCH__NEGATIVE_BLOCK_FILTER_HPP

# include <boost/shared_ptr.hpp>
# include <sbmt/search/span_filter_interface.hpp>
# include <sbmt/search/nested_filter.hpp>
# include <sbmt/grammar/grammar.hpp>

# include <boost/foreach.hpp>
# include <set>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

template <class Grammar>
struct is_virt_rule {
    typedef bool result_type;
    is_virt_rule(Grammar const& grammar) : grammar(&grammar) {}
    bool operator()(typename Grammar::rule_type const& r) const
    {
        return grammar->rule_lhs(r).type() == virtual_tag_token;
    }
private:
    Grammar const* grammar;
};

////////////////////////////////////////////////////////////////////////////////

template <class Edge, class Grammar, class Chart>
class negative_block_filter 
  : public nested_filter<Edge,Grammar,Chart>
{
private:
    typedef nested_filter<Edge,Grammar,Chart> base_t;
    typedef span_filter_interface<Edge,Grammar,Chart> span_filter_t;
public:
    negative_block_filter( std::auto_ptr<span_filter_t> filter
                         , span_t const& target
                         , typename base_t::grammar_type& gram
                         , concrete_edge_factory<Edge,Grammar>& ef
                         , typename base_t::chart_type& c
                         , bool banned ) 
      : base_t(filter,target,gram,ef,c)
      , banned(banned) {}
      
    virtual void apply_rules( typename base_t::rule_range const& rr
                            , typename base_t::edge_range const& er1
                            , typename base_t::edge_range const& er2 )
    {
        if (not banned) {
            base_t::apply_rules(rr, er1,er2);
        } else {
            using namespace boost;
            typename base_t::rule_iterator ritr, rend;
            tie(ritr,rend) = rr;
            base_t::apply_rules(
                make_tuple(
                    make_filter_iterator(is_virt_rule<Grammar>(base_t::gram), ritr, rend)
                  , make_filter_iterator(is_virt_rule<Grammar>(base_t::gram), rend, rend)
                )
              , er1
              , er2
            );
        }
    }
private:
    bool banned;
};

////////////////////////////////////////////////////////////////////////////////

template <class Edge, class Grammar, class Chart>
class negative_block_filter_factory
 : public span_filter_factory<Edge,Grammar,Chart>
{
public:
    typedef span_filter_factory<Edge,Grammar,Chart> base_t;
    typedef span_filter_interface<Edge,Grammar,Chart>  base_filter_type;
    typedef base_filter_type* result_type;

    template <class Blocks>
    negative_block_filter_factory( boost::shared_ptr<base_t> factory
                                 , span_t const& total_span
                                 , Blocks const& blks )
    : base_t(total_span)
    , factory(factory)
    , blocks(boost::begin(blks), boost::end(blks)) {}

    virtual void print_settings(std::ostream &o) const
    {  
        o << "negative-block-filter{filter:";
        factory->print_settings(o);
        o << "}";
    }

    virtual bool adjust_for_retry(unsigned retry_i) 
    { return factory->adjust_for_retry(retry_i); }  

    virtual result_type create( span_t const& target_span
                              , Grammar& gram
                              , concrete_edge_factory<Edge,Grammar>& ecs
                              , Chart& chart ) 
    {
        bool banned = (blocks.find(target_span) == blocks.end());

        std::auto_ptr<base_filter_type> ptr(factory->create( target_span
                                                           , gram
                                                           , ecs
                                                           , chart ));
        return new negative_block_filter<Edge,Grammar,Chart>( ptr
                                                            , target_span
                                                            , gram
                                                            , ecs
                                                            , chart
                                                            , banned
                                                            );
    }

    virtual edge_filter<Edge> unary_filter(span_t const& target_span) 
    {
        return factory->unary_filter(target_span);
    }

    virtual ~negative_block_filter_factory(){}

private:
    boost::shared_ptr<base_t> factory;
    std::set<span_t> blocks;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

# endif //     SBMT__SEARCH__NEGATIVE_BLOCK_FILTER_HPP
