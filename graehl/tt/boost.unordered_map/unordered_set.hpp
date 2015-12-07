
// JTC1/SC22/WG21 N1456 Hash table implementation
// http://std.dkuug.dk/jtc1/sc22/wg21/docs/papers/2003/n1456.html

// boost/unordered_set.hpp

// Copyright © 2003-2004 Jeremy B. Maitin-Shepard.

// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy
// at http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_UNORDERED_SET_HPP_INCLUDED
#define BOOST_UNORDERED_SET_HPP_INCLUDED

#include <functional>
#include <memory>

#include <boost/detail/hash_table.hpp>

namespace boost {

  template <class Value,
            class Hash = hash<Value>,
            class Pred = std::equal_to<Value>,
            class Alloc = std::allocator<Value> >
  class unordered_set : public detail::hash_table<Value, Value, Hash,
                                Pred, Alloc, false> {
    
    typedef detail::hash_table<
      Value, Value, Hash,
      Pred, Alloc, false
    > base;
    
  public:

    typedef typename base::size_type size_type;
    typedef typename base::hasher hasher;
    typedef typename base::key_equal key_equal;
    typedef typename base::allocator_type allocator_type;
    
    explicit unordered_set(size_type n = detail::hash::
                           default_initial_bucket_count,
                           const hasher &hf = hasher(),
                           const key_equal &eql = key_equal(),
                           const allocator_type &a = allocator_type())
      : base(n, hf, eql, a) {}

    template <class InputIterator>
    explicit unordered_set(InputIterator f, InputIterator l,
                           size_type n = detail::hash::
                           default_initial_bucket_count,
                           const hasher &hf = hasher(),
                           const key_equal &eql = key_equal(),
                           const allocator_type &a = allocator_type())
      : base(f, l, n, hf, eql, a) {}
    
  }; // class template unordered_set

  template <class T, class H, class P, class A>
  void swap(unordered_set<T, H, P, A> &m1,
            unordered_set<T, H, P, A> &m2) {
    m1.swap(m2);
  }

  template <class Value,
            class Hash = hash<Value>,
            class Pred = std::equal_to<Value>,
            class Alloc = std::allocator<Value> >
  class unordered_multiset
    : public detail::hash_table<Value, Value, Hash,
                                Pred, Alloc, true> {
    
    typedef detail::hash_table<
      Value, Value, Hash,
      Pred, Alloc, true
    > base;
    
  public:

    typedef typename base::size_type size_type;
    typedef typename base::hasher hasher;
    typedef typename base::key_equal key_equal;
    typedef typename base::allocator_type allocator_type;
    
    explicit unordered_multiset(size_type n = detail::hash::
                                default_initial_bucket_count,
                                const hasher &hf = hasher(),
                                const key_equal &eql = key_equal(),
                                const allocator_type &a
                                = allocator_type())
      : base(n, hf, eql, a) {}

    template <class InputIterator>
    explicit unordered_multiset(InputIterator f, InputIterator l,
                                size_type n = detail::hash::
                                default_initial_bucket_count,
                                const hasher &hf = hasher(),
                                const key_equal &eql = key_equal(),
                                const allocator_type &a
                                = allocator_type())
      : base(f, l, n, hf, eql, a) {}
  }; // class template unordered_multiset

  template <class T, class H, class P, class A>
  void swap(unordered_multiset<T, H, P, A> &m1,
            unordered_multiset<T, H, P, A> &m2) {
    m1.swap(m2);
  }
  
} // namespace boost

#endif // BOOST_UNORDERED_SET_HPP_INCLUDED
