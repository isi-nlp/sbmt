# if ! defined(GUSC__GENERATOR__SINGLE_VALUE_GENERATOR_HPP)
# define       GUSC__GENERATOR__SINGLE_VALUE_GENERATOR_HPP

# include <gusc/generator/peekable_generator_facade.hpp>

namespace gusc {

////////////////////////////////////////////////////////////////////////////////
///
///  convert a single value into a generator object that returns a single 
///  value
///
////////////////////////////////////////////////////////////////////////////////
template <class Value>
class single_value_generator
  : public peekable_generator_facade<single_value_generator<Value>, Value> {
public:
    template <class X>
    single_value_generator(X const& result) 
      : result(result)
      , called(false) {}
private:
    Value const& peek() const { return result; }
    void pop() { called = true; }
    bool more() const { return !called; }
    friend class generator_access;
    Value result;
    bool called;
};

////////////////////////////////////////////////////////////////////////////////

template <class Value>
single_value_generator<Value>
make_single_value_generator(Value const& value)
{
    return single_value_generator<Value>(value);
}

template <class Value>
single_value_generator<Value>
generate_single_value(Value const& value)
{
    return single_value_generator<Value>(value);
}

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc

# endif //     GUSC__GENERATOR__SINGLE_VALUE_GENERATOR_HPP


