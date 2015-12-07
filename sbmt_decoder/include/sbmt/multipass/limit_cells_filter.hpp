# if ! defined(SBMT__MULTIPASS__LIMIT_CELLS_FILTER_HPP)
# define       SBMT__MULTIPASS__LIMIT_CELLS_FILTER_HPP

# define SBMT_LIMIT_CELLS_SORT

# include <sbmt/search/nested_filter.hpp>
# include <sbmt/search/unary_filter_interface.hpp>
# include <sbmt/multipass/logging.hpp>
# include <sbmt/multipass/cell_heuristic.hpp>
# include <sbmt/grammar/sorted_rhs_map.hpp>
# include <boost/noncopyable.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

template <class GramT>
struct accept_cell : private boost::noncopyable
{
    typedef GramT gram_t;
    typedef cell_heuristic::seen_nts seen_nts;
    typedef typename gram_t::rule_type rule_t;

    seen_nts &nts;
#if      SBMT_PEDANTIC_LEVEL <= SBMT_LOGGING_MINIMUM_LEVEL
    span_t span;
#endif
    gram_t &gram;
    accept_cell(cell_heuristic &cells,span_t const& target_span,gram_t &gram)
        : nts(cells[target_span])
#if      SBMT_PEDANTIC_LEVEL <= SBMT_LOGGING_MINIMUM_LEVEL
        , span(target_span)
#endif
        , gram(gram)
    {}

    score_t operator()(rule_t const& r) const
    {
        seen_nts::const_iterator p=nts.find(gram.rule_lhs(r));
        if (p != nts.end()) {
            SBMT_PEDANTIC_STREAM(limit_cells, "pass outside="<<p->second<<' '
                                 <<print(gram.rule_lhs(r),gram) << span);
            //        return *p;
            return as_one(); // note: ef.score_estimate(g,r) now already includes *p ^ weight_cell_outside.  so we may as well return a bool
        } else {
            SBMT_PEDANTIC_STREAM(limit_cells, "block "
                                 <<print(gram.rule_lhs(r),gram) << span);
            return as_zero();
        }
    }

    bool empty() const
    {
        return nts.empty();
    }
private:
	
};

template <class EdgeT, class GramT, class ChartT>
class limit_cells_filter
 : public nested_filter<EdgeT,GramT,ChartT>
{
 public:
    typedef nested_filter<EdgeT,GramT,ChartT> base_t;
    typedef GramT gram_t;
    typedef span_filter_interface<EdgeT,GramT,ChartT> iface_t;
    typedef cell_heuristic::seen_nts seen_nts;
    typedef typename base_t::rule_range rule_range;
    typedef typename base_t::edge_range edge_range;
    typedef EdgeT edge_t;
    typedef concrete_edge_factory<edge_t,gram_t> efact_t;
# ifdef SBMT_LIMIT_CELLS_SORT
    typedef sorted_rules_for_rhs<gram_t> cache_t;
    typedef typename cache_t::rule_range_t cache_rule_range;
# endif

    limit_cells_filter(cell_heuristic &cells
                       , std::auto_ptr<iface_t> filter // we take ownership
                       , span_t const& target_span
                       , gram_t& gram
                       , efact_t & ef // for sorting by score
                       , typename base_t::chart_type& chart

        )
        : base_t(filter,target_span,gram,ef,chart)
        , accept(cells,target_span,gram)
#ifdef SBMT_LIMIT_CELLS_SORT
        , ef(ef)
        , sorted_cache(gram)
#endif
    {}

    virtual void apply_rules( rule_range const& rr
                            , edge_range const& er1
                            , edge_range const& er2 )
    {
        if (accept.empty()) {
            SBMT_PEDANTIC_STREAM(
                limit_cells
                , "blocked entire span " << base_t::target_span
                );
            return;
        }
        typedef typename iface_t::rule_iterator  rule_it;
        rule_it ritr, rend;
        tie(ritr,rend) = rr;
        if (ritr==rend)
            return;
        typedef typename iface_t::rule_range range_t;
#ifdef SBMT_LIMIT_CELLS_SORT
        cache_rule_range const& range=sorted_cache.lazy_filtered_rules(ritr,rend,accept,ef);
#endif
        base_t::filter->apply_rules(
#ifdef SBMT_LIMIT_CELLS_SORT
            range_t(range.first,range.second)
#else
            range_t(make_filter_iterator(accept, ritr, rend), make_filter_iterator(accept, rend, rend))
#endif
            , er1
            , er2
            );
    }

 private:
    accept_cell<gram_t> accept;
#ifdef SBMT_LIMIT_CELLS_SORT
    efact_t const &ef;
    cache_t sorted_cache;
#endif
};


template <class ET, class GT>
class limit_cells_unary_filter : public nested_unary_filter<ET,GT>
{
 public:
    typedef nested_unary_filter<ET,GT> nuf;
    typedef nuf base_t;
    typedef unary_filter_interface<ET,GT> uf;
    typedef typename uf::rule_range range_t;
    typedef unary_filter_interface<ET,GT> iface_t;
    typedef concrete_edge_factory<ET,GT> efact_t;
    typedef GT gram_t;
# ifdef SBMT_LIMIT_CELLS_SORT
    typedef sorted_rules_for_rhs<gram_t> cache_t;
    typedef typename cache_t::rule_range_t cache_rule_range;
# endif
    limit_cells_unary_filter(cell_heuristic &cells
                             , std::auto_ptr<iface_t> filter // we take ownership
                       , GT& gram
                       , span_t target_span
                        , efact_t & ef_ // for sorting by score
        )
        : base_t(filter,ef_,gram,target_span)
        , accept(cells,target_span,gram)
                            //FIXME: duplicated code from limit_cells_filter
#ifdef SBMT_LIMIT_CELLS_SORT
        , ef(ef_)
        , sorted_cache(gram)
#endif
    {}


    virtual void apply_rules( typename uf::edge_equiv_type source
                              , range_t rr )
    {
        if (accept.empty()) {
            SBMT_PEDANTIC_STREAM(
                limit_cells
                , "(unary) blocked entire span " << base_t::target_span
                );
            return;
        }
        typedef typename base_t::rule_iterator  rule_it;
        rule_it ritr, rend;
        tie(ritr,rend) = rr;
        if (ritr==rend)
            return;
#ifdef SBMT_LIMIT_CELLS_SORT
        cache_rule_range const& range=sorted_cache.lazy_filtered_rules(ritr,rend,accept,ef);
#endif
        base_t::filter->apply_rules(source,
#ifdef SBMT_LIMIT_CELLS_SORT
            range_t(range.first,range.second)
#else
            range_t(make_filter_iterator(accept, ritr, rend), make_filter_iterator(accept, rend, rend))
#endif
            );

    }

    virtual ~limit_cells_unary_filter() {}
private:
    accept_cell<gram_t> accept;
#ifdef SBMT_LIMIT_CELLS_SORT
    efact_t const &ef;
    cache_t sorted_cache;
#endif

};

template <class ET, class GT>
class limit_cells_unary_factory : public unary_filter_factory<ET,GT> {
public:
    typedef unary_filter_factory<ET,GT> unary_filter_factory_;
    typedef unary_filter_factory_ base_t;
    typedef boost::shared_ptr<base_t> factory_p;
    typedef unary_filter_interface<ET,GT> iface_t;
    typedef limit_cells_unary_filter<ET,GT> filter_type;
    typedef GT gram_t;

    virtual bool adjust_for_retry(unsigned retry_i)
    { return factory->adjust_for_retry(retry_i); }

    virtual typename base_t::result_type
    create( span_t const& target_span
                              , GT& gram
                                , concrete_edge_factory<ET,GT>& ef )
    {
        std::auto_ptr<iface_t> p(factory->create(target_span,gram,ef));
        return new filter_type(cells
                              , p
                              , gram
                              , target_span
                              , ef
            );
    }

    //FIXME: code duplication (limit_cells_factory)
    limit_cells_unary_factory(cell_heuristic &cells
                               , factory_p factory
                               , span_t const& total_span
                        ,  gram_t &gram
        )
    : base_t(total_span)
    , factory(factory)
    , cells(cells)
    ,  gram(gram)
    {
        cells.check_target_span(total_span);
    }

protected:
    factory_p factory;
private:
    cell_heuristic &cells;
    gram_t &gram;
};




////////////////////////////////////////////////////////////////////////////////

template <class EdgeT, class GramT, class ChartT>
class limit_cells_factory
 : public nested_filter_factory<EdgeT,GramT,ChartT>
{
    typedef span_filter_interface<EdgeT,GramT,ChartT> sfi_t;
public:
    typedef span_filter_factory<EdgeT,GramT,ChartT> iface_t;
    typedef nested_filter_factory<EdgeT,GramT,ChartT> base_t;
    typedef limit_cells_filter<EdgeT,GramT,ChartT> filter_type;
    typedef GramT gram_t;

    limit_cells_factory(cell_heuristic &cells
                               , boost::shared_ptr<iface_t> factory
                               , span_t const& total_span
                        ,  gram_t &gram
        )
        : base_t(factory,total_span)
    , cells(cells)
    ,  gram(gram)
    {
        cells.check_target_span(total_span);
    }

    virtual void print_settings(std::ostream &o) const
    {
        o << "limit-cells-filter{factory=";
        base_t::factory->print_settings(o);
        o << "}";
    }

    virtual typename iface_t::result_type create( span_t const& target_span
                                               , GramT& gram
                                               , concrete_edge_factory<EdgeT,GramT>& ef
                                               , ChartT& chart )
    {
        std::auto_ptr<sfi_t> p(base_t::create(target_span,gram,ef,chart));
        return new filter_type(cells
                              , p
                              , target_span
                              , gram
                              , ef
                              , chart );
    }

    typedef edge_filter<EdgeT> efilt_t;

    struct accept_edges : public efilt_t
    {
        typedef EdgeT edge_type;
        typedef cell_heuristic::seen_nts seen_nts;
        typedef edge_equivalence<edge_type> equiv_t;
        typedef edge_equivalence_pool<edge_type> epool_t;
        typedef EdgeT edge_t;
        typedef bool bool_t;

        seen_nts &nts;

#if      SBMT_PEDANTIC_LEVEL <= SBMT_LOGGING_MINIMUM_LEVEL
        indexed_token_factory &tf;
        span_t span;
#endif

        accept_edges(efilt_t f,cell_heuristic &cells,indexed_token_factory &tf,span_t const& target_span)
            : efilt_t(f)
            ,nts(cells[target_span])
#if      SBMT_PEDANTIC_LEVEL <= SBMT_LOGGING_MINIMUM_LEVEL
            ,tf(tf)
            ,span(target_span)
#endif
        {}
		
		// default copy constructor broken when dealing with
		// base class with templated parameter on msvc
		accept_edges(accept_edges const& other)
		  : efilt_t(static_cast<efilt_t const&>(other))
		  , nts(other.nts)
#if      SBMT_PEDANTIC_LEVEL <= SBMT_LOGGING_MINIMUM_LEVEL
          , tf(other.tf)
          , span(other.span)
#endif		  
          {}
		  
        bool keep(edge_t const &e)
        {
            bool ret = nts.find(e.root()) != nts.end();
            SBMT_PEDANTIC_STREAM(
                limit_cells
                , (ret?"pass":"block")<<" unary " << sbmt::print(e.root(),tf) << span << " score="<<e.score();
                );
            return ret;
        }

        bool_t insert(epool_t &epool,edge_t const& e)
        {
            bool ret=keep(e);
            if (!ret)
                return false;
            return efilt_t::insert(epool,e);
        }

        bool_t insert(equiv_t const& eq)
        {
            bool ret=keep(eq.representative());
            if (!ret)
                return false;
            return efilt_t::insert(eq);
        }
    };

    // note: should now be unnecessary because of early_exit_from_span_filter_factory
    virtual efilt_t unary_filter(span_t const& target_span)
    {
        return efilt_t(
            accept_edges(base_t::unary_filter(target_span)
                         , cells
                         , gram.dict()
                         , target_span)
            , false);
    }

private:
    accept_edges& operator=(accept_edges const&) { assert(0); }
    cell_heuristic &cells;
    gram_t &gram;
};


////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

# endif //     SBMT__MULTIPASS__LIMIT_CELLS_FILTER_HPP
