# include "info.hpp"
# include <boost/range.hpp>
namespace srl {



static boost::regex namere("(a|rev|no)greement\\[([^ ]*)\\]");

std::string feat2str(feature const& f, sbmt::indexed_token_factory& dict)
{
    std::string fname = dict.label(f.n);
    if (fname[0] == ':') fname = fname.substr(1,std::string::npos);
    if (f.t == feature::match) return "agreement[" + fname + "]";
    else if (f.t == feature::revmatch) return "revgreement[" + fname + "]";
    else if (f.t == feature::nomatch) return "nogreement[" + fname + "]";
    else throw std::logic_error("incomplete cases for agreement features");
}

std::pair<bool,feature> str2feat(std::string const& t, sbmt::indexed_token_factory& dict)
{
    boost::smatch m;
    bool br = false;
    sbmt::indexed_token sr;
    feature::type tr;
    if (regex_match(t,m,namere)) {
        br = true;
        if (m[1].str() == "a") tr = feature::match;
        else if (m[1].str() == "rev") tr = feature::revmatch;
        else if (m[1].str() == "no") tr = feature::nomatch;
        else throw std::logic_error("incomplete cases for agreement features");
        sr = dict.native_word(":" + m[2].str()); 
    }
    return std::make_pair(br,feature(sr,tr));
}

weight_vector weights( sbmt::weight_vector const& w
                     , sbmt::in_memory_dictionary& dict
                     , sbmt::feature_dictionary& fdict
                     )
{
    weight_vector wv;
    BOOST_FOREACH(boost::range_value<sbmt::weight_vector const>::type v, w) {
        std::string n = fdict.get_token(v.first);
        bool p;
        feature t(sbmt::indexed_token(),feature::match);
        boost::tie(p,t) = str2feat(n,dict);
        if (p) {
            wv[t] = v.second;
        }
    }
    std::cerr << "SRL weights: " << wv << '\n';
    return wv;
}

info_factory::info_factory( sbmt::in_memory_dictionary& dict
                          , sbmt::feature_dictionary& fdict
                          , sbmt::weight_vector const& w
                          , sbmt::property_map_type propmap
                          , sbmt::lattice_tree const& ltree
                          , alignment_score_map const* asmap
                          , label_map const* lblset
                          , label_map const* invlblset
                          , incidence_set incset
                          , bool match_label
                          , bool record_nomatch )
: headmarkerid(propmap["headmarker"])
, alignstrid(propmap["align"])
, wts(weights(w,dict,fdict))
, asmap(asmap)
, lblset(lblset)
, invlblset(invlblset)
, incset(incset)
, match_label(match_label)
, record_nomatch(record_nomatch)
{
    boost::tuple<int,int> p;
    sbmt::indexed_token t;
    BOOST_FOREACH(boost::tie(p,t),incset) std::cerr << p << " -> " << t << "\n";
    init(ltree.root(),dict);
}

void
info_factory::init( sbmt::lattice_tree::node const& lnode
                  , sbmt::in_memory_dictionary& dict )
{
    if (lnode.is_internal()) {
        BOOST_FOREACH(sbmt::lattice_tree::node lchld, lnode.children()) {
            init(lchld,dict);
        }
    } else {
        pmap.insert(std::make_pair(lnode.lat_edge().span.left(),lnode.lat_edge().source));
    }
}

void read(std::istream& in, alignment_score_map& amap, sbmt::in_memory_dictionary& dict)
{
    amap.clear();
    std::string from, to;
    std::string line;
    float w;
    while (in >> to >> from >> w) {
        amap[dict.native_word(to)][dict.foreign_word(from)] = w;
    }
}

void read( std::istream& in
         , sentence_incidence_map& smap
         , sbmt::in_memory_dictionary& dict )
{
    std::cerr << "reading srl incidence file\n";
    smap.clear();
    int sent, head, dep;
    std::string label;
    while (in >> sent >> head >> dep >> label) {
        ++sent; ++head; ++dep;
        if (label[0] != ':') label = ":" + label;
        //std::cerr << sent << ": " << boost::make_tuple(head,dep,label) << "\n";
        smap[sent].insert(std::make_pair(boost::make_tuple(head,dep),dict.native_word(label)));
    }
}

info_constructor::info_constructor()
: tgtmax(0)
, match_label(false)
, record_nomatch(true) {}

bool info_constructor::set_option(std::string const& nm, std::string const& vl)
{
    return false;
}

sbmt::options_map info_constructor::get_options()
{
    sbmt::options_map opts("semantic role label options");
    opts.add_option( "srl-match-label"
                   , sbmt::optvar(match_label)
                   , "match the label to get credit" 
                   );
    opts.add_option( "srl-record-nomatch"
                   , sbmt::optvar(record_nomatch)
                   , "record no-match"
                   );
    opts.add_option( "srl-alignment-prob-file"
                   , sbmt::optvar(asfile)
                   , "file of 'target source prob' lines"
                   );
    opts.add_option( "srl-file"
                   , sbmt::optvar(incfile)
                   , "file of 'sentid targetid sourceid role-label' lines"
                   );
    return opts;
}

void 
info_constructor::init(sbmt::in_memory_dictionary& dict)
{
    if (incfile != "" and incmap.size() == 0) {
        std::ifstream ifs(incfile.c_str());
        read(ifs,incmap,dict);
    } 
    if (asfile != "" and asmap.size() == 0) {
        std::ifstream ifs(asfile.c_str());
        read(ifs,asmap,dict);
    }
    BOOST_FOREACH(sbmt::indexed_token tok, dict.native_words(tgtmax)) {
        if (dict.label(tok)[0] == ':') {
            lblmap.insert(tok);
            if (dict.label(tok).find("-of") != std::string::npos) invlblmap.insert(tok);
        }
    }
    tgtmax = dict.native_word_count();
}

info_factory 
info_constructor::construct( sbmt::in_memory_dictionary& dict 
                           , sbmt::feature_dictionary& fdict
                           , sbmt::weight_vector const& weights
                           , sbmt::lattice_tree const& lat
                           , sbmt::property_map_type pmap )
{
    init(dict);
    std::cerr << "srl items for sentence "<< lat.id << "\n";
    int h, d; 
    sbmt::indexed_token lbl;
    //BOOST_FOREACH(boost::tie(h,d,lbl),incmap[lat.id]) std::cerr << boost::make_tuple(h,d,lbl) << "\n";
    return info_factory( dict
                       , fdict
                       , weights
                       , pmap
                       , lat
                       , &asmap
                       , &lblmap
                       , &invlblmap
                       , incmap[lat.id]
                       , match_label
                       , record_nomatch );
}

float alignment_score(alignment_score_map const& asmap, sbmt::indexed_token tgt, sbmt::indexed_token src)
{
    alignment_score_map::const_iterator pos = asmap.find(tgt);
    if (pos == asmap.end()) return 0.0;
    std::map<sbmt::indexed_token,float>::const_iterator ppos = pos->second.find(src);
    if (ppos == pos->second.end()) return 0.0;
    else return ppos->second;
}

} // namespace srl
