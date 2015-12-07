# if !defined(SBMT__HASH__CLONEABLE_HPP)
# define      SBMT__HASH__CLONEABLE_HPP

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////
///
///  allows for polymorphic deep copies and throws, without knowing the 
///  derived type:
///  \code
///    struct myclass : virtual cloneable {
///        void throw_self() const { throw *this; }
///        void cloneable* clone_self() const { return new myclass(*this); }
///     //... rest of implementation
///    };
///
///  \note: it is possible that c++0x will include this class or a variant,
///  and that all standard exceptions will inherit from it, although it is not
///  clear to me how this will play nicely with the bad_alloc exception.
///  in fact, in sbmt::threadpool, throwing of bad_allocs is handled without
///  the use of cloneable.
///
////////////////////////////////////////////////////////////////////////////////
struct cloneable {
    virtual void throw_self() const = 0;
    virtual cloneable* clone_self() const = 0;
    virtual ~cloneable() throw() {}
};

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

# endif   //  SBMT__HASH__CLONEABLE_HPP

