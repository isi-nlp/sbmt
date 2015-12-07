#ifndef _stupid_dependency_lm_info_h
#define _stupid_dependency_lm_info_h

#include <boost/shared_ptr.hpp>
#include <sbmt/edge/edge_info.hpp>
#include <sbmt/token/indexed_token.hpp>
#include <sbmt/edge/null_info.hpp>
#include <sbmt/dependency_lm/DLM.hpp>
#include <vector>
#include <string>
#include <iostream>

#define DEBUG_DLM 0



namespace sbmt {

/// N: how many boundary words need to be memoized. The head
/// word is memo-ized regardless. We need to set N=0 when
/// using bilexical dlm, and set N=2 when using BBN style 
/// trigram dlm.
template<unsigned N, class LMID_TYPE = unsigned int>
class stupid_dependency_lm_info
{
public:
    typedef stupid_dependency_lm_info info_type;
    enum {max_num_dlms=2};
private:
    LMID_TYPE head; 
    LMID_TYPE boundary_words[N];
    bool scoreable;
    // Sacraficing memeory for simplicity. \TODO: will get rid of these and compute
    // the scores on the fly.
    //score_t inside_score;
    //score_t multi_dlm_scores[max_num_dlms];  // assuming there are only 2 dlms at most.
 public:
     stupid_dependency_lm_info& operator=(stupid_dependency_lm_info const& o) 
     {
         unsigned i;

         head = o.head;
         scoreable = o.scoreable;
         //inside_score = o.inside_score;
         //for(i = 0; i < max_num_dlms; ++i){ multi_dlm_scores[i] = o.multi_dlm_scores[i]; }
         for(i = 0; i < N; ++i) { boundary_words[i] = o.boundary_words[i];}

         return *this;
     }

     stupid_dependency_lm_info() {
         int i;
         head = (LMID_TYPE) -1;
         for(i = 0; i < N; ++i) { boundary_words[i] = (LMID_TYPE)-1;}
         scoreable = false;
         //inside_score.set(as_one());
         //for(i = 0; i < max_num_dlms; ++i){ multi_dlm_scores[i].set(as_one()); }
     }

    bool is_scoreable() const { return scoreable; } 

    //score_t get_inside_score() const { return inside_score;}
    //void set_inside_score(const score_t s ) { inside_score.set(s);}

    //score_t get_single_dlm_score(int i) const { return multi_dlm_scores[i];}
    //void set_single_dlm_score(const score_t s, int i ) { multi_dlm_scores[i].set(s);}

    static bool has_component_features() { return true; }
    
    static std::string component_features() { return "deplm"; }
    
    //! compare the context words
    bool equal_to(info_type const& other) const
    {

        if(head == other.head && scoreable == other.scoreable ) {
            for(unsigned i = 0; i < N; ++i) { if(boundary_words[i] != other.boundary_words[i]){ return false;}}
            return true;
        } else {
            return false;
        }
    }

    void set_scoreable(const bool yes) { scoreable = yes;}
    void set_head(const LMID_TYPE h) { head = h;}
    void set_boundary(const LMID_TYPE bw, const unsigned i) { assert(i<N); boundary_words[i] = bw;}
    LMID_TYPE get_head() const { return head;}
    LMID_TYPE get_bw(const unsigned i) const { assert(i < N); return boundary_words[i];}
    
    //! hash the context words.
    std::size_t hash_value() const; 
    
    template <class C, class T, class LM>
    std::basic_ostream<C,T>& 
    print(std::basic_ostream<C,T>& o, LM const& lm) const;

    template <class C, class T>
    std::basic_ostream<C,T>& print_self(std::basic_ostream<C,T>& o) const;

    template <class C, class T, class TF>
    std::basic_ostream<C,T>& print_self(std::basic_ostream<C,T>& o, TF& tf) const {
        return print_self(o);
    }
};

////////////////////////////////////////////////////////////////////////////////

/*
// warning: not multi-LM safe.
template<class O,unsigned N,class LM>
O& operator <<(O& o, ngram_info<N,LM> const& info)
{
    info.print_self(o);
    return o;
}
*/

////////////////////////////////////////////////////////////////////////////////

template<class C, class T, class TF, class ID,unsigned N>
void print(std::basic_ostream<C,T> &o, stupid_dependency_lm_info<N, ID> const& info, TF const&tf)
{
    info.print_self(o,tf);
}

template<class C, class T, class TF, class ID,unsigned N, class LM>
void print(std::basic_ostream<C,T> &o, stupid_dependency_lm_info<N, ID> const& info, LM const& lm)
{
    info.print(o,lm);
}

////////////////////////////////////////////////////////////////////////////////

template <unsigned N, class LM>
class stupid_dependency_lm_info_factory
{
 public:
    typedef enum {BILEXICAL, BBN_TRIGRAM}  deplm_type; 


    template <class O>
    void print_stats(O &o) const;
    
    void reset() {}

    typedef LM lm_type;        
    typedef typename lm_type::lm_id_type lm_id_type;
    typedef stupid_dependency_lm_info<N, lm_id_type> info_type;
    typedef stupid_dependency_lm_info_factory<N, LM> self_type;
    typedef edge<info_type> edge_type;
    typedef typename edge_type::edge_equiv_type edge_equiv_type;
        
    stupid_dependency_lm_info_factory(
            //std::vector<boost::shared_ptr<lm_type> > lm,
            MultiDLM& lm,
    typename stupid_dependency_lm_info_factory::deplm_type dlmtype = BILEXICAL)
      : m_depLMs(lm), m_deplmType(dlmtype) {}

    template <class IT, class Sent, class Gram>
    score_t print_details( std::ostream& o
                         , edge_equivalence< edge<IT> > deriv
                         , Sent const& sent
                         , const Gram& g
                         , bool more=false) 
    {
        int k;
        vector<score_t> individual_dlm_scores;
        individual_dlm_scores.resize(m_depLMs.size());
        for(k = 0; k < m_depLMs.size(); ++k){ 
            individual_dlm_scores[k].set(as_one()); 
        }
        score_t ss = dlm_score<IT, Sent, Gram>(deriv.representative(), g ,
                                                        individual_dlm_scores);

        o<<" "<<info_type::component_features() <<"-w-sum="<<ss;
        for(k = 0; k < m_depLMs.size(); ++k){
            o<<" "<<info_type::component_features()<<k+1
                                            <<"="<<individual_dlm_scores[k];
        }
       return ss;
    }
    
    bool deterministic() const { return true; }

    template <class IT, class Sent, class Grammar>
    score_t dlm_score(const edge<IT> & e, const Grammar& g, 
                                      vector<score_t>& ind_dlm_scores) const 
    {
        score_t ret = as_one();

        vector<const edge<IT>*> edges;
        if (e.has_first_child()) {
            edges.push_back(&e.first_child());
            if (e.has_second_child()) edges.push_back(&e.second_child());
        }

        if(e.has_syntax_id()){
            typename Grammar::rule_type r = g.rule(e.rule_id());

            indexed_lm_string const& lmstr=g.template rule_property<indexed_lm_string>(r,lmid);
            if(edges.size()){
                ret *= compute_dlm_delta_score(g, lmstr, edges, ind_dlm_scores);
            } else {
                ret *= compute_dlm_delta_score(g, lmstr, ind_dlm_scores);
            }

        }
        for(size_t i = 0; i < edges.size(); ++i){
            ret *= dlm_score<IT, Sent, Grammar>(*edges[i], g, ind_dlm_scores);
        }
        return ret;
    }

    template <class IT, class Grammar>
    score_t compute_dlm_delta_score(const Grammar&g, 
                                    const indexed_lm_string& lmstr, 
                                    vector<const edge<IT>*> edges, 
                                    vector<score_t>& ind_dlm_scores) const; 
    template <class Grammar>
    score_t compute_dlm_delta_score(const Grammar&g, 
                                    const indexed_lm_string& lmstr, 
                                    vector<score_t>& ind_dlm_scores) const;
    //! Unary rule.
    template <class Gram,class IT> void
    create_info(info_type &n, score_t &inside, score_t &heuristic,Gram& gram
                , typename Gram::rule_type r
                , edge<IT> const& e )
    {
        assert(gram.is_dep_lm_scoreable(r));
        indexed_lm_string const& lmstr=gram.template rule_property<indexed_lm_string>(r,lmid);
        std::vector<const edge<IT> *> edgevec;
        edgevec.push_back(&e);
        bool is_toplevel = (gram.rule_lhs(r).type() == top_token);
        compute_ngrams(n, inside,heuristic,is_toplevel, gram,lmstr,edgevec);
    }
    
    void create_info(info_type& n);
    
	//! Initial inference.
    template <class GT> void
    create_info( info_type& n
               , score_t& i
               , score_t& h
               , GT& g
               , indexed_syntax_rule const& r )
    {
        indexed_lm_string const& lmstr=r.get_dep_lm_string();
        compute_ngrams(n,i,h,g,lmstr);
        //n.set_inside_score(i);
    }
    
    template <class Gram,class IT> void
    create_info(info_type &n,score_t &inside, score_t &heuristic,Gram& gram 
                , typename Gram::rule_type r
                , const edge<IT>& e1
                , const edge<IT>& e2)
    {
        if(gram.is_dep_lm_scoreable(r)){
            std::vector<const edge<IT>*> edgevec;
            indexed_lm_string const& lmstr=gram.template rule_property<indexed_lm_string>(r,lmid);
			edgevec.push_back(&e1);
			edgevec.push_back(&e2);
            bool is_toplevel = (gram.rule_lhs(r).type() == top_token);
            compute_ngrams(n,inside,heuristic,is_toplevel, gram,lmstr, edgevec);
        } else {
            inside = as_one();
            heuristic = as_one();
            //n.set_inside_score (inside);
            n.set_head( (lm_id_type) -1);
            //for(int i = 0; i < m_depLMs.size(); ++i){
             //   n.set_single_dlm_score(as_one(), i);
            //}
        }
    }

    
    
    template <class Gram>
    score_t score_estimate(Gram& gram, typename Gram::rule_type r) const; 
                           //called by ecs and cached in cube_span_filter

 private:

    score_t heuristic_score(info_type const& info) const; 
                     // used only with identity lmstring, or for debugging
    
    /// convert the indexed (d)lm string into a deque of strings, with
    /// variable indexes replaced with the head word of the corresponding
    /// edge. 
    /// -- The converted string vector will never be empty --- internally,
    ///    if it is empty, we put sequence "<H> <unk> <H>" in.
    template <class Gram>
    void deintegerize_dlm_string_bilexicalDLM(const indexed_lm_string& lmstr, 
                                 Gram& gram,
                                 std::vector<const info_type*> antecedent_ITs,
                                 std::deque<std::string>& vout) const;

    /// convert the indexed (d)lm string into a deque of strings, with
    /// variable indexes replaced with the head word of the corresponding
    /// edge. 
    /// if there is any <PH> in front of the variable index, we replace the
    /// <PH> string with the (left/right) boundary word of the edge, 
    /// depending on whether it is an dlm in left direction (to head) or
    /// the right direction.
    /// -- The converted string vector will never be empty --- internally,
    ///    if it is empty, we put sequence "<H> <unk> <H>" in.
    template <class Gram>
    void deintegerize_dlm_string_trigramDLM(const indexed_lm_string& lmstr, 
                                   Gram& gram,
                                   std::vector<const info_type*> antecedent_ITs,
                                   std::deque<std::string>& vout) const ;

    /// return the set of scoreable antecedent edges for the input vector
    /// of edges. The returned edges are ordered in the foreign sentence
    /// positions.
    template<class IT>
    void get_scoreable_antecedent_IT(std::vector<const edge<IT>*> edgevec,
                                     std::vector<const info_type*>& ants) const;


    template <class Gram, class IT>
    void compute_ngrams( info_type &n
                       , score_t &inside
                       , score_t &heuristic
                       , bool istoplevel
                       , Gram& gram
                       , indexed_lm_string const& lmstr
					   , std::vector<const edge<IT>*> edgevec);

    // I have to duplicate this because I dont want this method
    // to dependend on IT.
    template <class Gram>
    void compute_ngrams( info_type &n
                       , score_t &inside
                       , score_t &heuristic
                       , Gram& gram
                       , indexed_lm_string const& lmstr);
    
    
    
    //! The dependency LMs. Multiple ones in case the entire
    //! deplm is decomposed into different ones each for one
    //! type of event.
    //std::vector<boost::shared_ptr<lm_type> > m_depLMs;
    MultiDLM& m_depLMs;

    deplm_type  m_deplmType;
};

} // namespace sbmt

#include "sbmt/edge/impl/stupid_dependency_lm_info.ipp"

#endif
