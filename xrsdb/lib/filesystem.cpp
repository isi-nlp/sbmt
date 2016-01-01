# include <boost/archive/binary_oarchive.hpp>
# include <boost/archive/binary_iarchive.hpp>
# include <boost/archive/xml_oarchive.hpp>
# include <boost/archive/xml_iarchive.hpp>
# include <boost/iostreams/filtering_stream.hpp>
# include <boost/iostreams/device/file_descriptor.hpp>
# include <boost/iostreams/filter/gzip.hpp>
# include <boost/filesystem/path.hpp>
# include <boost/filesystem/convenience.hpp>
# include <boost/filesystem/fstream.hpp>
# include <boost/filesystem/operations.hpp>
# include <boost/foreach.hpp>
# include <boost/format.hpp>
# include <boost/serialization/map.hpp>
# include <boost/lexical_cast.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/cstdint.hpp>
# include <word_cluster.hpp>
# include <boost/iostreams/device/file_descriptor.hpp>
# include <boost/iostreams/copy.hpp>
# include <boost/iostreams/filter/zlib.hpp>
# include <boost/iostreams/device/back_inserter.hpp>

# include <gusc/trie/traverse_trie.hpp>

# include <iostream>
# include <sstream>
# include <string>

# include <filesystem.hpp>
# include <collins/lm.hpp>

using namespace boost::filesystem;
using namespace boost;
using namespace boost::archive;
using namespace sbmt;

namespace bio = boost::iostreams;
namespace fs = boost::filesystem;

using std::string;
using std::stringstream;
using std::map;
using std::make_pair;
using std::clog;
using std::endl;

namespace xrsdb {
    
void make_head_map(head_map& hmap, std::string const& str, sbmt::token_type_id type, sbmt::in_memory_dictionary const& dict)
{
    std::map< sbmt::indexed_token,boost::tuple<boost::uint32_t,boost::uint32_t> > mp;
    std::stringstream sstr(str);
    std::string nm;
    boost::uint32_t c, n;
    while (sstr >> nm >> c >> n) {
        mp.insert(std::make_pair(dict.find_token(nm,type),boost::make_tuple(c,n)));
    }
    hmap.reserve(mp.size());
    //    hmap.insert(boost::interprocess::ordered_unique_range,mp.begin(),mp.end());
    hmap.insert(mp.begin(),mp.end());
}

template <class Alloc>
void make_variable_head_map( variable_head_map& vhmap
                           , std::string const& str
                           , sbmt::token_type_id type
                           , fixed_rule const& rule
                           , sbmt::in_memory_dictionary const& dict
                           , Alloc const& a
                           )
{
    size_t numvars = 0;
    BOOST_FOREACH(fixed_rule::rule_node const& rnd, rule.rhs()) {
        if (rnd.indexed()) ++numvars;
    }
    variable_head_map(numvars,variable_head_map_member(gusc::less(),variable_head_map_member::allocator_type(a)),variable_head_map::allocator_type(a)).swap(vhmap);
    std::string nm;
    boost::uint32_t n;
    std::map<sbmt::indexed_token,boost::uint32_t> mp;
    int x = 0;
    std::stringstream sstr(str);
    while (sstr >> nm) {
        if (nm == "|||") {
            vhmap[x].reserve(mp.size());
            //vhmap[x].insert(boost::interprocess::ordered_unique_range,mp.begin(),mp.end());
            vhmap[x].insert(mp.begin(),mp.end());
            mp.clear();
            ++x;
        } else {
            sstr >> n;
            mp.insert(std::make_pair(dict.find_token(nm,type),n));
        }
    }
    vhmap[x].reserve(mp.size());
    //vhmap[x].insert(boost::interprocess::ordered_unique_range,mp.begin(),mp.end());
    vhmap[x].insert(mp.begin(),mp.end());
}

////////////////////////////////////////////////////////////////////////////////
std::vector<sbmt::indexed_lm_token> 
make_leaflm_string( rhs2var_map const& mp
                  , sbmt::indexed_token_factory const& dict
                  , std::string const& s )
{
    std::vector<sbmt::indexed_lm_token> ret;
    std::stringstream sstr(s);
    std::string tok;
    while (sstr >> tok) {
        if (tok[0] == '#' and tok.size() >= 2) {
            ret.push_back(
              sbmt::indexed_lm_token(
                mp.find(
                  boost::lexical_cast<int>(
                    tok.substr(1)
                  )
                )->second
              )
            );
        } else {
            ret.push_back(sbmt::indexed_lm_token(dict.native_word(tok)));
        }
    }
    return ret;
}


fixed_rule::lhs_preorder_iterator 
headword(std::string const& hwpos, sbmt::in_memory_dictionary& dict, fixed_rule const& rule)
{
    fixed_rule::lhs_preorder_iterator pos = rule.lhs_begin(), ret = rule.lhs_begin(), end = rule.lhs_end();
    if (hwpos != "" and hwpos[0] == '"') {
        sbmt::indexed_token wd = dict.native_word(hwpos.substr(1,hwpos.size() - 2));
        for (; pos != end; ++pos) if (pos->get_token() == wd) {
            ret = pos;
            break;
        }
        
    } else if (hwpos != "") {
        int n = boost::lexical_cast<int>(hwpos);
        int x = 0;
        for (; pos != end; ++pos) if (pos->indexed()) {
            if (n == x) {
                ret = pos;
                break;
            }
            else ++x;
        }
    } else {
        for (; pos != end; ++pos) if (pos->is_leaf()) ret = pos;
        pos = ret;
    }
    if (pos == end) {
        std::stringstream sstr;
        sstr << sbmt::token_label(dict) << "bad hwpos: " << hwpos << "\t" << rule << '\n';
        throw std::logic_error(sstr.str());
    }
    return pos;
}

        
weak_syntax_iterator::weak_syntax_iterator( fixed_rule::lhs_preorder_iterator itr
                                          , fixed_rule::lhs_preorder_iterator end
                                          , token_map const* inc
                                          , rule_application const* rule )
: itr(itr)
, end(end)
, inc(inc)
, rule(rule) 
{
    advance();
}

fixed_rule::lhs_preorder_iterator weak_syntax_iterator::endpt(fixed_rule::tree_node const& nd)
{
    fixed_rule::lhs_children_iterator itr, end;
    boost::tie(itr,end) = nd.children();
    assert(itr != end);
    fixed_rule::lhs_preorder_iterator after;
    for (; itr != end;) {
        after = itr.current();
        ++itr;
    }
    if (after->is_leaf()) return after + 1;
    else return endpt(*after);
}
    
weak_syntax_iterator::result_type weak_syntax_iterator::dereference() const
{
    if (not endpts.empty() and itr == endpts.back().get<1>()) {
        return result_type(inc->find(boost::make_tuple(endpts.back().get<0>(),false))->second);
    } else if (not itr->is_leaf()) {
        return result_type(inc->find(boost::make_tuple(itr->get_token(),true))->second);
    } else if (itr->lexical()){
        return result_type(itr->get_token());
    } else {
        return result_type(rule->rhs2var.find(itr->index())->second);
    }
}
    
void weak_syntax_iterator::increment()
{
    if (not endpts.empty() and itr == endpts.back().get<1>()) {
        endpts.pop_back();
    } else {
        ++itr;
    }
    advance();
}
    
bool weak_syntax_iterator::equal(weak_syntax_iterator const& other) const
{
    if (endpts.empty() != other.endpts.empty()) return false;
    if (not endpts.empty() and (other.endpts.back() != endpts.back())) return false;
    return itr == other.itr;
}

void weak_syntax_iterator::advance()
{
    while (itr != end) {
        if (itr->is_leaf()) break;
        else if (not endpts.empty() and itr == endpts.back().get<1>()) break;
        else if (inc->find(boost::make_tuple(itr->get_token(),true)) != inc->end()) {
            endpts.push_back(boost::make_tuple(itr->get_token(),endpt(*itr)));
            break;
        }
        ++itr;
    }
}

void rule_application::swap(rule_application& other)
{
    rule.swap(other.rule);
    std::swap(hwd,other.hwd);
    costs.swap(other.costs);
    std::swap(cost,other.cost);
    std::swap(heur,other.heur);
    rhs2var.swap(other.rhs2var);
    cross.swap(other.cross);
    std::swap(rldist,other.rldist);
    vldist.swap(other.vldist);
    sbmt::swap(lmstring,other.lmstring);
    #ifdef XRSDB_LEAFLM
    sbmt::swap(leaflmstring,other.leaflmstring);
    #endif
    sbmt::swap(froot,other.froot);
    fvars.swap(other.fvars);
    #ifdef XRSDB_SENTIDS
    sentids.swap(other.sentids);
    #endif
    tgt_src_aligns.swap(other.tgt_src_aligns);
    #ifdef XRSDB_HEADRULE
    hwdm.swap(other.hwdm);
    htgm.swap(other.htgm);
    vhwdm.swap(other.vhwdm);
    vhtgm.swap(other.vhtgm);
    #endif
    headmarker.swap(other.headmarker);
}

template <class A, class W>
rule_application::rule_application(sbmt::weight_vector const& fv, header& h, A& alloc, W const& w)
: rule(alloc)
, costs(fv.begin(),fv.end(),gusc::less(),fixed_feature_vector::allocator_type(alloc))
, cost(dot(costs,w))
, rhs2var( gusc::less()
         , rhs2var_map::allocator_type(alloc)
         )
, cross(fixed_bool_varray::allocator_type(alloc))
, rldist(boost::math::normal_distribution<float>(-1.0,1.0))
, vldist(fixed_rldist_varray::allocator_type(alloc))
, lmstring("1",h.dict,fixed_lm_string::allocator_type(alloc))
#ifdef XRSDB_LEAFLM
, leaflmstring("1",h.dict,fixed_lm_string::allocator_type(alloc))
#endif
, froot(h.dict.toplevel_tag())
, fvars(fixed_token_varray::allocator_type(alloc))
#ifdef XRSDB_SENTIDS
, sentids(sentid_varray::allocator_type(alloc))
#endif
, tgt_src_aligns(target_source_align_varray::allocator_type(alloc))
#ifdef XRSDB_HEADRULE
, hwdm(gusc::less(),head_map::allocator_type(alloc))
, htgm(gusc::less(),head_map::allocator_type(alloc))
, vhwdm(variable_head_map::allocator_type(alloc))
, vhtgm(variable_head_map::allocator_type(alloc))
#endif
, headmarker(fixed_byte_varray::allocator_type(alloc))
{}

template <class A, class W>
rule_application::rule_application(rule_data const& rd, header& h, A& alloc, W const& w)
: rule(rd,h.dict,alloc)
, costs( boost::make_transform_iterator(
           boost::make_filter_iterator(
             isnum()
           , rd.features.begin()
           , rd.features.end()
           )
         , tform(&h.fdict)
         )
       , boost::make_transform_iterator(
           boost::make_filter_iterator(
             isnum()
           , rd.features.end()
           , rd.features.end()
           )
         , 
         tform(&h.fdict)
         )
       , gusc::less()
       , fixed_feature_vector::allocator_type(alloc) 
       )
, cost(dot(costs,w))
, rhs2var( boost::make_transform_iterator(boost::make_filter_iterator(isvar(),rule.rhs_begin(),rule.rhs_end()),mapitem(&rule))
         , boost::make_transform_iterator(boost::make_filter_iterator(isvar(),rule.rhs_end(),rule.rhs_end()),mapitem(&rule))
         , gusc::less()
         , rhs2var_map::allocator_type(alloc)
         )
, cross(fixed_bool_varray::allocator_type(alloc))
, rldist(boost::math::normal_distribution<float>(-1.0,1.0))
, vldist(fixed_rldist_varray::allocator_type(alloc))
, lmstring("1",h.dict,fixed_lm_string::allocator_type(alloc))
#ifdef XRSDB_LEAFLM
, leaflmstring("1",h.dict,fixed_lm_string::allocator_type(alloc))
#endif
, froot(h.dict.toplevel_tag())
, fvars(fixed_token_varray::allocator_type(alloc))
#ifdef XRSDB_SENTIDS
, sentids(sentid_varray::allocator_type(alloc))
#endif
, tgt_src_aligns(target_source_align_varray::allocator_type(alloc))
#ifdef XRSDB_HEADRULE
, hwdm(gusc::less(),head_map::allocator_type(alloc))
, htgm(gusc::less(),head_map::allocator_type(alloc))
, vhwdm(variable_head_map::allocator_type(alloc))
, vhtgm(variable_head_map::allocator_type(alloc))
#endif
, headmarker(fixed_byte_varray::allocator_type(alloc))
{
    int ptr = get_feature(rd,"cross");
    if (ptr >= 0 and ptr < int(rd.features.size())) {
        fixed_bool_varray( read_cross()(*this,rd.features[ptr].str_value)
                         , fixed_bool_varray::allocator_type(alloc)
                         ).swap(cross);
    }
    ptr = get_feature(rd,"rldist");
    if (ptr >= 0 and ptr < int(rd.features.size())) {
        rldist = rule_length::make_rule_moments()(*this,rd.features[ptr].str_value);
    }
    ptr = get_feature(rd,"rldistpdf");
    if (ptr >= 0 and ptr < int(rd.features.size())) {
        rldist = rule_length::make_rule_params()(*this,rd.features[ptr].str_value);
    }
    ptr = get_feature(rd,"vldist");
    if (ptr >= 0 and ptr < int(rd.features.size())) {
        fixed_rldist_varray( rule_length::make_rule_var_moments()(*this,rd.features[ptr].str_value)
                           , fixed_rldist_varray::allocator_type(alloc)
                           ).swap(vldist);
    }
    ptr = get_feature(rd,"vldistpdf");
    if (ptr >= 0 and ptr < int(rd.features.size())) {
        fixed_rldist_varray( rule_length::make_rule_var_params()(*this,rd.features[ptr].str_value)
                           , fixed_rldist_varray::allocator_type(alloc)
                           ).swap(vldist);
    }
    ptr = get_feature(rd,"lm_string");
    if (ptr >= 0 and ptr < int(rd.features.size())) {
        fixed_lm_string(rd.features[ptr].str_value,h.dict,fixed_lm_string::allocator_type(alloc)).swap(lmstring);
    }
#ifdef XRSDB_LEAFLM
    ptr = get_feature(rd,"leaflm_string");
    if (ptr >= 0 and ptr < int(rd.features.size())) {
        std::vector<sbmt::indexed_lm_token> lms = make_leaflm_string(rhs2var,h.dict,rd.features[ptr].str_value);
        fixed_lm_string(lms.begin(),lms.end(),fixed_lm_string::allocator_type(alloc)).swap(leaflmstring);
    }
#endif
    ptr = get_feature(rd,"fvars");
    if (ptr >= 0 and ptr < int(rd.features.size())) {
        fixed_token_varray( ssyn::read_fvars()(h.dict,rd.features[ptr].str_value)
                          , fixed_token_varray::allocator_type(alloc)
                          ).swap(fvars);
    }
    ptr = get_feature(rd,"froot");
    if (ptr >= 0 and ptr < int(rd.features.size())) {
        froot = ssyn::read_froot()(h.dict,rd.features[ptr].str_value);
    }
#ifdef XRSDB_SENTIDS
    ptr = get_feature(rd,"sentids");
    if (ptr >= 0 and ptr < int(rd.features.size())) {
        std::stringstream sstr(rd.features[ptr].str_value);
        std::vector<boost::uint32_t> vpv;
            vpv.insert( vpv.end()
                      , std::istream_iterator<boost::uint32_t>(sstr)
                      , std::istream_iterator<boost::uint32_t>()
                      );

	sentid_varray(vpv.begin(),vpv.begin() + std::min(10UL,vpv.size()),sentid_varray::allocator_type(alloc)).swap(sentids);
    }
#endif
    ptr = get_feature(rd,"hwpos");
    if (ptr >= 0 and ptr < int(rd.features.size())) {
        hwd = headword(rd.features[ptr].str_value,h.dict,rule);
    } else {
        hwd = headword("",h.dict,rule);
    }
    
#ifdef XRSDB_HEADRULE    
    ptr = get_feature(rd,"rwprob");
    if (ptr >= 0 and ptr < int(rd.features.size())) {
        make_head_map(hwdm,rd.features[ptr].str_value,sbmt::native_token,h.dict);
    }
    ptr = get_feature(rd,"rtprob");
    if (ptr >= 0 and ptr < int(rd.features.size())) {
        make_head_map(htgm,rd.features[ptr].str_value,sbmt::tag_token,h.dict);
    }
    ptr = get_feature(rd,"vhwdist");
    if (ptr >= 0 and ptr < int(rd.features.size())) {
        make_variable_head_map(vhwdm,rd.features[ptr].str_value,sbmt::native_token,rule,h.dict,alloc);
    }
    ptr = get_feature(rd,"vhtdist");
    if (ptr >= 0 and ptr < int(rd.features.size())) {
        make_variable_head_map(vhtgm,rd.features[ptr].str_value,sbmt::tag_token,rule,h.dict,alloc);
    }
#endif
    ptr = get_feature(rd,"align");
    std::vector< boost::tuple<boost::uint8_t,boost::uint8_t> > tsalgns;
    sbmt::alignment aligns;
    if (ptr >= 0 and ptr < int(rd.features.size())) {
        aligns = sbmt::alignment(rd.features[ptr].str_value);
    } else {
        aligns = rule.default_alignment();
    }
    for (size_t i=0 , e=aligns.n_src(); i != e; ++i) {
        for (sbmt::alignment::tars::const_iterator s = aligns.sa[i].begin(), se = aligns.sa[i].end(); s!=se ; ++s) {
            tsalgns.push_back(boost::make_tuple(*s,i));
        }
    }
    std::sort(tsalgns.begin(),tsalgns.end());
    target_source_align_varray( tsalgns
                              , target_source_align_varray::allocator_type(alloc)
                              ).swap(tgt_src_aligns);
    ;
    ptr = get_feature(rd,"headmarker");
    std::string hstr = "R";
    if (ptr >= 0 and ptr < int(rd.features.size())) hstr = rd.features[ptr].str_value;
    std::vector<int> hva = collins::hpos_array(rule,hstr);
    fixed_byte_varray(hva.begin(),hva.end(),fixed_byte_varray::allocator_type(alloc)).swap(headmarker);
}

void rule_application::print(std::ostream& out, header& h) const
{
    out << rule;
    uint32_t k; float v;
    BOOST_FOREACH(boost::tie(k,v), costs) {
        out << ' ' << h.fdict.get_token(k) << '=' << v;
    }
    
    out << " cross={{{";
    BOOST_FOREACH(bool b, cross) out << ' ' << b;
    out << " }}}";
    
    out << " rldistpdf={{{ " << '('<< rldist.mean() << ',' << rldist.scale() << ')' << " }}}";
    
    out << " vldistpdf={{{";
    BOOST_FOREACH(rule_length::distribution_t r, vldist) {
        out << ' ' << '(' << r.mean() << ',' << r.scale() << ')';
    }
    out << " }}}";
    
    if (lmstring.size() != 1 or (not lmstring[0].is_index()) or lmstring[0].get_index() != 1) {
        out << " lm_string=";
        sbmt::print(out,lmstring,h.dict);
    }
    #ifdef XRSDB_LEAFLM
    if (leaflmstring.size() != 1 or (not leaflmstring[0].is_index()) or leaflmstring[0].get_index() != 1) {
        out << " leaflm_string={{{";
        BOOST_FOREACH(sbmt::indexed_lm_token tok, leaflmstring) {
            out << ' ';
            if (tok.is_index()) out << "#" << tok.get_index();
            else out << h.dict.label(tok.get_token());
        }
        out << " }}}";
    }
    #endif
    if (froot != h.dict.toplevel_tag()) out << " froot=" << h.dict.label(froot);
    
    if (fvars.size() > 0) {
        out << " fvars={{{";
        BOOST_FOREACH(sbmt::indexed_token idx, fvars) {
            out << " " << h.dict.label(idx);
        }
        out << " }}}";
    }
    #ifdef XRSDB_SENTIDS
    if (sentids.size() > 0) {
        out << " sentids={{{";
        BOOST_FOREACH(boost::uint32_t idx, sentids) {
            out << " " << idx;
        }
        out << " }}}";
    }
    #endif
    
}

void header::add_frequency(indexed_token wd, size_t f)
{
    map<indexed_token,size_t>::iterator pos = freq.find(wd);
    if (pos != freq.end()) pos->second += f;
    else freq.insert(make_pair(wd,f));
}

void header::add_offset(indexed_token wd, uint64_t off)
{
    offsets[wd] = off;
}

uint64_t header::offset(indexed_token const& wd) const
{
    return offsets.find(wd)->second;
}

void header::add_frequency(fat_token wd, size_t f)
{
    add_frequency(index(wd,dict),f);
}

////////////////////////////////////////////////////////////////////////////////

void header::add_frequencies(header const& other)
{
    BOOST_FOREACH(indexed_token wc, other.dict.virtual_tags()) {
        index(fatten(wc,other.dict), dict);
    }
    BOOST_FOREACH(indexed_token wc, other.dict.tags()) {
        index(fatten(wc,other.dict), dict);
    }
    BOOST_FOREACH(indexed_token wc, other.dict.foreign_words()) {
        index(fatten(wc,other.dict), dict);
    }
    BOOST_FOREACH(indexed_token wc, other.dict.native_words()) {
        index(fatten(wc,other.dict), dict);
    }
    map<indexed_token,size_t>::const_iterator i = other.freq.begin(),
                                              e = other.freq.end();
    for (; i != e; ++i) {
        add_frequency(index(fatten(i->first, other.dict), dict), i->second);
    }
}

////////////////////////////////////////////////////////////////////////////////

header::header()
{
    dict.tag("x");
}

header::~header() {}

////////////////////////////////////////////////////////////////////////////////

path path_from_token(sbmt::indexed_token tok)
{
    uint32_t idx = hash_value(tok);
    uint32_t d1 = idx % 254;
    idx = idx / 254;
    uint32_t d2 = idx % 254;
    idx = idx / 254;
    uint32_t d3 = idx % 254;
    idx = idx / 254;
    uint32_t d4 = idx % 254;
    idx = idx / 254;
    path rt(lexical_cast<string>(d4));
    return rt / lexical_cast<string>(d3) /
           lexical_cast<string>(d2) / lexical_cast<string>(d1);
}

tuple<path,string> structure_from_token(sbmt::indexed_token tok)
{
    uint32_t idx = hash_value(tok);
    if (idx > 254u * 254u * 254u * 254u) {
        throw std::runtime_error("xrsdb: vocabulary exceeds db limits");
    }
    uint32_t d1 = idx % 254;
    idx = idx / 254;
    uint32_t d2 = idx % 254;
    idx = idx / 254;
    uint32_t d3 = idx % 254;
    idx = idx / 254;
    uint32_t d4 = idx % 254;
    idx = idx / 254;
    path rt(lexical_cast<string>(d4));
    string s3 = lexical_cast<string>(d3);
    string s2 = lexical_cast<string>(d2);
    return make_tuple( rt / s3 / s2
                     , lexical_cast<string>(d1)
                     );
}

void save_word_cluster(word_cluster const& db, std::ostream& out)
{
    bio::filtering_stream<bio::output> o;
    o.push(bio::gzip_compressor());
    o.push(out);
    binary_oarchive ar(o);
    ar & db;
}

////////////////////////////////////////////////////////////////////////////////

bool cluster_exists(path const& p, header const& h, indexed_token word)
{
    if (not h.old_version()) return h.offsets.find(word) != h.offsets.end();
    else return fs::exists(p/path_from_token(word));
}

////////////////////////////////////////////////////////////////////////////////

word_cluster load_word_cluster( path const& rootdir
                              , header const& h
                              , indexed_token word )
{
    word_cluster wc;
    path cluster;
    string leaf;
    uint64_t offset = 0;
    if (not h.old_version()) {
        tie(cluster,leaf) = structure_from_token(word);
        offset = h.offset(word);
    } else {
        cluster = path_from_token(word);
    }
    wc = load_word_cluster(rootdir / cluster, offset);
    if (wc.root_word() != word) {
        throw std::runtime_error(
                "xrsdb: mismatch between cluser requested and recovered"
              );
    }
    return wc;
}

word_cluster
load_word_cluster( boost::filesystem::path const& p
                 , uint64_t offset )
{
    fs::ifstream in(p);
    in.seekg(offset);
    bio::filtering_stream<bio::input> i;
    i.push(bio::gzip_decompressor());
    i.push(in);
    
    word_cluster db;
    binary_iarchive ar(i);
    ar & db;
    return db;
}

////////////////////////////////////////////////////////////////////////////////

void load_header(header& dict, path const& p)
{
    fs::ifstream dictfs(p / "header");
    char head[6]; head[5] = 0;
    dictfs.read(head,5); 
    dictfs.close();
    dictfs.open(p / "header");
    if (head == string("<?xml")) {
        xml_iarchive da(dictfs);
        da & BOOST_SERIALIZATION_NVP(dict);
    } else {
        binary_iarchive ar(dictfs);
        ar & dict;
    }
}

////////////////////////////////////////////////////////////////////////////////

void save_header(header const& dict, path const& p)
{
    fs::ofstream dictfile(p / "header");
    xml_oarchive ar(dictfile);
    ar & BOOST_SERIALIZATION_NVP(dict);
}

////////////////////////////////////////////////////////////////////////////////

struct db_creation_visitor {

    db_creation_visitor(path const& p)
      : p(p) {}

    path p;


    template <class Trie>
    void at_state(Trie& tr, typename Trie::state s) const
    {
        create_directory(p / tr.key(s));
        //clog << (p / tr.key(s)) << endl;
    }

    template <class Trie>
    void begin_children(Trie& tr, typename Trie::state s)
    {
        p /= tr.key(s);
    }

    template <class Trie>
    void end_children(Trie& tr, typename Trie::state s)
    {
        p = p.branch_path();
    }
};

////////////////////////////////////////////////////////////////////////////////

void create_empty_database(header const& h, path const& rootdb)
{
    if (not rootdb.empty()) remove_all(rootdb);
    create_directories(rootdb);
    typedef map<indexed_token, size_t> freqmap_t;
    gusc::basic_trie<path,int> paths(0);
    for (freqmap_t::const_iterator i = h.freq.begin(); i != h.freq.end(); ++i) {
        path d = structure_from_token(i->first).get<0>().branch_path();
        //path d = path_from_token(i->first).branch_path();
        paths.insert(d.begin(),d.end(),1);
    }

    db_creation_visitor vis(rootdb);
    traverse_trie(paths, vis);

    save_header(h,rootdb);
}

////////////////////////////////////////////////////////////////////////////////

rule_application_array make_entry( external_buffer_type& subtrie_buffer
                                 , std::vector<rule_data>& rules
                                 , header& h
                                 , sbmt::weight_vector const& weights )
{
    rule_application_allocator alloc(subtrie_buffer.get_segment_manager());
    //sort(rules.begin(),rules.end(),&lower_rule_cost);
    rule_application_array 
        fra = boost::make_tuple(
                ip::offset_ptr<rule_application>(alloc.allocate(rules.size()))
              , rules.size()
              );
    ip::offset_ptr<rule_application> fri = fra.get<0>()
                                 , fre = fra.get<0>() + rules.size();
    BOOST_FOREACH(rule_data& rd, rules) {
        ::new (fri.get()) rule_application(rd,h,alloc,weights);
        //std::cerr << wvalue(rd) << " --vs-- " << fri->cost << '\n';
        ++fri;
    }

    return fra;
}

std::vector<char>
compress(char const* beg, char const* end)
{
    std::vector<char> out;
    boost::iostreams::filtering_streambuf<boost::iostreams::output> buf;
    buf.push(boost::iostreams::zlib_compressor());
    buf.push(boost::iostreams::back_inserter(out));
    boost::iostreams::copy(boost::make_iterator_range(beg,end),buf);
    return out;
}

compressed_signature_trie 
make_sig_entry( external_buffer_type& subtrie_buffer
              , external_buffer_type& wordtrie_buffer
              , subtrie_construct_t& sdbc )
{
    rule_application_allocator slloc(subtrie_buffer.get_segment_manager());
    subtrie_buffer.construct<signature_trie>("root")(sdbc,slloc);
    subtrie_buffer.get_segment_manager()->shrink_to_fit();
    std::vector<char> 
        cmprs = compress( (char const*)subtrie_buffer.get_address()
                        , (char const*)subtrie_buffer.get_address() + subtrie_buffer.get_size()
                        );
    
    char_allocator calloc(wordtrie_buffer.get_segment_manager());
    ip::offset_ptr<char> cptr = calloc.allocate(cmprs.size());
    std::copy(cmprs.begin(),cmprs.end(),cptr);
    return compressed_signature_trie(cptr,cmprs.size());
}

std::ostream& 
print_st_nodes( std::ostream& os
              , signature_trie const& trie
              , signature_trie::state state )
{
    if (trie.value(state) != trie.nonvalue()) {
        std::cerr << "++++++++\n";
        BOOST_FOREACH( rule_application const& ra
                     , rule_application_array_adapter(trie.value(state))) {
                         std::cerr << ra.rule << "\n";
        }
    }
    BOOST_FOREACH(signature_trie::state st, trie.transitions(state)) {
        print_st_nodes(os,trie,st);
    }
    return os;
}

std::ostream& operator<< (std::ostream& os, signature_trie const& trie)
{
    return print_st_nodes(os,trie,trie.start());
}


boost::tuple<boost::shared_array<char>,size_t>
save_word_trie_data( external_buffer_type& wordtrie_buffer
                   , trie_construct_t& dbc )
{
    char_allocator alloc(wordtrie_buffer.get_segment_manager());
    wordtrie_buffer.construct<word_trie>("root")(dbc,alloc);
    wordtrie_buffer.get_segment_manager()->shrink_to_fit();

    uint64_t sz = wordtrie_buffer.get_size();
    boost::shared_array<char> buf(new char[sz]);
    char const* beg = (char const*)wordtrie_buffer.get_address();
    char const* end = beg + sz;
    std::copy(beg,end,buf.get());
    return boost::make_tuple(buf,sz);
}

typedef std::multimap<
   boost::tuple<
     std::vector<
       tuple<short,indexed_token>
     >
   , std::vector<indexed_token>
   >
 , rule_data
 > rule_map_t;

typedef std::map<boost::int64_t, rule_data> rule_data_map;
typedef std::set<exmp::forest_ptr> forest_occurance_set;

void create_word_map( rule_application_map& mp
                    , boost::shared_array<char>& array
                    , std::istream& in
                    , exmp::forest const& frst
                    , sbmt::weight_vector const& weights
                    , header& h )
{
    //rule_data_map rdm;
    forest_occurance_set fos;
    size_t sz = 268435456;
    //    size_t sz = 134217728 * 7; // 256MB
    array.reset(new char[sz]);
    external_buffer_type buffer(ip::create_only,array.get(),sz);
    rule_application_allocator alloc(buffer.get_segment_manager());
    std::string line;
    while (getline(in,line) and line != "") {
        rule_data rd = parse_xrs(line);
        ip::offset_ptr<rule_application> rptr(alloc.allocate(1));
        ::new (rptr.get()) rule_application(rd,h,alloc,weights);
        mp[rd.id] = rptr;
    }
    //create_word_map(mp,rdm,fos,alloc,exmp::forest_ptr(new exmp::forest(frst)),weights,h);
}

boost::tuple<boost::shared_array<char>,size_t>
create_word_trie( rule_map_t const& rmap
                , sbmt::weight_vector const& weights
                , header& h )
{

    size_t sz = 134217728; //128MB

    std::vector< boost::tuple<short,indexed_token> > keys;
    std::vector<indexed_token> sig;
    rule_data rule;
    std::vector<rule_data> rules;

    boost::shared_array<char> subtrie_array, wordtrie_array;
    external_buffer_type subtrie_buffer;
    external_buffer_type wordtrie_buffer;
    subtrie_construct_t sdbc(rule_application_array(0,0));
    trie_construct_t dbc(compressed_signature_trie(0,0));
    subtrie_array.reset(new char[sz]);
    subtrie_buffer = external_buffer_type(ip::create_only,subtrie_array.get(),sz);
    wordtrie_array.reset(new char[sz]);
    wordtrie_buffer = external_buffer_type(ip::create_only,wordtrie_array.get(),sz);
    rule_map_t::const_iterator ri = rmap.begin();
    rule_map_t::const_iterator re = rmap.end();
    if (ri != re) {
        tie(keys,sig) = ri->first;
        rule = ri->second;       
        rules.push_back(rule);
        ++ri;
    }

    for(; ri != rmap.end(); ++ri) {
        std::vector< boost::tuple<short,indexed_token> > k;
        std::vector<indexed_token> s;
        tie(k,s) = ri->first;
        rule = ri->second;
        if (s != sig or k != keys) {
            sdbc.insert(sig.begin(),sig.end(),make_entry(subtrie_buffer,rules,h,weights));
            rules.clear();
            sig = s;
        }
        if (k != keys) {
            dbc.insert(keys.begin(),keys.end(),make_sig_entry(subtrie_buffer,wordtrie_buffer,sdbc));
            subtrie_construct_t(rule_application_array(0,0)).swap(sdbc);
            boost::shared_array<char> new_array(new char[sz]);
            external_buffer_type(ip::create_only,new_array.get(),sz).swap(subtrie_buffer);
            subtrie_array = new_array;
            keys = k;
        }
        rules.push_back(rule);
    }
    sdbc.insert(sig.begin(),sig.end(),make_entry(subtrie_buffer,rules,h,weights));
    dbc.insert(keys.begin(),keys.end(),make_sig_entry(subtrie_buffer,wordtrie_buffer,sdbc));
    return save_word_trie_data(wordtrie_buffer,dbc);
}

typedef std::map<sbmt::indexed_token,sbmt::weight_vector> cell_feature_map;
typedef std::map<sbmt::span_t,cell_feature_map> chart_feature_map;
void create_lattice_rules(gusc::lattice_line const& line, chart_feature_map& cfm, header& h) {
    if (not line.is_block()) {
        sbmt::span_t spn(line.span().from(),line.span().to());
        sbmt::indexed_token tok = h.dict.foreign_word(line.label());
        std::string key,val;
        cfm[spn][tok];
        BOOST_FOREACH(gusc::property_container_interface::key_value_pair kvp, line.properties()) {
            try {
                sbmt::score_t scr = boost::lexical_cast<sbmt::score_t>(kvp.value());
                cfm[spn][tok][h.fdict.get_index(kvp.key())] += scr.neglog10();
            } catch(...) {
                
            }
        }
    } else {
        BOOST_FOREACH(gusc::lattice_line const& cline, line.lines()) {
            create_lattice_rules(cline,cfm,h);
        }
    }
}

boost::tuple<boost::shared_array<char>,size_t>
create_lattice_rules(gusc::lattice_ast const& lat, sbmt::weight_vector const& weights, header& h, chart_initial_rule_map& cirm)
{
    chart_feature_map cfm;
    BOOST_FOREACH(gusc::lattice_line const& line, lat.lines()) { create_lattice_rules(line,cfm,h); }
    size_t sz = 10*1024*1024;
    size_t n = 0;
    BOOST_FOREACH(chart_feature_map::value_type const& vt, cfm) n +=  vt.second.size();
    
    boost::shared_array<char> array(new char[sz]);
    external_buffer_type buffer(ip::create_only,array.get(),sz);
    rule_application_allocator alloc(buffer.get_segment_manager());
    ip::offset_ptr<rule_application> rptr(alloc.allocate(n));
    buffer.construct<rule_application_array>("root")(rptr,n);
    std::vector< boost::tuple<sbmt::span_t,sbmt::indexed_token> > place(n);
    size_t x = 0;
    BOOST_FOREACH(chart_feature_map::value_type const& vt, cfm) {
        BOOST_FOREACH(cell_feature_map::value_type const& vvt, vt.second) {
            ::new (rptr.get()) rule_application(vvt.second,h,alloc,weights);
            place[x] = boost::make_tuple(vt.first,vvt.first);
            ++x;
            ++rptr;
        }
    }
    buffer.get_segment_manager()->shrink_to_fit();
    sz = buffer.get_size();
    boost::shared_array<char> newarray(new char[sz]);
    char const* beg = (char const*)buffer.get_address();
    char const* end = beg + sz;
    std::copy(beg,end,newarray.get());
    external_buffer_type newbuffer(ip::open_only,newarray.get(),sz);
    rptr = newbuffer.find<rule_application_array>("root").first->get<0>();
    sbmt::span_t spn;
    sbmt::indexed_token tok;
    BOOST_FOREACH(boost::tie(spn,tok),place) {
        cirm[spn][tok] = rptr;
        ++rptr;
    }
    return boost::make_tuple(newarray,sz);
}

boost::tuple<boost::shared_array<char>,size_t>
create_word_trie(std::istream& in, sbmt::weight_vector const& weights, header& h)
{
    rule_map_t rules;
    std::string line;
    wildcard_array wc(h.dict);
    while (getline(in,line)) {
        try {
            rule_data rule = parse_xrs(line);
            std::vector< boost::tuple<short,indexed_token> > keys;
            std::vector<indexed_token> sig;
            size_t skip = 0;
            short dir = 1;
            BOOST_FOREACH(rhs_node const& rn,rule.rhs) {
                if (rn.indexed) {
                    sig.push_back(h.dict.tag(label(rule,rn)));
                    ++skip;
                } else {
                    sig.push_back(h.dict.foreign_word(label(rule,rn)));
                    if (skip > 0) {
                        keys.push_back(boost::make_tuple(dir,wc[skip]));
                        skip = 0;
                    }
                    keys.push_back(boost::make_tuple(dir,h.dict.foreign_word(label(rule,rn))));
                }
            }
            if (skip > 0) keys.push_back(boost::make_tuple(dir,wc[skip]));
            rules.insert(std::make_pair(boost::make_tuple(keys,sig),rule));
        } catch(...) { std::cerr << "rejected: " << line << '\n'; }
    }
    return create_word_trie(rules,weights,h);
}

} // namespace xrsdb
