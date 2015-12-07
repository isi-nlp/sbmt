# if ! defined(SBMT__SEARCH__UNARY_FILTER_INTERFACE_HPP)
# define       SBMT__SEARCH__UNARY_FILTER_INTERFACE_HPP

# include <sbmt/edge/edge.hpp>
# include <sbmt/search/edge_filter.hpp>
# include <sbmt/search/concrete_edge_factory.hpp>
# include <sbmt/search/span_filter_interface.hpp>
# include <sbmt/hash/concrete_iterator.hpp>

# include <boost/shared_ptr.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/scoped_ptr.hpp>
# include <boost/iterator/transform_iterator.hpp>

# include <gusc/generator/generator_from_iterator.hpp>
# include <gusc/generator/union_heap_generator.hpp>
# include <gusc/iterator/iterator_from_generator.hpp>

# include <list>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT>
class unary_filter_interface {
public:
    
    typedef unary_filter_interface<ET,GT> unary_filter_;
    typedef typename GT::rule_type rule_type;
    typedef concrete_forward_iterator<rule_type const> rule_iterator;
    typedef boost::tuple<rule_iterator,rule_iterator>  rule_range;
    typedef ET                                         edge_type;
    typedef edge_equivalence<ET>                       edge_equiv_type;

    unary_filter_interface( concrete_edge_factory<ET,GT>& ef
                          , GT& gram
                          , span_t target_span )
      : ef(ef), gram(gram), target_span(target_span) {}
    
    virtual void apply_rules(edge_equiv_type source, rule_range rr) = 0;
    
    virtual edge_type const& top() const = 0;
    
    virtual void pop() = 0;
    
    virtual bool empty() const = 0;
    
    virtual ~unary_filter_interface() {}

    concrete_edge_factory<ET,GT>& ef;
    GT& gram;
    span_t target_span;
};

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT>
class unary_filter_factory {
public:
    typedef unary_filter_factory<ET,GT> unary_filter_factory_;
    
    typedef unary_filter_interface<ET,GT> filter_type;
//    typedef boost::shared_ptr<filter_type> result_type;
    typedef filter_type *result_type;
    typedef boost::shared_ptr<filter_type> shared_p;
    
    /// I assume this is used to adjust e.g. unary poplimit, fuzz, depthlimit when OOM
    virtual bool adjust_for_retry(unsigned retry_i) = 0;  
    
    unary_filter_factory(span_t const& total_span)
      : total_span(total_span) {}
    
    virtual result_type create( span_t const& target_span
                              , GT& gram
                              , concrete_edge_factory<ET,GT>& ef ) = 0;

    shared_p create_shared( span_t const& target_span
                              , GT& gram
                            , concrete_edge_factory<ET,GT>& ef )
    {
        return shared_p(create(target_span,gram,ef));
    }
    
    virtual ~unary_filter_factory() {}
protected:
    span_t total_span;
};

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT>
class nested_unary_filter : public unary_filter_interface<ET,GT> {
public:
    typedef unary_filter_interface<ET,GT> unary_filter_;
    
    // note: you don't own this pointer after you pass it in.
    // auto_ptr provides for exception-safe transfer
    nested_unary_filter( std::auto_ptr<unary_filter_> filter   
                       , concrete_edge_factory<ET,GT>& ef
                       , GT& gram
                       , span_t target_span )
      : unary_filter_(ef,gram,target_span)
      , filter(filter) {}
      
    virtual void apply_rules( typename unary_filter_::edge_equiv_type source
                            , typename unary_filter_::rule_range rr )
    {
        filter->apply_rules(source,rr);
    }
    
    virtual typename unary_filter_::edge_type const& top() const
    {
        return filter->top();
    }
    
    virtual void pop() { filter->pop(); }
    
    virtual bool empty() const { return filter->empty(); }
protected:
    boost::scoped_ptr<unary_filter_> filter;
};

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT>
class stub_unary_filter
  : public unary_filter_interface<ET,GT> {
public:
    typedef unary_filter_interface<ET,GT> unary_filter_;
    template <class F>
    stub_unary_filter( F filt
                     , concrete_edge_factory<ET,GT>& ef
                     , GT& gram
                     , span_t target_span )
      : unary_filter_(ef,gram,target_span)
      , filt(filt) {}
    
    virtual void pop() { storage.pop_front(); }
    
    virtual typename unary_filter_::edge_type const& top() const 
    { return storage.front(); }
    
    virtual bool empty() const { return storage.empty(); }
    
    virtual void apply_rules( typename unary_filter_::edge_equiv_type source
                            , typename unary_filter_::rule_range rr ) = 0;
    
protected:
    bool add_edge(typename unary_filter_::edge_type const& e) 
    {
        bool retval = filt.insert(dummy_pool,e);
        if (retval) {
            storage.push_back(e);
        }
        return retval;
    }
    
private:
    edge_equivalence_pool<ET> dummy_pool;
    edge_filter<ET> filt;
    std::list<ET> storage;
};

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT>
class early_exit_unary_filter
  : public stub_unary_filter<ET,GT> {
public:
    
    typedef unary_filter_interface<ET,GT> unary_filter_;
    typedef stub_unary_filter<ET,GT> base_;
    
    struct edge_creator {
        typedef gusc::iterator_from_generator<gusc::any_generator<ET> > result_type;
        GT* gram;
        concrete_edge_factory<ET,GT>* ef;
        typename unary_filter_::edge_equiv_type source;
        
        edge_creator( GT* gram
                    , concrete_edge_factory<ET,GT>* ef
                    , typename unary_filter_::edge_equiv_type source)
        : gram(gram)
        , ef(ef)
        , source(source) {}
        
        result_type operator()(typename unary_filter_::rule_type r) const
        {
            return result_type(ef->create_edge(*gram,r,source));
        }
    };
    
    template <class F>
    early_exit_unary_filter( F f
                           , concrete_edge_factory<ET,GT>& ef
                           , GT& gram
                           , span_t target )
      : base_(f,ef,gram,target)
      , grammar(gram)
      , cef(ef) {}
    
    GT& grammar;
    concrete_edge_factory<ET,GT>& cef;
    
    virtual void apply_rules( typename unary_filter_::edge_equiv_type source
                            , typename unary_filter_::rule_range rr) 
    {
        edge_creator ec(&grammar,&cef,source);
        typename unary_filter_::rule_iterator ritr = rr.template get<0>(), 
                                              rend = rr.template get<1>();
                                              
        typedef gusc::generator_from_iterator<
            boost::transform_iterator<
                edge_creator
              , typename unary_filter_::rule_iterator
            >
        > gen_t;
        gen_t gen(boost::make_transform_iterator(ritr,ec), boost::make_transform_iterator(rend,ec));
        
        gusc::union_heap_generator<gen_t,lesser_edge_score<ET> > union_gen(gen);
        
        while (union_gen) {
            if (not base_::add_edge(union_gen())) break;
        }
    }
};

template <class ET, class GT> class early_exit_unary_factory
  : public unary_filter_factory<ET,GT> {
public:
    typedef unary_filter_factory<ET,GT> unary_filter_factory_;
    template <class F>
    early_exit_unary_factory(F filt) : filt(filt) {}
    
    virtual bool adjust_for_retry(unsigned retry_i)
    {
        return filt.adjust_for_retry(retry_i);
    }  
    
    virtual typename unary_filter_factory_::result_type 
    create( span_t const& target
          , GT& gram
          , concrete_edge_factory<ET,GT>& ef )
    {
        return typename unary_filter_factory_::result_type(
                          new early_exit_unary_filter<ET,GT>(filt,ef,gram,target)
                        );
    }
    edge_filter<ET> filt;
};

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT>
class exhaustive_unary_filter
  : public stub_unary_filter<ET,GT> {
public:
    typedef unary_filter_interface<ET,GT> unary_filter_;
    typedef stub_unary_filter<ET,GT> base_;
    
    template <class F>
    exhaustive_unary_filter( F f
                           , concrete_edge_factory<ET,GT>& ef
                           , GT& gram
                           , span_t target )
      : base_(f,ef,gram,target) {}
    
    virtual void apply_rules( typename unary_filter_::edge_equiv_type source
                            , typename unary_filter_::rule_range rr ) 
    {
        typename unary_filter_::rule_iterator ritr = rr.template get<0>(), 
                                              rend = rr.template get<1>();
        for (; ritr != rend; ++ritr) {
            gusc::any_generator<ET> 
                gen = base_::ef.create_edge(base_::gram,*ritr,source);
            while (gen) {
                base_::add_edge(gen());
            }
        }
    }
};

template <class ET, class GT> class exhaustive_unary_factory
  : public unary_filter_factory<ET,GT> {
public:
    typedef unary_filter_factory<ET,GT> unary_filter_factory_;
    template <class F>
    exhaustive_unary_factory(F filt) : filt(filt) {}
    
    virtual bool adjust_for_retry(unsigned retry_i)
    {
        return filt.adjust_for_retry(retry_i);
    }  
    
    virtual typename unary_filter_factory_::result_type 
    create( span_t const& target_span
          , GT& gram
          , concrete_edge_factory<ET,GT>& ef )
    {
        return typename unary_filter_factory_::result_type(
                          new exhaustive_unary_filter<ET,GT>(filt,ef,gram,target_span)
                        );
    }
    edge_filter<ET> filt;
};

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
class early_exit_from_span_filter_factory
  : public unary_filter_factory<ET,GT>
{
public:
    typedef unary_filter_factory<ET,GT> unary_filter_factory_;
    typedef span_filter_factory<ET,GT,CT> span_filter_factory_;
    
    early_exit_from_span_filter_factory(
        boost::shared_ptr<span_filter_factory_> span_f
      , span_t total_span 
    ) 
      : unary_filter_factory_(total_span)
      , span_f(span_f) { }
    
    virtual bool adjust_for_retry(unsigned retry_i)
    {
        return span_f->adjust_for_retry(retry_i);
    }
    
    virtual typename unary_filter_factory_::result_type
    create ( span_t const& target_span
           , GT& gram
           , concrete_edge_factory<ET,GT>& ef )
    {
        return typename unary_filter_factory_::result_type(
                          new early_exit_unary_filter<ET,GT>(
                                  span_f->unary_filter(target_span)
                                , ef
                                , gram
                                , target_span )
                        );
    }
private:
    boost::shared_ptr<span_filter_factory_> span_f;
};

template <class ET, class GT, class CT>
class exhaustive_unary_from_span_filter_factory
  : public unary_filter_factory<ET,GT>
{
public:
    typedef unary_filter_factory<ET,GT> unary_filter_factory_;
    typedef span_filter_factory<ET,GT,CT> span_filter_factory_;
    
    exhaustive_unary_from_span_filter_factory(
        boost::shared_ptr<span_filter_factory_> span_f
      , span_t total_span 
    ) 
      : unary_filter_factory_(total_span)
      , span_f(span_f) { }
    
    virtual bool adjust_for_retry(unsigned retry_i)
    {
        return span_f->adjust_for_retry(retry_i);
    }
    
    virtual typename unary_filter_factory_::result_type
    create ( span_t const& target_span
           , GT& gram
           , concrete_edge_factory<ET,GT>& ef )
    {
        return typename unary_filter_factory_::result_type(
                          new exhaustive_unary_filter<ET,GT>(
                                  span_f->unary_filter(target_span)
                                , ef
                                , gram
                                , target_span )
                        );
    }
private:
    boost::shared_ptr<span_filter_factory_> span_f;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

# endif //     SBMT__SEARCH__UNARY_FILTER_INTERFACE_HPP
