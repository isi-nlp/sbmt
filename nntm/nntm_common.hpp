# ifndef NNTM__NNTM_COMMON_HPP
# define NNTM__NNTM_COMMON_HPP

template <class Grammar>
struct align_data {
    typedef std::vector< boost::tuple<boost::uint8_t,boost::uint8_t> > type;
    typedef type const& return_type;
    static return_type value( Grammar const& grammar
                            , typename Grammar::rule_type r
                            , size_t lmstrid )
    {
        return grammar.template rule_property<type>(r,lmstrid);
    }
};

typedef std::map<int,std::map<sbmt::indexed_token,int> > pathmap;

struct compare_target {
    bool operator()(boost::tuple<boost::uint8_t,boost::uint8_t> b, boost::uint8_t c) const
    {
        return b.get<0>() < c;
    }
    bool operator()(boost::uint8_t c, boost::tuple<boost::uint8_t,boost::uint8_t> b) const
    {
        return c < b.get<0>();
    }
};

# endif 