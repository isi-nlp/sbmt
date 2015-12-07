# ifndef XRSDB__SEARCH__SORT_HPP
# define XRSDB__SEARCH__SORT_HPP

# include <sbmt/feature/feature_vector.hpp>
# include <sbmt/grammar/tag_prior.hpp>
# include <iterator>
# include <search/grammar.hpp>

template<typename IT, typename CMP> 
IT partition(IT begin, IT end, IT pivot, CMP const& cmp)
{
    std::iter_swap(pivot, (end-1));
    typename std::iterator_traits<IT>::reference piv = *(end - 1);
    
    IT store=begin;
    for(IT it=begin; it!=end-1; ++it) {
        if (cmp(*it,piv)) {
            std::iter_swap(store, it);
            ++store;
        }
    }
    std::iter_swap(end-1, store);
    return store;
}

struct pivot_median {
template<typename IT,typename CMP>
IT operator()(IT begin, IT end, CMP const& cmp) const
{
    IT pivot(begin+(end-begin)/2);
    if(cmp(*begin,*pivot) and cmp(*(end-1),*begin)) pivot=begin;
    else if(cmp(*(end-1),*pivot) and cmp(*begin,*(end-1))) pivot=end-1;
    return pivot;
}
};

template<typename IT, typename PF, typename CMP> 
void quick_sort(IT begin, IT end, PF const& pf, CMP const& cmp)
{
    if((end-begin)>1) {
        IT pivot=pf(begin, end, cmp);

        pivot=partition(begin, end, pivot, cmp);

        quick_sort(begin, pivot, pf, cmp);
        quick_sort(pivot+1, end, pf, cmp);
    }
}

template<typename IT, typename CMP>
void quick_sort(IT begin, IT end, CMP const& cmp)
{
    quick_sort(begin, end, pivot_median(), cmp);
}

namespace xrsdb { namespace search { 

typedef 
    boost::function<
          void( rule_application_array_adapter::iterator
              , rule_application_array_adapter::iterator
              )
    > rule_sort_f;

struct null_sorter {
    void operator()( rule_application_array_adapter::iterator beg
                   , rule_application_array_adapter::iterator end ) 
    { return; }
};

struct info_rule_sorter {
    bool operator()(rule_application const& r1, rule_application const& r2) const
    {
        return (r1.cost + r1.heur) < (r2.cost + r2.heur);
    }
    void operator()( rule_application_array_adapter::iterator beg
                   , rule_application_array_adapter::iterator end )
    {
        BOOST_FOREACH(rule_application& ra, std::make_pair(beg,end)) {
            ra.cost = dot(*weights,ra.costs);
            ra.heur = (*priormap)[gram->rule_lhs(&ra)].neglog10();
            BOOST_FOREACH(any_xinfo_factory& fact, factories) {
                ra.heur += fact.rule_heuristic(*gram,&ra).neglog10();
            }
            //std::stringstream sstr;
            //sstr << "RULE: " << ra.rule.id() << "\n";
            //std::cerr << sstr.str();
        }
        quick_sort(beg,end,*this);
        //BOOST_FOREACH(rule_application& ra, std::make_pair(beg,end)) {
        //    std::cerr << '(' << ra.cost <<','<< ra.heur <<')' << ':' << ra.cost + ra.heur << ' ';
        //}
        //std::cerr << '\n';
    }
    info_rule_sorter( grammar_facade const* gram
                    , gusc::shared_varray<any_xinfo_factory> factories
                    , sbmt::weight_vector const* weights
                    , sbmt::tag_prior const* priormap )
    : gram(gram)
    , factories(factories)
    , weights(weights)
    , priormap(priormap) {}
    
    grammar_facade const* gram;
    gusc::shared_varray<any_xinfo_factory> factories;
    sbmt::weight_vector const* weights;
    sbmt::tag_prior const* priormap;
};

}} // xrsdb::search

# endif