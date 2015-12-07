#ifndef   SBMT_SEARCH_CONCRETE_EDGE_FACTORY_HPP
#define   SBMT_SEARCH_CONCRETE_EDGE_FACTORY_HPP
#include <graehl/shared/intrusive_refcount.hpp>
#include <sbmt/edge/edge.hpp>
#include <sbmt/grammar/grammar.hpp>
#include <gusc/functional.hpp>
#include <gusc/generator/any_generator.hpp>
#include <boost/utility/typed_in_place_factory.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp> // for boost::noncopyable
#include <boost/function.hpp>
#include <string>
#include <sbmt/feature/feature_vector.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
///
///  concrete_edge_factory is really an edge_factory.  its just that
///  edge factories depend on a template type that is completely irrelevent to
///  the way it interfaces with other strategies: the InfoFactoryType.  This
///  class does nothing more than erase the InfoFactoryType, by introducing
///  dependencies on an EdgeType and a GrammarType.  This is acceptable, because
///  those two types are the only ones that factor into an edge factories
///  interface, and because all the rest of the strategies depend upon those two
///  as well.
///
///  the net effect is that interface between strategies is cleaner.
/// 
///  it holds only a reference to underlying implementation factory.
///
////////////////////////////////////////////////////////////////////////////////
template <class EdgeT, class GrammarT>
class concrete_edge_factory
{
public:
    typedef gusc::any_generator<EdgeT>                       generator_type;
    typedef EdgeT                                            edge_type;
    typedef edge_equivalence<edge_type>                      edge_equiv_type;
    typedef GrammarT                                         grammar_type;
    typedef typename grammar_type::rule_type                 rule_type;

    concrete_edge_factory() {}
    //template <class InfoFactoryT>
    //concrete_edge_factory(boost::shared_ptr<edge_factory<InfoFactoryT> > ef)
    //: impl(new ecs_impl_derived<InfoFactoryT>(ef)) {}
    
    template <class InfoFactoryT>
    concrete_edge_factory(std::auto_ptr<edge_factory<InfoFactoryT> > ef)
    : impl(new ecs_impl_derived<InfoFactoryT>(ef)) {}
    
    template <class FactoryT>
    concrete_edge_factory(std::auto_ptr<FactoryT> ef)
    : impl(new ecs_impl_derived2<FactoryT>(ef)) {}
    
    template <class FactoryT>
    concrete_edge_factory(boost::shared_ptr<FactoryT> ef)
    : impl(new ecs_impl_derived2<FactoryT>(ef)) {}
    
    template <class Expr>
    concrete_edge_factory(Expr e)
    : impl(new ecs_impl_derived<typename Expr::value_type::info_factory_type>(e))
    {}  
    
    concrete_edge_factory(concrete_edge_factory const& other)
    : impl(other.impl) {}
    
    concrete_edge_factory& operator=(concrete_edge_factory const& other)
    {
        impl = other.impl;
        return *this;
    }
    
    edge_stats stats() const 
    { return impl->stats(); }
    
    void finish(span_t spn) { impl->finish(spn); }
    
    template <class InfoFactoryT>
    concrete_edge_factory& 
    operator=(boost::shared_ptr< edge_factory<InfoFactoryT> > ef)
    { impl.reset(new ecs_impl_derived<InfoFactoryT>(ef)); }
    
    template <class InfoFactoryT>
    concrete_edge_factory& 
    operator=(std::auto_ptr< edge_factory<InfoFactoryT> > ef)
    { impl.reset(new ecs_impl_derived<InfoFactoryT>(ef)); }
    
    void set_cells() { impl->set_cells(); }
    
    void set_cells( cell_heuristic& c
                  , double weight
                  , score_t unseen_cell_outside=as_zero() ) 
    {
        impl->set_cells(c,weight,unseen_cell_outside);
    }

    generator_type create_edge( grammar_type const& g 
                              , rule_type r
                              , edge_equiv_type const& eq1
                              , edge_equiv_type const& eq2 )
    { return impl->create_edge(g,r,eq1,eq2); }
    
    score_t rule_heuristic(grammar_type const& g, rule_type r) const
    { return impl->rule_heuristic(g,r); }
    
    std::string
    hash_string(grammar_type const& g, edge_type const& e) const
    { return impl->hash_string(g,e); }
                         
    generator_type create_edge( grammar_type const& g
                              , rule_type r
                              , edge_equiv_type const& eq )
    { return impl->create_edge(g,r,eq); }
    
    edge_type create_edge( indexed_token const& lex
                         , span_t s
                         , score_t scr = as_one() )
    { return impl->create_edge(lex,s,scr); }
    
    void component_info_scores( edge_type const& e
                              , grammar_type& g
                              , feature_vector& scores
                              , feature_vector& heuristics
                              , boost::function<bool(edge_type const&)> stop = gusc::always_false() ) const
    { impl->component_info_scores(e,g,scores,heuristics,stop); }
    
    void component_info_scores( edge_type const& e
                              , grammar_type& g
                              , feature_vector& scores
                              , boost::function<bool(edge_type const&)> stop = gusc::always_false() ) const
    { impl->component_info_scores(e,g,scores,stop); }

private:
    ////////////////////////////////////////////////////////////////////////////
    ///
    ///  type-erasure techniques are not known to many c++ developers, so a 
    ///  quick explanation:
    ///
    ///  template parameters can be thought of as generating a polymorphic c
    ///  set of classes, in some respect.  type erasure turns the compile-time
    ///  polymorphism into runtime polymorphism.  we make an abstract class
    ///  that has pure virtual functions for all the functions in the interface. 
    ///  then, we make a new templated class that wraps our old templated class
    ///  but makes the functions virtual, and inherits from our abstract class.
    ///
    ///  there is only one problem:  virtual functions can not be templated.
    ///  so any template arguments in our interface have to become template 
    ///  arguments in our new class.  also, any function output that depends on
    ///  the old classes template parameters also has to be dependent upon on
    ///  the new classes template parameters.
    ///
    ////////////////////////////////////////////////////////////////////////////
    class ecs_impl
    {
    public:
        virtual edge_stats stats() const = 0;
        
        virtual void set_cells() = 0;
        
        virtual void finish(span_t spn) = 0;
        
        virtual
        void set_cells( cell_heuristic& c
                      , double weight
                      , score_t unseen_cell_outside=as_zero() ) = 0;
        
        virtual
        std::string hash_string(grammar_type const& g, edge_type const& e) const = 0;

        virtual
        generator_type create_edge( grammar_type const& g 
                                  , rule_type r
                                  , edge_equiv_type const& eq1
                                  , edge_equiv_type const& eq2 ) = 0;

        virtual
        generator_type create_edge( grammar_type const& g
                                  , rule_type r
                                  , edge_equiv_type const& eq ) = 0;

        virtual 
        edge_type create_edge( indexed_token const& lex
                             , span_t s
                             , score_t scr ) = 0;
        
        virtual
        score_t rule_heuristic(grammar_type const& g, rule_type r) const = 0;

        
        virtual
        void component_info_scores( edge_type const& e
                                  , grammar_type& g
                                  , feature_vector& scores
                                  , feature_vector& heuristics
                                  , boost::function<bool(edge_type const&)> stop ) const = 0;
        
        virtual
        void component_info_scores( edge_type const& e
                                  , grammar_type& g
                                  , feature_vector& scores
                                  , boost::function<bool(edge_type const&)> stop ) const = 0;
                                  
        virtual ~ecs_impl(){}
    };
    
    
    template <class EdgeFactory>
    class ecs_impl_derived2 : public ecs_impl, boost::noncopyable
    {
    public:
        ecs_impl_derived2(boost::shared_ptr<EdgeFactory> ef)
        : ef(ef) {}
        
        ecs_impl_derived2(std::auto_ptr<EdgeFactory> ef)
        : ef(ef) {}
        
        /*
        template <class Pattern>
        ecs_impl_derived(Pattern pattern)
        : ef(storage())
        {
            pattern.apply(ef);
        }
        */
        
        virtual void finish(span_t spn) { get_ef().finish(spn); }

        virtual edge_stats stats() const
        { return get_ef().stats(); }
        
        virtual void set_cells() { get_ef().set_cells(); }
        
        virtual
        void set_cells( cell_heuristic& c
                      , double weight
                      , score_t unseen_cell_outside ) 
        {
            get_ef().set_cells(c,weight,unseen_cell_outside);
        }
        
        virtual std::string
        hash_string(grammar_type const& g, edge_type const& e) const
        { return get_ef().hash_string(g,e); }
        
        virtual
        generator_type create_edge( grammar_type const& g 
                                  , rule_type r
                                  , edge_equiv_type const& eq1
                                  , edge_equiv_type const& eq2 )
        { return get_ef().create_edge(g,r,eq1,eq2); }

        virtual
        generator_type create_edge( grammar_type const& g
                                  , rule_type r
                                  , edge_equiv_type const& eq )
        { return get_ef().create_edge(g,r,eq); }
        
        virtual 
        edge_type create_edge( indexed_token const& lex
                             , span_t s
                             , score_t scr )
        { return get_ef().create_edge(lex,s,scr); }
        
        virtual
        score_t rule_heuristic(grammar_type const& g, rule_type r) const
        { return get_ef().rule_heuristic(g,r); }
        
        virtual
        void component_info_scores( edge_type const& e
                                  , grammar_type& g
                                  , feature_vector& scores
                                  , feature_vector& heuristics
                                  , boost::function<bool(edge_type const&)> stop ) const
        { get_ef().component_info_scores(e,g,scores,heuristics,stop); }
        
        virtual
        void component_info_scores( edge_type const& e
                                  , grammar_type& g
                                  , feature_vector& scores
                                  , boost::function<bool(edge_type const&)> stop ) const
        { get_ef().component_info_scores(e,g,scores,stop); }
            
        virtual ~ecs_impl_derived2(){}
    private:
        boost::shared_ptr<EdgeFactory> ef;
        EdgeFactory& get_ef() { return *ef.get(); }
        EdgeFactory const& get_ef() const { return *ef.get(); }
    };
    
    
    template <class InfoFactory>
    class ecs_impl_derived : public ecs_impl, boost::noncopyable
    {
    public:
        //ecs_impl_derived(boost::shared_ptr< edge_factory<InfoFactory> > ef)
        //: ef(ef) {}
        
        ecs_impl_derived(std::auto_ptr< edge_factory<InfoFactory> > ef)
        : ef(ef.release()) {}
        
        template <class Pattern>
        ecs_impl_derived(Pattern pattern)
        : ef(storage())
        {
            pattern.apply(ef);
        }

        edge_stats stats() const
        { return get_ef().stats(); }
        
        virtual void set_cells() { get_ef().set_cells(); }
        
        virtual void finish(span_t spn) { get_ef().finish(spn); }
        
        virtual
        void set_cells( cell_heuristic& c
                      , double weight
                      , score_t unseen_cell_outside ) 
        {
            get_ef().set_cells(c,weight,unseen_cell_outside);
        }
        
        virtual std::string
        hash_string(grammar_type const& g, edge_type const& e) const
        { return get_ef().hash_string(g,e); }
        
        virtual
        generator_type create_edge( grammar_type const& g 
                                  , rule_type r
                                  , edge_equiv_type const& eq1
                                  , edge_equiv_type const& eq2 )
        { return get_ef().create_edge(g,r,eq1,eq2); }

        virtual
        generator_type create_edge( grammar_type const& g
                                  , rule_type r
                                  , edge_equiv_type const& eq )
        { return get_ef().create_edge(g,r,eq); }
        
        virtual 
        edge_type create_edge( indexed_token const& lex
                             , span_t s
                             , score_t scr )
        { return get_ef().create_edge(lex,s,scr); }
        
        virtual
        score_t rule_heuristic(grammar_type const& g, rule_type r) const
        { return get_ef().rule_heuristic(g,r); }
        
        virtual
        void component_info_scores( edge_type const& e
                                  , grammar_type& g
                                  , feature_vector& scores
                                  , feature_vector& heuristics
                                  , boost::function<bool(edge_type const&)> stop ) const
        { get_ef().component_info_scores(e,g,scores,heuristics,stop); }
        
        virtual
        void component_info_scores( edge_type const& e
                                  , grammar_type& g
                                  , feature_vector& scores
                                  , boost::function<bool(edge_type const&)> stop ) const
        { get_ef().component_info_scores(e,g,scores,stop); }
            
        virtual ~ecs_impl_derived()
        {
            ef->~edge_factory<InfoFactory>();
            delete[] ((char*)ef);
        }
    private:
        edge_factory<InfoFactory>* ef;
        edge_factory<InfoFactory>& get_ef() { return *ef; }
        edge_factory<InfoFactory> const& get_ef() const { return *ef; }
        edge_factory<InfoFactory>* storage() 
        { 
            return reinterpret_cast<edge_factory<InfoFactory>*>(
                       new char[sizeof(edge_factory<InfoFactory>)]
                   ); 
        }
    };
    
    boost::shared_ptr<ecs_impl> impl;
};

template <class Grammar, class IF>
concrete_edge_factory<edge<typename IF::info_type>,Grammar>
make_cef(edge_factory<IF>& ef,Grammar* grammar = NULL)
{
    return concrete_edge_factory<edge<typename IF::info_type>,Grammar>(ef);
}

} // namespace sbmt

#endif // SBMT_SEARCH_CONCRETE_EDGE_FACTORY_HPP
