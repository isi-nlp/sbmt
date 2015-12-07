# if ! defined(XRSPARSE__XRS_GRAMMAR_HPP)
# define       XRSPARSE__XRS_GRAMMAR_HPP
/*
 rule ::= lhs "->" rhs "###" features
 
 features ::= *feature
 feature ::= fname "=" fvalue
 fvalue ::= compound_string | simple_string
 
 rhs ::= +(rhs_item)
 
 lhs ::= non-terminal "(" lhs_seq ")"
 lhs_seq ::= word | +(lhs | indexed_variable)
 
lex:
 rhs_item ::= word | index
 word ::= '"' '"' '"' | '"' c_escape_string '"'
 index ::= x unsigned_int
 compound_string ::= "{{{" ^(~"}}}") "}}}"
 simple_string ::= not-a-space
 indexed_variable ::= index ":" non-terminal
 non-terminal ::= not-space-or-parens
 
 */
# define BOOST_SPIRIT_THREADSAFE 1

# if ! defined(PHOENIX_LIMIT) || PHOENIX_LIMIT < 6
# define  PHOENIX_LIMIT 6
# endif 

# if (!defined(PHOENIX_CONSTRUCT_LIMIT)) || (PHOENIX_CONSTRUCT_LIMIT < 6)
# define  PHOENIX_CONSTRUCT_LIMIT 6
# endif 

# if (!defined(BOOST_SPIRIT_CLOSURE_LIMIT)) || (BOOST_SPIRIT_CLOSURE_LIMIT < 6)
# define  BOOST_SPIRIT_CLOSURE_LIMIT 6
# endif


# include <boost/spirit.hpp>
# include <boost/foreach.hpp>
# include <vector>
# include <utility>
# include <boost/range.hpp>
# include <cmath>
# include <stdexcept>
# include <xrsparse/xrs.hpp>


template <class T>
struct closure : boost::spirit::closure<closure<T>,T> {
    typedef typename closure<T>::member1 type;
    type value;
};

        
template <class R>
struct assign0_ {
    R const* r;
    assign0_(R const& r) : r(&r) {}
    template <class I>
    void operator()(I const& i) const { const_cast<R&>(*r) = R(i); }
    template <class I>
    void operator()(I const& i1, I const& i2) const { const_cast<R&>(*r) = R(i1,i2); }
};

template <class R, class A>
struct assign1_ {
    R const* r;
    A const* a;
    assign1_(R const& r, A const& a) : r(&r), a(&a) {}
    
    void set() const { const_cast<R&>(*r) = R(*a); }
    template <class I>
    void operator()(I const& i) const { set(); }
    template <class I>
    void operator()(I const& i1, I const& i2) const { set(); }
};

template <class R>
struct pop_back_c {
    R* r;
    void set() const { r->pop_back(); }
    pop_back_c(R& r) : r(&r) {}
    template <class I>
    void operator()(I const& i) const { set(); }
    template <class I>
    void operator()(I const& i1, I const& i2) const { set(); }
};

template <class R>
pop_back_c<R> pop_back_(R& r) { return pop_back_c<R>(r); }

template <class R, class A>
struct assign_at_back_c {
    typedef typename R::value_type V;
    R* r;
    A const* a;
    assign_at_back_c(R& r, A const& a) : r(&r), a(&a) {}
    
    void set() const { r->back() = *a; }
    template <class I>
    void operator()(I const& i) const { set(); }
    template <class I>
    void operator()(I const& i1, I const& i2) const { set(); }
};

template <class R, class A1, class A2>
struct assign2_ {
    R& r;
    A1 const& a1;
    A2 const& a2;
    assign2_(R& r, A1 const& a1, A2 const& a2) : r(r), a1(a1), a2(a2) {}
    
    void set() const { r = R(a1,a2); }
    template <class I>
    void operator()(I const& i) const { set(); }
    template <class I>
    void operator()(I const& i1, I const& i2) const { set(); }
};

template <class R, class N, class D>
struct assign_divide_c {
    R const* r;
    N const* n;
    D const* d;
    assign_divide_c(R const& r, N const& n, D const& d) : r(&r), n(&n), d(&d) {}
    void set() const { const_cast<R&>(*r) = R((*n) / (*d)); }
    template <class I>
    void operator()(I const& i) const { set(); }
    template <class I>
    void operator()(I const& i1, I const& i2) const { set(); }
};

template <class R, class N, class D>
assign_divide_c<R,N,D> assign_divide_(R const& r, N const& n, D const& d)
{
    return assign_divide_c<R,N,D>(r,n,d);
}

template <class R, class N>
struct assign_negative_c {
    R const* r;
    N const* n;
    assign_negative_c(R const& r, N const& n) : r(&r), n(&n) {}
    void set() const { const_cast<R&>(*r) = R(-(*n)); }
    template <class I>
    void operator()(I const& i) const { set(); }
    template <class I>
    void operator()(I const& i1, I const& i2) const { set(); }
};

template <class R, class N>
assign_negative_c<R,N> assign_negative_(R const& r, N const& n)
{
    return assign_negative_c<R,N>(r,n);
}

template <class R>
assign0_<R> assign_(R const& r) 
{ 
    return assign0_<R>(r); 
}

template <class R, class A>
assign1_<R,A> assign_(R const& r, A const& a) 
{ 
    return assign1_<R,A>(r,a); 
}


template <class R, class A>
assign_at_back_c<R,A> assign_at_back_(R& r, A const& a) 
{ 
    return assign_at_back_c<R,A>(r,a); 
}

template <class R, class A1, class A2>
assign2_<R,A1,A2> assign_(R& r, A1 const& a1, A2 const& a2) 
{ 
    return assign2_<R,A1,A2>(r,a1,a2); 
}


class brf_grammar 
  : public boost::spirit::grammar<brf_grammar> {
public:
    mutable brf_data value;
    template <class S>
    struct definition {

        typedef boost::spirit::rule<S> rule_t;
        typedef typename S::iterator_t iterator_t;
        typedef boost::iterator_range<iterator_t> range_t;
        typedef std::pair<size_t,range_t> pair_t;
        typedef feature_value<range_t> fvalue_t;
        typedef std::pair<range_t,fvalue_t> fpair_t;
        
        rule_t rule, features, rhs, lhs; 
        
        brf_data::var rhs_item_;
        boost::spirit::rule<S> rhs_item; 
        
        std::string nt_, word_;
        rule_t nt, word;
        
        fpair_t feat_;
        rule_t feat;
        
        range_t fname_;
        rule_t fname;
        
        fvalue_t fvalue_;
        rule_t fvalue;
        
        range_t str_;
        rule_t str;
        
        range_t str2_;
        rule_t str2;
        
        double scr_;
        double real_;
        rule_t scr;
        
        bool ntvar;
        bool wvar;
        int one;
        int two;
        
        double neglog10;
        
        definition(brf_grammar const& g) 
        {
            using namespace boost::spirit;
            neglog10 = -log(10.0);
            ntvar = true;
            wvar = false;
            one = 1;
            two = 2;
            
            rule = lhs >> "->" >> rhs >> !("###" >> features);
            lhs = nt[ assign_(g.value.lhs,nt_,ntvar) ]
                ;
            rhs = rhs_item[assign_(g.value.rhs[0],rhs_item_)]
                >> ( rhs_item[assign_(g.value.rhs[1],rhs_item_)][assign_(g.value.rhs_size,two)]
                   | eps_p[assign_(g.value.rhs_size,one)]
                   )
                ;
            rhs_item = word[assign_(rhs_item_,word_,wvar)] 
                     | nt[assign_(rhs_item_,nt_,ntvar)]
                     ;
            features = *( 
                         feat[push_back_a(g.value.features,feat_)]
                        )
                        ;
            feat = lexeme_d[
                      fname[assign_(feat_.first,fname_)] 
                   >> '=' 
                   >> fvalue[assign_(feat_.second,fvalue_)]
                   ]
                   ;
            fvalue = str[assign_(fvalue_,str_)]  
                   | scr[assign_(fvalue_,scr_)] 
                   | str2[assign_(fvalue_,str2_)]
                   ;
            fname = lexeme_d[ +(graph_p - (ch_p(':') | ',' | '='))][assign_(fname_)];
            scr = lexeme_d[
                    ( "e^" >> real_p[assign_(real_)][assign_divide_(scr_,real_,neglog10)]
                    | "10^" >> real_p[assign_(real_)][assign_negative_(scr_,real_)] 
                    | real_p[assign_(scr_)] 
                    ) >> (end_p|space_p)
                  ];
            str = confix_p(
                    "{{{"
                  , (*anychar_p)[assign_(str_)]
                  , "}}}"
                  )
                  ;
            str2 = lexeme_d[+(~space_p)]
                   [assign_(str2_)]
                  ;
            nt = lexeme_d[ (+(anychar_p - space_p)) - str_p("###") ][assign_(nt_)];
            word = (ch_p('"') >> ch_p('"') >> ch_p('"'))[assign_(word_,"\"")] 
                 | confix_p('"',(*anychar_p)[assign_(word_)],'"');
        }
        rule_t const& start() const { return rule; }
    };
};

class xrs_grammar 
  : public boost::spirit::grammar<xrs_grammar> {
public:
    mutable rule_data value;
    template <class S>
    struct definition {
        typedef typename S::iterator_t iterator_t;
        typedef boost::iterator_range<iterator_t> range_t;
        typedef std::pair<size_t,range_t> pair_t;
        typedef std::pair<range_t,feature_value<range_t> > fpair_t;
        typedef boost::spirit::rule<S> rule_t;
        typedef feature_value<range_t> fvalue_t;
        rule_t const& start() const { return rule; }
        
        boost::spirit::subrule<0> feats;
        
        fpair_t feat_;
        boost::spirit::subrule<1> feat;
        range_t fname_;
        boost::spirit::subrule<2> fname;
        fvalue_t fvalue_;
        boost::spirit::subrule<3> fvalue;
        range_t simple_str_;
        boost::spirit::subrule<4> simple_str;
        range_t compound_str_;
        boost::spirit::subrule<5> compound_str;
        double real_;
        double score_;
        boost::spirit::subrule<6> score;
            
        boost::spirit::subrule<0> rhs_items;
        rhs_node rhs_item_;
        boost::spirit::subrule<1> rhs_item;
        size_t rhs_index_;
        boost::spirit::subrule<2> rhs_index;
        range_t rhs_word_;
        boost::spirit::subrule<3> rhs_word;
        
        lhs_node lhs_node_;
        size_t lhs_root_;
        std::vector<size_t> lhs_root_stack_;
        boost::spirit::subrule<0> lhs_root;
        std::vector<size_t> lhs_seq_;
        boost::spirit::subrule<1> lhs_seq;
        size_t lhs_seq_item_;
        boost::spirit::subrule<2> lhs_seq_item;
        range_t lhs_word_;
        boost::spirit::subrule<3> lhs_word;
        size_t lhs_index_;
        boost::spirit::subrule<4> lhs_index;
        range_t nt_;
        boost::spirit::subrule<5> nt;
        pair_t indexed_nt_;
        boost::spirit::subrule<6> indexed_nt;
        
        rule_t rule, features, rhs, lhs;
        
        struct push_back_pos_ {
            std::vector<size_t>* stk;
            std::vector<lhs_node> const* vec;
            push_back_pos_(std::vector<size_t>& stk, std::vector<lhs_node> const& vec)
            : stk(&stk)
            , vec(&vec) {}
            void set() const { stk->push_back(vec->size() - 1); }
            
            template <class I> 
            void operator()(I const&) const { set(); }
            template <class I>
            void operator()(I const&, I const&) const { set(); }
        };
        
        struct pop_back_pos_ {
            size_t* val;
            std::vector<size_t>* stk;
            
            pop_back_pos_(size_t& val, std::vector<size_t>& stk)
            : val(&val)
            , stk(&stk) {}
            void set() const { *val = stk->back(); stk->pop_back(); }
            
            template <class I> 
            void operator()(I const&) const { set(); }
            template <class I>
            void operator()(I const&, I const&) const { set(); }
        };
        
        struct assign_back_pos_ {
            std::size_t* val;
            std::vector<lhs_node> const* vec;
            
            assign_back_pos_(size_t& val, std::vector<lhs_node> const& vec)
            : val(&val)
            , vec(&vec) {}
            void set() const { *val = (vec->size() - 1); }
            
            template <class I> 
            void operator()(I const&) const { set(); }
            template <class I>
            void operator()(I const&, I const&) const { set(); }
        };
        
        struct assign_next_at_ {
            std::vector<lhs_node>* vec;
            std::vector<size_t> const* pos; 
            std::size_t const* val;
            
            assign_next_at_(std::vector<lhs_node>& vec,std::vector<size_t> const& pos, std::size_t const& val)
            : vec(&vec)
            , pos(&pos)
            , val(&val) {}
            
            void set() const { vec->at(pos->back()).next = *val; }
            
            template <class I> 
            void operator()(I const&) const { set(); }
            template <class I>
            void operator()(I const&, I const&) const { set(); }
        };
        double neglog10;
        bool true_;
        bool false_;
        definition(xrs_grammar const& g) 
        {
            true_ = true;
            false_ = false;
            neglog10 = -log(10.0);
            using namespace boost::spirit;

            rule = lhs >> "->" >> rhs >> "###" >> features;
            
            features = 
             ( feats = *( lexeme_d["id=" >> int_p[assign_(g.value.id)]]
                        | feat[ push_back_a(g.value.features,feat_) ]
                        )
             , feat = lexeme_d[ 
                         fname[ assign_(feat_.first,fname_) ] 
                      >> '=' 
                      >> fvalue[ assign_(feat_.second,fvalue_) ]
                      ]
             , fname = lexeme_d[
                          +(graph_p - (ch_p(':') | ',' | '='))
                       ][ assign_(fname_) ]
             , fvalue = compound_str[ assign_(fvalue_,compound_str_,true_) ] 
                      | score[ assign_(fvalue_,score_) ]
                      | simple_str[ assign_(fvalue_,simple_str_,false_) ]
             , simple_str = lexeme_d[
                                +(~space_p)
                            ][ assign_(simple_str_) ]
             , compound_str = confix_p(
                                "{{{"
                              , (*anychar_p)[ assign_(compound_str_) ]
                              , "}}}"
                              )
             , score = "e^" >> real_p[ assign_(real_) ][ assign_divide_(score_,real_,neglog10) ]
                     | "10^" >> real_p[ assign_(real_) ][ assign_negative_(score_,real_) ]
                     | real_p[ assign_(score_) ]
             )
             ;
                   
            rhs = 
            ( rhs_items = +( rhs_item[ push_back_a(g.value.rhs,rhs_item_) ] )
            , rhs_item = rhs_word[ assign_(rhs_item_,rhs_word_) ]
                       | rhs_index[ assign_(rhs_item_,rhs_index_) ]
            , rhs_index = lexeme_d[ 'x' >> uint_p[ assign_(rhs_index_) ] ]
            , rhs_word = lexeme_d[
                            (str_p("\"") >> str_p("\"")[ assign_(rhs_word_) ] >> "\"")
                         | confix_p(
                             '"'
                           , (*(~ch_p('"')))[ assign_(rhs_word_) ]
                           , '"'
                           )
                         ]
            )
            ;
            
            lhs =
            ( lhs_root =   nt[ assign_(lhs_node_,true_,nt_) ]
                             [ push_back_a(g.value.lhs,lhs_node_) ]
                             [ push_back_pos_(lhs_root_stack_,g.value.lhs) ]
                       >> ch_p('(')[push_back_a(lhs_seq_,0)] 
                       >> lhs_seq 
                       >> ch_p(')')[pop_back_(lhs_seq_)][ pop_back_pos_(lhs_root_,lhs_root_stack_) ]
            , lhs_seq = lhs_word[ assign_(lhs_node_,false_,lhs_word_) ]
                                [ push_back_a(g.value.lhs,lhs_node_) ]
                      | 
                        (  lhs_seq_item[ assign_at_back_(lhs_seq_,lhs_seq_item_) ] 

                        >> *( lhs_seq_item[ assign_next_at_(g.value.lhs,lhs_seq_,lhs_seq_item_) ]
                                          [ assign_at_back_(lhs_seq_,lhs_seq_item_) ]
                            )
                        )
            , lhs_seq_item = indexed_nt[assign_back_pos_(lhs_seq_item_,g.value.lhs) ]
                           | lhs_root[assign_(lhs_seq_item_,lhs_root_)]
            , lhs_word = lexeme_d[
                           (str_p("\"") >> str_p("\"")[ assign_(lhs_word_)] >> '"') 
                         | confix_p(
                             '"'
                           , (*(~ch_p('"')))[assign_(lhs_word_)]
                           , '"'
                           )
                         ]
            , lhs_index = lexeme_d[ 'x' >> uint_p[ assign_(lhs_index_) ] ]
            , nt = lexeme_d[ +(graph_p - (ch_p('(') | ')')) ][ assign_(nt_) ]
            , indexed_nt = lexeme_d[
                              lhs_index[ assign_(indexed_nt_.first,lhs_index_) ] 
                           >> ':' 
                           >> nt[ assign_(indexed_nt_.second,nt_) ]
                           ][ push_back_a(g.value.lhs,indexed_nt_) ]
            )
            ;
        }
    };
};

template <class Range>
rule_data parse_xrs(Range const& line)
{
    xrs_grammar xrs_g;
    
    using namespace boost::spirit;
    using boost::range_iterator;
    using boost::begin;
    using boost::end;

    parse_info<typename range_iterator<Range const>::type> info;
    info = parse(begin(line),end(line), (xrs_g) >> (!end_p), space_p);
    if (not info.full) {
        std::stringstream sstr;
        sstr << "failed to parse "<< line << ": " << std::string(info.stop,end(line));
        throw std::runtime_error(sstr.str());
    }
    assert(xrs_g.value.lhs[0].indexed == false);
    assert(xrs_g.value.lhs[0].children == true);
    
    int lidx = 0; int ridx = 0;
    BOOST_FOREACH(lhs_node const& nd, xrs_g.value.lhs) {
        if (nd.children) assert(nd.indexed == false);
        if (nd.indexed) {
            ++lidx;
            assert(nd.children == false);
        }
    }
    BOOST_FOREACH(rhs_node const& nd, xrs_g.value.rhs) {
        if (nd.indexed) ++ridx;
    }
    assert(ridx == lidx);
    return xrs_g.value;
}

template <class Range>
brf_data parse_brf(Range const& line)
{
    brf_grammar brf_g;
    brf_data rd;
    
    using namespace boost::spirit;
    using boost::range_iterator;
    using boost::begin;
    using boost::end;

    parse_info<typename range_iterator<Range const>::type> info;
    info = parse(begin(line),end(line), (!(brf_g)) >> (!end_p), space_p);
    if (not info.full) {
        std::stringstream sstr;
        sstr << "failed to parse "<< line << ": " << std::string(info.stop,end(line));
        throw std::runtime_error(sstr.str());
    }
    return brf_g.value;
}

# endif  //    XRSPARSE__XRS_GRAMMAR_HPP
