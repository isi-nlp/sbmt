# if ! defined(SBMT__EDGE__ANY_INFO_HPP)
# define       SBMT__EDGE__ANY_INFO_HPP


# include <graehl/shared/intrusive_refcount.hpp>

# include <boost/shared_ptr.hpp>
# include <boost/range.hpp>
# include <boost/iterator/transform_iterator.hpp>
# include <boost/function_output_iterator.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/any_iterator/any_iterator.hpp>
# include <boost/intrusive_ptr.hpp>


# include <vector>
# include <iterator>

# include <sbmt/feature/feature_vector.hpp>
# include <sbmt/edge/constituent.hpp>
# include <sbmt/edge/info_base.hpp>
# include <sbmt/logmath.hpp>
# include <sbmt/grammar/grammar_in_memory.hpp>
# include <gusc/generator/any_generator.hpp>
# include <gusc/iterator/any_output_iterator.hpp>
# include <sbmt/edge/options_map.hpp>
# include <sbmt/search/block_lattice_tree.hpp>
# include <sbmt/feature/accumulator.hpp>


namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

template <class Grammar>
class any_type_info : public info_base<any_type_info<Grammar> >
{
private:
  struct placeholder {
    virtual bool equal_to(placeholder const& other) const = 0;
    virtual size_t hash_value() const = 0;
    virtual ~placeholder() {}
    //virtual placeholder* clone() const = 0;
  };

  template <class Info>
  struct holder : placeholder {
    typedef holder<Info> self_t;
    Info info;

    virtual bool equal_to(placeholder const& other) const
    {

      return info == static_cast<self_t const&>(other).info;
    }

    virtual size_t hash_value() const
    {
      return boost::hash<Info>()(info);
    }

    // virtual placeholder* clone() const { return new holder<Info>(info); }

    holder(Info const& info) : info(info) {}
  };

  boost::shared_ptr<placeholder> pimpl;
public:
  operator bool() const { return bool(pimpl); }
  any_type_info(any_type_info const& other)
    : pimpl(other.pimpl) {} // this is okay because any_type_info is not
  // modifyable other than through operator=

  bool equal_to(any_type_info const& other) const
  {
    if (bool(pimpl) != bool(other.pimpl)) return false;
    if (not bool(pimpl) and not bool(other.pimpl)) return true;
    if (pimpl == other.pimpl) return true;
    return (not pimpl) or pimpl->equal_to(*(other.pimpl));
  }

  size_t hash_value() const
  {
    if (not pimpl) return 0;
    else return pimpl->hash_value();
  }

  template <class I>
  any_type_info(I const& info) : pimpl(new holder<I>(info)) {}

  any_type_info() {}

  any_type_info& operator=(any_type_info const& other)
  {
    pimpl = other.pimpl;
    //pimpl.reset(other.pimpl ? other.pimpl->clone() : NULL);
    return *this;
  }

  template <class Info>
  any_type_info& operator=(Info const& other)
  {
    return operator=(any_type_info(other));
  }

  template <class Info>
  Info const* info_cast() const
  {
      return pimpl ? &(static_cast<holder<Info> const&>(*pimpl).info)
                   : 0 
                   ;
  }
};

template <class Info, class Grammar>
Info const* info_cast(any_type_info<Grammar> const* info)
{
  return info->info_cast<Info>();
}

template <class Info, class Grammar>
constituent<Info> constituent_cast(constituent< any_type_info<Grammar> > const& c)
{
  //if (is_lexical(c.root())) return constituent<Info>(c.root());
  //else
  return constituent<Info>(info_cast<Info,Grammar>(c.info()),c.root(),c.span());
}

template <class Info>
struct constituent_caster {
  typedef constituent<Info> result_type;
  template <class Grammar>
  result_type operator()(constituent<any_type_info<Grammar> > const& c) const
  {
    return constituent_cast<Info,Grammar>(c);
  }
};

////////////////////////////////////////////////////////////////////////////////
//
// if your InfoFactory provides the old component_scores method, and never
// uses info heuristics, you can just inherit from this class. as in:
//
// class my_info_factory : public info_factory_new_component_scores<my_info_factory> {...};
//
////////////////////////////////////////////////////////////////////////////////
template <class InfoFactory,bool Deterministic = true>
struct info_factory_new_component_scores {

  InfoFactory& base() { return static_cast<InfoFactory&>(*this); }

  template < class Grammar
             , class ScoreOutputIterator
             , class HeurOutputIterator
             , class ConstituentRange
             , class InfoType >
    boost::tuple<ScoreOutputIterator,HeurOutputIterator>
    component_scores( Grammar& gram
                    , typename Grammar::rule_type rule
                    , span_t const& span
                    , ConstituentRange const& constituents
                    , InfoType const& result
                    , ScoreOutputIterator scores_out
                    , HeurOutputIterator heuristics_out )
    {
        return boost::make_tuple(
                 base().component_scores_old(gram,rule,span,constituents,result,scores_out)
               , heuristics_out
               );
    }
    bool deterministic() const { return Deterministic; }
};

template <class Grammar>
class any_type_info_factory
{
public:
    typedef any_type_info<Grammar> info_type;

    typedef boost::tuple<info_type,score_t,score_t> result_type;

    typedef gusc::any_generator<result_type> result_generator;

    typedef gusc::any_output_iterator<
                std::pair<boost::uint32_t,score_t>
            > score_output_iterator;

    typedef boost::any_iterator< constituent<info_type> const
                               , boost::single_pass_traversal_tag
                               , constituent<info_type> > constituent_iterator;

    typedef boost::iterator_range<constituent_iterator> constituent_range;
    
    template <class CR>
    static constituent_range make_constituent_range(CR const& cr)
    {
        return constituent_range(constituent_iterator(boost::begin(cr)),constituent_iterator(boost::end(cr)));
    }
private:
    struct placeholder {
        virtual bool deterministic() const = 0;
        virtual bool scoreable_rule( Grammar const& gram
                                   , typename Grammar::rule_type rule ) = 0;

        virtual score_t rule_heuristic( Grammar const& gram
                                      , typename Grammar::rule_type rule ) = 0;

        virtual
        result_generator
        create_info( Grammar const& gram
                   , typename Grammar::rule_type rule
                   , span_t const& span
                   , constituent_range const& constituents ) = 0;

        virtual
        std::string
        hash_string(Grammar const& gram, any_type_info<Grammar> const& info) const = 0;

        virtual
        score_output_iterator
        component_scores( Grammar& gram
                        , typename Grammar::rule_type rule
                        , span_t const& span
                        , constituent_range const& constituents
                        , any_type_info<Grammar> const& result
                        , score_output_iterator ) = 0;

        virtual ~placeholder() {}
    };

    struct placeholder_new_component_scores : placeholder {
        virtual
        boost::tuple<score_output_iterator,score_output_iterator>
        component_scores( Grammar& gram
                        , typename Grammar::rule_type rule
                        , span_t const& span
                        , constituent_range const& constituents
                        , any_type_info<Grammar> const& result
                        , score_output_iterator scores
                        , score_output_iterator heuristics ) = 0;
    };


    template <class InfoFactory>
    struct holder : placeholder_new_component_scores {
        InfoFactory info_factory_;

        typedef typename InfoFactory::info_type info_type;

        typedef boost::transform_iterator< constituent_caster<info_type>
                                         , constituent_iterator >
                transform_iterator;

        boost::iterator_range<transform_iterator>
        transform_range(constituent_range const& c) // c must not be singular (default constructed) or debug build will assert-fail
        {
            return
            boost::make_iterator_range( transform_iterator(boost::begin(c))
                                      , transform_iterator(boost::end(c)) );
        }

        holder(InfoFactory const& info_factory_)
          : info_factory_(info_factory_) {}
          
        virtual bool deterministic() const { return info_factory_.deterministic(); }

        virtual bool scoreable_rule( Grammar const& gram
                                   , typename Grammar::rule_type rule )
        {
            return info_factory_.scoreable_rule(gram,rule);
        }

        virtual score_t rule_heuristic( Grammar const& gram
                                      , typename Grammar::rule_type rule )
        {
            return info_factory_.rule_heuristic(gram, rule);
        }

        virtual result_generator
        create_info( Grammar const& gram
                   , typename Grammar::rule_type rule
                   , span_t const& span
                   , constituent_range const& constituents )
        {
            return result_generator(
                     info_factory_.create_info( gram
                                              , rule
                                              , span
                                              , transform_range(constituents) 
                                              )
                   );
        }

        virtual std::string
        hash_string(Grammar const& gram, any_type_info<Grammar> const& info) const
        {
            return bool(info) ? info_factory_.hash_string(gram,*info_cast<info_type>(&info))
                              : "."
                              ;
        }

        virtual boost::tuple<score_output_iterator,score_output_iterator>
        component_scores( Grammar& gram
                        , typename Grammar::rule_type rule
                        , span_t const& span
                        , constituent_range const& constituents
                        , any_type_info<Grammar> const& result
                        , score_output_iterator scores_out
                        , score_output_iterator heuristics_out )
        {
            return info_factory_.component_scores( gram
                                                 , rule
                                                 , span
                                                 , transform_range(constituents)
                                                 , *info_cast<info_type>(&result)
                                                 , scores_out
                                                 , heuristics_out );
        }

        virtual score_output_iterator
        component_scores( Grammar& gram
                        , typename Grammar::rule_type rule
                        , span_t const& span
                        , constituent_range const& constituents
                        , any_type_info<Grammar> const& result
                        , score_output_iterator scores_out )
        {
            ignore_accumulator ignore;
            score_output_iterator so = scores_out;
            score_output_iterator ho = score_output_iterator(boost::make_function_output_iterator(ignore));
            boost::tie(so,ho) =
                  component_scores( gram
                                  , rule
                                  , span
                                  , constituents
                                  , result
                                  , so
                                  , ho
                                  );
            return so;
        }

    };

    boost::shared_ptr<placeholder_new_component_scores> pimpl;

public:
    any_type_info_factory(any_type_info_factory const& other)
      : pimpl(other.pimpl) { }

    template <class InfoFactory>
    any_type_info_factory(InfoFactory const& info_factory_)
      : pimpl(new holder<InfoFactory>(info_factory_)) {}

    template <class ScoreOutputIterator, class HeurOutputIterator, class ConstituentRange>
    boost::tuple<ScoreOutputIterator,HeurOutputIterator>
    component_scores( Grammar& gram
                    , typename Grammar::rule_type rule
                    , span_t const& span
                    , ConstituentRange const& constituents
                    , any_type_info<Grammar> const& result
                    , ScoreOutputIterator scores_out
                    , HeurOutputIterator heuristics_out )
    {
        score_output_iterator so(scores_out);
        score_output_iterator ho(heuristics_out);
        boost::tie(so,ho) =
             pimpl->component_scores( gram
                                    , rule
                                    , span
                                    , make_constituent_range(constituents)
                                    , result
                                    , so
                                    , ho );
        return boost::make_tuple(
                 so.any_iterator_cast<ScoreOutputIterator>()
               , ho.any_iterator_cast<HeurOutputIterator>()
               );
    }

    bool deterministic() const { return pimpl->deterministic(); }
    template <class ConstituentRange>
    result_generator
    create_info( Grammar const& gram
               , typename Grammar::rule_type rule
               , span_t const& span
               , ConstituentRange const& constituents )
    {
        return pimpl->create_info(gram,rule,span,make_constituent_range(constituents));
    }

    bool scoreable_rule( Grammar const& gram
                       , typename Grammar::rule_type rule )
    {
        return pimpl->scoreable_rule(gram,rule);
    }

    score_t rule_heuristic( Grammar const& gram
                          , typename Grammar::rule_type rule )
    {
        return pimpl->rule_heuristic(gram, rule);
    }

    std::string hash_string(Grammar const& gram, any_type_info<Grammar> const& info ) const
    {
        return pimpl->hash_string(gram,info);
    }
};

////////////////////////////////////////////////////////////////////////////////

typedef property_constructors<> property_constructors_type;

////////////////////////////////////////////////////////////////////////////////

template <class Grammar>
class any_type_info_factory_constructor
{
    typedef any_type_info_factory<Grammar> factory_type;
public:
    options_map get_options()
    {
        return pimpl->get_options();
    }

    bool set_option(std::string key, std::string value)
    {
        return pimpl->set_option(key,value);
    }
    
    void init(in_memory_dictionary& dict) { return pimpl->init(dict); }

    factory_type construct( Grammar& grammar
                          , lattice_tree const& lattice
                          , property_map_type const& pmap )
    {
        return pimpl->construct(grammar, lattice, pmap);
    }

    any_type_info_factory_constructor(any_type_info_factory_constructor const& other)
      : pimpl(other.pimpl) {}

    template <class RIFC>
    any_type_info_factory_constructor(RIFC const& rifc)
      : pimpl(new holder<RIFC>(rifc)) {}

    template <class RIFC>
    any_type_info_factory_constructor(boost::shared_ptr<RIFC> const& ptr)
      : pimpl(ptr) {}

    template <class RIFC>
    any_type_info_factory_constructor(std::auto_ptr<RIFC> const& ptr)
      : pimpl(ptr) {}

private:
    struct placeholder {
        virtual ~placeholder() {}
        virtual options_map get_options() = 0;
        virtual bool set_option(std::string key, std::string value) = 0;
        virtual void init(in_memory_dictionary& dict) = 0;
        virtual factory_type construct( Grammar& grammar
                                      , lattice_tree const& lattice
                                      , property_map_type const& pmap ) = 0;
    };

    template <class RIFC>
    struct holder : placeholder {
        RIFC rifc;

        holder(RIFC const& rifc) : rifc(rifc) {}

        virtual options_map get_options()
        {
            return rifc.get_options();
        }
        
        virtual void init(in_memory_dictionary& dict) { return rifc.init(dict); }

        virtual bool set_option(std::string key, std::string val)
        {
            return rifc.set_option(key,val);
        }

        virtual factory_type construct( Grammar& grammar
                                      , lattice_tree const& lattice
                                      , property_map_type const& pmap )
        {
            return rifc.construct(grammar, lattice, pmap);
        }
    };

    boost::shared_ptr<placeholder> pimpl;
};

typedef any_type_info<grammar_in_mem> any_info;
typedef any_type_info_factory<grammar_in_mem> any_info_factory;
typedef any_type_info_factory_constructor<grammar_in_mem> any_info_factory_constructor;

////////////////////////////////////////////////////////////////////////////////

template <class InfoFactoryConstructor>
void register_info_factory_constructor( std::string name
                                      , InfoFactoryConstructor const& ifc );

template <class RulePropertyConstructor>
void register_rule_property_constructor( std::string info_name
                                       , std::string property_name
                                       , RulePropertyConstructor const& rpc );

void unregister_rule_property_constructor( std::string info_name
                                         , std::string property_name );

boost::program_options::options_description info_registry_options();

void info_registry_set_option( std::string info_name
                             , std::string option_name
                             , std::string option_value );

any_info_factory get_registry_info_factory(std::string name);

////////////////////////////////////////////////////////////////////////////////


} // namespace sbmt

# include <sbmt/edge/impl/any_info.ipp>

# endif //     SBMT__EDGE__ANY_INFO_HPP
