# include <bitset>
# include <string>
# include <collins/lm.hpp>
namespace collins { namespace model1 {




template <class Token>
struct state {
    Token root;
    Token constit;
    Token tag;
    Token word;
    typedef std::bitset<4> distance_type;
    distance_type distance;
    enum distance_query {left_adjacent = 0, right_adjacent = 1, verb_under = 2, verb_will_be_under = 3 };
    
    state(Token root, Token constit, Token tag, Token word, bool verb = false)
    : root(root)
    , constit(constit)
    , tag(tag)
    , word(word) 
    {
        if (verb) {
            distance[verb_under] = true;
            distance[verb_will_be_under] = true;
        }
        distance[left_adjacent] = true;
        distance[right_adjacent] = true;

    }
    
    state(){}
};

template <class Token>
state<Token> attach_left(state<Token> dep, state<Token> head)
{
    head.distance[state<Token>::verb_will_be_under] = head.distance[state<Token>::verb_will_be_under] || dep.distance[state<Token>::verb_under];
    head.distance[state<Token>::left_adjacent] = false;
    return head;
}

template <class Token>
state<Token> attach_right(state<Token> head, state<Token> dep)
{
    head.distance[state<Token>::verb_will_be_under] = head.distance[state<Token>::verb_will_be_under] || dep.distance[state<Token>::verb_under];
    head.distance[state<Token>::right_adjacent] = false;
    return head;
}

template <class Token>
state<Token> introduce_head(Token root, state<Token> head)
{
    head.constit = head.root;
    head.root = root;
    head.distance[state<Token>::left_adjacent] = true;
    head.distance[state<Token>::right_adjacent] = true;
    head.distance[state<Token>::verb_under] = head.distance[state<Token>::verb_under] || head.distance[state<Token>::verb_will_be_under];
    return head;
}

template <class Token>
struct rule {
    struct variable {
        Token constit;
        Token tag;
        Token word;
        variable() {}
        variable(Token constit, Token tag, Token head)
        : constit(constit)
        , tag(tag)
        , word(word) {}
    };
    uint32_t hpos;
    variable r[3];
    uint32_t sz;
    
    rule() : hpos(0), sz(0) {}

    template <class Iterator>
    rule(Token lhs, uint32_t hpos, Iterator itr, Iterator end)
    : hpos(hpos)
    , sz(0)
    {
        if (itr == end) throw std::runtime_error("bad input for collins::rule");
        Token constit, tag, word;
        r[sz++] = variable(lhs,lhs,lhs);
        while (itr != end) {
            constit = *itr++;
            if (itr == end) throw std::runtime_error("bad input for collins::rule");
            tag = *itr++;
            if (itr == end) throw std::runtime_error("bad input for collins::rule");
            word = *itr++;
            r[sz++] = variable(constit,tag,word);
        }
        if (hpos > sz) throw std::runtime_error("bad input for collins::rule");
        if (hpos > 0) r[0].head = r[hpos].head;
        else r[0].head = r[1].head;
        
    }
    
    rule(rule const& other)
    : hpos(other.hpos)
    , sz(other.sz)
    {
        for (size_t x = 0; x != sz; ++x) r[x] = other.r[x];
    }
    
    rule& operator=(rule const& other) 
    {
        if (this != &other) {
            hpos = other.hpos;
            sz = other.sz;
            for (size_t x = 0; x != sz; ++x) r[x] = other.r[x];
        }
        return *this;
    }
    
    variable const& lhs() const { return r[0]; }
    
    std::pair<variable const*, variable const*> 
    rhs() const { return std::make_pair(&r[0] + 1, &r[0] + sz); }
};

template <class Token>
std::istream& operator >> (std::istream& in, rule<Token>& r)
{
    in >> r.r[0].constit >> r.hpos;
    int x = 1;
    while ((x <= 2) and (in >> r.r[x].constit >> r.r[x].tag >> r.r[x].word)) ++x;
    r.sz = x;
    if (r.hpos == 0) {
        r.r[0].tag = r.r[1].tag;
        r.r[0].word = r.r[1].word;
    } else {
        r.r[0].tag = r.r[r.hpos].tag;
        r.r[0].word = r.r[r.hpos].word;
    }
    return in;
}

template <class Token>
std::ostream& operator << (std::ostream& out, rule<Token> const& r)
{
    map_token<Token> mp(get_model(out));
    out << mp(r.r[0].constit) << ' ' << r.hpos;
    for (size_t x = 1; x != r.sz; ++x) {
        out << ' ' << mp(r.r[x].constit) << ' ' << mp(r.r[x].tag) << ' ' << mp(r.r[x].word);
    }
    return out;
}

typedef rule<uint32_t> idxrule;
typedef rule<std::string> strrule;


} } // collins::model1