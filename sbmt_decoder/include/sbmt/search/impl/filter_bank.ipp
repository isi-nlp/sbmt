#include <sbmt/hash/swap.hpp>
#include <sbmt/hash/hash_set.hpp>
#include <sbmt/search/filter_predicates.hpp>
#include <sbmt/grammar/grammar.hpp>
#include <sbmt/search/unary_applications.hpp>
#include <sbmt/io/log_auto_report.hpp>
//FIXME: runtime config
//#define SBMT_FILTER_BANK_UNARY_APPLICATION_MAX 70000000 

namespace sbmt {
    
template <class ET, class GT, class CT>
unsigned int filter_bank<ET,GT,CT>::default_max_unary_loop_count = 3;

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
filter_bank<ET,GT,CT>::filter_bank(
                           span_filter_factory_p sff
                         , unary_filter_factory_p uff
                         , GT& g
                         , concrete_edge_factory<ET,GT>& ef
                         , edge_equivalence_pool<ET>& epool
                         , CT& c
                         , span_t ts 
                       )
: gram(g)
, ef(ef)
, epool(epool)
, chart(c)
, total_span(ts)
, span_filt_factory(sff)
, unary_filt_factory(uff)
, rhs_map(g,ef)
, sig_map(g)
{}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
void filter_bank<ET,GT,CT>::finalize(span_t const& s)
{
    span_filter_p& pf=get_create(s);
    if (pf and !pf->is_finalized()) {
        
        SBMT_VERBOSE_STREAM(
            fb_domain        
          , "finalize called on " << s << "using edge filter " 
            << span_filt_factory->unary_filter(s)
        );
        pf->finalize();
        edge_queue<ET> equeue;
        std::vector< edge_equivalence<ET> > source_words;
        while(!pf->empty()) {
            edge_equiv_type eq = epool.transfer( pf->top() 
                                               , pf->edge_equiv_pool() );
            chart.insert_edge(eq);
            pf->pop();
        }
        
        typename chart_traits<CT>::cell_range cr;
        typename chart_traits<CT>::cell_iterator itr, end;

        // edge-equivs are placed into temporary storage for unary 
        // rule application.
        //equeue.insert(eq);
        {io::log_time_report report( io::registry_log(fb_domain)
                                   , io::lvl_verbose
                                   , "unary-applications: ");
        cr=chart.cells(s);
        for (itr=cr.begin(), end=cr.end(); itr != end; ++itr) {
            typename chart_traits<CT>::edge_range er = itr->edges();
            typename chart_traits<CT>::edge_iterator eitr = er.begin();
            typename chart_traits<CT>::edge_iterator eend = er.end();
            if (not is_lexical(itr->root())) {
                for (; eitr != eend; ++eitr) {
                    equeue.insert(*eitr);
                }
            } else {
                // make sure all foreign-preterminal rules are applied
                for (; eitr != eend; ++eitr) {
                    source_words.push_back(*eitr);
                    typename GT::rule_range rr = gram.unary_rules(itr->root());
                    typename GT::rule_iterator ritr = rr.begin(), rend = rr.end();
                    for (; ritr != rend; ++ritr) {
                        gusc::any_generator<ET> 
                            gen =  ef.create_edge(gram,*ritr,*eitr);
                        while (gen) {
                            equeue.insert(epool, gen());
                        }
                    }
                }
            }
        }
        chart.clear(s);
        apply_unary_rules(equeue.begin(),equeue.end(),s);
        typename std::vector< edge_equivalence<ET> >::iterator si, se;
        si = source_words.begin();
        se = source_words.end();
        for (;si != se; ++si) { chart.insert_edge(*si); }
        }
        pf.reset(); // free memory
        if (total_span == s) {
             io::log_time_space_report( io::registry_log(fb_domain)
                                      , io::lvl_info
                                      , "toplevel-rule applications: " );
             apply_toplevel_rules();
         }
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
typename filter_bank<ET,GT,CT>::span_filter_p &
filter_bank<ET,GT,CT>::get_create(span_t const& s)
{
    boost::mutex::scoped_lock lk(filter_map_mtx);
    typename span_filter_map_type::iterator pos = span_filter_map.find(s);
    if (pos == span_filter_map.end()) {
        boost::shared_ptr< span_filter_interface<ET,GT,CT> > 
            sf(span_filt_factory->create(s,gram,ef,chart));
        pos=insert_into_map(span_filter_map, s, sf).first;
    }
    return pos->second;
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
void filter_bank<ET,GT,CT>::apply_rules( span_t const& first_constituent
                                       , span_index_t right_boundary)
{
    /// for now, not supporting quasi-binary rules, so we know the other
    /// constituent and target root without having to calculate on per-rule
    /// basis:
    span_t second_constituent = span_t( first_constituent.right()
                                      , right_boundary);
    span_t target = combine(first_constituent, second_constituent);
    
    /// probably, finalize should become public method that search-order can
    /// call, since with quasi-binary, search order will probably know best time
    /// to call.                       
    //finalize(first_constituent);
    //finalize(second_constituent);
    
    // limit number of calls to get_create.  each call has a lock on it.
    span_filter_p& ptr = get_create(target);
    //read_write_mutex::scoped_read_lock lk(chart_mtx);
    
    typename chart_traits<CT>::cell_range cr = chart.cells(first_constituent);
    typename chart_traits<CT>::cell_iterator itr, end;
    itr = cr.begin();
    end = cr.end();
    
    for (; itr != end; ++itr) {
        SBMT_PEDANTIC_STREAM(fb_domain
          , "apply_rules " << first_constituent << "," << second_constituent <<
            " first-cell: " << print(itr->root(),gram.dict())
        );
        indexed_token rhs1 = itr->root();
        typename signature_index_map<GT>::iterator sigitr, sigend;
        boost::tie(sigitr,sigend) = sig_map.reps(rhs1);
        
        for (;sigitr != sigend; ++sigitr) {
            indexed_token rhs2 = gram.rule_rhs(*sigitr,1);
            SBMT_PEDANTIC_STREAM(fb_domain
              , "\tapply_rules " << first_constituent << "," << second_constituent <<
                " second-cell: " << print(rhs2,gram.dict())
            );
            
            typename chart_traits<CT>::edge_range er2 = 
                                    chart.edges(rhs2, second_constituent);
            if (er2.begin() == er2.end()) {
                SBMT_PEDANTIC_STREAM(fb_domain
                  , "\t\tapply_rules " << first_constituent << "," << second_constituent <<
                    " no edges for second-cell"
                );
                continue;
            }
            else {
                typename chart_traits<CT>::edge_range er1 = itr->edges();
                typename span_filter_type::rule_iterator ritr, rend;
                boost::tie(ritr,rend) = rhs_map.rules(*sigitr);
                ptr->apply_rules( boost::make_tuple(ritr,rend)
                                , er1
                                , er2 );
            }
        }
    }
    
}
                                       
////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
template <class ItrT> void 
filter_bank<ET,GT,CT>::apply_unary_rules(ItrT begin, ItrT end, span_t const& spn)
{
    typedef stlext::hash_set< edge_equiv_type
                            , representative_hash<ET>
                            , representative_equal<ET>
                            > edge_set_t;
    typedef unary_applications<ET,GT> unary_apps_t;
    
    unary_apps_t unary_apps( begin, end
                           , gram, rhs_map
                           , ef, unary_filt_factory
                           , spn, false );
    edge_set_t start_edges;
	insert_range(start_edges, begin, end);
    
    typename unary_apps_t::iterator uitr = unary_apps.begin(),
                                    uend = unary_apps.end();

    for (; uitr != uend; ++uitr) {
        // if its an edge not already in the chart. remember, edge-equivs are
        // pointers, so any additional edges made by unary rules that match
        // a previously created edge-equiv is already in the chart.
        if(start_edges.find(*uitr) == start_edges.end()) {
            chart.insert_edge(*uitr);
        } 
    }
    for (ItrT itr = begin; itr != end; ++itr) {
        if (chart.find(itr->representative()).second == false) {
            chart.insert_edge(*itr);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
void apply_binary_rule( GT& gram
                      , edge_filter<ET>& filter
                      , concrete_edge_factory<ET,GT>& ef
                      , edge_equivalence_pool<ET>& epool
                      , typename GT::rule_type const& rule
                      , typename chart_traits<CT>::edge_range const& er1
                      , typename chart_traits<CT>::edge_range const& er2 )
{
    typename chart_traits<CT>::edge_iterator ei1 = er1.begin(), 
                                             ee1 = er1.end();
    for (; ei1 != ee1; ++ei1) {
        typename chart_traits<CT>::edge_iterator ei2 = er2.begin(), 
                                                 ee2 = er2.end();
        for (; ei2 != ee2; ++ei2) {
            gusc::any_generator<ET> 
                gen = ef.create_edge(gram,rule,*ei1, *ei2);
            while (gen) {
                filter.insert(epool,gen());
            }
            
            //std::cerr << "apply " << print(rule,gram) << "  to  " 
            //          << print(*ei1,gram.dict()) << " and "
            //          << print(*ei2,gram.dict()) << std::endl;
        }
    }
    //std::cerr << "used filter: ";
    //filter.print(std::cerr);
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
void apply_unary_rule( GT& gram
                     , edge_filter<ET>& filter
                     , concrete_edge_factory<ET,GT>& ef
                     , edge_equivalence_pool<ET>& epool
                     , typename GT::rule_type const& rule
                     , typename chart_traits<CT>::edge_range const& er )
{
    typename chart_traits<CT>::edge_iterator ei = er.begin(), 
                                             ee = er.end();
    for (; ei != ee; ++ei) {
        //std::cerr << "apply " << print(rule,gram) << "  to  " 
        //          << print(*ei,gram.dict()) << std::endl;
        gusc::any_generator<ET> 
            gen = ef.create_edge(gram,rule,*ei);
        while (gen) {
            filter.insert(epool,gen());
        }
    }
    //std::cerr << "used filter: ";
    //filter.print(std::cerr);
    //std::cerr << std::endl;
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
void filter_bank<ET,GT,CT>::apply_toplevel_rules()
{
    io::logging_stream& log = io::registry_log(fb_domain);
    partitions_generator pg(total_span);
    edge_filter<ET> unary_filter = span_filt_factory->unary_filter(total_span);
    span_filter_p 
        binary_filter = span_filter_p(
                            span_filt_factory->create(total_span,gram,ef,chart)
                        );
    
    partitions_generator::iterator pgi = pg.begin(), pge = pg.end();
    for (; pgi != pge; ++pgi) {
        span_t first_constit = pgi->first;
        span_t second_constit = pgi->second;
        typename chart_traits<CT>::cell_range cr = chart.cells(first_constit);
        typename chart_traits<CT>::cell_iterator ci = cr.begin(), 
                                                 ce = cr.end();

        for (; ci != ce; ++ci) {
            typename signature_index_map<GT>::iterator sigitr, sigend;
            boost::tie(sigitr,sigend) = sig_map.toplevel_reps(ci->root());

            for (;sigitr != sigend; ++sigitr) {
                typename span_filter_type::rule_iterator ritr, rend;
                boost::tie(ritr,rend) = rhs_map.toplevel_rules(*sigitr);
                indexed_token rhs2 = gram.rule_rhs(*sigitr,1);
                typename chart_traits<CT>::edge_range 
                    er1 = ci->edges(),
                    er2 = chart.edges(rhs2,second_constit);
                binary_filter->apply_rules( boost::make_tuple(ritr,rend)
                                          , er1
                                          , er2 );
            }
        }
    }
    typename chart_traits<CT>::cell_range cr = chart.cells(total_span);
    typename chart_traits<CT>::cell_iterator ci = cr.begin(), ce = cr.end();
    for (; ci != ce; ++ci) {
        typename chart_traits<CT>::edge_range er = 
                                   chart.edges(ci->root(), total_span);
        typename GT::rule_range rr = gram.toplevel_unary_rules(ci->root());
        typename GT::rule_iterator ri = rr.begin(), re = rr.end();
        for (;ri != re; ++ri) {
            apply_unary_rule<ET,GT,CT>(gram,unary_filter,ef,epool,*ri,er);
        }
    }
    {
    io::log_time_space_report(log,io::lvl_verbose,"binary_filter::finalize(top):");
    binary_filter->finalize();
    }
    {
    io::log_time_space_report(log,io::lvl_verbose,"unary_filter::finalize(top):");
    unary_filter.finalize();
    }
    while (not binary_filter->empty()) {
        chart.insert_edge(binary_filter->top());
        binary_filter->pop();
    }
    while (not unary_filter.empty()) {
        chart.insert_edge(unary_filter.top());
        unary_filter.pop();
    }
}


} // namespace sbmt
