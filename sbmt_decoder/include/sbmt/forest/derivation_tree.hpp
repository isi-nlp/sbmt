#ifndef SBMT__FOREST__SYNTAX_DERIVATION_TREE_HPP
#define SBMT__FOREST__SYNTAX_DERIVATION_TREE_HPP

//FIXME: add some interface so force decode edge transforms, given grammar, binary rule id into underlying grammar rule id (which is what you will see on binary deriv. tree

#include <sbmt/forest/logging.hpp>
#include <sbmt/span.hpp>
#include <sbmt/grammar/syntax_id_type.hpp>
#include <sbmt/hash/oa_hashtable.hpp>
#include <graehl/shared/tree.hpp>
#include <graehl/shared/stream_util.hpp>
#include <graehl/shared/optional_pair.hpp>
#include <stdexcept>

namespace sbmt {

template <class id_type>
struct id_and_span : public graehl::optional_pair<id_type,span_t>
{
    typedef graehl::optional_pair<id_type,span_t> parent;
    id_and_span() {}
    id_and_span(id_type const& i) : parent(i) {}
    id_type const& id() const
    {
        return this->first;
    }
    span_t const& span() const 
    {
        assert(this->has_second);
        return this->second;
    }
    id_type & id()
    {
        return this->first;
    }
    span_t & span()
    {
        assert(this->has_second);
        return this->second;
    }

    bool has_span() const 
    {
        return this->has_second;
    }
    
    typedef id_and_span self_type;
    
    template <class I>
    void read(I& in)
    {
        char c;
        if (!(in >> c)) return;
        if (c=='[') {
            this->has_second=true;
            in.unget();
            if (!(in >> this->second)) return;
        } else
            in.unget();
        in >> this->first;
    }
    template <class O>
    void print_span(O &o) const
    {
        if (has_span())
            o << span();
    }
    
    template <class O>
    void print(O& o,bool never_span=false) const
    {
        if (!never_span)
            print_span(o);        
       o << id();
   }

    TO_OSTREAM_PRINT
    FROM_ISTREAM_READ
};

typedef id_and_span<syntax_id_type> sid_span;

typedef graehl::tree<sid_span> syntax_derivation_tree;

typedef id_and_span<grammar_rule_id> gid_span;
typedef graehl::tree<gid_span> binary_derivation_tree;
typedef boost::shared_ptr<binary_derivation_tree> binary_derivation_tree_p;

/**

What's a binary derivation tree? - note: some unary and leaves also :)

Labels are NULL_GRAMMAR_RULE_ID wherever you have a foreign word (without a lattice-provenance, which we have no way of indicating in syntax_derivation_tree anyway, or else grammar rule id (such that g.rule(id) exists)

The order of the children of a rule-labelled node is exactly foreign binary-rhs order:

 typedef binary_rule<indexed_token> indexed_binary_rule;
 indexed_binary_rule br=g.binary_rule(g.rule(id));
 if (br.rhs_size()>0) br.rhs(0) gives left child
 if (br.rhs_size()>1) br.rhs(1) gives right child

 rhs token = foreign -> (except lattice tbd) NULL_GRAMMAR_RULE_ID leaf
 rhs token = tag -> insert next syntax deriv tree child (same order)
 rhs token = virtual tag -> recurse expanding 
**/

typedef std::runtime_error malformed_syntax_derivation_tree;

inline void throw_bad_syntax_derivation_tree(std::string const& reason) 
{
    throw malformed_syntax_derivation_tree("malformed syntax-id derivation tree: "+reason);
}

struct binary_derivation_rule 
{
    grammar_rule_id id;
    syntax_id_type sid;
    rule_topology<indexed_token> t;
    template <class G>
    binary_derivation_rule(G const& g,grammar_rule_id id)
        : id(id)
        , sid(g.get_syntax_id(g.rule(id)))
        , t(g.binary_rule(g.rule(id)).topology())
    {}
    typedef binary_derivation_rule self_type;
    template <class O,class TF>
    void print(O&o,TF const& tf) const
    {
        o << '[';
        t.print(o,tf);
        if (sid!=NULL_SYNTAX_ID)
            o << " id="<<sid;
        o << ']';
    }
};

template <class G>
struct binary_id_writer
{
    G const& g;
    explicit binary_id_writer(G const& g) : g(g) {}
    template <class O>
    void operator()(O &o,gid_span const& ids) const
    {
        ids.print_span(o);
        (*this)(o,ids.id());
    }
    template <class O>
    void operator()(O &o,grammar_rule_id id) const
    {
        if (id==NULL_GRAMMAR_RULE_ID)
            o << "foreign";
        else
            binary_derivation_rule(g,id).print(o,g.dict());
    }
};

/// builds a complete index over the grammar, so this is a heavy object to copy.  build it once and use it for many derivations
/// FIXME:  support grammars with token type other than indexed_token (via G::token_type)
template <class G>
struct syntax_to_binary_derivation
{
 private:
    typedef rule_topology<indexed_token> topo_t;
    
        
    // syntax id -> rule.  then recurse on virtual child NTs: virtual-NT -> rule (there can be only one)
    typedef oa_hash_map<syntax_id_type,binary_derivation_rule> sid_map_t;
    sid_map_t sid_map;
    typedef oa_hash_map<indexed_token,binary_derivation_rule> lhs_map_t;
    lhs_map_t lhs_map;
    G &g; // we may need to add unary rules for foreign words in lattice.  //FIXME: until then, we may not need to template on G at all
 public:
//    template <class G>
    syntax_to_binary_derivation(G & g) : g(g) {
        //for (typename G::rule_range a=g.all_rules();a;++a) {
        BOOST_FOREACH(typename G::rule_type r, g.all_rules()) {
            grammar_rule_id id=g.id(r);
            binary_derivation_rule br(g,id);
            if (br.sid!=NULL_SYNTAX_ID) {
                sid_map.insert(std::make_pair(br.sid,br));
            } else { // virtual
                assert(br.t.lhs().type()==virtual_tag_token);
                lhs_map.insert(std::make_pair(br.t.lhs(),br));
            }
        }    
    }
    
    typedef syntax_derivation_tree::const_iterator child_it;
    
    //FIXME: throw exceptions when invalid sid, virtual, etc.
    // you must deallocate
    binary_derivation_tree *to_binary(syntax_derivation_tree const& s) const
    {
        child_it c=s.begin(),e=s.end();
        binary_derivation_tree *ret=to_binary_step(s.label.id(),c,e);
        if (c!=e) {
            io::logging_stream& logstr = io::registry_log(forest_domain);
            logstr<<io::warning_msg<<e-c<<" unused syntax id subtrees for binary rule id="<<s.label.id()<<" in subtree"<<s<<io::endmsg;
            throw_bad_syntax_derivation_tree("syntax id subtree had more children than binarization for id had real nonterminals");
        }
        return ret;
    }
    binary_derivation_tree_p to_binary_p(syntax_derivation_tree const& s) const
    {
        return binary_derivation_tree_p(to_binary(s));
    }
    
    binary_derivation_tree to_binary_val(syntax_derivation_tree const& s) const
    {
        binary_derivation_tree ret;
        binary_derivation_tree *p=to_binary(s);
        ret=*p;
        delete p;
        return ret;
    }

 private:
    // end is unused except to check for bug in derivation tree w.r.t binary tree
    binary_derivation_tree *to_binary_step(syntax_id_type sid,child_it &c,child_it end) const
    {
        return to_binary_step(sid_map.at_throw(sid),c,end);
    }

    binary_derivation_tree *to_binary_step(indexed_token virt,child_it &c,child_it end) const
    {
        return to_binary_step(lhs_map.at_throw(virt),c,end);
    }
    
    binary_derivation_tree *to_binary_step(binary_derivation_rule const& r,child_it &c,child_it end) const
    {
        topo_t const& topo=r.t;
        binary_derivation_tree *ret=new binary_derivation_tree(gid_span(r.id),topo.rhs_size());
        try {
            for (unsigned i=0,e=topo.rhs_size();i!=e;++i) {
                indexed_token t=topo.rhs(i);
                binary_derivation_tree *&child=ret->child(i);
                if (t.type()==virtual_tag_token) {
                    child=to_binary_step(t,c,end);
                } else if (t.type()==tag_token) {
                    if (c>=end)
                        throw_bad_syntax_derivation_tree("syntax id subtree had fewer children than binarization for id had real nonterminals");
                    child=to_binary(**c++);
                } else {
                    child=new binary_derivation_tree(NULL_GRAMMAR_RULE_ID,0);
                }   
            }
        } catch(...) {
            delete ret;
            throw;
        }
        return ret;
    }
};

template <class O,class G>
void print_binary_derivation(O &o,binary_derivation_tree const& b,G const& g,bool lisp_style=true) 
{
    b.print_writer(o,binary_id_writer<G>(g),lisp_style);
}

    
template <class G>
binary_derivation_tree_p binary_derivation(syntax_derivation_tree const& s,G &g) 
{
    return syntax_to_binary_derivation<G>(g).to_binary_p(s);
}


template <class Edge>
struct binary_derivation_in_forest 
{
    typedef Edge edge_type;
    
    typedef typename edge_type::edge_equiv_type edge_equiv_type;
    typedef typename edge_equiv_type::impl_type edge_equiv_impl;
    typedef typename edge_equiv_type::impl_shared_ptr edge_equiv_ptr; //FIXME: is this necessary?  maybe only if deletion happens during visits.  get_shared
    
    typedef std::pair<binary_derivation_tree *,edge_equiv_ptr> intersect_state; // boost::hash on std::pair defined already?
        
};

    
    

}

#endif
