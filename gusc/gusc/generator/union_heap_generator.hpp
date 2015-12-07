# if ! defined(GUSC__GENERATOR__UNION_HEAP_GENERATOR_HPP)
# define       GUSC__GENERATOR__UNION_HEAP_GENERATOR_HPP

# include <boost/utility/result_of.hpp>
# include <boost/iterator/iterator_facade.hpp>
# include <boost/shared_ptr.hpp>
# include <gusc/functional.hpp>
# include <queue>
# include <vector>
# include <iterator>

namespace gusc {

template <class Generator>
class union_heap_result_of {
    typedef typename boost::result_of<Generator()>::type gen_type;
public:
    typedef typename boost::result_of<gen_type()>::type type;
};

////////////////////////////////////////////////////////////////////////////////

template <class Generator, class Compare = less>
class union_heap_generator 
  : public boost::iterator_facade<
      union_heap_generator<Generator,Compare>
    , typename union_heap_result_of<Generator>::type const
    , std::input_iterator_tag
    >
{
    typedef typename boost::result_of<Generator()>::type gen_type;
public:
    typedef typename boost::result_of<gen_type()>::type result_type;
    explicit 
    union_heap_generator( Generator const& gen 
                        , Compare const& compare = Compare() ) 
      : impl(new impl_(gen,compare,std::numeric_limits<size_t>::max())) { init(1); }
    
    union_heap_generator( Generator const& gen 
                        , size_t lookahead
                        , Compare const& compare = Compare() ) 
      : impl(new impl_(gen,compare,std::numeric_limits<size_t>::max())) { init(lookahead); }
      
    union_heap_generator( Generator const& gen 
                        , size_t lookahead
                        , size_t 
                        , Compare const& compare = Compare() ) 
      : impl(new impl_(gen,compare,std::numeric_limits<size_t>::max())) { init(lookahead); }
    
    operator bool() const { return impl and not impl->heap.empty() and not empties_exhausted(); }
    
    result_type operator()()
    {
        result_type r(dereference());
        increment();
        return r;
    }
    union_heap_generator() {}
private:
    friend class boost::iterator_core_access;
    
    bool empties_exhausted() const
    {
        return impl->empty_count >= impl->empty_max;
    }
    
    void increment()
    {
        gen_type g(impl->heap.top());
        impl->heap.pop();
        ++g;
        if (g) {
            impl->heap.push(g);
        }
        if ( (!impl->window.empty()) && 
             (impl->heap.empty() || impl->compare(impl->heap.top(),impl->window.top()))
           ) {
            push_to_heap();
        }
    }
    
    result_type const& dereference() const
    {
        return *(impl->heap.top());
    }
    
    bool equal(union_heap_generator const& other) const
    {
        return (not bool(*this)) and (not bool(other));
    }
    
    void skip_empties()
    {
        while (impl->gen and not empties_exhausted()) {
            if (!bool(*(impl->gen))) {
                ++(impl->empty_count);
                impl->gen();
            } else {
                break;
            }
        }
    }

    void push_to_window()
    {
        if (impl->gen) impl->window.push(impl->gen());
        skip_empties();
    }
    
    void push_to_heap()
    {
        if (!impl->window.empty()) {
            size_t lookahead = impl->window.size();
            for (size_t x = 0; x != lookahead; ++x) {
                impl->heap.push(impl->window.top());
                impl->window.pop();
            }
            for (size_t x = 0; x != lookahead; ++x) {
                push_to_window();
            }
        }
    }
    
    void init(size_t window) 
    {
        skip_empties();
        for (size_t x = 0; x != window; ++x) {
            push_to_window();
        }
        push_to_heap();
    }
    
    struct compare_current {
        Compare compare;
        bool operator()(gen_type const& g1, gen_type const& g2) const
        {
            return compare(*g1,*g2);
        }
        compare_current(Compare const& compare) : compare(compare) {}
    };
    
    struct impl_ {
        impl_( Generator const& gen
             , Compare const& compare
             , size_t empty_max
             )
          : gen(gen)
          , compare(compare)
          , heap(compare_current(compare))
          , window(compare_current(compare))
          , empty_max(empty_max)
          , empty_count(0) {}
        Generator gen;
        compare_current compare;
        typedef 
          std::priority_queue<gen_type, std::vector<gen_type>, compare_current>
          heap_type;
        heap_type heap;
        heap_type window;
        size_t empty_max;
        size_t empty_count;
    };
    
    boost::shared_ptr<impl_> impl;
};

////////////////////////////////////////////////////////////////////////////////

template <class Compare, class Generator>
union_heap_generator<Generator,Compare>
generate_union_heap( Generator const& gen
                   , Compare const& compare
                   , size_t lookahead = 1
                   , size_t max_empties = std::numeric_limits<size_t>::max()
                   )
{
    return union_heap_generator<Generator,Compare>(gen,lookahead,max_empties,compare);
}

} // namespace gusc

# endif //     GUSC__GENERATOR__UNION_HEAP_GENERATOR_HPP
