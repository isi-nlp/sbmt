
# define BOOST_ENABLE_ASSERT_HANDLER 1

# include <boost/spirit/include/qi.hpp>
# include <sbmt/feature/feature_vector.hpp>
# include <boost/spirit/include/phoenix_core.hpp>
# include <boost/spirit/include/phoenix_operator.hpp>
# include <boost/spirit/include/phoenix_object.hpp>
# include <boost/spirit/include/phoenix_fusion.hpp>
# include <boost/spirit/include/phoenix_stl.hpp>
# include <boost/variant/recursive_variant.hpp>
# include <boost/fusion/adapted/boost_tuple.hpp>
# include <boost/fusion/include/boost_tuple.hpp>
# include <boost/fusion/adapted/std_pair.hpp>
# include <boost/fusion/include/std_pair.hpp>
# include <boost/fusion/include/adapt_struct.hpp>
# include <forest_reader.hpp>
# include <iostream>
# include <boost/spirit/home/support.hpp>
# include <boost/spirit/home/support/multi_pass.hpp>

namespace boost
{
    void assertion_failed(char const * expr, char const * function, char const * file, long line)
    {
        throw std::runtime_error(expr + std::string(" ") + function + std::string(" ") +  file);
    }
}

namespace std {
ostream& operator << (std::ostream& os, std::vector<exmp::hyp_ptr> const& v)
{
    os << "vec_of_h[";
    BOOST_FOREACH(exmp::hyp_ptr p, v) os << ' ' << p;
    os << " ]";
    return os;
}

ostream& operator << (std::ostream& os, std::vector<exmp::forest_ptr> const& v)
{
    os << "vec_of_f[ ";
    BOOST_FOREACH(exmp::forest_ptr p, v) os << ' ' << p;
    os << " ]";
    return os;
}

}
namespace exmp {
    

sbmt::span_t forest::span() const { return s; }
std::vector<hyp_ptr>::const_iterator forest::begin() const { return opts.begin(); }
std::vector<hyp_ptr>::const_iterator forest::end() const { return opts.end(); }
forest::forest(sbmt::span_t const& s) : s(s) {}
forest::forest(std::vector<hyp_ptr> const& v) : s(v[0]->span()), opts(v) {}


boost::int64_t hyp::ruleid() const { return rid; }
sbmt::span_t hyp::span() const { return s; }
sbmt::weight_vector const& hyp::weights() const { return w; }
std::vector<forest_ptr>::const_iterator hyp::begin() const { return c.begin(); }
std::vector<forest_ptr>::const_iterator hyp::end() const { return c.end(); }  
hyp::hyp(boost::int64_t rid, sbmt::weight_vector const& w, std::vector<forest_ptr> const& c)
: rid(rid), w(w), c(c)
{
    int m = 10000000;
    int M = -9999999;
    BOOST_FOREACH(forest_ptr f, c) {
        m = std::min(m,int(f->span().left()));
        M = std::max(M,int(f->span().right()));
    }
    s = sbmt::span_t(m,M);
} 

namespace fusion = boost::fusion;
namespace phoenix = boost::phoenix;
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

struct forest_map {
    typedef std::map<int,forest_ptr> PairMap;
    typedef PairMap::key_type key_type;
    typedef PairMap::mapped_type data_type;
    typedef PairMap::value_type value_type;
    typedef PairMap::mapped_type mapped_type;
    void clear() { mp.clear(); }
    forest_ptr& operator[](boost::tuple<boost::int64_t,std::vector<hyp_ptr> > const& t) 
    {
        std::map<int,forest_ptr>::iterator pos = mp.find(t.get<0>());
        if (pos == mp.end()) {
            forest_ptr f(new forest(t.get<1>()));
            pos = mp.insert(std::make_pair(t.get<0>(),f)).first;

        }
        return pos->second;
    }
    
    forest_ptr& operator[](sbmt::span_t span)
    {
        int x = -span.left() * 1000000 - span.right();
        std::map<int,forest_ptr>::iterator pos = mp.find(x);
        if (pos == mp.end()) {
            forest_ptr f(new forest(span));
            pos = mp.insert(std::make_pair(x,f)).first;
        }
        return pos->second;
    }
    std::map<int,forest_ptr> mp;
};

typedef std::map<forest_ptr,int> forest_print_map;
std::ostream& print(std::ostream& os, forest_ptr f, forest_print_map& fm);
std::ostream& print(std::ostream& os, hyp_ptr f, forest_print_map& fm);
std::ostream& print(std::ostream& os, forest_ptr f, forest_print_map& fm) 
{
    forest_print_map::iterator pos = fm.find(f);
    if (f->begin() == f->end()) {
        os << f->span();
    } else if (pos != fm.end()) {
        os << "#" << pos->second;
    } else {
        pos = fm.insert(std::make_pair(f,fm.size())).first;
        os << "#" << pos->second << "(OR";
        BOOST_FOREACH(hyp_ptr h, *f) { os << " "; print(os,h,fm); }
        os << ")";
    }
    return os;
}

std::ostream& print(std::ostream& os, hyp_ptr h, forest_print_map& fm) 
{
    os << "(";
    os << h->ruleid() << "<" << h->weights() << ">";
    BOOST_FOREACH(forest_ptr f, *h) { os << " "; print(os,f,fm); }
    os << ")";
    return os;
}

std::ostream& operator << (std::ostream& os, forest const& f)
{
    forest_ptr fp(new forest(f));
    forest_print_map fm;
    return print(os,fp,fm);
}

}

BOOST_FUSION_ADAPT_STRUCT(
    exmp::hyp,
    (int, ruleid)
    (sbmt::weight_vector, weights)
    (std::vector<exmp::forest_ptr>, children)
)

namespace exmp {

template <class Iterator>
struct forest_grammar : qi::grammar< Iterator, forest_ptr(), ascii::space_type>
{
    
    forest_grammar(sbmt::feature_dictionary* fdict) 
    : forest_grammar::base_type(s)
    , fmp(new forest_map())
    {
        using phoenix::at_c;
        using phoenix::insert;
        using phoenix::push_back;
        using phoenix::construct;
        using phoenix::new_;
        using qi::char_;
        using qi::eps;
        using qi::double_;
        using qi::int_;
        using qi::long_long;
        using qi::uint_;
        using qi::lexeme;
        using qi::_val;
        using qi::_1;
        using qi::_a;
        using phoenix::clear;
        using phoenix::find;
        
        w  %= '<' >> -(kv % ',') >> '>';
        kv =  eps                         [ _a = fdict ] 
           >> k                           [ phoenix::at_c<0>(_val) = (*_a)[_1] ] 
           >> ':' 
           >> double_                     [ phoenix::at_c<1>(_val) = _1 ]
           ;
        k  %= lexeme[+(char_ - (char_('<')|'>'|','|':'|'='))];
        
        sn = '[' 
          >> uint_        [ phoenix::at_c<0>(_a) = _1 ] 
          >> ',' 
          >> uint_        [ phoenix::at_c<1>(_a) = _1 ] 
          >> char_(']')   [ _val = construct<sbmt::span_t>(phoenix::at_c<0>(_a),phoenix::at_c<1>(_a)) ]
          ;
        
        h  =  '(' 
           >> long_long   [ phoenix::at_c<0>(_a) = _1 ] 
           >>  w          [ phoenix::at_c<1>(_a) = _1 ]
           >>  +( fr      [ push_back(phoenix::at_c<2>(_a),_1) ]
                ) 
           >> char_(')')  [ _val = construct<hyp_ptr>(new_<hyp>(phoenix::at_c<0>(_a),phoenix::at_c<1>(_a),phoenix::at_c<2>(_a))) ]
           ;
        
        p1 %= '#' >> int_ ;
        
        p2 %= '#' >> int_ >> "(OR" >> +h >> ")";
        
        p3 = '#' >> int_ [ phoenix::at_c<0>(_val) = _1] >> h [ push_back(phoenix::at_c<1>(_val),_1) ];

        p  = p2         [_val = _1] 
           | p3         [_val = _1] 
           | p1         [_val = _1]
           ;

        fr  = eps        [ _a = fmp ] 
            >>( sn       [ _val = (*_a)[_1] ]
              | p        [ _val = (*_a)[_1] ]
              )
            ;
           
        sp =  eps                         [ _a = fmp ] 
           >> eps                         [ clear(*_a) ] 
           >> fr                          [ _val = _1 ]
           ;

        s %= sp;
        /*
        qi::debug(h);
        qi::debug(sn);
        qi::debug(sp);
        qi::debug(fr);
        qi::debug(p);
        qi::debug(p1);
        qi::debug(p2);
        qi::debug(p3);
        qi::debug(w);
        qi::debug(k);
        */
    }
    
    ~forest_grammar() { delete fmp; }
    forest_map* fmp;
    
    qi::rule<Iterator, sbmt::span_t(), qi::locals<boost::tuple<int,int> >, ascii::space_type> sn;
    qi::rule<Iterator, boost::tuple<int,std::vector<hyp_ptr> >(), ascii::space_type> p, p1, p2, p3;
    qi::rule<Iterator, forest_ptr(), qi::locals<forest_map*>, ascii::space_type> sp;
    qi::rule<Iterator, forest_ptr(), ascii::space_type> s;
    qi::rule<Iterator, forest_ptr(), qi::locals<forest_map*>, ascii::space_type> fr;
    qi::rule<Iterator, hyp_ptr(), qi::locals< boost::tuple<boost::int64_t,sbmt::weight_vector,std::vector<forest_ptr> > >, ascii::space_type> h;
    qi::rule<Iterator, sbmt::weight_vector(), ascii::space_type> w;
    qi::rule<Iterator, std::string(), ascii::space_type> k;
    qi::rule<Iterator, std::pair<int,double>(), qi::locals<sbmt::feature_dictionary*>, ascii::space_type> kv;
};


std::istream& getforest(std::istream& in, forest& f, sbmt::feature_dictionary& fdict)
{
    namespace spirit = boost::spirit;
    using spirit::ascii::space;
    using spirit::multi_pass;
    using spirit::make_default_multi_pass;
    typedef std::istreambuf_iterator<char> base_iterator_type;

    multi_pass<base_iterator_type> 
        first = make_default_multi_pass(base_iterator_type(in)),
        last  = make_default_multi_pass(base_iterator_type());
    
    forest_grammar< multi_pass<base_iterator_type> > fg(&fdict);
    forest_ptr fptr;
    //std::cerr << "parsing forest\n";
    bool result = phrase_parse(first,last,fg,space,fptr);
    //std::cerr << "\nparsed\n";
    if (not result) in.setstate(std::ios::failbit);
    else f = *fptr;
    return in;
}

}
