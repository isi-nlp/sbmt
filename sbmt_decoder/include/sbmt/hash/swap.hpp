#ifndef   SBMT_HASH_SWAP_HPP
#define   SBMT_HASH_SWAP_HPP

#include <algorithm>

namespace std {

template <class T>
void resolve_swap(T& x, T& y)
{
    ////////////////////////////////////////////////////////////////////////////
    ///
    /// if sbmt::T has a free function sbmt::swap(sbmt::T&,sbmt::T&)
    /// then that function is used.  otherwise, std::swap is used.
    /// note: you cant do this in a member swap function like
    ///
    /// \code
    /// void sbmt::T::swap(sbmt::T& other)
    /// { using std::swap; swap(x, other.x); }
    /// \endcode
    ///
    /// std::swap will always be selected.  hence, the use of resolve_swap
    ///
    ////////////////////////////////////////////////////////////////////////////
    using std::swap;
    swap(x,y);
}

} // namespace sbmt

#endif // SBMT_HASH_SWAP_HPP
