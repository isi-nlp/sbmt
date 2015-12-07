#ifndef   SBMT_GRAMMAR_BRF_ARCHIVE_IO_HPP
#define   SBMT_GRAMMAR_BRF_ARCHIVE_IO_HPP

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <sbmt/logmath.hpp> // for score_t
#include <map>
#include <string>
#include <vector>
#include <sbmt/grammar/brf_reader.hpp>
#include <sbmt/token/in_memory_token_storage.hpp>
#include <boost/utility/enable_if.hpp>
////////////////////////////////////////////////////////////////////////////////
///
/// forward declaration to avoid including <boost/archive/binary_oarchive.hpp>
/// because boost::serialization is sensitive to the order in which archive
/// code and serialization code is includes
///
////////////////////////////////////////////////////////////////////////////////
namespace boost{ namespace archive{ class binary_oarchive; } }

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

namespace brf_archive_writer_detail {
template <class Token, class Archive> class on_brf;
template <class Token, class Archive> class on_xrs;
}

enum archive_type { brf = 0, gar, text_gar, fat_gar, fat_text_gar };
std::ostream& operator<<(std::ostream&, archive_type const&);
std::istream& operator>>(std::istream&, archive_type&);



////////////////////////////////////////////////////////////////////////////////

template <class Token = indexed_token_factory>
class brf_archive_writer_tmpl
{
public:
    /// \param in   a brf rule file, as created by, say, itg-binarizer
    /// \param out  a binary archive, readable by brf_archive_reader
    template <class Archive>
    typename boost::enable_if<typename Archive::is_saving>::type 
    operator() (std::istream& in, Archive& out);
private:
    in_memory_token_storage scores,texts; // feature names                             
    template <class T, class A>
    friend class brf_archive_writer_detail::on_brf;
    template <class T, class A>
    friend class brf_archive_writer_detail::on_xrs;
};

typedef brf_archive_writer_tmpl<> brf_archive_writer;
typedef brf_archive_writer_tmpl<fat_token_factory> brf_fat_archive_writer;

////////////////////////////////////////////////////////////////////////////////

template <class Archive, class TF>
class brf_archive_reader_tmpl : public brf_reader_tmpl<TF>
{
public:
    typedef brf_reader_tmpl<TF> base_type;
    brf_archive_reader_tmpl(std::istream& input)
    : input(input) {}
    
    template <class SyntaxHandler, class BinaryHandler>
    brf_archive_reader_tmpl( std::istream& input
                           , SyntaxHandler sf
                           , BinaryHandler bf
                           , TF& tf
                           , in_memory_token_storage& dict )
    : brf_reader_tmpl<TF>(sf, bf, tf, dict)
    , input(input)
     {}
    
    virtual void read();
    
    virtual ~brf_archive_reader_tmpl() {}
private:
    std::istream&         input;
};

template <class Archive>
class brf_fat_to_indexed_archive_reader_tmpl : public brf_reader
{
public:
    brf_fat_to_indexed_archive_reader_tmpl(std::istream& input)
      : input(input) {}
    
    template <class SyntaxHandler, class BinaryHandler>
    brf_fat_to_indexed_archive_reader_tmpl( std::istream& input
                                          , SyntaxHandler sf
                                          , BinaryHandler bf
                                          , indexed_token_factory& tf
                                          , in_memory_token_storage& dict )
      : brf_reader(sf, bf, tf, dict)
      , input(input) {}
      
    virtual void read();
    virtual ~brf_fat_to_indexed_archive_reader_tmpl() {}
private:
    std::istream& input;
};

typedef brf_fat_to_indexed_archive_reader_tmpl<boost::archive::binary_iarchive>
        brf_fat_to_indexed_archive_reader;

typedef brf_fat_to_indexed_archive_reader_tmpl<boost::archive::text_iarchive>
        brf_fat_to_indexed_text_archive_reader;

typedef brf_archive_reader_tmpl< boost::archive::binary_iarchive
                               , fat_token_factory
                               > brf_fat_archive_reader;

typedef brf_archive_reader_tmpl< boost::archive::text_iarchive
                               , fat_token_factory
                               > brf_fat_text_archive_reader;

typedef brf_archive_reader_tmpl< boost::archive::text_iarchive
                               , indexed_token_factory
                               > brf_text_archive_reader;
                               
typedef brf_archive_reader_tmpl< boost::archive::binary_iarchive
                               , indexed_token_factory
                               > brf_archive_reader;


////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

#endif // SBMT_GRAMMAR_BRF_ARCHIVE_IO
