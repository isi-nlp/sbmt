namespace sbmt {

////////////////////////////////////////////////////////////////////////////////

template <class Ch, class TF> 
char_separator<Ch,TF>::char_separator( TF& tf
                                     , token_type_id t
                                     , Ch const* dropped_delims
                                     , Ch const* kept_delims
                                     , boost::empty_token_policy empty_tokens ) 
: char_sep(dropped_delims,kept_delims,empty_tokens)
, typ(t) 
, tf(&tf) {}

////////////////////////////////////////////////////////////////////////////////

template <class Ch, class TF>  
char_separator<Ch,TF>::char_separator(TF& tf, token_type_id t)
: typ(t)
, tf(&tf) {}

////////////////////////////////////////////////////////////////////////////////

template <class Ch, class TF>
template <class InputItr>
bool char_separator<Ch,TF>::operator()( InputItr& next
                                     , InputItr end
                                     , token_t& tok)
{
    std::string s;
    if (char_sep(next,end,s)) {
        tok = tf->create_token(s,typ);
        return true;
    } else return false;
}

////////////////////////////////////////////////////////////////////////////////

template <class TokFunc, class InputItr>
tokenizer<TokFunc,InputItr>::tokenizer( InputItr begin
                                      , InputItr end
                                      , TokFunc const& f)
: toker(begin,end,f){}

////////////////////////////////////////////////////////////////////////////////
  
template <class TokFunc, class InputItr>  
template <class ContainerT> 
tokenizer<TokFunc,InputItr>::tokenizer(ContainerT const& c, TokFunc const& f)
: toker(c,f) {}

////////////////////////////////////////////////////////////////////////////////
    
template <class TokFunc, class InputItr>   
void tokenizer<TokFunc,InputItr>::assign(InputItr begin, InputItr end) 
{ toker.assign(begin,end); }

////////////////////////////////////////////////////////////////////////////////

template <class TokFunc, class InputItr>
void tokenizer<TokFunc,InputItr>::assign( InputItr begin
                                        , InputItr end
                                        , TokFunc const& f)
{ toker.assign(begin,end,f); }

////////////////////////////////////////////////////////////////////////////////
    
template <class TokFunc, class InputItr>
template <class ContainerT>
void tokenizer<TokFunc,InputItr>::assign(ContainerT const& c)
{ toker.assign(c); }

////////////////////////////////////////////////////////////////////////////////

template <class TokFunc, class InputItr>    
template <class ContainerT>
void tokenizer<TokFunc,InputItr>::assign(ContainerT const& c, TokFunc const& f)
{ toker.assign(c,f); }

////////////////////////////////////////////////////////////////////////////////
    
template <class TokFunc, class InputItr>
typename tokenizer<TokFunc,InputItr>::iterator 
tokenizer<TokFunc,InputItr>::begin()
{ return toker.begin(); }

////////////////////////////////////////////////////////////////////////////////

template <class TokFunc, class InputItr>
typename tokenizer<TokFunc,InputItr>::iterator 
tokenizer<TokFunc,InputItr>::end()
{ return toker.end(); }

////////////////////////////////////////////////////////////////////////////////
  
template <class TokFunc, class InputItr>     
typename tokenizer<TokFunc,InputItr>::const_iterator 
tokenizer<TokFunc,InputItr>::begin() const
{ return toker.begin(); }

////////////////////////////////////////////////////////////////////////////////

template <class TokFunc, class InputItr>
typename tokenizer<TokFunc,InputItr>::const_iterator 
tokenizer<TokFunc,InputItr>::end() const
{ return toker.end(); }

////////////////////////////////////////////////////////////////////////////////
    
} // namespace sbmt
