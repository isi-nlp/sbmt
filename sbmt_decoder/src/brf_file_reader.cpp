#include <sbmt/grammar/brf_file_reader.hpp>
#include <boost/regex.hpp>
#include <graehl/shared/string_to.hpp>
#include <xrsparse/xrs.hpp>
#include <set>
#include <iostream>
#include <iterator>
#include <string>
#include <RuleReader/Rule.h>
#include <sbmt/logmath.hpp>
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
# include <tbb/task_scheduler_init.h>
# include <tbb/pipeline.h>

using namespace std;
using namespace ns_RuleReader;

namespace sbmt {

namespace detail {

////////////////////////////////////////////////////////////////////////////////

namespace {
const int excluded_properties_size = 9;

string excluded_properties[excluded_properties_size] = {
                    string("sblm")
                  , string("sblm_string")
                  , string("lm")
                  , string("lm_string")
                  , string("rule_file_line_number")
                  , string("id")
                  , string("rhs") 
                  , string("virtual_label")
                  , string("complete_subtree")
              };

set<string> excluded_property_list = 
                set<string>( excluded_properties
                           , excluded_properties + excluded_properties_size );
} // unnamed namespace

inline bool nonexcluded_property(std::string const& name) 
{
    return excluded_property_list.find(name) == excluded_property_list.end();
}


////////////////////////////////////////////////////////////////////////////////

namespace {
  template <class I,class To>
  bool try_stream_into(I & i,To &to,bool complete=true)
  {
    i >> to;
    if (i.fail()) return false;
    if (complete) {
      char c;
      return !(i >> c);
    }
    return true;
  }

  template <class To>
  bool try_string_into(std::string const& str,To &to,bool complete=true)
  {
    std::istringstream i(str);
    return try_stream_into(i,to,complete);
  }
}
template <class ItrT, class SO, class TO>
void extract_features( ItrT i
                     , ItrT end
                     , SO& out_scores
                     , TO& out_texts
                     , in_memory_token_storage& dict
                     , bool skiptext=false
                     , bool verbose=false )
{
    for (; i != end; ++i) {
        std::string const& name=i->first;
        std::string const& val=i->second.value;
        if (nonexcluded_property(name)) {
            score_t s;
            if ((i->second.bracketed == false) and try_string_into(val,s)) {
                out_scores.insert(std::make_pair(dict.get_index(name),s));
            } else {
                if (skiptext) {
                    if (verbose)
                        std::cerr << "brf rule contained unexpected non-numeric attribute "
                                  << name << "=" << val <<" - "
                                  << "attribute stripped from archive.\n";
                } else {
                    out_texts.insert(std::make_pair(dict.get_index(name),val));
                }
            }
        }
    }
}

template <class ItrT, class SO, class TO>
void extract_features2(ItrT i, ItrT end, SO out_scores, TO out_texts,bool skiptext=false,bool verbose=false)
{
    for (; i != end; ++i) {
        std::string const& name=i->first;
        std::string const& val=i->second.value;
        if (nonexcluded_property(name)) {
            score_t s;
            if ((not i->second.bracketed) and try_string_into(val,s)) {
                *out_scores++=std::make_pair(name,s);
            } else {
                if (skiptext) {
                    if (verbose)
                        std::cerr << "brf rule contained unexpected non-numeric attribute "
                                  << name << "=" << val <<" - "
                                  << "attribute stripped from archive.\n";
                } else
                    *out_texts++=std::make_pair(name,val);
            }
        }
    }
}

template <class OutItrT>
void extract_syntax_ids(ns_RuleReader::Rule& r, OutItrT out)
{
   boost::char_delimiters_separator<char> sep(false,""," \t");
   std::string s = r.getAttributeValue("id");
   boost::tokenizer<> toker(s,sep);
   boost::tokenizer<>::iterator itr = toker.begin();
   boost::tokenizer<>::iterator end = toker.end();
   for (; itr != end; ++itr) {
       out = boost::lexical_cast<syntax_id_type>(*itr);
       ++out;
   }
}


////////////////////////////////////////////////////////////////////////////////


template <class TF>
syntax_rule<typename TF::token_t>
create_syntax_rule(ns_RuleReader::Rule& r, TF & tf)
{
    assert(!r.is_virtual_label());
    std::stringstream rule_str;
    rule_str << r.getLHSRoot()->treeString() << " -> "
             << r.getAttributeValue("rhs") << " ### " // magical: we had binary rhs on input, but original (nonbinar.) rhs was saved in a feature.  so the type of rule we get as input is not the type we provide to constructor below ...
             << "id=" << r.getAttributeValue("id");

    syntax_rule<typename TF::token_t> retval(rule_str.str(), tf);
    //std::cerr << "created "<< print(retval,tf) << std::endl;
    return retval;
}

} // namespace sbmt::detail


////////////////////////////////////////////////////////////////////////////////

static boost::regex blank("^(\\w)*$") ;

template <class TF, class SynCB, class BinCB>
void read_brf( string line
             , std::istream& in
             , TF& tf
             , in_memory_token_storage& dict
             , SynCB& syncb
             , BinCB& bincb
             , bool verbose )
{
    
    typedef typename TF::token_t token_t;
    do {
        if (boost::regex_match(line,blank)) continue;
        Rule r(line);

        score_map_type score_map;
        texts_map_type texts_map;
        
        // \todo configure (most likely configure on rule_action level) 
        //       instead - throw away unwanted features?
        bool skip_text_features=false; 
        detail::extract_features( r.getAttributes()->begin()
                                , r.getAttributes()->end()
                                , score_map
                                , texts_map
                                , dict
                                , skip_text_features
                                , verbose );
        // implies syntax rule (future sblm binarization w/
        // complete subtrees: may not follow?)
        if (!r.is_virtual_label()) { 
            syntax_rule<token_t> syn = detail::create_syntax_rule(r,tf);
            syncb(syn,texts_map);
        }
        rule_topology<token_t> binrule(r,tf);
        texts_map_type properties;
        BOOST_FOREACH(Rule::attribute_map::value_type& p, *r.getAttributes())
        {
            properties.insert(make_pair(dict.get_index(p.first),p.second.value));
        }
        
        vector<syntax_id_type> rule_list;
        detail::extract_syntax_ids(r, std::inserter(rule_list, rule_list.end()));
        bincb(binary_rule<token_t>(binrule),score_map,properties,rule_list);
    } while (getline(in,line));
}

template <class TF>
typename TF::token_t
tok(brf_data::var const& v, TF& tf)
{
    if (v.nt) {
        if (v.label[0] == 'V' and (v.label[1] == '[' or v.label[1] == '<')) {
            return tf.virtual_tag(v.label);
        } else if (v.label == "TOP") {
            return tf.toplevel_tag();
        } else {
            return tf.tag(v.label);
        }
    } else {
        return tf.foreign_word(v.label);
    }
}

template <class Range, class TF>
rule_topology<typename TF::token_t>
parse_brf_rule( Range const& rng
              , TF& tf
              , in_memory_token_storage& dict
              , texts_map_type& tm
              , score_map_type& sm )
{
    brf_data rd = parse_brf(rng);
    assert(rd.lhs.nt);
    //std::cerr << "brf: "<< rd << std::endl;
    BOOST_FOREACH(feature const& f, rd.features)
    {
        if (f.key != "id") {
            if (f.number) {
                sm.insert(std::make_pair(dict.get_index(f.key),score_t(f.num_value,as_neglog10())));
            } else {
                tm.insert(std::make_pair(dict.get_index(f.key),f.str_value));
            }
        }
    }
    
    assert(not is_foreign(tok(rd.lhs,tf)));
    if (rd.rhs_size == 1) {
        return rule_topology<typename TF::token_t>(tok(rd.lhs,tf),tok(rd.rhs[0],tf));
    } else {
        return rule_topology<typename TF::token_t>(tok(rd.lhs,tf),tok(rd.rhs[0],tf),tok(rd.rhs[1],tf));
    }
    
}

template <class TF>
struct workspace {
    typedef typename TF::token_t token_type;
    std::string line;
    bool is_xrs;
    rule_data xrs_parse;
    brf_data brf_parse;
    syntax_rule<token_type> xrs_rule;
    binary_rule<token_type> brf_rule;
    texts_map_type tm;
    score_map_type sm;
    std::vector<syntax_id_type> id;
};

template <class TF>
struct file_input : tbb::filter {
    std::istream& infile;
    typedef std::vector< workspace<TF> > wsvector;
    typename wsvector::iterator current;
    typename wsvector::iterator end;

    void* operator()(void*)
    {
        if (current == end) return 0;
        bool good = getline(infile,current->line);
        if (not good) return 0;
        return &(*(current++));
    }
    
    void reset(wsvector& ws)
    {
        current = ws.begin();
        end = ws.end();
    }
    
    file_input(std::istream& in, wsvector& ws) 
      : tbb::filter(tbb::filter::serial)
      , infile(in)
      , current(ws.begin())
      , end(ws.end()) {}
};

template <class TF>
struct vec_input : tbb::filter {
    typedef std::vector< workspace<TF> > wsvector;
    typename wsvector::iterator current;
    typename wsvector::iterator end;

    void* operator()(void*)
    {
        if (current == end) return 0;
        else return &(*(current++));
    }
    
    void reset(wsvector& ws)
    {
        current = ws.begin();
        end = ws.end();
    }
    
    vec_input(wsvector& ws) 
      : tbb::filter(tbb::filter::serial)
      , current(ws.begin())
      , end(ws.end()) {}
};

template <class TF>
struct parse_line : tbb::filter {
    void* operator()(void* ptr)
    {
        workspace<TF>& ws = *static_cast<workspace<TF>*>(ptr);
        if (ws.line[0] == 'X'  and ws.line[1] == ':') {
            ws.is_xrs = true;
            ws.xrs_parse = parse_xrs( 
                             boost::make_iterator_range( ws.line.begin() + 2
                                                       , ws.line.end()
                                                       )
                           ); 
        } else {              
            ws.is_xrs = false;
            ws.brf_parse = parse_brf(
                             boost::make_iterator_range( ws.line.begin() + 2
                                                       , ws.line.end()
                                                       )
                           );
        }
        return ptr;
    }
    parse_line() : tbb::filter(tbb::filter::parallel) {}
};

template <class TF>
struct index_line : tbb::filter {
    typedef typename TF::token_t token_type;
    TF& tf;
    in_memory_token_storage& dict;
    
    void* operator()(void* ptr)
    {
        workspace<TF>& ws = *static_cast<workspace<TF>*>(ptr);
        
        if (ws.is_xrs) {
            BOOST_FOREACH(lhs_node const& lhs, ws.xrs_parse.lhs) {
                if (lhs.indexed or lhs.children) {
                    tf.tag(lhs.label);
                } else {
                    tf.native_word(lhs.label);
                }
            }
            BOOST_FOREACH(rhs_node const& rhs, ws.xrs_parse.rhs) {
                if (not rhs.indexed) {
                    tf.foreign_word(rhs.label);
                }
            }
        } else {
            brf_data& rd = ws.brf_parse;
            BOOST_FOREACH(feature const& f, rd.features)
            {
                if (f.key != "id") {
                    dict.get_index(f.key);
                }
            }
            tok(rd.lhs,tf);
            BOOST_FOREACH(brf_data::var const& rhs, rd.rhs) {
                tok(rhs,tf);
            }
        }
        return 0;
    }
    
    index_line(TF& tf, in_memory_token_storage& dict) 
    : tbb::filter(tbb::filter::serial)
    , tf(tf)
    , dict(dict) {}
};

template <class TF, class SynCB, class BinCB>
struct make_rule : tbb::filter {
    typedef typename TF::token_t token_type;
    void* operator()(void* ptr) 
    {
        workspace<TF>& ws = *static_cast<workspace<TF>*>(ptr);
        texts_map_type& tm = ws.tm;
        score_map_type& sm = ws.sm;
        std::vector<syntax_id_type>& id = ws.id;
        
        if (ws.is_xrs) {
            tm.clear();
            ws.xrs_rule = syntax_rule<token_type>(ws.xrs_parse,tf);
            syncb(ws.xrs_rule,ws.tm);
        } else {
            tm.clear();
            sm.clear();
            id.clear();
            
            brf_data& rd = ws.brf_parse;

            BOOST_FOREACH(feature const& f, rd.features)
            {
                if (f.key != "id") {
                    if (f.number) {
                        sm.insert(std::make_pair(dict.get_index(f.key),score_t(f.num_value,as_neglog10())));
                    } else {
                        tm.insert(std::make_pair(dict.get_index(f.key),f.str_value));
                    }
                } else {
                    id.push_back(f.num_value);
                }
            }

            if (ws.brf_parse.rhs_size == 1) {
                ws.brf_rule = binary_rule<token_type>(rule_topology<token_type>(tok(rd.lhs,tf),tok(rd.rhs[0],tf)));
            } else {
                ws.brf_rule = binary_rule<token_type>(rule_topology<token_type>(tok(rd.lhs,tf),tok(rd.rhs[0],tf),tok(rd.rhs[1],tf)));
            }
            bincb(ws.brf_rule,ws.sm,ws.tm,ws.id);
        }
        return ptr;
    }
    make_rule(TF const& tf, in_memory_token_storage const& dict, SynCB& syncb, BinCB& bincb) 
      : tbb::filter(tbb::filter::serial)
      , tf(tf)
      , dict(dict)
      , syncb(syncb)
      , bincb(bincb) {}

    TF const& tf;
    in_memory_token_storage const& dict;
    SynCB& syncb;
    BinCB& bincb;
};

template <class TF>
struct null_sink : tbb::filter {
    void* operator()(void* ptr) 
    { 
        workspace<TF>& ws = *static_cast<workspace<TF>*>(ptr);
        //if (ws.is_xrs) std::cerr << ws.xrs_parse << '\n';
        return 0; 
    }
    null_sink() : tbb::filter(tbb::filter::serial) {}
};

template <class TF, class SynCB, class BinCB>
void read_brf2_mt( std::istream& input
                 , TF& tf
                 , in_memory_token_storage& dict
                 , SynCB& syncb
                 , BinCB& bincb
                 , bool verbose )
{
    tbb::task_scheduler_init init;
    tbb::pipeline pipe1, pipe2;
    std::vector< workspace<TF> > ws(50000);
    
    file_input<TF> in1(input,ws);
    parse_line<TF> p1;
    index_line<TF> out1(tf,dict);
    
    vec_input<TF> in2(ws);
    make_rule<TF,SynCB,BinCB> p2(tf,dict,syncb,bincb);
    null_sink<TF> out2;
    
    pipe1.add_filter(in1);
    pipe1.add_filter(p1);
    pipe1.add_filter(out1);
    
    pipe2.add_filter(in2);
    pipe2.add_filter(p2);
    pipe2.add_filter(out2);

    int stage = 0;
    while(input) {
        //std::cerr << "start stage " << stage << '\n';
        in1.reset(ws);
        in2.reset(ws);
        pipe1.run(100);
        //std::cerr << "end stage " << stage++ << '\n';
        pipe2.run(100);
    }
}

template <class TF, class SynCB, class BinCB>
void read_brf2( std::istream& input
              , TF& tf
              , in_memory_token_storage& dict
              , SynCB& syncb
              , BinCB& bincb
              , bool verbose )
{
    std::vector<syntax_id_type> synid(1);
    std::vector<syntax_id_type> emptysynid;
    std::string line;
    typedef typename TF::token_t token_type;
    while (getline(input,line)) {
        //std::cerr << "proc: " << line << std::endl;
        if (line[0] == 'X' and line[1] == ':') {
            rule_data rd = parse_xrs(
                             boost::make_iterator_range( line.begin() + 3
                                                       , line.end()
                                                       )
                          );
            //std::cerr << "parsed: " << rd << std::endl;
            syntax_rule<token_type> rule(rd,tf);
            //std::cerr << "interpreted: " << token_label(tf) << rule << std::endl;
            texts_map_type tm;
            score_map_type sm;
            BOOST_FOREACH(feature const& f,rd.features){
                if (f.number) {
                    sm.insert(std::make_pair(dict.get_index(f.key),score_t(f.num_value,as_neglog10())));
                } else {
                    tm.insert(std::make_pair(dict.get_index(f.key),f.str_value));
                }
            }
            synid[0] = rd.id;
            syncb(rule,tm);
            getline(input,line);
            //std::cerr << "proc: " << line << std::endl;
            binary_rule<token_type> 
                brule( parse_brf_rule(
                         std::make_pair(line.begin() + 2, line.end())
                       , tf
                       , dict
                       , tm
                       , sm
                       )
                     )
                     ;
            bincb(brule,sm,tm,synid);
        } else {
            texts_map_type tm;
            score_map_type sm;
            binary_rule<token_type> 
                brule( parse_brf_rule(
                         std::make_pair(line.begin() + 2, line.end())
                       , tf
                       , dict
                       , tm
                       , sm
                       )
                     )
                     ;
            bincb(brule,sm,tm,emptysynid);
        }
    }
}

template <class TF>
void brf_stream_reader_tmpl<TF>::read()
{
    typedef brf_reader_tmpl<TF> base;
    std::string line;
    getline(input,line);
    if (line == "BRF version 2") {
        read_brf2(input, base::tf(), base::dict(), base::syntax_rule_cb, base::binary_rule_cb, verbose);
    } else {
        read_brf(line, input, base::tf(), base::dict(), base::syntax_rule_cb, base::binary_rule_cb, verbose);
    }
}

template class brf_stream_reader_tmpl<>;
template class brf_stream_reader_tmpl<fat_token_factory>;

////////////////////////////////////////////////////////////////////////////////
//
// NP-BAR(x0:NP x1:, x2:NP x3:,) -> x0 x1 x2 x3 x4 x5 x6 x7 x8 ### id=74436157
// NP-BAR(x0:NP x1:, x2:NP x3:, NP(x4:NPB) x5:, NP(x6:NPB) x7:, x8:NP ,(",")) -> x0 x1 x2 x3 x4 x5 x6 x7 x8 ### id=74436157


} // namespace sbmt
