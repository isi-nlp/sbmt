#ifndef   SBMT_TOKEN_TOKEN_IO_HPP
#define   SBMT_TOKEN_TOKEN_IO_HPP

#include <sbmt/token/fat_token.hpp>
#include <sbmt/token/indexed_token.hpp>
#include <sbmt/printer.hpp>
#include <iostream>

namespace sbmt {

template <class C, class CT, class TokFactory>
void print( std::basic_ostream<C,CT>& out
          , indexed_token const& t
          , TokFactory const& tf )
{
    out << tf.label(t);
}

template <class C, class CT, class TokFactory>
void print( std::basic_ostream<C,CT>& out
          , fat_token const& t
          , TokFactory const& tf )
{
    out << t.label();
}

template <class OldDict, class NewDict>
typename NewDict::token_type
reindex(typename OldDict::token_type tok, OldDict& olddict, NewDict& newdict)
{
    return newdict.create_token(olddict.label(tok), tok.type());
}

template <class TokFactory>
indexed_token index( fat_token const& t
                   , TokFactory& tf )
{
    return reindex(t,fat_tf,tf);
}

template <class TokFactory>
fat_token fatten( indexed_token const& t
                , TokFactory& tf )
{
    return reindex(t,tf,fat_tf);                                   
}

template <class F, class I, class TokFactory>
class fatten_op
{
    TokFactory* tf;
public:
    fatten_op(TokFactory& tf) : tf(&tf) {}
    typedef F result_type;
    result_type operator()(I const& t) const
    {
        return fatten(t,*tf);
    }
};

template <class I, class F, class TokFactory>
class index_op
{
    TokFactory* tf;
public:
    index_op(TokFactory& tf) : tf(&tf) {}
    
    typedef I result_type;
    result_type operator()(F const& t) const
    {
        return index(t,*tf);
    }
};

} // namespace sbmt

#endif // SBMT_TOKEN_TOKEN_IO_HPP

