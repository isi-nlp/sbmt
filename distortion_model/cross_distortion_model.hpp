# if ! defined(DISTORTION_MODEL__CROSS_DISTORTION_MODEL_HPP)
# define       DISTORTION_MODEL__CROSS_DISTORTION_MODEL_HPP

#include <sbmt/edge/any_info.hpp>
#include <gusc/generator/single_value_generator.hpp>

#include <boost/tuple/tuple.hpp>
#include <boost/functional/hash.hpp>


#include <sbmt/hash/hash_map.hpp>

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <boost/tokenizer.hpp>
#include <gusc/varray.hpp>
# include <sbmt/logging.hpp>

namespace distortion { namespace cross {

SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(gen_distortion_log,"distortion",sbmt::root_domain);

using namespace boost;
using namespace std;
using namespace sbmt;
namespace ext = ::stlext;

//typedef boost::tuple<int,indexed_token,int> distortion_key;
struct distortion_key {
  distortion_key(int span, indexed_token label, bool cross) :
    span(span), label(label), cross(cross) { }
  int span;
  indexed_token label;
  bool cross;
};
inline bool operator==(distortion_key const& k1, distortion_key const& k2) {
  return k1.span == k2.span && k1.label == k2.label && k1.cross == k2.cross;
}

inline std::size_t hash_value(distortion_key const& k) 
{
  size_t seed = 0;
  boost::hash_combine(seed, k.span);
  boost::hash_combine(seed, k.label);
  boost::hash_combine(seed, k.cross);
  return seed;
}

typedef ext::hash_map<distortion_key,score_t,boost::hash<distortion_key> > 
        distortion_table;

// cross={{{ 1 0 1 }}}
// gets read into a vector<int>
// and means, x0 was crossed, x1 wasn't, x2 was
struct read_cross {
  typedef gusc::varray<bool> result_type;

  template <class Dictionary>
  result_type operator()(Dictionary& dict, std::string const& as_string) const {
    vector<bool> r;

    typedef boost::tokenizer<boost::char_separator<char> > tok_t;
    tok_t tok(as_string, boost::char_separator<char>(", "));
    for (tok_t::const_iterator it = tok.begin(); it != tok.end(); ++it) {
      if (*it == "10^-") // workaround for ndwf run amok
          continue;
      r.push_back(lexical_cast<bool>(*it));
    }
    return gusc::varray<bool>(r.begin(),r.end());

    //stringstream stream(as_string);
    //copy(istream_iterator<int>(stream), istream_iterator<int>(), back_inserter(result));
  }
};

////////////////////////////////////////////////////////////////////////////////
//
// from a c++ interface perspective, an info type needs to be
// -- hashable
// -- equality comparable
// -- assignable and copy constructable
// -- default constructable
//
// deriving from info_base is not necessary, but it spares you having to
// write operator== operator!= and a free-function version of hash_value()
//
////////////////////////////////////////////////////////////////////////////////

class distortion_info : public info_base<distortion_info> {
public:
    distortion_info(unsigned short len = 0) : len(len) {}

    unsigned short length() const { return len; }

    bool equal_to(distortion_info const& other) const
    {
        return length() == other.length();
    }

    size_t hash_value() const { return length(); }

private:
    unsigned short len;
};

template <class C, class T, class TF>
void print(std::basic_ostream<C,T>& os, distortion_info const& d, TF const&)
{
    os << d.length();
}

////////////////////////////////////////////////////////////////////////////////
template <class cross_type>
class distortion_info_factory {
public:
    // required by interface
    typedef distortion_info info_type;

    // info, inside-score, heuristic
    typedef tuple<info_type,score_t,score_t> result_type;

    /// exp_weight and normal_weight are the feature-vector weights for this
    /// info
    ///
    /// feature_id is the id used to extract our distortion pdfs from the
    /// grammar rules
    distortion_info_factory( double weight
                           , boost::uint32_t weight_id
                           , size_t feature_id
                           , distortion_table m
                           , indexed_token null_label
                           , bool binarized 
                           , int max_len )
      : weight(weight)
      , weight_id(weight_id)
      , feature_id(feature_id)
      , table(m)
      , null_label(null_label)
      , binarized(binarized)
      , max_len(max_len)
      {
          unseen_score[0] = table.find(distortion_key(0, null_label, 0))->second;
          unseen_score[1] = table.find(distortion_key(0, null_label, 1))->second;
      }

    template <class Grammar>
    std::string
    hash_string(Grammar const& grammar, distortion_info const& di) const
    {
        return boost::lexical_cast<std::string>(di.length());
    }
    
    template <class Grammar>
    score_t
    rule_heuristic( Grammar const& gram, typename Grammar::rule_type rule) const
    {
        return 1.0;
        
        if (is_lexical(gram.rule_lhs(rule))) return 1.0;
        if (!gram.rule_has_property(rule,feature_id)) return 1.0;
        cross_type const& cross = gram.template rule_property<cross_type>(rule,feature_id);
        if (binarized) {
            indexed_token token[2];
            size_t ts = 0;
            for (size_t s = 0; s != gram.rule_rhs_size(rule); ++s) {
                if (is_native_tag(gram.rule_rhs(rule,s))) {
                    token[ts] = gram.rule_rhs(rule,s);
                    ++ts;
                }
            }
            indexed_token const* begin = &token[0];
            return rule_heuristic_(cross,std::make_pair(begin, begin + ts));
        } else {
            std::vector<indexed_token> token;
            typename Grammar::syntax_rule_type::rhs_iterator 
                itr = gram.get_syntax(rule).rhs_begin(),
                end = gram.get_syntax(rule).rhs_end();
            for (; itr != end; ++itr) {
                if (itr->indexed()) token.push_back(itr->get_token());
            }
            return rule_heuristic_(cross,token);
        }
        
    }

    ////////////////////////////////////////////////////////////////////////////
    ///
    ///  this function determines what sort of constituents get put together using
    ///  your info type.  the decoder will only ask you for scores at boundaries
    ///  you select, and it will only pass you constituents that you have already
    ///  scored.
    ///
    ///  the more rules you can score, the more efficient the decoder can be.
    ///
    ///  in this example, only syntax rules are scored.
    ///
    ////////////////////////////////////////////////////////////////////////////
    template <class Grammar>
    bool
    scoreable_rule(Grammar& grammar, typename Grammar::rule_type rule) const
    {
        // if cross feature is binarized, then all rules are scoreable
        if (binarized) return true;
        // we only score at syntax-rule boundaries
        else return !is_virtual_tag(grammar.rule_lhs(rule));
    }
    
    template <class Grammar>
    score_t weighted_heur( Grammar const& grammar
                         , typename Grammar::rule_type rule
                         , unsigned short len ) const
    {
         indexed_token lhs = grammar.rule_lhs(rule);
         return is_lexical(lhs) ? score_t(1.0)
                                : std::max( scr(lhs,len,true) ^ weight
                                          , scr(lhs,len,false) ^ weight )
                                ;
    }

    ////////////////////////////////////////////////////////////////////////////
    ///
    ///  result_generator: required by interface
    ///  info factories are allowed to return multiple results for a given
    ///  set of constituents.  it returns them as a generator.
    ///  a generator is a result_type functor(void) object that is convertible to
    ///  bool.  the generator converts to false when there are no more results
    ///  to retrieve.  this is analogous to an input iterator.
    ///
    ///  your generator will be called like so:
    ///  while (generator) { result_type res = generator(); }
    ///
    ///  if your create_info method only ever returns one result, you
    ///  can use single_value_generator as your result_generator
    ///
    ////////////////////////////////////////////////////////////////////////////
    typedef gusc::single_value_generator<result_type> result_generator;
    template <class Grammar, class ConstituentIterator>
    result_generator
    create_info( Grammar const& grammar
               , typename Grammar::rule_type rule
               , span_t const&
               , iterator_range<ConstituentIterator> const& range ) const
    {
        score_t score;
        unsigned short len;
        tie(score,len) = create_info_data(grammar,rule,range);

        /// note: linear domain scores; exponential combination of weights
        score_t inside = score ^ weight;
        info_type info(len);
        
        score_t heur = weighted_heur(grammar,rule,len);

        return make_tuple(info,inside,heur);
    }
    
    bool deterministic() const { return true; }

    ////////////////////////////////////////////////////////////////////////////

    template < class Grammar
             , class ConstituentIterator
             , class ScoreOutputIterator
             , class HeurOutputIterator >
    boost::tuple<ScoreOutputIterator,HeurOutputIterator>
    component_scores( Grammar const& grammar
                    , typename Grammar::rule_type rule
                    , span_t const&
                    , iterator_range<ConstituentIterator> const& range
                    , info_type const& result
                    , ScoreOutputIterator scores_out
                    , HeurOutputIterator heurs_out ) const
    {
        score_t score;
        score_t heur;
        unsigned short len;
        tie(score,len) = create_info_data(grammar,rule,range);
        *scores_out = std::make_pair(weight_id,score); ++scores_out;
        
        heur = weighted_heur(grammar,rule,len); 
        if (weight != 0) heur = heur ^ (1.0/weight);
        *heurs_out = std::make_pair(weight_id,heur); ++heurs_out;
        
        return boost::make_tuple(scores_out,heurs_out);
    }

    ////////////////////////////////////////////////////////////////////////////

private:
    score_t max_over_length(indexed_token var, bool crossed) const
    {
        score_t ret = scr(var,1,crossed) ^ weight;
        for (int len = 2; len <= max_len; ++len) {
            ret = std::max(ret, scr(var,len,crossed) ^ weight);
        }
        return ret;
    }
    
    template <class IndexedTokens>
    score_t
    rule_heuristic_( gusc::varray<bool> const& cross
                   , IndexedTokens const& tokens ) const
    {
        score_t score = as_one();

        size_t crossid = 0;
        //cerr << "heuristic: ";
        BOOST_FOREACH(indexed_token const& tok, tokens) {
            assert(crossid < cross.size());
            assert(is_native_tag(tok));
            score *= max_over_length(tok,cross[crossid]);
            //cerr << tok << '[' << boolalpha << cross[crossid] << ']' << ' ';
            ++crossid;
        }
        //cerr << "-> "<< score << endl;
        assert(crossid == cross.size());
        
        return score;
    }
    
    double weight;
    boost::uint32_t weight_id;
    size_t feature_id;
    distortion_table table;
    score_t unseen_score[2];
    indexed_token null_label;
    bool binarized;
    int max_len;

    template <class Grammar>
    void throw_cross_mismatch( Grammar const& grammar
                             , typename Grammar::rule_type rule ) const
    {
        stringstream sstr;
        sstr << "size mismatch between rule-rhs, cross-vector, "
                "and constituent range for rule: "
            << sbmt::print(rule,grammar);
        throw runtime_error(sstr.str());
    }
    
    score_t scr(indexed_token var, unsigned short len, bool crossed) const
    {
        distortion_table::const_iterator
            pos = table.find(distortion_key(len,var,crossed));
        score_t p;
        if (pos != table.end()) {
            p = pos->second;
        } else {
            p = unseen_score[crossed];
            SBMT_VERBOSE_MSG(
              gen_distortion_log
            , "variable not found in table: %s (length: %s, cross: %s)"
            , var % len % crossed
            );
        }
        return p;
    }

    template <class Grammar, class ConstituentIterator>
    tuple<score_t,unsigned short>
    create_info_data( Grammar const& grammar
                    , typename Grammar::rule_type rule
                    , iterator_range<ConstituentIterator> const& range ) const
    {
        cross_type const* cross = NULL;
        if (grammar.rule_has_property(rule, feature_id)) {
            cross = &grammar.template rule_property<cross_type>(rule, feature_id);
        }
        ConstituentIterator citr = begin(range), cend = end(range);
        unsigned short len = 0;
        score_t score = as_one();

        // this is a gotcha:  lattice component scores are stored in rules of
        // the form "A" -> "A", where "A" is a source word.  we dont want to
        // count these or we end up doubling up on length!
        //
        // if we decide to do some length normalization for lattices, the
        // feature from the lattice will be carried on these rules
        //
        // note: now in decoder, the input is __always__ a lattice.  so we can
        // use this rule to count the length as 1 here, and get rid of the 
        // incrementing based on
        if (is_lexical(grammar.rule_lhs(rule))) {
            return make_tuple(score,0);
        }

        size_t cross_idx = 0;
        for (size_t rhs_idx = 0; rhs_idx != grammar.rule_rhs_size(rule); ++rhs_idx) {
            while ((citr != cend) and is_lexical(citr->root())) ++citr;
            indexed_token rhs_token = grammar.rule_rhs(rule,rhs_idx);
            if (is_lexical(rhs_token)) {
                ++len;
            } else {
                unsigned short clen = citr->info()->length();
                len += clen;
                if (is_native_tag(rhs_token) and cross) {
                    // ndwf workaround unnecessary in rule processing tool versions
                    // supporting binarized rhs features.
                    if (cross_idx >= cross->size()) {
                        if (cross->size() == 0) // treat as empty
                            break;
                        throw_cross_mismatch(grammar,rule);
                    }

                    bool crossed = cross->at(cross_idx);
                    ++cross_idx;
                    score *= scr(rhs_token,clen,crossed);
                }
                ++citr;
            }
        }
        if (citr != cend or (cross and cross_idx != cross->size())) {
            throw_cross_mismatch(grammar,rule);
        }

        SBMT_DEBUG_MSG(
          gen_distortion_log
        , "rule: %s -> (%s,%s)"
        , sbmt::print(rule,grammar) % score % len
        );

        return make_tuple(score,len);
    }
};

////////////////////////////////////////////////////////////////////////////////

} } // namespace distortion::cross

# endif //     DISTORTION_MODEL__CROSS_DISTORTION_MODEL_HPP
