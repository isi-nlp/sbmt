# if ! defined(GUSC__GENERATOR__TRANSFORM_GENERATOR_HPP)
# define       GUSC__GENERATOR__TRANSFORM_GENERATOR_HPP

# include <boost/utility/result_of.hpp>
# include <boost/optional.hpp>
# include <gusc/generator/peekable_generator_facade.hpp>


namespace gusc {

template <class Generator, class Filter>
class filter_generator 
: public peekable_generator_facade<
    filter_generator<Generator,Filter>
  , typename Generator::result_type
  > {
public:
    typedef typename Generator::result_type result_type;

    filter_generator(Generator const& g, Filter const& f)
      : generator(g)
      , filter(f) {
          while (bool(generator) and not filter(*generator)) ++generator;
      }
private:
    friend class generator_access;
    result_type const& peek() const { return *generator; }
    bool more() const { return bool(generator); }
    void pop() { 
        ++generator; 
        while (bool(generator) and not filter(*generator)) ++generator; 
    }
    Generator generator;
    Filter    filter;
};

template <class Filter, class Generator>
filter_generator<Generator,Filter>
generate_filtered(Generator const& generator, Filter const& filter = Filter())
{
    return filter_generator<Generator,Filter>(generator,filter);
}

////////////////////////////////////////////////////////////////////////////////

template <class Generator, class Transform>
class transform_generator {
public:
    typedef typename boost::result_of<
                         Transform(typename boost::result_of<Generator()>::type)
                     >::type 
            result_type;
    operator bool() const { return bool(generator); }
    result_type operator()() { return transform(generator()); } 
    transform_generator(Generator const& generator, Transform const& transform)
      : generator(generator)
      , transform(transform) {}
private:
    Generator generator;
    Transform transform;
};

////////////////////////////////////////////////////////////////////////////////

template <class Transform, class Generator>
transform_generator<Generator,Transform>
generate_transform( Generator const& generator
                  , Transform const& transform = Transform() )
{
    return transform_generator<Generator,Transform>(generator,transform);
}

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc

# endif //     GUSC__GENERATOR__TRANSFORM_GENERATOR_HPP
