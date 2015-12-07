#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/cstdint.hpp>
#include <gusc/const_any.hpp>
#include <sbmt/grammar/brf_archive_io.hpp>
#include <sbmt/grammar/brf_file_reader.hpp>
#include <sbmt/grammar/syntax_rule.hpp>
#include <sbmt/grammar/rule_input.hpp>
#include <algorithm>
#include <iostream>

using boost::archive::binary_oarchive;
using boost::archive::binary_iarchive;
using boost::archive::text_oarchive;
using boost::archive::text_iarchive;
using namespace std;

#define BRF_ARCHIVE_CLEAR_MAP 1
#define ARCHIVE_SANITY_CHECK 0

////////////////////////////////////////////////////////////////////////////////
///
/// used to mark sections in the binary archive
/// dictionary_section 
///     indicates that the next section are the labels of the 
///     indexed tokens
/// xrs_item 
///     indicates that what follows is a syntax_rule / component score list
/// brf_item 
///     indicates a binary_rule / syntax_rule id set follows
/// component_score_label 
///     indicates a previously unseen component score has 
///     appeared.  using this allows us to not redundently keep component score 
///     names in the archive.
/// 
////////////////////////////////////////////////////////////////////////////////

#define ARCHIVE_DICTIONARY_SECTION    0
#define ARCHIVE_XRS_ITEM              1
#define ARCHIVE_BRF_ITEM              2
#define ARCHIVE_COMPONENT_SCORE_LABEL 3
#define ARCHIVE_TEXT_FEATURE_LABEL    4
#define ARCHIVE_SPARSE_XRS_ITEM       5
#define ARCHIVE_VERSION_LABEL         6

static const char archive_dictionary_section    = ARCHIVE_DICTIONARY_SECTION;
static const char archive_xrs_item              = ARCHIVE_XRS_ITEM;
static const char archive_sparse_xrs_item       = ARCHIVE_SPARSE_XRS_ITEM;
static const char archive_brf_item              = ARCHIVE_BRF_ITEM;
static const char archive_component_score_label = ARCHIVE_COMPONENT_SCORE_LABEL;
static const char archive_text_feature_label    = ARCHIVE_TEXT_FEATURE_LABEL;
static const char archive_version_label         = ARCHIVE_VERSION_LABEL;

namespace boost { namespace serialization {

// no-op
template<class Ar>
void serialize(Ar & ar, sbmt::fat_token_factory& tf, const unsigned int) {}

} } // namespace boost::serialization


/*
namespace boost{ namespace serialization {
    
template <class AR>
void save(AR& ar, sbmt::archive_token_id const& x, const unsigned int version)
{
    ar & std::size_t(x);
}

template <class AR>
void load(AR& ar, sbmt::archive_token_id const& x, const unsigned int version)
{
    std::size_t y;
    ar & y;
    x = y;
}

BOOST_SERIALIZATION_SPLIT_FREE(sbmt::archive_token_id)

}} // namespace boost::serialization
*/

namespace sbmt {
    
const char* archive_lbl[] = { "brf", "archive", "text-archive", "fat-archive", "fat-text-archive" };

std::ostream& operator<<(std::ostream& os, archive_type const& a)
{
    return os << archive_lbl[a];
}

std::istream& operator>>(std::istream& is, archive_type& a)
{
    std::string s;
    is >> s;
    for (size_t i = 0; i != 5; ++i) if (s == archive_lbl[i]) {
        a = archive_type(i);
        return is;
    }
    throw std::runtime_error("archive_type enum unmatched");
}

////////////////////////////////////////////////////////////////////////////////

namespace brf_archive_writer_detail {


template < class string_vals
         , class vals
         , class oarchive
         , class archive_header >
void named_features_to_sparse_vector( in_memory_token_storage& names
                                    , in_memory_token_storage const& fsnames
                                    , string_vals const& features
                                    , vals& outmap
                                    , oarchive& names_archive
                                    , archive_header label_header )
{
    typedef in_memory_token_storage::index_type index_type;
    typename string_vals::const_iterator f = features.begin(), 
                                         fe = features.end();
    outmap.clear();
    for (; f != fe; ++f) {
        index_type added = names.size();
        index_type i = names.get_index(fsnames.get_token(f->first));
        if (i == added) {
            names_archive & label_header;
            names_archive & fsnames.get_token(f->first);
        } 
        outmap.insert(std::make_pair(i,f->second));
    }
}


template <class TF, class Archive>
class on_xrs
{
public:
    on_xrs( brf_archive_writer_tmpl<TF>& writer
          , in_memory_token_storage& fsdict
          , Archive& out )
    : out(out)
    , writer(writer)
    , fsdict(&fsdict) {}
    
    void operator()( syntax_rule<typename TF::token_t> const& xrs_rule
                   , texts_map_type const& texts )
    {
        named_features_to_sparse_vector( writer.texts
                                       , *fsdict
                                       , texts
                                       , text_vec
                                       , out
                                       , archive_text_feature_label );
        // note: new feature names are written before xrs item starts - 
        // see brf_archive_writer::read()
        out & archive_sparse_xrs_item;
        out & xrs_rule;
        out & text_vec;
    }
private:
    Archive&    out;
    brf_archive_writer_tmpl<TF>& writer;
    in_memory_token_storage* fsdict;
    map<boost::uint32_t,std::string> text_vec;
};

////////////////////////////////////////////////////////////////////////////////

template <class TF, class Archive>
class on_brf
{
public:
    on_brf( brf_archive_writer_tmpl<TF>& writer
          , in_memory_token_storage& fsdict
          , Archive& out )
    : out(out)
    , writer(writer)
    , fsdict(&fsdict) {}
    void operator()( binary_rule<typename TF::token_t> const& brf_rule
                   , score_map_type const& scores
                   , texts_map_type const& properties
                   , vector<syntax_id_type> xrs_rule_ids )
    {
        named_features_to_sparse_vector( writer.scores
                                       , *fsdict
                                       , scores
                                       , score_vec
                                       , out
                                       , archive_component_score_label );
        named_features_to_sparse_vector( writer.texts
                                       , *fsdict
                                       , properties
                                       , text_vec
                                       , out
                                       , archive_text_feature_label );
        out & archive_brf_item;
        out & brf_rule;
        out & score_vec;
        out & text_vec;
        out & xrs_rule_ids;
    }
private:
    Archive& out;
    brf_archive_writer_tmpl<TF>& writer;
    in_memory_token_storage* fsdict;
    map<boost::uint32_t,score_t>     score_vec;
    map<boost::uint32_t,std::string> text_vec;
};

} // namespace brf_archive_writer_detail


////////////////////////////////////////////////////////////////////////////////
//
// version 3: syntax ids are now 64 bit. thanks, jason...
//
////////////////////////////////////////////////////////////////////////////////
# define GAR_VERSION 3
template <class TF>
template <class Archive>
typename boost::enable_if<typename Archive::is_saving>::type
brf_archive_writer_tmpl<TF>::operator()(istream& in, Archive& ar)
{
    using namespace brf_archive_writer_detail;
    
    TF dict;
    in_memory_token_storage featnames;
    brf_stream_reader_tmpl<TF> reader( in
                                     , on_xrs<TF, Archive>(*this,featnames,ar)
                                     , on_brf<TF, Archive>(*this,featnames,ar)
                                     , dict
                                     , featnames
                                     );
    unsigned int version = GAR_VERSION;
    ar & archive_version_label;
    ar & version;
    reader.read();
    ar & archive_dictionary_section;
    ar & dict;
}

template void 
brf_archive_writer_tmpl<indexed_token_factory>::operator()<binary_oarchive>(istream&,binary_oarchive&);
template void 
brf_archive_writer_tmpl<indexed_token_factory>::operator()<text_oarchive>(istream&,text_oarchive&);
template void 
brf_archive_writer_tmpl<fat_token_factory>::operator()<binary_oarchive>(istream&,binary_oarchive&);
template void 
brf_archive_writer_tmpl<fat_token_factory>::operator()<text_oarchive>(istream&,text_oarchive&);

////////////////////////////////////////////////////////////////////////////////

template <class SR>
void log_syntax_rule( SR const& xrs_rule
                    , score_map_type const& score_map )
{
    #if ARCHIVE_SANITY_CHECK
    cerr << "syntax_rule id=" << xrs_rule.id();
    score_map_type::const_iterator 
        itr=score_map.begin(), end=score_map.end();
    for (; itr != end; ++itr) cerr << " " << itr->first << "=" << itr->second;
    cerr << endl;
    #endif // ARCHIVE_SANITY_CHECK
}

////////////////////////////////////////////////////////////////////////////////

namespace brf_archive_reader_detail {

template <class fnames,class string_vals,class vals>
void vector_to_named_features(fnames const& names,vals const& vec,string_vals & outmap,typename vals::value_type const& default_val)
{
    typename vals::const_iterator v=vec.begin(),ve=vec.end();
#if BRF_ARCHIVE_CLEAR_MAP
    outmap.clear();
#endif 
    for (unsigned i=0;v!=ve;++v,++i) {
        assert(i<names.size());
#if BRF_ARCHIVE_CLEAR_MAP
        if (*v!=default_val) // if you don't clear map, then you must set everything
#endif 
            outmap[names[i]]=*v;
    }
    
}

template <class fnames,class string_vals,class vals>
void sparse_vector_to_named_features( in_memory_token_storage& dict
                                    , fnames const& names
                                    , vals const& vec
                                    , string_vals & outmap )
{
    typename vals::const_iterator v=vec.begin(),ve=vec.end();
    outmap.clear();
    for (; v != ve; ++v) {
        outmap.insert(std::make_pair(dict.get_index(names[v->first]),v->second));
    }
}

template <class iarchive,class fnames>
void new_feature_name(iarchive & ia,fnames &names,in_memory_token_storage& dict)
{
    std::string l;
    ia >> l;
    /// \todo check this - pust had a bug with serialization and needing to do 
    /// string(l.c_str()) - but was reusing the same string variable.  shouldn't 
    /// need it now
    names.push_back(string(l.c_str()));
    dict.get_index(l);
}

}

template <class AR, class TF>
void brf_archive_reader_tmpl<AR,TF>::read()
{
    AR ar(input);
    typedef typename TF::token_t token_t;
    typedef brf_reader_tmpl<TF> base_;

    binary_rule<token_t> bin_rule;
    vector<syntax_id_type> xrs_ids;
    vector<old_syntax_id_type> old_xrs_ids;
    syntax_rule<token_t> xrs_rule;
    
    typedef std::vector<std::string> names_type;
    //indexed_token_factory
    
    names_type score_names,text_names;
    
    score_map_type score_map;
    vector<score_t> score_vec;
    map<boost::uint32_t,score_t> score_sparse_vec;
    
    texts_map_type text_map;
    vector<std::string> text_vec;    
    map<boost::uint32_t,string> text_sparse_vec;

    using namespace brf_archive_reader_detail;
    
    unsigned int version = 0;
    while (true) {
        char c;
        ar & c;
        switch(c) {
        case archive_dictionary_section:
            ar & base_::tf();
            return; // the dictionary section ends the archive.  or would you rather loop forever?  :)
            //FIXME: end of archive constant for clarity/checking?
        case archive_sparse_xrs_item:
            ar & xrs_rule;
            if (version == 0) {
                ar & score_sparse_vec;
                sparse_vector_to_named_features(base_::dict(),score_names,score_sparse_vec,score_map);
            }
            ar & text_sparse_vec;
            sparse_vector_to_named_features(base_::dict(),text_names,text_sparse_vec,text_map);
	    base_type::syntax_rule_cb(xrs_rule, text_map);
            break;
        case archive_brf_item:
            ar & bin_rule;
            if (version > 0) {
                ar & score_sparse_vec;
                sparse_vector_to_named_features(base_::dict(),score_names,score_sparse_vec,score_map);
            }
            if (bin_rule.num_properties() == 0) {
                ar & text_sparse_vec;
                sparse_vector_to_named_features(base_::dict(),text_names,text_sparse_vec,text_map);
            }
            if (version < 3) {
                ar & old_xrs_ids;
                xrs_ids.clear();
                copy(old_xrs_ids.begin(),old_xrs_ids.end(),back_inserter(xrs_ids));
            } else {
                ar & xrs_ids;
            }
	    base_type::binary_rule_cb(bin_rule, score_map, text_map, xrs_ids);
            if (version == 0) {
                score_map.clear();
            }
            break;
        case archive_component_score_label:
            new_feature_name(ar,score_names,base_::dict());
            break;
        case archive_text_feature_label:
            new_feature_name(ar,text_names,base_::dict());
            break;
        case archive_version_label:
            ar & version;
            break;
        default:
            std::stringstream sstr;
            sstr << "Unknown grammar archive section type (wrong version?) c=" << (unsigned int)c;
            throw std::runtime_error(sstr.str());
        }
    }
}

template class brf_archive_reader_tmpl< boost::archive::binary_iarchive
                                      , fat_token_factory
                                      >;

template class brf_archive_reader_tmpl< boost::archive::text_iarchive
                                      , fat_token_factory
                                      >;

template class brf_archive_reader_tmpl< boost::archive::text_iarchive
                                      , indexed_token_factory
                                      >;

template class brf_archive_reader_tmpl< boost::archive::binary_iarchive
                                      , indexed_token_factory
                                      >;

namespace {

template <class OPT>
struct index_syn {
    void operator()( fat_syntax_rule fr
                   , texts_map_type const& tm )
    {
        indexed_syntax_rule sr = index(fr,*tf);
        op(sr,tm);
    }
    index_syn(OPT op, indexed_token_factory& tf) : tf(&tf), op(op) {}
private:
    indexed_token_factory* tf;
    OPT op;
};

template <class OPT> index_syn<OPT> 
make_index_syn_cb(OPT const& op, indexed_token_factory& tf)
{
    return index_syn<OPT>(op,tf);
}

template <class OPT>
struct index_bin {
    void operator()( binary_rule<fat_token> fr
                   , score_map_type const& scores
                   , texts_map_type const& properties
                   , std::vector<syntax_id_type> const& v )
    {
        binary_rule<indexed_token> sr = index(fr,*tf);
        if (fr.num_properties() >= 1) {
            std::vector<gusc::const_any> props;
            props.push_back(reindex(fr.get_property<fat_lm_string>(0),fat_tf,*tf));
            if (fr.num_properties() >= 2) {
                props.push_back(reindex(fr.get_property<fat_lm_string>(1),fat_tf,*tf));
            }
            sr = binary_rule<indexed_token>(sr.topology(),props);
        }
        op(sr,scores,properties,v);
    }
    index_bin(OPT op, indexed_token_factory& tf) : tf(&tf), op(op) {}
private:
    indexed_token_factory* tf;
    OPT op;
};

template <class OPT> index_bin<OPT> 
make_index_bin_cb(OPT const& op, indexed_token_factory& tf)
{
    return index_bin<OPT>(op,tf);
}

} // unnamed namespace

template <class AR>
void brf_fat_to_indexed_archive_reader_tmpl<AR>::read()
{
    typedef brf_reader base;
    brf_archive_reader_tmpl<AR,fat_token_factory> 
        ar( (*this).input
          , make_index_syn_cb(base::syntax_rule_cb,base::tf())
          , make_index_bin_cb(base::binary_rule_cb,base::tf())
          , fat_tf
          , base::dict()
          );
    ar.read();
}

template class brf_fat_to_indexed_archive_reader_tmpl<binary_iarchive>;
template class brf_fat_to_indexed_archive_reader_tmpl<text_iarchive>;

} // namespace sbmt 
