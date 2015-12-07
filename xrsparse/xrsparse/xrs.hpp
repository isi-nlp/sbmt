# if ! defined(XRSPARSE__XRS_HPP)
# define       XRSPARSE__XRS_HPP

# include <boost/foreach.hpp>
# include <vector>
# include <utility>
# include <boost/range.hpp>
# include <cmath>
# include <boost/cstdint.hpp>
# include <stdexcept>

struct lhs_node {
    lhs_node() : indexed(true), children(false), index(0), next(0) {}
  template <class R> // variable
    lhs_node(std::pair<size_t,R> const& p)
     : indexed(true)
     , children(false)
     , label(boost::begin(p.second),boost::end(p.second))
     , index(p.first)
     , next(0){}
  template <class R> // internal node or lex leaf
    lhs_node(bool chldrn, R s)
    : indexed(false)
    , children(chldrn)
    , label(boost::begin(s),boost::end(s))
    , index(0)
    , next(0) {}
  bool indexed; // variable
  bool children;
  std::string label;
  unsigned int index;
  unsigned int next; // next child 0=no more linked list. first child=next index (preorder)
  bool is_terminal() const {
    return !children && !indexed;
  }
};


struct lhs_pos {
    std::vector<lhs_node> const* vec;
    std::vector<lhs_node>::const_iterator pos;
    lhs_pos(std::vector<lhs_node> const& vec, std::vector<lhs_node>::const_iterator pos)
    : vec(&vec), pos(pos) {}
  lhs_node const* operator->() const {
//    return &*pos;
    return pos.operator->();
  }
  bool can_descend() const {
    assert(!pos->children || pos+1<vec->end());
    return pos->children;
  }
  void descend() {
    ++pos;
  }
  bool can_advance() const {
    return pos->next;
  }
  void advance() {
    pos=vec->begin()+pos->next;
  }
};

std::ostream& operator<< (std::ostream& os, lhs_pos const& r);

//std::ostream& operator<< (std::ostream& os, lhs_node const& r);

struct rhs_node {
    bool indexed;
    size_t index;
    std::string label;
    rhs_node() {}
    explicit rhs_node(size_t x) : indexed(true), index(x) {}
    rhs_node(rhs_node const& r)
      : indexed(r.indexed)
      , index(r.index)
      , label(r.label) {}
    template <class R>
    explicit rhs_node(R v) : indexed(false), label(boost::begin(v),boost::end(v)) {}
};

std::ostream& operator<< (std::ostream& os, rhs_node const& r);

template <class R>
struct feature_value {
    feature_value(feature_value const& v)
    : number(v.number)
    , compound(v.compound)
    , num_value(v.num_value)
    , str_value(v.str_value)
    {

    }

    feature_value& operator=(feature_value const& v)
    {
        number = v.number;
        compound = v.compound;
        num_value = v.num_value;
        str_value = v.str_value;
        return *this;
    }

    bool number;
    bool compound;
    double num_value;
    R str_value;
    
    feature_value(R v, bool compound = true) 
    : number(false)
    , compound(compound)
    , str_value(v) {}
    
    feature_value(double f = 0.0) 
    : number(true)
    , compound(false)
    , num_value(f == 0.0 ? 0.0 : f) {}
};


////////////////////////////////////////////////////////////////////////////////
///
/// ties a dictionary to an iostream for using standard iostream functions with
/// indexed_tokens
///
////////////////////////////////////////////////////////////////////////////////
struct rule_data_iomanip {
public:
    enum which { lhs=0, rhs=1, features=2 };

    rule_data_iomanip(which w, bool p) : w(w), p(p) {}
    static int get_index(which w)
    {
        static int indices[3] = { std::ios_base::xalloc()
                                , std::ios_base::xalloc()
                                , std::ios_base::xalloc() };
        return indices[w];
    }
    static bool print(std::ios_base& ios, which w) { return !ios.iword(get_index(w)); }
    void set_ios(std::ios_base& ios)
    {
        ios.iword(get_index(w)) = !p;
    }
private:
    which w;
    bool p;
};

inline rule_data_iomanip print_rule_data_lhs(bool yesno)
{ return rule_data_iomanip(rule_data_iomanip::lhs,yesno); }

inline rule_data_iomanip print_rule_data_rhs(bool yesno)
{ return rule_data_iomanip(rule_data_iomanip::rhs,yesno); }

inline rule_data_iomanip print_rule_data_features(bool yesno)
{ return rule_data_iomanip(rule_data_iomanip::features,yesno); }

inline std::ostream& operator << (std::ostream& os, rule_data_iomanip manip)
{
    manip.set_ios(os);
    return os;
}

struct feature {
    template <class R>
    feature(std::pair<R,feature_value<R> > const& p)
    : key(boost::begin(p.first),boost::end(p.first))
    , number(p.second.number)
    , compound(p.second.compound)
    , num_value(p.second.num_value) {
        if (!number) str_value = std::string( boost::begin(p.second.str_value)
                                            , boost::end(p.second.str_value) );
    }
    feature() {}
    std::string key;
    bool number;
    bool compound;
    std::string str_value;
    double num_value;
};

std::ostream& operator<< (std::ostream& os, feature const& f);


struct rule_data {
    typedef boost::int64_t rule_id_type;
    typedef std::vector<lhs_node> lhs_list;
    lhs_list lhs;
    lhs_pos lhs_root() const 
    {
        return lhs_pos(lhs,lhs.begin());
    }
    typedef std::vector<rhs_node> rhs_list;
    rhs_list rhs;
    typedef std::vector<feature> feature_list;
    feature_list features;
    rule_id_type id;
};


size_t rhs_position(rule_data const& rd, size_t idx);

std::string label(rule_data const& rd, rhs_node const& rhs);

std::ostream& operator<< (std::ostream& os, rule_data const& rd);

struct brf_data {
    struct var {
        var() : nt(true) {}
        template <class R>
        var(R const& r, bool nt) : nt(nt), label(boost::begin(r),boost::end(r)) {}
        template <class I>
        var(I begin, I end, bool nt) : nt(nt), label(begin,end) {}
        bool nt;
        std::string label;
    };

    var lhs;
    var rhs[2];
    size_t rhs_size;
    std::vector<feature> features;
};

std::ostream& operator<< (std::ostream& out, brf_data::var const& rd);

std::ostream& operator<< (std::ostream& out, brf_data const& rd);

std::ptrdiff_t get_feature(rule_data const& rd, std::string const& name);

template <class Range>
rule_data parse_xrs(Range const& line);

template <class Range>
brf_data parse_brf(Range const& line);

# endif  //    XRSPARSE__XRS_HPP

