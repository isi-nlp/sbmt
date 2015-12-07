# if ! defined(GUSC__GENERATOR__CHAIN_HPP)
# define       GUSC__GENERATOR__CHAIN_HPP

# include <gusc/generator/peekable_generator_facade.hpp>
# include <boost/optional.hpp>
# include <boost/utility/result_of.hpp>

namespace gusc {

////////////////////////////////////////////////////////////////////////////////

template <class GenGen>
class generator_chain_result {
    typedef typename boost::result_of<GenGen()>::type gen_type;
public:
    typedef typename boost::result_of<gen_type()>::type type;
};

////////////////////////////////////////////////////////////////////////////////

template <class GenGen>
class generator_chain 
: public peekable_generator_facade<
           generator_chain<GenGen>
         , typename generator_chain_result<GenGen>::type
         > {
    typedef typename generator_chain_result<GenGen>::type result_;
    typedef typename boost::result_of<GenGen()>::type gen_type;
    GenGen gg;
    boost::optional<gen_type> g;
    
    result_ const& peek() const { return *(g.get()); }
    
    bool more() const { return g.is_initialized(); }
    
    void pop()
    {
        ++g.get();
        skip_empties();
    }
    
    void skip_empties()
    {
        while (gg && !g.get()) g = gg();
        if (!g.get()) g = boost::none_t();
    }
    
    friend class gusc::generator_access;
    
public:
    explicit generator_chain(GenGen const& ggg) : gg(ggg) 
    {
        if (gg) {
            g = gg();
            skip_empties();
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

template <class GenGen>
generator_chain<GenGen> chain_generators(GenGen const& gg)
{
    return generator_chain<GenGen>(gg);
}

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc

# endif //     GUSC__GENERATOR__CHAIN_HPP

