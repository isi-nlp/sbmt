# include <graehl/shared/intrusive_refcount.hpp>
# include <sbmt/search/lazy/rules.hpp>
# include <sbmt/search/lazy/indexed_varray.hpp>
# include <sbmt/grammar/grammar_in_memory.hpp>
# include <boost/array.hpp>


namespace sbmt { namespace lazy {
    
bool var_compatible(std::set<int> const& vertex, std::set<int> const& rule)
{
    if (vertex.empty()) return true;
    assert(rule.empty() == false);
    
    int n = *rule.begin();
    std::set<int>::const_iterator pos = vertex.lower_bound(n);
    return pos != vertex.end() and n <= *pos;
}

bool lex_compatible(std::set<int> const& vertex, std::set<int> const& rule)
{
    BOOST_FOREACH(int n, rule) {
        std::set<int>::const_iterator pos = vertex.lower_bound(n);
        if (pos == vertex.end()) continue;
        if (n == 0 and *pos == 0) return true;
        else return n <= *pos;
    }
    return false;
}
    
bool rule_compatible( lex_outside_distances const& vertex
                    , lex_outside_distances const& rule )
{
    BOOST_FOREACH(lex_outside_distances::value_type const& vt, rule) {
        lex_outside_distances::const_iterator pos = vertex.find(vt.first);
        if (pos != vertex.end()) {
            if (is_lexical(vt.first)) {
                if (lex_compatible(pos->second, vt.second)) return true;
            } else {
                if (var_compatible(pos->second, vt.second)) return true;
            }
        }
    }
    return false;
}

void print_lex_outside_distances( std::ostream& out
                                , lex_outside_distances const& dist )
{
    BOOST_FOREACH(lex_outside_distances::value_type const& vt, dist) {
        out << vt.first << "=[ ";
        BOOST_FOREACH(int d, vt.second) {
            out << d << ' ';
        }
        out << "] ";
    }
}

bool rule_compatible( indexed_token tok
                    , lex_distance_outside_map const& ldom
                    , lex_outside_distances const& left
                    , lex_outside_distances const& right )
{
    assert(is_virtual_tag(tok));
    lex_distance_outside_map::const_iterator pos = ldom.find(tok);
    bool keep = rule_compatible(left,pos->second.get<0>()) and
                rule_compatible(right,pos->second.get<1>());
    if (false) {
        std::cerr << tok << " rejected\n";
        print_lex_outside_distances(std::cerr,pos->second.get<0>());
        std::cerr << " <--> ";
        print_lex_outside_distances(std::cerr,pos->second.get<1>());
        std::cerr << "\n";
        print_lex_outside_distances(std::cerr,left);
        std::cerr << " <--> ";
        print_lex_outside_distances(std::cerr,right);
        std::cerr << "\n";
    }
    //else std::cerr << tok << " accepted\n";
    return keep;
}

ruleset make_ruleset(preruleset const& prs)
{
    ruleset rs(prs.size());
    size_t x = 0;
    for (preruleset::const_iterator itr = prs.begin(); itr != prs.end(); ++itr)
    {
        rs[x] = itr->second;
        ++x;
    }
    return rs;
}

rulesetset make_rulesetset(prerulesetset const& prss)
{
    gusc::shared_varray<ruleset> rss(prss.size());
    size_t x = 0;
    BOOST_FOREACH(preruleset const& prs, prss.get<1>())
    {
        rss[x] = make_ruleset(prs);
        ++x;
    }
    return rulesetset(rss,prss.key_extractor());
}

lhs_map make_lhs_map(grammar_in_mem const& g)
{
    lhs_map lmap;
    BOOST_FOREACH(grammar_in_mem::rule_type r, g.all_rules()) {
        if (is_virtual_tag(g.rule_lhs(r))) lmap[g.rule_lhs(r)] = r;
    }
    return lmap;
}

boost::tuple<indexed_token,int>& ld_get(lex_distance& ld, int x)
{
    if (x == 0) return ld.get<0>();
    else if (x == 1) return ld.get<1>();
    else throw std::runtime_error("ld_get out-of-range");
}

boost::tuple<indexed_token,int> const& ld_get(lex_distance const& ld, int x)
{
    if (x == 0) return ld.get<0>();
    else if (x == 1) return ld.get<1>();
    else throw std::runtime_error("ld_get out-of-range");
}

lex_distance&
make_lex_distance_inside_map( grammar_in_mem const& g
                            , indexed_token lhs
                            , lhs_map const& lmap
                            , lex_distance_map& ldmap )
{
    lex_distance_map::iterator pos = ldmap.find(lhs);
    if (pos != ldmap.end()) {
        return pos->second;
    }
    grammar_in_mem::rule_type r = lmap.find(lhs)->second;
    int scan[2][2] = {{1,0},{0,1}};
    
    boost::tuple<indexed_token,int> b(g.dict().toplevel_tag(),0);
    lex_distance ld(b,b);
    
    for (int left_or_right = 0; left_or_right != 2; ++left_or_right) {
        int* itr = scan[left_or_right];
        int* end = itr + 2;

        for (; itr != end; ++itr) {
            indexed_token rhs = g.rule_rhs(r,*itr);
            if (is_lexical(rhs)) {
                ld_get(ld,left_or_right).get<0>() = rhs;
                break;
            } else if (is_native_tag(rhs)) {
                ld_get(ld,left_or_right).get<1>() += 1;
            } else {
                lex_distance ld_over = make_lex_distance_inside_map(g,rhs,lmap,ldmap);
                boost::tuple<indexed_token,int > b = ld_get(ld_over,left_or_right);
                
                if (is_lexical(b.get<0>())) {
                    ld_get(ld,left_or_right).get<0>() = b.get<0>();
                    ld_get(ld,left_or_right).get<1>() += b.get<1>();
                    break;
                } else {
                    ld_get(ld,left_or_right).get<1>() += b.get<1>();
                }
            }
        }
    }
    return ldmap[lhs] = ld;
}

lex_distance_map make_lex_distance_inside_map(grammar_in_mem const& g)
{
    lhs_map lmap = make_lhs_map(g);
    lex_distance_map ldmap;
    BOOST_FOREACH(lhs_map::value_type v, lmap) {
        make_lex_distance_inside_map(g,v.first,lmap,ldmap);
    }
    return ldmap;
}

rhs_map make_rhs_map(grammar_in_mem const& g)
{
    rhs_map rmap;
    BOOST_FOREACH(grammar_in_mem::rule_type r, g.all_rules()) {
        for ( size_t idx = 0; idx != g.rule_rhs_size(r); ++idx) {
            indexed_token rhs = g.rule_rhs(r,idx);
            if (is_virtual_tag(rhs)) {
                rmap.insert(std::make_pair(rhs,r));
            }
        }
    }
    return rmap;
}
        
void extend_outside_map(lex_outside_distances& outmap, indexed_token tok, int d) {
    outmap[tok].insert(d);
}

lex_outside_distances&
make_lex_distance_outside_map( grammar_in_mem const& g
                             , lex_distance_map const& limap
                             , rhs_map const& rmap
                             , indexed_token lhs
                             , single_lex_distance_outside_map& lomap
                             , int left_or_right 
                             )
{
    single_lex_distance_outside_map::iterator pos = lomap.find(lhs);
    if (pos != lomap.end()) return pos->second;
    lex_outside_distances outside_map;
    BOOST_FOREACH(rhs_map::value_type rd, rmap.equal_range(lhs)) {
        grammar_in_mem::rule_type r = rd.second;
        if (g.rule_rhs(r,1-left_or_right) == lhs) {
            indexed_token rhs = g.rule_rhs(r,left_or_right);
            bool keep_going = false;
            int var = 0;
            if (is_lexical(rhs)) {
                extend_outside_map(outside_map,rhs,0);
            } else if (is_virtual_tag(rhs)) {
                boost::tuple<indexed_token,int> b = ld_get(limap.find(rhs)->second,left_or_right);
                if (is_lexical(b.get<0>())) {
                    extend_outside_map(outside_map,b.get<0>(),b.get<1>());
                } else {
                    var = b.get<1>();
                    keep_going = true;
                }
            } else {
                var = 1;
                keep_going = true;
            }
            if (keep_going and is_virtual_tag(g.rule_lhs(r))) {
                lex_outside_distances& 
                    outside_outside_map = make_lex_distance_outside_map(g,limap,rmap,g.rule_lhs(r),lomap,left_or_right);
                BOOST_FOREACH(lex_outside_distances::value_type& vt, outside_outside_map) {
                    BOOST_FOREACH(int d, vt.second) {
                        extend_outside_map(outside_map,vt.first,d+var);
                    }
                }
            } else if (keep_going) {
                extend_outside_map(outside_map,g.dict().toplevel_tag(),var);
            }
        }
        if (g.rule_rhs(r,left_or_right) == lhs and is_virtual_tag(g.rule_lhs(r))) {
            lex_outside_distances& 
                outside_outside_map = make_lex_distance_outside_map(g,limap,rmap,g.rule_lhs(r),lomap,left_or_right);
            BOOST_FOREACH(lex_outside_distances::value_type& vt, outside_outside_map) {
                BOOST_FOREACH(int d, vt.second) {
                    extend_outside_map(outside_map,vt.first,d);
                }
            }
        }
    }
    if (outside_map.empty()) extend_outside_map(outside_map,g.dict().toplevel_tag(),0);
    return lomap[lhs] = outside_map;
}

single_lex_distance_outside_map
make_lex_distance_outside_map( grammar_in_mem const& g
                             , lex_distance_map const& limap
                             , int left_or_right 
                             )
{
    rhs_map rmap = make_rhs_map(g);
    single_lex_distance_outside_map lomap;
    BOOST_FOREACH(grammar_in_mem::rule_type r, g.all_rules()) {
        indexed_token lhs = g.rule_lhs(r);
        if (is_virtual_tag(lhs)) {
            make_lex_distance_outside_map(g,limap,rmap,lhs,lomap,left_or_right);
        }
    }
    return lomap;
}

lex_distance_outside_map
make_lex_distance_outside_map( grammar_in_mem const& g
                             , lex_distance_map const& limap
                             )
{
    single_lex_distance_outside_map 
        left = make_lex_distance_outside_map(g,limap,0),
        right = make_lex_distance_outside_map(g,limap,1);
    
    lex_distance_outside_map out;
    BOOST_FOREACH(single_lex_distance_outside_map::value_type& vt, left) {
        out[vt.first] = boost::make_tuple(vt.second,right[vt.first]);
    }
    return out;
}

} } // namespace sbmt::lazy
