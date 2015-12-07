
namespace sbmt {
    
template <class IT>
std::pair<span_t,bool>
join_spans( span_string const& sstr
          , edge<IT> const& e1
          , edge<IT> const& e2 )
{
    if (e1.root().type() == foreign_token) {
        return join_spans( 
                   sstr
                 , e2.info().info().espan()
               );          
    } else if (e2.root().type() == foreign_token) {
        return join_spans( 
                   sstr
                 , e1.info().info().espan()
               );
    } else {
        return join_spans(
                   sstr
                 , e1.info().info().espan()
                 , e2.info().info().espan()
               );
    }
}
// for non-scoreable edges, ensures that what is built so far is consistent with
// the force-string.  itg-binarizer publishes for non-scoreable rules the 
// permutation order of elements built up by that rule.  we check to make sure
// the scoreable items in the edge have english spans that are in a consistent
// order with the permutation order.
template <class ET>
bool consistent_edge(ET const& e, span_string const& sstr, std::vector<ET const*>& frontier)
{
    if (sstr.size() == 0 or sstr.size() == 1) return true;
    assert(not e.get_info().scoreable());
    frontier.clear();
    frontier.reserve(sstr.size());
    scoreable_children(e,std::back_inserter(frontier));
    assert(frontier.size() == sstr.size());
    //std::map<int,span_t> m;
    span_string::iterator sprev = sstr.begin(), send = sstr.end();
    span_string::iterator sitr = sprev; ++sitr;
    
    for (; sitr != send; ++sitr, ++sprev) {
        if ( frontier[sprev->get_index()]->info().info().espan().right() > 
             frontier[sitr->get_index()]->info().info().espan().left() )
             return false;
    }
    
    return true;
    
//    typename std::vector<ET const*>::iterator fitr = frontier.begin(), fend = frontier.end();
//    for (; fitr != fend; ++fitr, ++sitr) {
//        assert(sitr->is_index());
//        m.insert(std::make_pair( 
//                   sitr->get_index()
//                 , (*fitr)->template cast_info<sentence_info>().espan()
//                ) );
//    }
//    assert(m.size() == frontier.size());
//    std::map<int,span_t>::iterator itr, end, prev;
//    prev = m.begin();
//    itr = prev; ++itr;
//    end  = m.end();
//    for (; itr != end; ++itr) {
//        if (prev->second.right() > itr->second.left()) return false;
//    }
//    return true;
}

////////////////////////////////////////////////////////////////////////////////

template <class GT>
force_grammar<GT>::rule_iterator::rule_iterator( typename GT::rule_iterator itr
                                               , typename GT::rule_iterator end
                                               , rule_match_map_t const& m )
: ritr(itr)
, rend(end)
, matches(&m)
{
    if (ritr == rend) { sitr = send; }
    else {
        std::list<pair_ptr_t> const& sstr = matches->find(*ritr)->second;
        sitr = sstr.begin();
        send = sstr.end();
        if (sitr == send) advance_ritr();
        else current = sitr->get();
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class GT>
void force_grammar<GT>::rule_iterator::advance_ritr()
{
    ++ritr;
    while (ritr != rend) {
        std::list<pair_ptr_t> const& sstr = matches->find(*ritr)->second;
        sitr = sstr.begin();
        send = sstr.end();
        if (sitr == send) ++ritr;
        else break;
    }
    if (ritr != rend) {
        current = sitr->get();
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class GT>
void force_grammar<GT>::rule_iterator::increment()
{
    ++sitr;
    if (sitr == send) advance_ritr();
    else current = sitr->get();
}

////////////////////////////////////////////////////////////////////////////////

template <class GT>
bool force_grammar<GT>::rule_iterator::equal(rule_iterator const& o) const
{
    if (ritr == rend) { return ritr == o.ritr; }
    else { return ritr == o.ritr and sitr == o.sitr; }
}

template <class GT>
force_grammar<GT>::rule_iterator::rule_iterator()
: matches(NULL)
{
    ritr = rend;
    sitr = send;
}


////////////////////////////////////////////////////////////////////////////////
    
template <class GT>
typename force_grammar<GT>::rule_range
force_grammar<GT>::all_rules() const
{
    typename GT::rule_range rr = gram.all_rules();
    return rule_range( rule_iterator(rr.begin(), rr.end(), rule_matches)
                     , rule_iterator(rr.end(), rr.end(), rule_matches) );
}

////////////////////////////////////////////////////////////////////////////////

template <class GT>
typename force_grammar<GT>::rule_range
force_grammar<GT>::unary_rules(token_type t) const
{
    typename GT::rule_range rr = gram.unary_rules(t);
    return rule_range( rule_iterator(rr.begin(), rr.end(), rule_matches)
                     , rule_iterator(rr.end(), rr.end(), rule_matches) );
}

////////////////////////////////////////////////////////////////////////////////

template <class GT>
typename force_grammar<GT>::rule_range
force_grammar<GT>::toplevel_unary_rules(token_type t) const
{
    typename GT::rule_range rr = gram.toplevel_unary_rules(t);
    return rule_range( rule_iterator(rr.begin(), rr.end(), rule_matches)
                     , rule_iterator(rr.end(), rr.end(), rule_matches) );
}

////////////////////////////////////////////////////////////////////////////////

template <class GT>
typename force_grammar<GT>::rule_range
force_grammar<GT>::binary_rules(token_type t) const
{
    typename GT::rule_range rr = gram.binary_rules(t);
    return rule_range( rule_iterator(rr.begin(), rr.end(), rule_matches)
                     , rule_iterator(rr.end(), rr.end(), rule_matches) );
}

////////////////////////////////////////////////////////////////////////////////

template <class GT>
typename force_grammar<GT>::rule_range
force_grammar<GT>::toplevel_binary_rules(token_type t) const
{
    typename GT::rule_range rr = gram.toplevel_binary_rules(t);
    return rule_range( rule_iterator(rr.begin(), rr.end(), rule_matches)
                     , rule_iterator(rr.end(), rr.end(), rule_matches) );
}

////////////////////////////////////////////////////////////////////////////////

template <class InsertItrT, class LMSTRINGITR> void
span_strings_from_potentials( std::list< std::vector<span_t> >::iterator pitr
                            , std::list< std::vector<span_t> >::iterator pend
                            , span_index_t idx
                            , LMSTRINGITR litr
                            , LMSTRINGITR lend
                            , span_string& str
                            , InsertItrT str_out )
{
    using namespace std;
    
    if (litr == lend) {
        //std::cerr << "span_string: " << str << std::endl;
        *str_out = str;
        ++str_out;
    } 
    else if (litr->is_index()) {
        str.push_back(litr->get_index());
        ++litr;
        span_strings_from_potentials(pitr,pend,idx,litr,lend,str,str_out);
        str.pop_back();
    } 
    else {
        for (; litr != lend and litr->is_token(); ++litr) {}
        vector<span_t>::iterator itr = pitr->begin(),
                                 end = pitr->end();
        for (; itr != end and itr->left() < idx; ++itr) {}
        ++pitr;
        for (; itr != end;++itr) {
            //span_string sstr = str;
            str.push_back(*itr);
            span_strings_from_potentials( pitr, pend
                                        , itr->right()
                                        , litr, lend
                                        , str, str_out );
            str.pop_back(*itr);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class InsertItrT, class LMSTRING> void
span_strings_from_lm_string( LMSTRING const& lmstr
                           , substring_hash_match<indexed_token> const& match
                           , InsertItrT span_str_out )
{
    using namespace std;
    
    typename LMSTRING::iterator itr = lmstr.begin(),
                                end = lmstr.end();

    list< vector<span_t> > potentials;
    while (itr != end) {
        if (itr->is_index()) { ++itr; continue; }
        potentials.push_back(vector<span_t>());
        typename LMSTRING::iterator jtr = itr;
        
        for (;(itr != end) and itr->is_token(); ++itr) {}

        if (!match(jtr,itr,back_inserter(potentials.back()))) {
            return; // this rule cant match
        }
        sort(potentials.back().begin(),potentials.back().end());
    }
    //special case: non-lexical lm-string
    if (potentials.size() == 0) {
        span_string sstr;
        itr = lmstr.begin();
        end = lmstr.end();
        for (; itr != end; ++itr) {
            sstr.push_back(span_string::token(itr->get_index()));
        }
        *span_str_out = sstr;
        ++span_str_out;
        return;
    }
    span_string sstr;
    span_strings_from_potentials( potentials.begin()
                                , potentials.end()
                                , 0
                                , lmstr.begin()
                                , lmstr.end()
                                , sstr
                                , span_str_out );
}

////////////////////////////////////////////////////////////////////////////////


template <class ET, class GT, class CT>
void 
force_sentence_filter<ET,GT,CT>::apply_rule( rule_type const& r
                                           , span_t const& first_constituent )
{
    //std::cerr << "begin apply_rule: ";
    assert(base_t::gram.rule_rhs_size(r) == 2);
    std::pair<typename CT::cell_iterator,bool>
        p1 = base_t::chart.cell( first_constituent
                               , base_t::gram.rule_rhs(r,0) );
    
    span_t second_constituent( first_constituent.right()
                             , base_t::target_span.right() );
    
    std::pair<typename CT::cell_iterator,bool>
        p2 = base_t::chart.cell( second_constituent
                               , base_t::gram.rule_rhs(r,1) );
                               
    if (!p1.second or !p2.second) return;
    
    std::pair<int,int> gap_result = gap(base_t::gram.rule_span_string(r));
    
    if (not base_t::gram.template rule_property<bool>(r,lm_scoreable_id)) {
        //std::cerr << "****************************************** " << std::endl;
        //std::cerr << "* non-scoreable rule " << print(r,base_t::gram) << std::endl;
        //std::cerr << "* target: " << base_t::target_span 
        //          << "left-constit: " << first_constituent << std::endl;
        typename chart_traits<CT>::edge_range er1 = p1.first->edges();
        typename chart_traits<CT>::edge_iterator e1itr = er1.begin();
        typename chart_traits<CT>::edge_iterator e1end = er1.end();
        
        bool dont_check_consistency = (is_lexical(base_t::gram.rule_rhs(r,0))) or
                                      (is_lexical(base_t::gram.rule_rhs(r,1)));
        std::vector<ET const*> frontier_scratch;
        for (; e1itr != e1end; ++e1itr) {
            typename chart_traits<CT>::edge_range 
                er2 = p2.first->edges();
            typename chart_traits<CT>::edge_iterator e2itr = er2.begin();
            typename chart_traits<CT>::edge_iterator e2end = er2.end();
            
            for (; e2itr != e2end; ++e2itr) {
                typename base_t::edge_type e = base_t::ef.create_edge(
                                                       base_t::gram
                                                     , r
                                                     , *e1itr
                                                     , *e2itr
                                                ) ;  
                if ( dont_check_consistency or 
                     consistent_edge(e, base_t::gram.rule_span_string(r),frontier_scratch)
                   ) {
                       //std::cerr << "* created " << print(e,base_t::gram.dict()) << std::endl;  
                       filter.insert(base_t::epool, e) ;
                   }
                //else std::cerr << "* rejected " << print(e,base_t::gram.dict()) << std::endl;
            }
        }
        //std::cerr << "****************************************** " << std::endl;
    } else if (gap_result.second == 1) {
        span_sig sig = signature(base_t::gram.rule_span_string(r),0);
        typename chart_traits<CT>::edge_range er1 = p1.first->edges();
        typename chart_traits<CT>::edge_iterator e1itr = er1.begin();
        typename chart_traits<CT>::edge_iterator e1end = er1.end();
        
        for (; e1itr != e1end; ++e1itr) {
            if (not e1itr->representative().info().scoreable()) continue;
            
            span_t espn = e1itr->representative().info().info().espan();
            if (!sig.match(espn)) continue;
            typename chart_traits<CT>::edge_range 
                er2 = p2.first->edges(espn.right() + gap_result.first);
            typename chart_traits<CT>::edge_iterator e2itr = er2.begin();
            typename chart_traits<CT>::edge_iterator e2end = er2.end();
            
            for (; e2itr != e2end; ++e2itr) {
                if (not e2itr->representative().get_info().scoreable()) continue;
                std::pair<span_t,bool> p = join_spans( base_t::gram.rule_span_string(r)
                                                     , e1itr->representative()
                                                     , e2itr->representative() );                           
                bool create_edge(false) ;
                                       
                if (base_t::gram.rule_lhs(r).type() == top_token) {
                    create_edge = p.second and 
                                  p.first == span_t(0,target.get().size());
                } else {
                    create_edge = p.second;
                }
            
                if (create_edge) {
                    typename base_t::edge_type e = base_t::ef.create_edge(
                                                       base_t::gram
                                                     , r
                                                     , *e1itr
                                                     , *e2itr
                                                   ) ;  
                                                             
                    filter.insert(base_t::epool, e) ;
                }
            }
        }
    } 
    else if (gap_result.second == -1) {
        span_sig sig = signature(base_t::gram.rule_span_string(r),1);
        typename chart_traits<CT>::edge_range er2 = p2.first->edges();
        typename chart_traits<CT>::edge_iterator e2itr = er2.begin();
        typename chart_traits<CT>::edge_iterator e2end = er2.end();
        
        for (; e2itr != e2end; ++e2itr) {
            if (not e2itr->representative().get_info().scoreable()) continue;
            span_t espn = e2itr->representative().info().info().espan();
                        
            if (!sig.match(espn)) continue;
                        
            typename chart_traits<CT>::edge_range 
                er1 = p1.first->edges(espn.right() + gap_result.first);
            typename chart_traits<CT>::edge_iterator e1itr = er1.begin();
            typename chart_traits<CT>::edge_iterator e1end = er1.end();
            
            for (; e1itr != e1end; ++e1itr) {
                if (not e1itr->representative().get_info().scoreable()) continue;
                std::pair<span_t,bool> p = join_spans( base_t::gram.rule_span_string(r)
                                                     , e1itr->representative()
                                                     , e2itr->representative() );                           
                bool create_edge(false) ;
                                       
                if (base_t::gram.rule_lhs(r).type() == top_token) {
                    create_edge = p.second and 
                                  p.first == span_t(0,target.get().size());
                } else {
                    create_edge = p.second;
                }
            
                if (create_edge) {
                    typename base_t::edge_type e = base_t::ef.create_edge(
                                                       base_t::gram
                                                     , r
                                                     , *e1itr
                                                     , *e2itr
                                                   ) ;  
                                                             
                    filter.insert(base_t::epool, e) ;
                }
            }
        }
    } 
    else {
        
        typename chart_traits<CT>::edge_range er1 = 
            base_t::chart.edges( base_t::gram.rule_rhs(r,0)
                               , first_constituent );

         

        typename chart_traits<CT>::edge_range er2 = 
                     base_t::chart.edges( base_t::gram.rule_rhs(r,1)
                                        , second_constituent );

        typename chart_traits<CT>::edge_iterator e1itr = er1.begin();
        typename chart_traits<CT>::edge_iterator e1end = er1.end();

        for (; e1itr != e1end; ++e1itr) {
            if (not e1itr->representative().get_info().scoreable()) continue;
            typename chart_traits<CT>::edge_iterator e2itr = er2.begin();
            typename chart_traits<CT>::edge_iterator e2end = er2.end();
            for (; e2itr != e2end; ++e2itr) {
                if (not e2itr->representative().get_info().scoreable()) continue;
                std::pair<span_t,bool> p = join_spans( base_t::gram.rule_span_string(r)
                                                     , e1itr->representative()
                                                     , e2itr->representative() );                           
                bool create_edge(false) ;
                                       
                if (base_t::gram.rule_lhs(r).type() == top_token) {
                    create_edge = p.second and 
                                  p.first == span_t(0,target.get().size());
                } else {
                    create_edge = p.second;
                }
            
                if (create_edge) {
                    typename base_t::edge_type e = base_t::ef.create_edge(
                                                       base_t::gram
                                                     , r
                                                     , *e1itr
                                                     , *e2itr
                                                   ) ;  
                                                             
                    filter.insert(base_t::epool, e) ;
                }
            }
        }
    }
    
    if (base_t::gram.template rule_property<bool>(r,lm_scoreable_id)) {
        typename chart_traits<CT>::edge_range er1 = p1.first->edges();
        typename chart_traits<CT>::edge_iterator e1itr = er1.begin();
        typename chart_traits<CT>::edge_iterator e1end = er1.end();
        
        for (; e1itr != e1end; ++e1itr) {
            bool e1scoreable = e1itr->representative().get_info().scoreable();
            typename chart_traits<CT>::edge_range 
                er2 = p2.first->edges();
            typename chart_traits<CT>::edge_iterator e2itr = er2.begin();
            typename chart_traits<CT>::edge_iterator e2end = er2.end();
            
            for (; e2itr != e2end; ++e2itr) {  
                if (not e1scoreable or not e2itr->representative().get_info().scoreable()) {                       
                    typename base_t::edge_type e = base_t::ef.create_edge(
                                                           base_t::gram
                                                         , r
                                                         , *e1itr
                                                         , *e2itr
                                                   ) ;
                    if (e.get_info().info().valid()) {  
                        //std::cerr << ">>>> created " << print(e,base_t::gram.dict()) 
                        //          << "from unscoreable parts" << std::endl;  
                        filter.insert(base_t::epool, e) ;
                    }
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
void force_sentence_filter<ET,GT,CT>::finalize()
{
    finalized = true ;
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
bool force_sentence_filter<ET,GT,CT>::is_finalized() const
{
    return finalized ;
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
bool force_sentence_filter<ET,GT,CT>::empty() const
{
    return filter.empty() ;
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
void force_sentence_filter<ET,GT,CT>::pop()
{
    return filter.pop() ;
}

////////////////////////////////////////////////////////////////////////////////

template <class ET, class GT, class CT>
edge_equivalence<ET> const& force_sentence_filter<ET,GT,CT>::top() const
{
    return filter.top() ;
}

////////////////////////////////////////////////////////////////////////////////

template <class E> bool 
force_sentence_predicate::keep_edge(edge_queue<E> const& pq, E const& e) const
{
    edge_info<sentence_info> const& s = e.info();
    if (e.root().type() == top_token) {
        return target_span == s.info().espan() and s.info().valid();
    } else {
        return s.info().valid();
    }
}

////////////////////////////////////////////////////////////////////////////////
    
template <class E> bool 
force_sentence_predicate::pop_top(edge_queue<E> const& pqueue) const
{
    E const& e = pqueue.top().representative() ;
    return not keep_edge(pqueue, e) ;
}

////////////////////////////////////////////////////////////////////////////////

template <class GT>
template <class ITRangeT>
force_grammar<GT>::force_grammar( GT& g
                                , ITRangeT const& constraint)
: gram(g)
{
    substring_hash_match<indexed_token> m(constraint.begin(),constraint.end());
    typename GT::rule_range rr = gram.all_rules();
    typename GT::rule_iterator ritr = rr.begin();
    for (; ritr != rr.end(); ++ritr) {
	//std::cerr << "proc: " << std::flush;
	//std::cerr << print(*ritr,g) << " lmstring={{{" << print(gram.rule_lm_string(*ritr),g) << "}}}" << std::endl; 
        typename rule_match_map_t::iterator pos = 
            rule_matches.insert(std::make_pair( 
                                    *ritr
                                  , std::list<pair_ptr_t>())).first;
        std::list<span_string> slist;
        span_strings_from_lm_string( gram.template rule_property<indexed_lm_string>(*ritr,lm_string_id)
                                   , m
                                   , std::back_inserter(slist));
                                   
        std::list<span_string>::iterator si = slist.begin(), se = slist.end();
        for (;si != se; ++si) {
            pos->second.push_back(pair_ptr_t(new pair_t(*ritr,*si)));
        }
    } 
}

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt
