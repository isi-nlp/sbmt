# if ! defined(GUSC__MOD_SEQUENCE_HPP)
# define       GUSC__MOD_SEQUENCE_HPP

namespace gusc {

////////////////////////////////////////////////////////////////////////////////
///
///  outputs a sequence of mod-p digits for n, from most to least significance.
///  example: after
///  \code
///    vector<int> v;
///    mod_sequence(145,10, back_inserter(v));
///  \code
///  v contains 1, 4, 5
///
////////////////////////////////////////////////////////////////////////////////
template <class Int1, class Int2, class OutIterator>
OutIterator mod_sequence(Int1 n, Int2 p, OutIterator out, Int1 max = 0)
{
    Int1 r = n % p;
    Int1 nn = n / p;
    Int1 nm = max / p;
    if (nn != 0 || nm != 0) {
        out = mod_sequence(nn,p,out,nm);
    } 
    *out = r;
    ++out;
    return out;
}

} // namespace gusc

# endif //     GUSC__MOD_SEQUENCE_HPP
