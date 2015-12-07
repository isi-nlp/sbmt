# if ! defined(SBMT__FEATURE__ACCUMULATOR_HPP)
# define       SBMT__FEATURE__ACCUMULATOR_HPP

# include <utility>

namespace sbmt {

template <class Container>
struct multiply_accumulator {
    Container* c;
    explicit multiply_accumulator(Container& c)
    : c(&c) {}
    template <class X, class Y>
    void operator()(std::pair<X,Y> const& x) const
    {
        (*c)[x.first] *= x.second;
    }
};

template <class Container>
struct replacement {
    Container* c;
    explicit replacement(Container& c)
    : c(&c) {}
    template <class X, class Y>
    void operator()(std::pair<X,Y> const& x) const
    {
        (*c)[x.first] = x.second;
    }
};


struct ignore_accumulator {
    ignore_accumulator() {}
    template <class X, class Y>
    void operator()(std::pair<X,Y> const& x) const { }
};

#if 0
template <class Container>
struct multiply_accumulator : pair_container_accumulator<Container,gusc::times>
{
    multiply_accumulator(Container& c)
     : pair_container_accumulator<Container,gusc::times>(c,gusc::times()) {}
};
#endif

} // namespace sbmt

# endif //     SBMT__FEATURE__ACCUMULATOR_HPP
