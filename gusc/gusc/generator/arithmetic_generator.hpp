# if ! defined(GUSC__GENERATOR__ARITHMETIC_GENERATOR_HPP)
# define       GUSC__GENERATOR__ARITHMETIC_GENERATOR_HPP
// compare to: generator from iterator of boost::arithmetic_iterator

# include <gusc/generator/any_generator.hpp>

namespace gusc {

template <class I> struct arithmetic_generator {
  typedef I result_type;
  I i,end;
  arithmetic_generator(I i,I end) : i(i),end(end) {}
  I operator()() const {
    return i;
  }
  I operator *() const {
    return i;
  }
  void operator++() const {
    ++i;
  }
  operator bool() const {
    return i<end;
  }
};

template <class V>
any_generator<V,iterator_tag> make_arithmetic_generator(V begin,V end) {
  return arithmetic_generator<V>(begin,end);
}

} // namespace gusc

# endif //     GUSC__GENERATOR__ANY_GENERATOR_HPP
