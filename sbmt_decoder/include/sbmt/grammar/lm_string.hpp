# ifndef   SBMT__GRAMMAR__LM_STRING_HPP
# define   SBMT__GRAMMAR__LM_STRING_HPP

# include <sbmt/token.hpp>
# include <boost/variant.hpp>
# include <boost/serialization/variant.hpp>
# include <boost/serialization/access.hpp>
# include <boost/variant/apply_visitor.hpp>
# include <boost/serialization/split_member.hpp>
# include <gusc/varray.hpp>
# include <boost/iterator/transform_iterator.hpp>

namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

struct hash_visitor
{
    typedef std::size_t result_type;
    
    template <class T>
    std::size_t operator()(T const& x) const
    {
        boost::hash<T> hasher;
        return hasher(x);
    }
};

template <class TokenT>
class lm_token
{
public:
    typedef TokenT token_type;
    
    /// exactly one of these is true.
    bool is_token() const;
    bool is_index() const;

    // creates a token representing a native word.
    //template <class TokenFactoryT>
    //lm_token(std::string const& tok, TokenFactoryT& tf);
    
    /// creates a token representing a permutation index (x0, x1, etc.)
    lm_token(unsigned int idx);
    
    /// create a non-index token
    lm_token(token_type const& tok);
    
    lm_token(){}
    
    template <class C>
    lm_token(lm_token<C> const& c);
    
    /// throws if is_token() == false. otherwise, returns a native_word token
    token_type const& get_token() const;
    token_type& get_token();
    
    /// throws if is_index() == false. otherwise, returns a number representing
    /// a permutation element.
    unsigned int      get_index() const;
    
    bool operator==(lm_token const& other) const { return var == other.var; }
    bool operator==(unsigned int other) const { return var == variant_t(other); }
    bool operator==(token_type const& other) const { return var == variant_t(other); }
    bool operator!=(lm_token const& other) const { return !(var == other.var); }
    bool operator!=(unsigned int other) const { return !(var == variant_t(other)); }
    bool operator!=(token_type const& other) const { return !(var == variant_t(other)); }
    
    std::size_t hash_value() const { return boost::apply_visitor(hash_visitor(),var); }
private:
    typedef boost::variant<token_type,unsigned int> variant_t;
    variant_t var;
    
    template <class T>
    friend std::ostream& operator<<(std::ostream&, lm_token<T> const&);
    
    template <class T>
    friend void swap(lm_token<T>&, lm_token<T>&);
    
    friend class boost::serialization::access;
    
    template <class ArchiveT>
    void serialize(ArchiveT & ar, const unsigned int version);
};

////////////////////////////////////////////////////////////////////////////////

template <class OldTF, class NewTF>
lm_token<typename NewTF::token_type>
reindex(lm_token<typename OldTF::token_type> const& tok, OldTF& otf, NewTF& ntf)
{
    if (tok.is_index()) 
        return lm_token<typename NewTF::token_type>(tok.get_index());
    else 
        return lm_token<typename NewTF::token_type>(reindex(tok.get_token(),otf,ntf));
}

template <class New, class OldTF, class NewTF>
struct reindexer
{
    typedef New result_type;
    template <class Old>
    result_type operator()(Old const& tok) const
    {
        return reindex(tok,*otf,*ntf);
    }
    
    reindexer(OldTF& otf, NewTF& ntf) : otf(&otf), ntf(&ntf) {}
private:
    OldTF* otf;
    NewTF* ntf;
};

////////////////////////////////////////////////////////////////////////////////

template <class T>
std::size_t hash_value(lm_token<T> const& tok)
{
    return tok.hash_value();
}

////////////////////////////////////////////////////////////////////////////////

template <class TokenT, class TokenFactoryT>
void print(std::ostream&, lm_token<TokenT> const&, TokenFactoryT const&);

template <class TokenT>
std::ostream& operator<<(std::ostream& out, lm_token<TokenT> const& tok)
{
    return out << tok.var;
}

////////////////////////////////////////////////////////////////////////////////

template <class TokenT, class Allocator = std::allocator<TokenT> >
class lm_string
{
    typedef gusc::varray<lm_token<TokenT>,Allocator> lm_vec_type;
public:
    typedef typename lm_vec_type::allocator_type allocator_type;
    typename lm_vec_type::allocator_type get_allocator() const 
    {
        return lm_vec.get_allocator();
    }
    typedef TokenT token_type;
    typedef typename lm_vec_type::const_iterator
            iterator;
    typedef iterator const_iterator;
    
    template <class TokenFactoryT>
    lm_string(std::string const& str, TokenFactoryT& tf, Allocator const& a = Allocator());
    
    lm_string(Allocator const& a = Allocator()) : lm_vec(allocator_type(a)) {} 
    
    template <class Iterator>
    lm_string(Iterator itr, Iterator end, Allocator const& a = Allocator()) 
    : lm_vec(itr,end,a) {}
    
    lm_token<token_type> const& at(std::size_t idx) const 
    { 
       return lm_vec.at(idx); 
    }
    
    lm_token<token_type> const& operator[](std::size_t idx) const 
    { 
       return lm_vec[idx];
    }

    bool is_identity() const 
    {
        return size()==1 && lm_vec.front()==lm_token<TokenT>(0);
    }
    
    std::size_t size() const { return lm_vec.size(); }

    // for text-length feature - because force decoder (tree mode) uses tags (and parens?) in its lmstrings!
    unsigned n_native_tokens() const
    {
        unsigned n=0;
        for (iterator i=begin(),e=end();i!=e;++i)
            if (i->is_token() && i->get_token().type()==native_token)
                ++n;
        return n;
    }

    unsigned n_tokens() const
    {
        unsigned n=0;
        for (iterator i=begin(),e=end();i!=e;++i)
            if (i->is_token())
                ++n;
        return n;
    }
    
    unsigned n_variables() const
    {
        return size()-n_tokens();
    }
    
    
    iterator begin() const { return lm_vec.begin(); }
    iterator end() const { return lm_vec.end(); }
    
    bool operator==(lm_string const& o) const { return lm_vec == o.lm_vec; }
    bool operator!=(lm_string const& o) const { return !(*this == o); }
    std::size_t hash_value() const { return boost::hash<lm_vec_type>()(lm_vec); }
    
    void swap(lm_string& other) { lm_vec.swap(other.lm_vec); }
private:
    lm_vec_type lm_vec;
    
    friend class boost::serialization::access;
    
    template <class ArchiveT>
    void save(ArchiveT & ar, const unsigned int version) const
    {
        ar << lm_vec;
    }
    
    template <class ArchiveT>
    void load(ArchiveT & ar, const unsigned int version)
    {
        if (version == 0) {
            std::vector<lm_token<token_type> > v;
            ar >> v;
            lm_vec_type(v).swap(lm_vec);
        } else {
            ar >> lm_vec;
        }
    }
    
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

template <class Old, class New, class A>
lm_string<typename New::token_type, typename A::template rebind<typename New::token_type>::other>
reindex(lm_string<typename Old::token_type, A> const& lmstr, Old& oldtf, New& newtf)
{
    reindexer<lm_token<typename New::token_type>,Old,New> ridx(oldtf,newtf);
    return lm_string< typename New::token_type
                    , typename A::template rebind<typename New::token_type>::other
                    >( boost::make_transform_iterator(lmstr.begin(), ridx)
                     , boost::make_transform_iterator(lmstr.end(), ridx)
                     , lmstr.get_allocator() 
                     );
}

template <class C, class T, class A> std::basic_ostream<C,T>&
operator << (std::basic_ostream<C,T>& o, lm_string<fat_token,A> const& lmstr)
{
    return o << print(lmstr,fat_tf);
}

template <class T, class A>
std::size_t hash_value(lm_string<T,A> const& s)
{
    return s.hash_value();
}

////////////////////////////////////////////////////////////////////////////////

template <class TT, class A, class TokenFactoryT>
void print( std::ostream& out
          , lm_string<TT,A> const& lms
          , TokenFactoryT const& tf ); 


////////////////////////////////////////////////////////////////////////////////

typedef 
    lm_string<indexed_token, std::allocator<indexed_token> > 
    indexed_lm_string;
typedef 
    lm_token<indexed_token> 
    indexed_lm_token;
typedef 
    lm_string<fat_token, std::allocator<fat_token> > 
    fat_lm_string;
typedef 
    lm_token<fat_token> 
    fat_lm_token;

////////////////////////////////////////////////////////////////////////////////

} // namespace sbmt

#include <sbmt/grammar/impl/lm_string.ipp>

#endif // SBMT_GRAMMAR_LM_STRING_HPP
