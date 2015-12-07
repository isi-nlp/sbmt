#ifndef SBMT__FOREST__KBEST_HPP
#define SBMT__FOREST__KBEST_HPP

//FIXME: oh my, it's called nbest in apps, kbest in papers, and here!  the agony.

#include <sbmt/forest/logging.hpp>
#include <sbmt/forest/unique_string_filter.hpp>
#include <sbmt/forest/dfs_forest.hpp>
#include <sbmt/forest/derivation.hpp>
#include <graehl/shared/lazy_forest_kbest.hpp>

namespace sbmt {

/**
   \defgroup Kbest K-best Derivations

   Uses a top-down, lazy, best-first kbest-derivation from a forest algorithm
   described at http://www.cis.upenn.edu/~lhuang3/huang-iwpt-correct.pdf.
   Generating, say, the 1-million best should be no problem.

   Every non-optimal kth-best derivation uses either a non-best edge from an
   equivalence, or a best edge but using nonoptimal subderivations.  Typically,
   the kbest list will contain only a small number of unique "sidetracks"
   (non-optimal edges), but may choose different subsets of them (should they
   occur in different subtrees, their use is nonconflicting).  So, it is
   typically best to integrate models into search, or at least provide some
   forest expansion rescoring, as the size of the kbest list needed to get the
   desired variation may be huge.

   A helper class serves as a visitor to dfs_forest, which builds a
   lazy_forest (graehl/shared/lazy_forest_kbest.hpp) that is essentially a
   copy of the parse forest, with per-or-node priority queues and memo
   facilitating the lazy kbest generation.

   You can generate kbest items incrementally, on demand, or just up to some constant k.  You pass a visitor, which is called for each 1,...kth best derivation, stopping early if you return false, e.g.
   \code
   class nbest_printer {
   bool operator()(derivation_type const& deriv,unsigned nbest_rank=0) const
   {
   interp.print_nbest_header(out,sent,nbest_rank);
   interp.print_all(out,deriv);
   return true;
   }
   ...
   };
   \endcode

   For example, to print the 100 best (whole-sentence) derivations in chart to cout:
   \code
   // given Edge type,edge_factory,chart,grammar
   edge_equiv_type top_equiv = chart.top_equiv(grammar);
   derivation_output_options deriv_opt;
   edge_equivalence_pool<edge_type> epool(max_items);
   unsigned sentid=2
   kbest_output<edge_type,grammar_in_mem> kout(nbest_out,log_out,grammar,epool,sentid,deriv_opt)

   kout.print_kbest(
   // prints NBEST sent=2 N=1...100
   \endcode
**/
///\{


template <class Edge>
class kbest_derivation_factory
{
 public:
    BOOST_STATIC_CONSTANT(bool,require_sorted=true);

    typedef Edge edge_type;

    typedef typename edge_type::edge_equiv_type edge_equiv_type;
    typedef typename edge_equiv_type::impl_type edge_equiv_impl;
//    typedef typename edge_equiv_type::impl_shared_ptr edge_equiv_ptr;
    typedef edge_equiv_impl *edge_equiv_ptr;

    typedef edge_equivalence_pool<edge_type> EQPool;
    kbest_derivation_factory() : ecsp(0) {}
//    kbest_derivation_factory(kbest_derivation_factory<Edge> &o) : ecs(o.ecs) {}


    explicit kbest_derivation_factory(EQPool & edge_factory) : ecsp(&edge_factory)
    {
    }
    typedef kbest_derivation_factory<Edge> kbest_derivation_factory_type;
    template <class Filter>
    void set_lazy_forest_factory()
    {
        graehl::lazy_forest<kbest_derivation_factory_type,Filter>::set_derivation_factory(*this);
    }
    typedef typename derivation_traits<edge_type>::forest_type forest_type;
    template <class Filter,class Deriv_visitor>
    graehl::lazy_kbest_stats enumerate_kbest(forest_type top_equiv,Filter const& f,unsigned n_deriv,Deriv_visitor visitor)
    {
        try {
            typedef graehl::lazy_forest<kbest_derivation_factory_type,Filter> lazy_type;
            lazy_type::set_filter_factory(f);
            lazy_type *lazy=0;
            visited_forest<lazy_forest_builder<Filter>,edge_type> dfs(*this);
            SBMT_LOG_TIME_SPACE(forest_domain,verbose,"Lazy forest transcribed for nbest: ");
            lazy=&(dfs.compute(top_equiv)); // we only want to catch failure in phase 1; it's guaranteed that if this succeeds, then any OOM in enumerate_kbest below will at least print 1best already
            assert(lazy);
            return lazy->enumerate_kbest(n_deriv,visitor);
        } catch (std::bad_alloc const&) {
            visitor(top_equiv,0); // print 1best
            throw;
        }
    }

//    typedef typename derivation_traits<edge_type>::derivation_type derivation_type; // note: somehow this fails.  following is equivalent:

    typedef edge_equivalence<edge_type> derivation_type;

    friend inline bool derivation_better_than(derivation_type const& me, derivation_type const& than)
    {
        return inside_score(me) > inside_score(than); // bug - not finding
//        return ::sbmt::inside_score(me) > ::sbmt::inside_score(than); // bug - not finding
//        return me.representative().inside_score() > than.representative().inside_score();
    }

//    static inline derivation_type NONE() { return edge_type::NULL_CHILD();}
//    static inline derivation_type PENDING() { return edge_type::PENDING_CHILD();}

    static inline derivation_type NONE() { return derivation_type::NONE(); }
    static inline derivation_type PENDING() { return derivation_type::PENDING(); }

    /// all the edges created in parsing are built based on their *best* antecedent subderivations.  but now we want to substitute a nonbest derivation for the left (changed_child_index=0) or right (=1).  all we have to build a copy, but change the antecedent pointer, and worsen the score accordingly (since every derivation for an equivalence is ... well, equivalent)
    derivation_type make_worse(const derivation_type &prototype, const derivation_type &old_subderiv, const derivation_type &new_subderiv, unsigned changed_child_index)
    {
        derivation_type ret = ecsp->create(prototype.representative());
        ret.representative().adjust_child(new_subderiv,changed_child_index);
        return ret;
    }

    /// computes a lazy_forest corresponding to each edge_equiv in dfs_forest::compute(top_equiv)
    template <class Filter>
    class lazy_forest_builder
        : public dfs_forest_visitor_base_ptr_local_state<
              lazy_forest_builder<Filter>,
              graehl::lazy_forest<kbest_derivation_factory_type,Filter>,
              edge_equiv_ptr,
              kbest_derivation_factory_type::require_sorted>
    {
        EQPool &ecs;
     public:
//        BOOST_STATIC_CONSTANT(bool,require_sorted=kbest_derivation_factory_type::require_sorted);
        //        static result_type initial_result() { return result_type(); }

        typedef graehl::lazy_forest<kbest_derivation_factory_type,Filter> lazy_forest_type;
        typedef lazy_forest_type result_type; /// what gets built by dfs_forest


        lazy_forest_builder(kbest_derivation_factory_type &kbest_factory) : ecs(*kbest_factory.ecsp)
        {
            kbest_factory.set_lazy_forest_factory<Filter>();
        }

        void start_equiv(edge_equiv_type equiv,result_type &result,edge_equiv_ptr &item)
        {
            rank_i=0;
            item=equiv.get_raw();
            result.reserve(equiv.size());
        }
        void visit_edge_ptr(edge_type const& edge,result_type &result,
                            result_type *left,result_type *right,edge_equiv_ptr & item)
        {
            if (require_sorted)
                result.add_sorted(derivation(edge,item),left,right);
            else
                result.add(derivation(edge,item),left,right);
            set_rank(edge);
        }
        void finish_equiv(edge_equiv_type equiv,result_type &result,edge_equiv_ptr const& item)
        {
            if (!require_sorted)
                result.sort();
        }
     private:
        unsigned rank_i;

        void set_rank(edge_type const& edge) {
            //TODO: store in edge (over heuristic?)
            ++rank_i;
        }
        derivation_type derivation(edge_type const& edge,edge_equiv_ptr item)
        {
            if (rank_i==0)
                return edge_equiv_type(item);
            ///else edge=2nd-best or worst
                return ecs.create(edge);
        }
    };
//    typedef dfs_forest<build_lazy_forest> dfs_build_type;

    EQPool *ecsp;
 private:
};

//FIXME: not concurrent (given edge,grammar) due to lazy_forest_kbest.hpp
template <class Edge,class Grammar>
struct kbest_output
{
    typedef Grammar grammar_type;
    typedef Edge edge_type;
    typedef edge_equivalence<edge_type> equiv_type;
    typedef derivation_interpreter<edge_type,grammar_type> Interp;
    typedef typename Interp::template nbest_printer<std::ostream> N_printer;
    typedef edge_equivalence_pool<Edge> Epool;
    typedef concrete_edge_factory<Edge,Grammar> edge_factory_type;

    std::ostream &kbest_out;
    grammar_type &gram;
    Epool &epool;
    Interp interp;
    kbest_derivation_factory<Edge> kfact;

    kbest_output(std::ostream &kbest_out
                 ,grammar_type &gram
                 , edge_factory_type &efact
                 , Epool &epool
                 ,derivation_output_options const& deriv_opt=derivation_output_options()
        )
        : kbest_out(kbest_out)
        ,gram(gram)
        , epool(epool)
        ,interp(gram,efact,deriv_opt)
        ,kfact(epool)
    {}

    template <class FilterFactory>
    void print_kbest(equiv_type const& top_equiv,unsigned sentid,unsigned pass,unsigned maxkbests,FilterFactory const& f
                     , extra_english_output_t *eo=0
)
    {
        N_printer nprint(interp,kbest_out,sentid,pass,eo);
        enumerate_kbest(top_equiv,nprint,maxkbests,f);
    }

    template <class FilterFactory,class Printer>
    void enumerate_kbest(equiv_type const& top_equiv,Printer const& nprint,unsigned maxkbests,FilterFactory const& f)
    {
        io::logging_stream& logstr = io::registry_log(forest_domain);
        logstr << io::info_msg << kfact.enumerate_kbest(top_equiv,f,maxkbests,nprint) << std::endl;
    }

    BOOST_STATIC_CONSTANT(unsigned,UNLIMITED_PER_STRING=0);

    void print_kbest(equiv_type const& top_equiv,unsigned sentid,unsigned pass,unsigned maxkbests,unsigned max_deriv_per_estring=UNLIMITED_PER_STRING
                     , extra_english_output_t *eo=0
        )
    {
        if (max_deriv_per_estring==UNLIMITED_PER_STRING)
            print_kbest(top_equiv,sentid,pass,maxkbests,graehl::permissive_kbest_filter_factory(),eo);
        else
            print_kbest(top_equiv,sentid,pass,maxkbests,filter_by_unique_string(interp,max_deriv_per_estring),eo);
    }

};

///\}

} //sbmt


#endif
