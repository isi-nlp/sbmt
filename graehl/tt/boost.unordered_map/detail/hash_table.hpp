
// JTC1/SC22/WG21 N1456 Hash table implementation
// http://std.dkuug.dk/jtc1/sc22/wg21/docs/papers/2003/n1456.html

// boost/detail/hash_table.hpp

// Copyright © 2003-2004 Jeremy B. Maitin-Shepard.

// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy
// at http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_DETAIL_HASH_TABLE_HPP_INCLUDED
#define BOOST_DETAIL_HASH_TABLE_HPP_INCLUDED

#include <cstddef>
#include <algorithm>
#include <utility>
#include <functional>
#include <string>

#include <boost/config.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/iterator_traits.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/mpl/if.hpp>

namespace boost {

  template <class T> struct hash;

  namespace detail {

    namespace hash {
      const static std::size_t default_initial_bucket_count = 8;
      inline std::size_t next_prime(std::size_t n);
      template <class> struct equivalent_keys_operations;
      template <class> struct unique_keys_operations;
    }

    template <class ValueType, class KeyType,
              class Hash, class Pred,
              class Alloc, bool EquivalentKeys>
    class hash_table {

      typedef hash_table<
        ValueType, KeyType, Hash, Pred, Alloc, EquivalentKeys
      > table_type;

      // Select insert, erase, equal_range, count implementation

      template <class> friend struct hash::equivalent_keys_operations;
      template <class> friend struct hash::unique_keys_operations;

      template <template <class> class X>
      struct operations_selector {
        template <class TableType>
        struct apply {
          typedef X<TableType> type;
        };
      };

      typedef typename mpl::if_c<
        EquivalentKeys,
        operations_selector<hash::equivalent_keys_operations>,
        operations_selector<hash::unique_keys_operations>
        >::type::apply<table_type>::type operations;

    public:
    
      // Type definitions
      
      typedef KeyType key_type;
      typedef ValueType value_type;
      typedef Hash hasher;
      typedef Pred key_equal;
      typedef Alloc allocator_type;
      typedef typename allocator_type::pointer pointer;
      typedef typename allocator_type::const_pointer const_pointer;
      typedef typename allocator_type::reference reference;
      typedef typename allocator_type::const_reference const_reference;
      typedef std::size_t size_type;
      typedef std::ptrdiff_t difference_type;
      
    protected:

      // node handling

      struct node {
        node(value_type const &value, node *next)
          : value(value), next(next) {}
        value_type value;
        node *next;
      };

    private:

      typedef typename Alloc::rebind<node>::other node_allocator;
      
      // data members

      node_allocator alloc;
      hasher hf;
      key_equal eq;
      size_type size_;

    protected:
      
      node **buckets;

    private:
      
      size_type bucket_count_;
      float mlf;
      size_type max_load;
      size_type cached_begin_bucket;

    public:

      // accessors

      allocator_type get_allocator() const {
        return allocator_type(alloc);
      }

      hasher hash_function() const {
        return hf;
      }

      key_equal key_eq() const {
        return eq;
      }

      size_type size() const {
        return size_;
      }

      bool empty() const {
        return size() == 0;
      }

      size_type max_size() const {
        return alloc.max_size();
      }
      
      float max_load_factor() const {
        return mlf;
      }

      size_type bucket(const key_type &k) const {
        return hf(k) % bucket_count();
      }

      size_type bucket_count() const {
        return bucket_count_;
      }

      size_type max_bucket_count() const {
        return get_bucket_allocator().max_size();
      }

      size_type bucket_size(size_type n) {
        size_type count = 0;
        for (node *node_pointer = buckets[n];
             node_pointer;
             node_pointer = node_pointer->next)
          ++count;
        return count;
      }

    private:
      
      void calculate_max_load() {
        max_load = static_cast<size_type>(mlf * bucket_count_);
      }

    public:

      void max_load_factor(float z) {
        mlf = z;
        calculate_max_load();
      }

      float load_factor() const {
        return size() / bucket_count();
      }

      // key extractors

    private:

      template <class V, class K>
      struct key_extractor {
        static K const &extract_key(const V &v) {
          return v.first;
        }
      };

      template <class K>
      struct key_extractor<K, K> {
        static K const &extract_key(const K &v) {
          return v;
        }
      };

      static const key_type &extract_key(const value_type &v) {
        return key_extractor<value_type, key_type>
          ::extract_key(v);
      }

    protected:

      bool equal(const key_type &k, node *n) const {
        return eq(k, extract_key(n->value));
      }

      // iterators

    private:

      template <class T>
      class iterator_base
        : public boost::iterator_facade<
            iterator_base<T>,
            T,
            boost::forward_traversal_tag
          > {
        
        friend class boost::iterator_core_access;
        friend class table_type;

        template <class> friend class iterator_base;

      protected:

        table_type const *table_pointer;
        node *node_pointer;

        explicit iterator_base(node *node_pointer,
                               table_type const *
                               table_pointer)
          : node_pointer(node_pointer),
            table_pointer(table_pointer) {}

        iterator_base()
          : table_pointer(0), node_pointer(0) {}

      private:

        T &dereference() const {
          return node_pointer->value;
        }

        template <class A>
        bool equal(iterator_base<A> const &y) const {
          return node_pointer == y.node_pointer
            && table_pointer == y.table_pointer;
        }

        void increment() {
          if (node_pointer->next)
            node_pointer = node_pointer->next;
          else {

            size_type index
              = table_pointer->bucket(extract_key(node_pointer->value));
          
            node_pointer = 0;
            
            while (++index < table_pointer->bucket_count()) {
              if (table_pointer->buckets[index]) {
                node_pointer = table_pointer->buckets[index];
              break;
              }
            }
          }
        }
      };

    public:

      class const_iterator;

      class iterator : public iterator_base<value_type> {
        friend class table_type;
        template <class> friend struct hash::equivalent_keys_operations;
        template <class> friend struct hash::unique_keys_operations;      
        friend class const_iterator;
        
        typedef iterator_base<
          typename table_type::value_type
        > iterator_base_type;
        
        explicit iterator(node *n, table_type const *t)
          : iterator_base_type(n, t) {}

      public:

        iterator() {}
      };

      class const_iterator : public iterator_base<value_type const> {
        
        friend class table_type;
        template <class> friend struct hash::equivalent_keys_operations;
        template <class> friend struct hash::unique_keys_operations;      

        typedef iterator_base<
          typename table_type::value_type const
        > iterator_base_type;
        
        explicit const_iterator(node *n, table_type const *t)
          : iterator_base_type(n, t) {}

      public:
        
        const_iterator() {}
        const_iterator(iterator i)
          : iterator_base_type(i.node_pointer, i.table_pointer) {}
        
      };

      // local iterators

      class const_local_iterator;
	  typedef iterator find_return_type;

    private:

      template <class T>
      class local_iterator_base
        : public boost::iterator_facade<
            local_iterator_base<T>,
            T,
            boost::forward_traversal_tag
          > {

        friend class boost::iterator_core_access;

        template <class> friend class local_iterator_base;
        friend class const_local_iterator;

        node *node_pointer;

      protected:

        explicit local_iterator_base(node *node_pointer)
          : node_pointer(node_pointer) {}

        local_iterator_base()
          : node_pointer(0) {}

      private:

        template <class A>
        bool equal(local_iterator_base<A> const &y) const {
          return node_pointer == y.node_pointer;
        }

        T &dereference() const {
          return node_pointer->value;
        }

        void increment() {
          node_pointer = node_pointer->next;
        }
      };

    public:

      class local_iterator : public local_iterator_base<value_type> {

        friend class table_type;

        typedef local_iterator_base<
          typename table_type::value_type
        > iterator_base_type;

        explicit local_iterator(node *node_pointer)
          : iterator_base_type(node_pointer) {}
        
      public:

        local_iterator() {}
      };

      class const_local_iterator
        : public local_iterator_base<value_type const> {

        friend class table_type;

        typedef local_iterator_base<
          typename table_type::value_type const
        > iterator_base_type;

        explicit const_local_iterator(node *node_pointer)
          : iterator_base_type(node_pointer) {}
        
      public:

        const_local_iterator() {}

        const_local_iterator(local_iterator i)
          : iterator_base_type(i.node_pointer) {}
      };

      // iterator accessors

      iterator end() {
        return iterator(0, this);
      }

      iterator begin() {

        if (empty())
          return end();

        else
          return iterator(buckets[cached_begin_bucket], this);
        
      }

      const_iterator end() const {
        return const_iterator(0, this);
      }

      const_iterator begin() const {

        if (empty())
          return end();

        else
          return const_iterator(buckets[cached_begin_bucket], this);
      }

      local_iterator begin(size_type n) {
        return local_iterator(buckets[n]);
      }

      local_iterator end(size_type) {
        return local_iterator(0);
      }

      const_local_iterator begin(size_type n) const {
        return const_local_iterator(buckets[n]);
      }

      const_local_iterator end(size_type) const {
        return const_local_iterator(0);
      }

      // bucket allocation and rehashing

    private:

      typedef typename Alloc::rebind<node *>::other bucket_allocator;

      bucket_allocator get_bucket_allocator() {
        return bucket_allocator(alloc);
      }

      void initialize_buckets(size_type n) {
        n = hash::next_prime(n);

        // set this first in case of an exception
        buckets = 0;

        buckets = get_bucket_allocator().allocate(n);
        std::fill(buckets, buckets + n, (node *)0);

        bucket_count_ = n;
        calculate_max_load();

        cached_begin_bucket = bucket_count();
      }

      void destroy_node(node *n) {
        n->~node(); // must not throw
        try {
          alloc.deallocate(n, 1);
        } catch(...) {} // ignore exceptions
      }        

      size_type destroy_bucket_nodes(node *node_pointer) {
        size_type count = 0;
        for (node *next; node_pointer; node_pointer = next) {
          ++count;
          next = node_pointer->next;
          destroy_node(node_pointer);
        }
        return count;
      }

      size_type destroy_bucket_nodes(node **bucket_array, size_type n) {
        size_type count = 0;
        for (size_type i = 0; i < n; ++i)
          count += destroy_bucket_nodes(bucket_array[i]);
        return count;
      }

    public:

      void rehash(size_type n) {
        n = hash::next_prime(n);
        if (n == bucket_count())
          return; // nothing to do

        cached_begin_bucket = n;
        node **new_buckets = 0;
        try {
          // allocate and initialize new buckets
          new_buckets = get_bucket_allocator().allocate(n);
          std::fill(new_buckets, new_buckets + n, (node *)0);

          // move the nodes to the new buckets
          for (size_type i = 0; i < bucket_count(); ++i) {
            for (node *node_pointer = buckets[i], *next;
                 node_pointer;
                 node_pointer = next) {
              next = node_pointer->next;

              // This next line throws iff the hash function throws.
              size_type j = hf(extract_key(node_pointer->value)) % n;

              // Note: Because equivalent elements are already
              // adjacent to each other in the existing buckets, this
              // simple rehashing technique is sufficient to ensure
              // that they remain adjacent to each other in the new
              // buckets (but in reverse order).
              
              node_pointer->next = new_buckets[j];
              new_buckets[j] = node_pointer;

              if (j < cached_begin_bucket)
                cached_begin_bucket = j;
            }
          }
        } catch(...) {
          if (new_buckets) { // the hash function threw an exception
            // destroy nodes in new buckets
            //size_ -= destroy_bucket_nodes(new_buckets, n); // WTF.  why destroy?  and why decrease size_ to negative?
            try {
              get_bucket_allocator().deallocate(new_buckets, n);
            } catch (...) {} // ignore exceptions
			
          }
          throw;
        }

        // deallocate old buckets
        try {
          get_bucket_allocator().deallocate(buckets, bucket_count());
        } catch(...) {} // ignore exceptions

        // update bucket state information
        bucket_count_ = n;
        calculate_max_load();
        buckets = new_buckets;
      }

    protected:

      void reserve(size_type n) {
        if (n > max_load)
          rehash(static_cast<size_type>(n/mlf)+1);
      }

      // insert

      node *insert_node(const value_type &v, node **position,
                        size_type i) {
        
        node *new_node = alloc.allocate(1, *position);
        try {
            new(new_node) node(v, *position);
        } catch(...) {
          new_node->~node(); // must not throw
          try {
            alloc.deallocate(new_node, 1);
          } catch(...) {} // ignore exceptions
          throw;
        }
        
        if (i < cached_begin_bucket) cached_begin_bucket = i;

        *position = new_node;
        ++size_;
        return new_node;
      }

    public:
	  typedef typename operations::insert_return_type insert_return_type;
      typename operations::insert_return_type
      insert(const value_type &v) {
        reserve(size());
        const key_type &k = extract_key(v);
        size_type i = bucket(k);
        node **node_pointer = buckets + i;
        while (*node_pointer && !equal(k, *node_pointer))
          node_pointer = &((*node_pointer)->next);
        return operations::insert(this, v, i, node_pointer);
      }

      iterator insert(iterator, const value_type &v) {
        return operations
          ::get_iterator(insert(v));
      }

      // range insert

    private:

      struct reserve_for_random_access_range_impl {
        template <class I>
        struct apply {
          static void reserve(I i, I j, table_type *table) {
            table->reserve(table->size() + (j - i));
          }
          
          static void initialize(I i, I j, table_type *table, size_type x) {
            size_type d = j - i;
            size_type n = std::max(x, static_cast<size_type>
                                   (table->size() + d)/table->mlf );
            table->initialize_buckets(n);
          }
        };
      };

      struct reserve_for_generic_range_impl {
        template <class I>
        struct apply {
          static void reserve(I, I, table_type *) {}
          static void initialize(I, I, table_type *table, size_type i) {
            table->initialize_buckets(i);
          }
        };
      };

      template <class I>
      struct reserve_for_range
        : mpl::if_<
            boost::is_convertible<
              typename boost::iterator_traversal<I>::type,
              boost::random_access_traversal_tag
            >,
            reserve_for_random_access_range_impl,
            reserve_for_generic_range_impl
          >::type::apply<I> {};

    public:

      template <class InputIterator>
      void insert(InputIterator i, InputIterator j) {
        reserve_for_range<InputIterator>::reserve(i, j, this);
        for (; i != j; ++i)
          insert(*i);
      }

      // erase

    private:

      void erase_node(node *n) {
        destroy_node(n);
        --size_;
      }
      
      void recompute_begin_bucket(size_type i) {
        if (i == cached_begin_bucket
            && !buckets[i]) {
          if (!empty()) {
            while (!buckets[++cached_begin_bucket]);
          } else {
            cached_begin_bucket = bucket_count();
          }
        }
      }

    public:

      void erase(const_iterator r) {
        size_type i = bucket(extract_key(*r));
        node **node_pointer = buckets + i;
        while (*node_pointer != r.node_pointer)
          node_pointer = &((*node_pointer)->next);
        node *n = *node_pointer;
        *node_pointer = n->next;
        erase_node(n);
        recompute_begin_bucket(i);
      }
      
    public:

      void erase(const_iterator r1, const_iterator r2) {
        size_type i = bucket(extract_key(*r1)), x = i;
        size_type j;
        if (r2.node_pointer) {
          j = bucket(extract_key(r2.node_pointer->value));
          node **node_pointer = buckets + i;
          while (*node_pointer != r1.node_pointer)
            node_pointer = &((*node_pointer)->next);
          if (i == j) {
            node *n = *node_pointer, *next;
            *node_pointer = r2.node_pointer;
            while (true) {
              next = n->next;
              erase_node(n);
              if (next == r2.node_pointer)
                break;
              else
                n = next;
            }
          } else {
            size_ -= destroy_bucket_nodes(r1.node_pointer);
            *node_pointer = 0;
            while (++i < j) {
              size_ -= destroy_bucket_nodes(buckets[i]);
              buckets[i] = 0;
            }
            node *n = buckets[j];
            while (true) {
              node *next = n->next;
              erase_node(n);
              if (next == r2.node_pointer)
                break;
              else
                n = next;
            }
            buckets[j] = r2.node_pointer;
          }
        } else {
          // erase to end
          node **node_pointer = buckets + i;
          while (*node_pointer != r1.node_pointer)
            node_pointer = &((*node_pointer)->next);
          *node_pointer = 0;
          size_ -= destroy_bucket_nodes(r1.node_pointer);
          while (++i < bucket_count()) {
            size_ -= destroy_bucket_nodes(buckets[i]);
            buckets[i] = 0;
          }
        }
        recompute_begin_bucket(i);
      }

      size_type erase(const key_type &k) {
        size_type i = bucket(k);
        for (node **node_pointer = buckets + i;
             *node_pointer;
             node_pointer = &((*node_pointer)->next))
          if (equal(k, *node_pointer)) {
            size_type count = operations::erase(this, k, node_pointer);
            recompute_begin_bucket(i);
            return count;
          }
        return 0;
      }

      // clear

      void clear() {
        for (size_type i = 0; i < bucket_count() ; ++i) {
          for (node *node_pointer = buckets[i], *next;
               node_pointer;
               node_pointer = next) {
            next = node_pointer->next;
            destroy_node(node_pointer);
          }
          buckets[i] = 0;
        }
        cached_begin_bucket = bucket_count();
        size_ = 0;
      }

      // count

      size_type count(const key_type &k) const {
        size_type i = bucket(k);
        for (node *node_pointer = buckets[i];
             node_pointer;
             node_pointer = node_pointer->next)
          if (equal(k, node_pointer))
            return operations::count(this, k, node_pointer);
        return 0;
      }

      // find

      iterator find(const key_type &k) {
        size_type i = bucket(k);
        for (node *node_pointer = buckets[i];
             node_pointer;
             node_pointer = node_pointer->next)
          if (equal(k, node_pointer))
            return iterator(node_pointer, this);
        return end();
      }

      const_iterator find(const key_type &k) const {
        size_type i = bucket(k);
        for (node *node_pointer = buckets[i];
             node_pointer;
             node_pointer = node_pointer->next)
          if (equal(k, node_pointer))
            return const_iterator(node_pointer, this);
        return end();
      }

	  value_type *find_value(const key_type &k) const {
		size_type i = bucket(k);
        for (node *node_pointer = buckets[i];
             node_pointer;
             node_pointer = node_pointer->next)
		if (equal(k, node_pointer))
		  return &(node_pointer->value);
		return NULL;
	  }
      // equal_range

    private:
      
      void increment_node(node *&n, size_type index) const {
        if (n->next)
          n = n->next;
        else {
          n = 0;
          while (++index < bucket_count()) {
            if (buckets[index]) {
              n = buckets[index];
              break;
            }
          }
        }
      }
      
    public:

      std::pair<iterator, iterator> equal_range(const key_type &k) {
        size_type i = bucket(k);
        for (node *node_pointer = buckets[i];
             node_pointer;
             node_pointer = node_pointer->next)
          if (equal(k, node_pointer)) {
            node *second = operations::equal_range(this, k, node_pointer);
            increment_node(second, i);
            return std::pair<iterator, iterator>
              (iterator(node_pointer, this), iterator(second, this));
          }
        return std::pair<iterator, iterator>(end(), end());
      }

      std::pair<const_iterator,
                const_iterator> equal_range(const key_type &k) const {
        size_type i = bucket(k);
        for (node *node_pointer = buckets[i];
             node_pointer;
             node_pointer = node_pointer->next)
          if (equal(k, node_pointer)) {
            node *second = operations::equal_range(this, k, node_pointer);
            increment_node(second, i);
            return std::pair<const_iterator, const_iterator>
              (const_iterator(node_pointer, this),
               const_iterator(second, this));
          }
        return std::pair<const_iterator, const_iterator>
          (end(), end());
      }

      // swap

      void swap(table_type &x) {
        std::swap(size_, x.size_);
        std::swap(hf, x.hf);
        std::swap(eq, x.eq);
        std::swap(mlf, x.mlf);
        std::swap(alloc, x.alloc);
        std::swap(max_load, x.max_load);
        std::swap(buckets, x.buckets);
        std::swap(bucket_count_, x.bucket_count_);
        std::swap(cached_begin_bucket, x.cached_begin_bucket);
      }

      // assignment

      table_type &operator=(table_type &x) {
        clear();
        hf = x.hf;
        eq = x.eq;
        alloc = x.alloc;
        mlf = x.mlf;
        reserve(x.size());
        for (iterator i = x.begin(); i != x.end(); ++i)
          insert(*i);
      }

      // constructors

      explicit hash_table(size_type n,
                                const hasher &hf,
                                const key_equal &eq,
                                const allocator_type &a)
        : hf(hf), eq(eq), alloc(a), size_(0), mlf(0.8f)
      {
        initialize_buckets(n);
      }

      template <class InputIterator>
      explicit hash_table(InputIterator f, InputIterator l,
                                size_type n,
                                const hasher &hf,
                                const key_equal &eq,
                                const allocator_type &a)
        : hf(hf), eq(eq), alloc(a), size_(0), mlf(0.8f)
      {
        reserve_for_range<InputIterator>::initialize(f, l, this, n);
        for (; f != l; ++f)
          insert(*f);
      }

      explicit hash_table(table_type const &x)
        : hf(x.hf), eq(x.eq), alloc(x.alloc), size_(0), mlf(0.8f)
      {
        initialize_buckets(static_cast<size_type>(x.size()/mlf));
        for (iterator i = x.begin(); i != x.end(); ++i)
          insert(*i);
      }

      // destructor

      ~hash_table() {
        if (buckets) {
          destroy_bucket_nodes(buckets, bucket_count());
          try {
            get_bucket_allocator().deallocate(buckets, bucket_count());
          } catch(...) {} // ignore exceptions
        }
      }

    }; // class template hash_table
    
    namespace hash {
      
      inline std::size_t hash_bytes(unsigned char *begin,
                                    std::size_t count) {
        unsigned char *end = begin + count;
        std::size_t result = 0;
        while (end - begin > sizeof(std::size_t)) {
          result ^= *(reinterpret_cast<std::size_t *>(begin));
          begin += sizeof(std::size_t);
        }

        std::size_t remainder = 0;
        std::copy(begin, end,
                  reinterpret_cast<unsigned char *>(&remainder));
        result ^= remainder;
        return result;
      }
      
    } // namespace boost::detail::hash
  } // namespace boost::detail


  // hash function specializations

  template <>
  struct hash<bool>
    : std::unary_function<bool, std::size_t> {
    std::size_t operator()(bool val) const {
      return static_cast<std::size_t>(val);
    }
  };

  template <>
  struct hash<char>
    : std::unary_function<char, std::size_t> {
    std::size_t operator()(char val) const {
      return static_cast<std::size_t>(val);
    }
  };  

  template <>
  struct hash<signed char>
    : std::unary_function<signed char, std::size_t> {
    std::size_t operator()(signed char val) const {
      return static_cast<std::size_t>(val);
    }
  };

  template <>
  struct hash<unsigned char>
    : std::unary_function<unsigned char, std::size_t> {
    std::size_t operator()(unsigned char val) const {
      return static_cast<std::size_t>(val);
    }
  };

#ifndef BOOST_NO_INTRINSIC_WCHAR_T
  template <>
  struct hash<wchar_t>
    : std::unary_function<wchar_t, std::size_t> {
    std::size_t operator()(wchar_t val) const {
      return static_cast<std::size_t>(val);
    }
  };
#endif 

  template <>
  struct hash<short>
    : std::unary_function<short, std::size_t> {
    std::size_t operator()(short val) const {
      return static_cast<std::size_t>(val);
    }
  };

  template <>
  struct hash<int>
    : std::unary_function<int, std::size_t> {
    std::size_t operator()(int val) const {
      return static_cast<std::size_t>(val);
    }
  };

  template <>
  struct hash<long>
    : std::unary_function<long, std::size_t> {
    std::size_t operator()(long val) const {
      return static_cast<std::size_t>(val);
    }
  };

  template <>
  struct hash<unsigned short>
    : std::unary_function<unsigned short, std::size_t> {
    std::size_t operator()(unsigned short val) const {
      return static_cast<std::size_t>(val);
    }
  };

  template <>
  struct hash<unsigned int>
    : std::unary_function<unsigned int, std::size_t> {
    std::size_t operator()(unsigned int val) const {
      return static_cast<std::size_t>(val);
    }
  };

  template <>
  struct hash<unsigned long>
    : std::unary_function<unsigned long, std::size_t> {
    std::size_t operator()(unsigned long val) const {
      return static_cast<std::size_t>(val);
    }
  };

  template <>
  struct hash<float>
    : std::unary_function<float, std::size_t> {
    std::size_t operator()(float val) const {
      return detail::hash::hash_bytes
        (reinterpret_cast<unsigned char *>(&val), sizeof(float));
    }
  };

  template <>
  struct hash<double>
    : std::unary_function<double, std::size_t> {
    std::size_t operator()(double val) const {
      return detail::hash::hash_bytes
        (reinterpret_cast<unsigned char *>(&val), sizeof(double));
    }
  };

  template <>
  struct hash<long double>
    : std::unary_function<long double, std::size_t> {
    std::size_t operator()(long double val) const {
      return detail::hash::hash_bytes
        (reinterpret_cast<unsigned char *>(&val),
         sizeof(long double));
    }
  };

  template <class T>
  struct hash<T *>
    : std::unary_function<T *, std::size_t> {
    std::size_t operator()(T *val) const {
      return static_cast<std::size_t>(val - (T *)0);
    }
  };

  template <class charT, class traits, class Allocator>
  struct hash<std::basic_string<charT, traits, Allocator> > {
    std::size_t operator()
      (std::basic_string<charT, traits, Allocator> const &val) const {
      std::size_t value = 0;
      for(typename std::basic_string<charT, traits, Allocator>
            ::const_iterator it = val.begin();
          it != val.end();
          ++it) {
        value *= 31;
        value += static_cast<std::size_t>(*it);
      }
      return value;
    }
  };

    template<>
  struct hash<const char *> {
    std::size_t operator()
      (const char *p) const {
		std::size_t h=0;
	  	while (*p != '\0')
		  h = 31 * h + *p++; // should optimize to ( h << 5 ) - h if faster
	  return h;
	  }
  };

      

  namespace detail {
  
    namespace hash {

      // prime number list, accessor

      static const std::size_t prime_list[] = { 13ul, 29ul,
        53ul, 97ul, 193ul, 389ul, 769ul,
        1543ul, 3079ul, 6151ul, 12289ul, 24593ul,
        49157ul, 98317ul, 196613ul, 393241ul, 786433ul,
        1572869ul, 3145739ul, 6291469ul, 12582917ul, 25165843ul,
        50331653ul, 100663319ul, 201326611ul, 402653189ul, 805306457ul,
        1610612741ul, 3221225473ul, 4294967291ul };

      inline std::size_t next_prime(std::size_t n) {
        std::size_t const *bound;
		static const size_t n_prime=sizeof(prime_list)/sizeof(prime_list[0])+1;
        bound = std::upper_bound(prime_list,prime_list + n_prime, n);
		//static const std::size_t biggest = prime_list + n_prime - 1;
        if(bound == prime_list + n_prime )
          --bound;
        return *bound;
      }

      // specialized operations

      template <class TableType>
      struct equivalent_keys_operations {

        typedef typename TableType::iterator iterator;
        typedef typename TableType::node node;
        typedef typename TableType::key_type key_type;
        typedef typename TableType::value_type value_type;
        typedef typename TableType::size_type size_type;

        typedef iterator insert_return_type;

        static iterator get_iterator(insert_return_type const &r) {
          return r;
        }

        static insert_return_type
        insert(TableType *table, const value_type &v, size_type i,
               node **node_pointer) {
          node *new_node = table->insert_node(v, node_pointer, i);
          return iterator(new_node, table);
        }

        static size_type erase(TableType *table, const key_type &k,
                               node **node_pointer) {
          size_type count = 0;
          node *next;
          node *n = *node_pointer;
          while (true) {
            next = n->next;
            ++count;
            n->~node(); // must not throw
            try {
              table->alloc.deallocate(n, 1);
            } catch(...) {} // ignore exceptions
            --table->size_;
            try {
              if (next && table->equal(k, next))
                n = next;
              else
                break;
            } catch(...) {
              *node_pointer = next;
              throw;
            }
          }
          *node_pointer = next; // fix linked list
          return count;
        }

        static size_type count(TableType const *table,
                               const key_type &k,
                               node *node_pointer) {
          size_type count = 0;
          do {
            ++count;
            node_pointer = node_pointer->next;
          } while (node_pointer && table->equal(k, node_pointer));
          return count;
        }

        static node *equal_range(TableType const *table,
                                 const key_type &k,
                                 node *node_pointer) {
          while (node_pointer->next && table->equal(k, node_pointer))
            node_pointer = node_pointer->next;
          return node_pointer;
        }
      };

      template <class TableType>
      struct unique_keys_operations {
        typedef typename TableType::iterator iterator;
        typedef typename TableType::node node;
        typedef typename TableType::key_type key_type;
        typedef typename TableType::value_type value_type;
        typedef typename TableType::size_type size_type;
          
        typedef std::pair<iterator, bool> insert_return_type;
          
        static iterator get_iterator(insert_return_type const &r) {
          return r.first;
        }
          
        static insert_return_type
        insert(TableType *table, const value_type &v, size_type i,
               node **node_pointer) {
          if (*node_pointer) {
            // key already exists in table
            return insert_return_type
              (iterator(*node_pointer, table), false);
          } else {
            node *new_node = table->insert_node(v, node_pointer, i);
            return insert_return_type(iterator(new_node, table), true);
          }
        }

        static size_type erase(TableType *table, const key_type &k,
                               node **node_pointer) {
          node *n = *node_pointer;
          *node_pointer = n->next;
          n->~node(); // must not throw
          try {
            table->alloc.deallocate(n, 1);
          } catch(...) {} // ignore exceptions
          --table->size_;
          return 1;
        }

        static size_type count(TableType const *table,
                               const key_type &k,
                               node *node_pointer) {
          return 1;
        }
        
        static node *equal_range(TableType const *,
                                 const key_type &,
                                 node *node_pointer) {
          return node_pointer;
        }
      };

    } // namespace boost::detail::hash
  
  } // namespace boost::detail
} // namespace boost

#endif // BOOST_DETAIL_HASH_TABLE_HPP_INCLUDED
