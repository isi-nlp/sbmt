# if ! defined(DISTORTION_MODEL__DISC_DISTORTION_MODEL_HPP)
# define       DISTORTION_MODEL__DISC_DISTORTION_MODEL_HPP

#include <sbmt/edge/any_info.hpp>
#include <gusc/generator/single_value_generator.hpp>
#include <sbmt/grammar/features_byid.hpp>

#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/functional/hash.hpp>
#include <boost/regex.hpp>

#include <sbmt/hash/hash_map.hpp>

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <boost/tokenizer.hpp>
#include <gusc/varray.hpp>
#include <sbmt/logging.hpp>

namespace distortion { namespace disc {

SBMT_REGISTER_CHILD_LOGGING_DOMAIN_NAME(disc_distortion_log,"distortion",sbmt::root_domain);

using namespace boost;
using namespace std;
using namespace sbmt;
namespace ext = ::stlext;

static const int MAX_SPAN = 10;

// A distortion_key represents a feature of the distortion model



struct distortion_key {
    //distortion_key(int span, indexed_token label, bool cross) :
    //  span(span), label(label), cross(cross) { }
    distortion_key(int span, bool cross) : span(span), cross(cross) { }
    int span; // 0 = any, MAX_SPAN = MAX_SPAN or more
    //indexed_token label;
    bool cross;

    static auto_ptr<distortion_key> from_feature_name(string const& s)
    {
        static const regex re("dist\\[(\\d+)\\.(\\d+)\\]");
        smatch what;
        if (regex_match(s, what, re)) {
            return auto_ptr<distortion_key>(new distortion_key(lexical_cast<int>(what[1]), lexical_cast<int>(what[2])));
        } else {
            return auto_ptr<distortion_key>();
        }
    }

    bool operator < (distortion_key const& other) const
    {
        return make_tuple(span,cross) < make_tuple(other.span,other.cross);
    }

    string feature_name() const
    {
        stringstream sstr;
        sstr << "dist[" << span << "." << cross << "]";
        return sstr.str();
    }
};

typedef gusc::sparse_vector<distortion_key, double> distortion_weight_vector;

// cross attribute on rules:
//   cross={{{ 1 0 1 }}}
// gets read into a vector<int>
// and means, x0 was crossed, x1 wasn't, x2 was
struct read_cross {
    typedef gusc::varray<bool> result_type;

    template <class Dictionary>
    result_type operator()(Dictionary& dict, std::string const& as_string) const
    {
        vector<bool> r;

        typedef boost::tokenizer<boost::char_separator<char> > tok_t;
        tok_t tok(as_string, boost::char_separator<char>(", "));
        for (tok_t::const_iterator it = tok.begin(); it != tok.end(); ++it) {
            if (*it == "10^-") continue; // workaround for ndwf run amok

            r.push_back(lexical_cast<bool>(*it));
        }
        return gusc::varray<bool>(r.begin(),r.end());
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
// write operator== operator!= and a free-function version of hash_info
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
class distortion_info_factory 
  : public sbmt::info_factory_new_component_scores<distortion_info_factory<cross_type> >
{
public:
    // required by interface
    typedef distortion_info info_type;

    // info, inside-score, heuristic
    typedef tuple<info_type,score_t,score_t> result_type;

    distortion_info_factory( gusc::sparse_vector<distortion_key,double> const& weights
                           , size_t cross_id )
    : cross_id(cross_id), feature_weights(weights) {}

    ////////////////////////////////////////////////////////////////////////////

    template <class Grammar>
    score_t
    rule_heuristic(Grammar& grammar, typename Grammar::rule_type rule) const
    {
        return as_one();
    }
    
    ////////////////////////////////////////////////////////////////////////////
    
    template <class Grammar>
    std::string
    hash_string(Grammar const& grammar, distortion_info const& di) const
    {
        return boost::lexical_cast<std::string>(di.length());
    }

    ////////////////////////////////////////////////////////////////////////////

    template <class Grammar>
    bool
    scoreable_rule(Grammar& grammar, typename Grammar::rule_type rule) const
    { return true; }

    ////////////////////////////////////////////////////////////////////////////

    struct weighted_product {
        weighted_product(distortion_weight_vector& v) : v(v), score(as_one()){}
        distortion_weight_vector const& v;
        score_t score;
        void operator()(pair<distortion_key,score_t> const& p)
        {
            score *= p.second ^ v[p.first];
        }
    };

    typedef gusc::single_value_generator<result_type> result_generator;
    template <class Grammar, class ConstituentRange>
    result_generator
    create_info( Grammar const& grammar
               , typename Grammar::rule_type rule
               , span_t const&
               , ConstituentRange const& range )
    {
        weighted_product accum(feature_weights);
        unsigned short
            len = create_info_data_bin( grammar
                                      , rule
                                      , range
                                      , accum
                                      );

        return make_tuple(distortion_info(len),accum.score,as_one());
    }

    ////////////////////////////////////////////////////////////////////////////

    template <class Accumulator>
    struct convert_feature_id {
        Accumulator scores;
        feature_dictionary& dict;
        convert_feature_id(feature_dictionary& dict, Accumulator const& scores)
        : scores(scores), dict(dict) {}

        void operator()(pair<distortion_key,score_t> const& p)
        {
            // taking advantage of the fact that ive decided to decree that
            // the output-iterator argument to component_scores is actually an
            // accumulator.
            // saves everyone from having to allocate a feature_vector just
            // to calculate some scores.
            *scores = make_pair(dict.get_index(p.first.feature_name()),p.second);
            ++scores;
        }
    };

    template <class Grammar, class ConstituentRange, class Accumulator>
    Accumulator
    component_scores_old( Grammar& grammar
                        , typename Grammar::rule_type rule
                        , span_t const&
                        , ConstituentRange const& range
                        , info_type const& result
                        , Accumulator scores )
    {
        convert_feature_id<Accumulator> accum(grammar.feature_names(),scores);
        create_info_data_bin( grammar
                            , rule
                            , range
                            , accum
                            );
        return accum.scores;
    }

    ////////////////////////////////////////////////////////////////////////////

private:
    size_t cross_id;
    gusc::sparse_vector<distortion_key,double> feature_weights;

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


    template <class Grammar, class ConstituentRange, class Accumulator>
    unsigned short
    create_info_data_bin( Grammar const& grammar
                        , typename Grammar::rule_type rule
                        , ConstituentRange const& range
                        , Accumulator& accum )
    {
        cross_type const* cross = NULL;
        if (grammar.rule_has_property(rule, cross_id)) {
            cross = &grammar.template rule_property<cross_type>(rule, cross_id);
        }
        typename boost::range_iterator<ConstituentRange const>::type
            citr = begin(range),
            cend = end(range);
        unsigned short len = 0;

        // this is a gotcha:  lattice component scores are stored in rules of
        // the form "A" -> "A", where "A" is a source word.  we dont want to
        // count these or we end up doubling up on length!
        //
        // if we decide to do some length normalization for lattices, the
        // feature from the lattice will be carried on these rules
        if (is_lexical(grammar.rule_lhs(rule))) return len;

        size_t cross_idx = 0;
        for (size_t rhs_idx = 0; rhs_idx != grammar.rule_rhs_size(rule); ++rhs_idx) {
            while ((citr != cend) and is_native_tag(citr->root())) ++citr;
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
                    score_t indicator(1,as_neglog10());
                    if (clen > MAX_SPAN) clen = MAX_SPAN;
                    accum(make_pair(distortion_key(clen,crossed), indicator));
                    accum(make_pair(distortion_key(0,crossed), indicator));
                }
            }
        }

        SBMT_DEBUG_MSG(
          disc_distortion_log
        , "rule: %s -> (%s)"
        , sbmt::print(rule,grammar) % len
        );

        return len;
    }
};

////////////////////////////////////////////////////////////////////////////////
} }// namespace distortion::disc

# endif //     DISTORTION_MODEL__DISC_DISTORTION_MODEL_HPP

