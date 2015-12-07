# if ! defined(SBMT__SEARCH__LIMIT_SYNTAX_LENGTH_FILTER_HPP)
# define       SBMT__SEARCH__LIMIT_SYNTAX_LENGTH_FILTER_HPP

# include <sbmt/search/nested_filter.hpp>
# include <sbmt/search/logging.hpp>

namespace sbmt {
    
SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME( lsf_domain
                                       , "limit_syntax_length"
                                       , filter_domain );

////////////////////////////////////////////////////////////////////////////////

template <class EdgeT, class GramT, class ChartT>
class limit_syntax_length_filter
 : public nested_filter<EdgeT,GramT,ChartT>
{
public:
    typedef nested_filter<EdgeT,GramT,ChartT> base_t;
    typedef span_filter_interface<EdgeT,GramT,ChartT> iface_t;
    
    limit_syntax_length_filter( std::string glue_symbol
                              , bool limit_syntax
                              , std::auto_ptr<iface_t> filter // note: you don't own this pointer after you pass it in.
                              , span_t const& target_span 
                              , typename base_t::grammar_type& gram
                              , concrete_edge_factory<EdgeT,GramT>& ef
                              , typename base_t::chart_type& chart )
      : base_t(filter,target_span,gram,ef,chart)
      , glue_symbol(gram.dict().tag(glue_symbol))
      , toplevel_symbol(gram.dict().toplevel_tag())
      , limit_syntax(limit_syntax) {}
                         
    struct top_or_glue {
        top_or_glue( typename base_t::grammar_type const& gram
                   , indexed_token glue_symbol
                   , indexed_token toplevel_symbol )
          : gram(&gram)
          , glue_symbol(glue_symbol)
          , toplevel_symbol(toplevel_symbol) {}
        
        typedef bool result_type;
        bool operator()(typename base_t::grammar_type::rule_type const& r) const
        {
            return gram->rule_lhs(r) == glue_symbol
                or gram->rule_lhs(r) == toplevel_symbol;
        }
        
        typename base_t::grammar_type const* gram;
        indexed_token glue_symbol;
        indexed_token toplevel_symbol;
    };
    
    virtual void apply_rules( typename base_t::rule_range const& rr
                            , typename base_t::edge_range const& er1
                            , typename base_t::edge_range const& er2 )
    {
        if (limit_syntax) {
            using namespace boost;
            typename base_t::rule_iterator ritr, rend;
            tie(ritr,rend) = rr;
            
            base_t::filter->apply_rules(
                make_tuple(
                    make_filter_iterator(
                        top_or_glue(base_t::gram,glue_symbol,toplevel_symbol)
                      , ritr
                      , rend
                    )
                  , make_filter_iterator(
                        top_or_glue(base_t::gram,glue_symbol,toplevel_symbol)
                      , rend
                      , rend
                    )
                )
              , er1
              , er2
            );
        } else {
            base_t::filter->apply_rules(rr,er1,er2);
        }
    }

    virtual ~limit_syntax_length_filter() {}
private:
    indexed_token glue_symbol;
    indexed_token toplevel_symbol;
    bool limit_syntax;
};

////////////////////////////////////////////////////////////////////////////////

template <class EdgeT, class GramT, class ChartT>
class limit_syntax_length_factory
 : public nested_filter_factory<EdgeT,GramT,ChartT>
{
    typedef span_filter_interface<EdgeT,GramT,ChartT> sfi_t;
public:
    typedef nested_filter_factory<EdgeT,GramT,ChartT> base_t;
    typedef span_filter_factory<EdgeT,GramT,ChartT> iface_t;
    typedef limit_syntax_length_filter<EdgeT,GramT,ChartT> filter_type;
    
    limit_syntax_length_factory( std::string glue_symbol
                               , std::size_t max_length
                               , boost::shared_ptr<iface_t> factory
                               , span_t const& total_span )
    : base_t(factory,total_span)
    , glue_symbol(glue_symbol)
    , max_length(max_length) {}

    virtual void print_settings(std::ostream &o) const
    { 
        std::stringstream sstr;
        base_t::factory->print_settings(sstr);
        o << "limit-syntax-length{"
          << "glue-symbol="<<glue_symbol << ","
          << "max-length=" <<max_length << ","
          << "factory=" << sstr.str() << "}";
    }
    
    virtual typename base_t::result_type create( span_t const& target_span
                                               , GramT& gram
                                               , concrete_edge_factory<EdgeT,GramT>& ef
                                               , ChartT& chart ) 
    {
        std::auto_ptr<sfi_t> p(base_t::create(target_span,gram,ef,chart));
        return new filter_type( glue_symbol
                              , max_length < length(target_span)
                              , p
                              , target_span
                              , gram
                              , ef
                              , chart );
    }
    
    virtual ~limit_syntax_length_factory(){}
private:
    std::string glue_symbol;
    std::size_t max_length;
};

////////////////////////////////////////////////////////////////////////////////
    
} // namespace sbmt

# endif //     SBMT__SEARCH__LIMIT_SYNTAX_LENGTH_FILTER_HPP
