# if ! defined(GUSC__ITERATOR__FUNCTION_OUTPUT_ITERATOR)
# define       GUSC__ITERATOR__FUNCTION_OUTPUT_ITERATOR

# include <boost/function_output_iterator.hpp>
# include <boost/iterator/iterator_adaptor.hpp>

namespace gusc {

template <class UnaryFunction>
class function_output_iterator {
typedef function_output_iterator self;
public:
    
    struct output_proxy {
      output_proxy(UnaryFunction& f) : m_f(f) { }
      template <class T> output_proxy& operator=(const T& value) {
        m_f(value); 
        return *this; 
      }
      UnaryFunction& m_f;
    };
    
    typedef std::output_iterator_tag iterator_category;
    typedef output_proxy             value_type;
    typedef long                     difference_type;
    typedef output_proxy*            pointer;
    typedef output_proxy             reference;
    typedef output_proxy             const_reference;

    explicit function_output_iterator() {}

    explicit function_output_iterator(UnaryFunction const& f)
      : m_f(f) {}

    output_proxy operator*() { return output_proxy(m_f); }
    self& operator++() { return *this; } 
    self& operator++(int) { return *this; }
private:
    UnaryFunction m_f;
};

template <class UnaryFunction>
inline function_output_iterator<UnaryFunction>
make_function_output_iterator(UnaryFunction const& f = UnaryFunction()) {
return function_output_iterator<UnaryFunction>(f);
}
} // namespace gusc

# endif //     GUSC__ITERATOR__FUNCTION_OUTPUT_ITERATOR