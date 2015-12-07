
// JTC1/SC22/WG21 N1456 Hash table implementation
// http://std.dkuug.dk/jtc1/sc22/wg21/docs/papers/2003/n1456.html

// boost/unordered_map.hpp

// Copyright © 2003-2004 Jeremy B. Maitin-Shepard.

// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy
// at http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_UNORDERED_MAP_HPP_INCLUDED
#define BOOST_UNORDERED_MAP_HPP_INCLUDED

#include <functional>
#include <memory>

#include <boost/detail/hash_table.hpp>

namespace boost {

  template <class Key,
            class T,
            class Hash = hash<Key>,
            class Pred = std::equal_to<Key>,
            class Alloc = std::allocator<std::pair<const Key, T> > >
  class unordered_map
    : public detail::hash_table<std::pair<const Key, T>, Key,
                                      Hash, Pred, Alloc, false> {
    
    typedef detail::hash_table<
      std::pair<const Key, T>, Key, Hash,
      Pred, Alloc, false
    > base;

    typedef typename base::node node;
    
  public:
    
    typedef T mapped_type;
    typedef typename base::key_type key_type;
    typedef typename base::size_type size_type;
    typedef typename base::hasher hasher;
    typedef typename base::key_equal key_equal;
    typedef typename base::allocator_type allocator_type;
    
    explicit unordered_map(size_type n = detail::hash::
                           default_initial_bucket_count,
                           const hasher &hf = hasher(),
                           const key_equal &eql = key_equal(),
                           const allocator_type &a = allocator_type())
      : base(n, hf, eql, a) {}

    template <class InputIterator>
    explicit unordered_map(InputIterator f, InputIterator l,
                           size_type n = detail::hash::
                           default_initial_bucket_count,
                           const hasher &hf = hasher(),
                           const key_equal &eql = key_equal(),
                           const allocator_type &a = allocator_type())
      : base(f, l, n, hf, eql, a) {}

    mapped_type &operator[](const key_type &k) {
      base::reserve(base::size());
      
      size_type i = base::bucket(k);
      node **node_pointer = base::buckets + i;
      while (*node_pointer && !base::equal(k, *node_pointer))
        node_pointer = &((*node_pointer)->next);

      if (*node_pointer)
        return (**node_pointer).value.second;
      
      else
        return base::insert_node(base::value_type(k, mapped_type()),
                                 node_pointer, i)->value.second;
    }
      T *find_second(const key_type &k) const {
		size_type i = bucket(k);
        for (node *node_pointer = buckets[i];
             node_pointer;
             node_pointer = node_pointer->next)
		if (equal(k, node_pointer))
		  return &(node_pointer->value.second);
		return NULL;
	  }

  }; // class template unordered_map

  template <class K, class T, class H, class P, class A>
  void swap(unordered_map<K, T, H, P, A> &m1,
            unordered_map<K, T, H, P, A> &m2) {
    m1.swap(m2);
  }

  template <class Key,
            class T,
            class Hash = hash<Key>,
            class Pred = std::equal_to<Key>,
            class Alloc = std::allocator<std::pair<const Key, T> > >
  class unordered_multimap
    : public detail::hash_table<std::pair<const Key, T>, Key,
                                      Hash, Pred, Alloc, true> {
    
    typedef detail::hash_table<
      std::pair<const Key, T>, Key, Hash,
      Pred, Alloc, true
    > base;
    
  public:

    typedef T mapped_type;
    typedef typename base::size_type size_type;
    typedef typename base::hasher hasher;
    typedef typename base::key_equal key_equal;
    typedef typename base::allocator_type allocator_type;
    
    explicit unordered_multimap(size_type n = detail::hash::
                                default_initial_bucket_count,
                                const hasher &hf = hasher(),
                                const key_equal &eql = key_equal(),
                                const allocator_type &a
                                = allocator_type())
      : base(n, hf, eql, a) {}

    template <class InputIterator>
    explicit unordered_multimap(InputIterator f, InputIterator l,
                                size_type n = detail::hash::
                                default_initial_bucket_count,
                                const hasher &hf = hasher(),
                                const key_equal &eql = key_equal(),
                                const allocator_type &a
                                = allocator_type())
      : base(f, l, n, hf, eql, a) {}

  }; // class template unordered_multimap

  template <class K, class T, class H, class P, class A>
  void swap(unordered_multimap<K, T, H, P, A> &m1,
            unordered_multimap<K, T, H, P, A> &m2) {
    m1.swap(m2);
  }


} // namespace boost
/*
template <class K, class T, class H, class P, class A>
inline std::pair<const K,V> * find_value (const unordered_map<K, T, H, P, A> &ht,const K &k) {
  typedef unordered_map<K, T, H, P, A> &ht HT;
  if ((HT::iterator i=ht.find(k)) != ht.end())
	return &*i;
  else
	return NULL;
}


template <class K, class T, class H, class P, class A>
inline T* find_second (const unordered_map<K, T, H, P, A> &ht,const K &k) {
  typedef unordered_map<K, T, H, P, A> &ht HT;
  if ((HT::iterator i=ht.find(k)) != ht.end())
	return i->second;
  else
	return NULL;
}
*/

#endif // BOOST_UNORDERED_MAP_HPP_INCLUDED
