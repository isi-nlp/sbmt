#ifndef   SBMT_SEARCH_concrete_edge_factory_HPP
#define   SBMT_SEARCH_concrete_edge_factory_HPP


#include <sbmt/edge/edge.hpp>
#include <sbmt/grammar/grammar.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp> // for boost::noncopyable

#include <string>

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
    typedef EdgeT                                            edge_type;
    typedef edge_equivalence<edge_type>                      edge_equiv_type;
    typedef GrammarT                                         grammar_type;
    typedef typename grammar_type::rule_type rule_type;

    concrete_edge_factory() {}
    template <class InfoFactoryT>
    concrete_edge_factory(edge_factory<InfoFactoryT>& ef)
    : impl(new ecs_impl_derived<InfoFactoryT>(ef)) {}
   
    edge_factory_stats const& stats() const 
    { return impl->stats(); }
    
    template <class InfoFactoryT>
    concrete_edge_factory& operator=(edge_factory<InfoFactoryT>& ef)
    { impl.reset(new ecs_impl_derived<InfoFactoryT>(ef)); }

    edge_type create_edge( grammar_type const& g 
                         , rule_type r
                         , edge_equiv_type const& eq1
                         , edge_equiv_type const& eq2 )
    { return impl->create_edge(g,r,eq1,eq2); }
    
    score_t rule_heuristic(grammar_type const& g, rule_type r)
    { return impl->rule_heuristic(g,r); }
                         
    edge_type create_edge( grammar_type const& g
                         , rule_type r
                         , edge_equiv_type const& eq )
    { return impl->create_edge(g,r,eq); }
    
    edge_type create_edge( grammar_type& g
                         , std::string const& word
                         , span_t s )
    { return impl->create_edge(g,word,s); }
    
    edge_type create_edge( grammar_type& g
                         , fat_token const& lex
                         , span_t s )
    { return impl->create_edge(g,lex,s); }

    ///FIXME: test if this really chains return value optimization and only creates once.
    edge_equiv_type create_edge_equivalence( edge_type const& e )
    { return impl->create_edge_equivalence(e); }

    edge_equiv_type create_singleton_sidetrack_equivalence(edge_type const& prototype,  edge_equiv_type const &new_child, unsigned changed_child_index) 
    {
        edge_equiv_type ret=create_edge_equivalence(prototype);
        ret.representative().adjust_child(new_child,changed_child_index);
        return ret;
    }

    void reset() 
    { impl->reset(); }
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
        virtual edge_factory_stats const& stats() const = 0;
        
        virtual
        edge_type create_edge( grammar_type const& g 
                             , rule_type r
                             , edge_equiv_type const& eq1
                             , edge_equiv_type const& eq2 ) = 0;

        virtual
        edge_type create_edge( grammar_type const& g
                             , rule_type r
                             , edge_equiv_type const& eq ) = 0;
                             
        virtual
        edge_type create_edge( grammar_type& g
                             , std::string const& word
                             , span_t s ) = 0;
        virtual 
        edge_type create_edge( grammar_type& g
                             , fat_token const& lex
                             , span_t s ) = 0;
                             
        virtual
        score_t rule_heuristic(grammar_type const& g, rule_type r) = 0;

        virtual
        edge_equiv_type create_edge_equivalence( edge_type const& e ) = 0;

        virtual void reset() = 0;
        virtual ~ecs_impl(){}
    };
    
    template <class InfoFactoryT>
    class ecs_impl_derived : public ecs_impl, boost::noncopyable
    {
    public:
        ecs_impl_derived(edge_factory<InfoFactoryT>& ef)
        : ef(ef) {}

        edge_factory_stats const& stats() const
        { return ef.stats(); }
        
        virtual
        edge_type create_edge( grammar_type const& g 
                             , rule_type r
                             , edge_equiv_type const& eq1
                             , edge_equiv_type const& eq2 )
        { return ef.create_edge(g,r,eq1,eq2); }

        virtual
        edge_type create_edge( grammar_type const& g
                             , rule_type r
                             , edge_equiv_type const& eq )
        { return ef.create_edge(g,r,eq); }

        virtual
        edge_type create_edge( grammar_type& g
                             , std::string const& word
                             , span_t s )
        { return ef.create_edge(g,word,s); }
        
        virtual 
        edge_type create_edge( grammar_type& g
                             , fat_token const& lex
                             , span_t s )
        { return ef.create_edge(g,lex,s); }
        
        virtual
        score_t rule_heuristic(grammar_type const& g, rule_type r)
        { return ef.rule_heuristic(g,r); }
        
        virtual
        edge_equiv_type create_edge_equivalence( edge_type const& e )
        { return ef.create_edge_equivalence(e); }

        virtual void reset()
        { return ef.reset(); }
            
        virtual ~ecs_impl_derived(){}
    private:
        edge_factory<InfoFactoryT>& ef;
        edge_factory<InfoFactoryT>& get_ef() { return ef; }
    };
    
    boost::shared_ptr<ecs_impl> impl;
};

} // namespace sbmt

#endif // SBMT_SEARCH_CONCRETE_EDGE_FACTORY_HPP
