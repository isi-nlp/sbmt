# if ! defined(XRSDB__COLLAPSED_SIGNATURE_ITERATOR_HPP)
# define       XRSDB__COLLAPSED_SIGNATURE_ITERATOR_HPP

# include <boost/iterator/iterator_adaptor.hpp>
# include <boost/range.hpp>

namespace xrsdb {

////////////////////////////////////////////////////////////////////////////////

template <class TI> class collapsed_signature_iterator;

////////////////////////////////////////////////////////////////////////////////

template <class TI>
std::pair< collapsed_signature_iterator<TI>
         , collapsed_signature_iterator<TI>
         >
collapse_signature2(TI beg, TI end, sbmt::indexed_token wc);

////////////////////////////////////////////////////////////////////////////////

template <class TokItr>
class collapsed_signature_iterator
  : public boost::iterator_adaptor<
        collapsed_signature_iterator<TokItr>
      , TokItr
      , sbmt::indexed_token
      , boost::use_default
      , sbmt::indexed_token
    >
{
    typedef collapsed_signature_iterator<TokItr> self_t;
    typedef sbmt::indexed_token reference_;
    sbmt::indexed_token wc;
    TokItr beg;
    TokItr end;
    reference_ dereference() const;
    void increment();
    void decrement();
    TokItr& base_refe();
    TokItr const& base_refe() const;

    friend class boost::iterator_core_access;

    friend std::pair<self_t,self_t>
        collapse_signature2<TokItr>(TokItr,TokItr,sbmt::indexed_token);

    collapsed_signature_iterator( TokItr beg
                                , TokItr itr
                                , TokItr end
                                , sbmt::indexed_token wc );
public:
    collapsed_signature_iterator() {}
};

////////////////////////////////////////////////////////////////////////////////

template <class TI>
std::pair< collapsed_signature_iterator<TI>
         , collapsed_signature_iterator<TI>
         >
collapse_signature2(TI beg, TI end, sbmt::indexed_token wc)
{
    return std::make_pair( collapsed_signature_iterator<TI>(beg,beg,end,wc)
                         , collapsed_signature_iterator<TI>(beg,end,end,wc)
                         );
}

template <class R>
std::pair<
  collapsed_signature_iterator<typename boost::range_result_iterator<R const>::type>
, collapsed_signature_iterator<typename boost::range_result_iterator<R const>::type>
>
collapse_signature(R const& r, sbmt::indexed_token wc)
{
	typedef typename boost::range_result_iterator<R const>::type ti;
	return collapse_signature2(boost::begin(r),boost::end(r),wc);
}

////////////////////////////////////////////////////////////////////////////////

template <class TI>
collapsed_signature_iterator<TI>::collapsed_signature_iterator(
                                      TI b
                                    , TI i
                                    , TI e
                                    , sbmt::indexed_token wc
                                  )
  : collapsed_signature_iterator::iterator_adaptor_(i)
  , wc(wc)
  , beg(b)
  , end(e)
{
//    TI prev = base_refe();
//    while((base_refe() != end) and (not is_lexical(*base_refe()))) {
//        prev = base_refe();
//        ++base_refe();
//    }
//    base_refe() = prev;
//    beg = prev;
}

////////////////////////////////////////////////////////////////////////////////

template <class TI>
typename collapsed_signature_iterator<TI>::reference_
collapsed_signature_iterator<TI>::dereference() const
{
    if (is_lexical(*base_refe())) return *base_refe();
    else return wc;
}

////////////////////////////////////////////////////////////////////////////////

template <class TI>
void
collapsed_signature_iterator<TI>::increment()
{
    if (base_refe() != end and not is_lexical(*base_refe())) {
        while ((base_refe() != end) and (not is_lexical(*base_refe()))) {
            ++base_refe();
        }
    } else {
        ++base_refe();
    }
}

template <class TI>
void
collapsed_signature_iterator<TI>::decrement()
{
    //std::clog << "1";
    if (base_refe() == beg) {
        //std::clog << "2";
        //  omitted on account of stringent stlport checks // --base_refe();
        //std::clog << "/2";
    } else {
        --base_refe();
        if (base_refe() != beg and not is_lexical(*base_refe())) {
            //std::clog << "3";
            TI prev = base_refe();
            while (prev != beg and not is_lexical(*base_refe())) {
                //std::clog << "4";
                prev = base_refe();
                // condittional added on account of stl debug checks
                if (base_refe() != beg) --base_refe();
                //std::clog << "/4";
            }
            base_refe() = prev;
            //std::clog << "/3";
        }
    }
    //std::clog << "/1\n";
}

////////////////////////////////////////////////////////////////////////////////

template <class TI>
TI&
collapsed_signature_iterator<TI>::base_refe()
{
    return collapsed_signature_iterator::iterator_adaptor_::base_reference();
}

////////////////////////////////////////////////////////////////////////////////

template <class TI>
TI const&
collapsed_signature_iterator<TI>::base_refe() const
{
    return collapsed_signature_iterator::iterator_adaptor_::base_reference();
}

////////////////////////////////////////////////////////////////////////////////


} // namespace xrsdb

# endif //     XRSDB__COLLAPSED_SIGNATURE_ITERATOR_HPP
