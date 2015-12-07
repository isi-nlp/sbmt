# if ! defined(GUSC__LATTICE__GRAMMAR_HPP)
# define       GUSC__LATTICE__GRAMMAR_HPP

# define BOOST_SPIRIT_THREADSAFE 1

# if PHOENIX_LIMIT < 6
# define PHOENIX_LIMIT 6
# endif

# if BOOST_SPIRIT_CLOSURE_LIMIT < 6
# define BOOST_SPIRIT_CLOSURE_LIMIT 6
# endif

# include <boost/spirit/include/classic_exceptions.hpp>
# include <boost/spirit/include/classic_core.hpp>
# include <boost/spirit/include/classic_attribute.hpp>
# include <boost/spirit/include/classic_confix.hpp>
# include <boost/spirit/include/classic_escape_char.hpp>
# include <boost/spirit/include/classic_flush_multi_pass.hpp>
# include <boost/spirit/include/phoenix1_binders.hpp>
# include <boost/spirit/include/phoenix1_primitives.hpp>
# include <boost/spirit/include/phoenix1_operators.hpp>
# include <boost/spirit/include/phoenix1_special_ops.hpp>
# include <string>
# include <stdexcept>
# include <boost/tuple/tuple_io.hpp>
# include <boost/tuple/tuple.hpp>
# include <gusc/phoenix_helpers.hpp>

namespace gusc {

////////////////////////////////////////////////////////////////////////////////

template <class Grph>
struct lattice_closure : boost::spirit::classic::closure<lattice_closure<Grph>, Grph> {
    typename lattice_closure::member1 val;
};

////////////////////////////////////////////////////////////////////////////////
///
///  the template CLBCK parameter is a callback object that implements the 
///  following methods:
///    new_vertex(index,label,property_begin,property_end)
///    new_edge(span,label,property_begin,property_end)
///    begin_block(property_begin,property_end)
///    end_block()
///    properties(property_begin,property_end)
///    
////////////////////////////////////////////////////////////////////////////////
template <class CLBCK>
class lattice_grammar 
  : public boost::spirit::classic::grammar< 
             lattice_grammar<CLBCK>
           , typename lattice_closure<CLBCK>::context_t
           >
{
    struct properties_ph {
        template <class G, class I1, class I2>
        struct result { typedef void type; };
        
        template <class G, class I1, class I2>
        void operator() (G& g, I1 const& beg, I2 const& end) const
        {
            g.properties(beg,end);
        }
    };
    
    struct begin_block_ph {
        template <class G, class I1, class I2>
        struct result { typedef void type; };
        
        template <class G, class I1, class I2>
        void operator() (G& g, I1 const& itr, I2 const& end) const
        {
            g.begin_block(itr,end);
        }
    };
    
    struct end_block_ph {
        template <class G> 
        struct result { typedef void type; };

        template <class G>
        void operator() (G& g) const
        {
            g.end_block();
        }
    };
    
    struct new_vertex_ph {
        template <class G, class V, class S, class I1, class I2>
        struct result { typedef void type; };
        
        template <class G, class V, class S, class I1, class I2>
        void operator() (G& g, V const& v, S const& s, I1 const& itr, I2 const& end) const
        {
            g.new_vertex(v,s,itr,end);
        }
    };
    
    struct new_edge_ph {
        template <class G, class V, class S, class I1, class I2>
        struct result { typedef void type; };
        
        template <class G, class V, class S, class I1, class I2>
        void operator() (G& g, V const& v, S const& s, I1 const& itr, I2 const& end)  const
        {
            g.new_edge(v,s,itr,end);
        }
    };
    ////////////////////////////////////////////////////////////////////////////
    //
    //  closures: local-variables used during parsing
    //
    ////////////////////////////////////////////////////////////////////////////
    struct uint_pair_c
      : boost::spirit::classic::closure< 
          uint_pair_c
        , boost::tuple<unsigned, unsigned>
        , unsigned
        , unsigned >
    {
        typename uint_pair_c::member1 val;
        typename uint_pair_c::member2 _0;
        typename uint_pair_c::member3 _1;
    };
    
    struct string_pair_c
      : boost::spirit::classic::closure< 
          string_pair_c
        , boost::tuple<std::string, std::string>
        , std::string
        , std::string 
        >
    {
        typename string_pair_c::member1 val;
        typename string_pair_c::member2 _0;
        typename string_pair_c::member3 _1;
    };
    
    struct string_pair_vec_c
      : boost::spirit::classic::closure< 
          string_pair_vec_c
        , std::vector< boost::tuple<std::string, std::string> >
        >
    {
        typename string_pair_vec_c::member1 val;
    };

    struct uint_c : boost::spirit::classic::closure<uint_c,unsigned>
    {
        typename uint_c::member1 val;
    };

    struct string_c : boost::spirit::classic::closure<string_c,std::string>
    {
        typename string_c::member1 val;
    };
    
    struct block_c
      : boost::spirit::classic::closure<
          block_c
        , std::vector< boost::tuple<std::string,std::string> >
        >
    {
        typename block_c::member1 feats;
    };

    struct edge_c 
      : boost::spirit::classic::closure<
          edge_c
        , boost::tuple<unsigned, unsigned>
        , std::string
        , std::vector< boost::tuple<std::string,std::string> >
        >
    {
        typename edge_c::member1 vtxpair;
        typename edge_c::member2 lbl;
        typename edge_c::member3 feats;
    };  
    
    struct vertex_c
      : boost::spirit::classic::closure<
          vertex_c
        , unsigned
        , std::string
        , std::vector< boost::tuple<std::string,std::string> >
        >
    {
        typename vertex_c::member1 vtx;
        typename vertex_c::member2 lbl;
        typename vertex_c::member3 feats;
    };
    
    
    
    ////////////////////////////////////////////////////////////////////////////

public:
    template <typename ST>
    class definition {
        typedef lattice_grammar<CLBCK> lat_gram_;
        typedef boost::spirit::classic::rule<ST>  
                rule_t;
                
        typedef boost::spirit::classic::rule<
                    typename boost::spirit::classic::lexeme_scanner<ST>::type
                > 
                lex_t;

        typedef boost::spirit::classic::rule<
                    typename boost::spirit::classic::lexeme_scanner<ST>::type
                  , typename string_c::context_t
                > 
                string_lex_t;

        typedef boost::spirit::classic::rule<ST,typename string_c::context_t>
                string_rule_t;

        typedef boost::spirit::classic::rule<ST,typename uint_pair_c::context_t>
                uint_pair_rule_t;

        typedef boost::spirit::classic::rule<ST,typename uint_c::context_t>
                uint_rule_t;

        typedef boost::spirit::classic::rule<ST,typename edge_c::context_t>
                edge_rule_t;
                
        typedef boost::spirit::classic::rule<ST,typename block_c::context_t>
                block_rule_t;
                
        typedef boost::spirit::classic::rule<ST,typename vertex_c::context_t>
                vertex_rule_t;
                
        typedef boost::spirit::classic::rule<ST,typename string_pair_c::context_t>
                feat_rule_t;
                
        typedef boost::spirit::classic::rule<ST,typename string_pair_vec_c::context_t>
                feats_rule_t;

        block_rule_t lat, vtx_edge_block, edge_block, block;       
        feats_rule_t feats;
        feat_rule_t feat;
        string_rule_t ufeat;
        edge_rule_t edge;
        vertex_rule_t vtx;
        uint_pair_rule_t vtxid_pair;
        uint_rule_t vtxid;
        
        string_lex_t cstr, featname;

    public:
        definition(lattice_grammar const& d)
        {
            using namespace boost::spirit::classic;
            using namespace phoenix;
            using namespace boost;
            using std::string;
            
            phoenix::function<properties_ph> properties_ = properties_ph();
            phoenix::function<new_edge_ph> new_edge_ = new_edge_ph();
            phoenix::function<new_vertex_ph> new_vertex_ = new_vertex_ph();
            phoenix::function<begin_block_ph> begin_block_ = begin_block_ph();
            phoenix::function<end_block_ph> end_block_ = end_block_ph();

            ////////////////////////////////////////////////////////////////////
            //
            // lat ::= "lattice" feats '{' (vtx_edge_block ';')+ '}' 
            //
            ////////////////////////////////////////////////////////////////////
            lat 
                = ("lattice" 
                  >> feats[ lat.feats = arg1 ]
                  >> '{')[properties_(d.val,begin_(lat.feats), end_(lat.feats))] 
                  >> +(vtx_edge_block >> ';' >> flush_multi_pass_p) 
                  >> '}'
                ;

            ////////////////////////////////////////////////////////////////////
            //
            // vtx_edge_block ::= vtx | edge | block
            //
            ////////////////////////////////////////////////////////////////////
            vtx_edge_block
                = vtx 
                | edge
                | block
                ;
                
            ////////////////////////////////////////////////////////////////////
            //
            // edge_block ::= edge | block
            //
            ////////////////////////////////////////////////////////////////////
            edge_block = edge | block ;
            
            ////////////////////////////////////////////////////////////////////
            //
            // block ::= "block" feats "{" (edge_block ";")+ "}"
            //
            ////////////////////////////////////////////////////////////////////
            block
                =  
                ( ("block" 
                  >> feats[ block.feats = arg1 ]
                  >> '{')[ begin_block_( d.val
                                       , cbegin_(block.feats)
                                       , cend_(block.feats)
                                       )
                         ] 
                  >> +((edge_block) >> ';' >> flush_multi_pass_p) 
                  >> '}'
                ) [ end_block_(d.val) ]
                ;
                
            ////////////////////////////////////////////////////////////////////
            //
            // vtx ::= vtxid ufeat? feats
            //
            ////////////////////////////////////////////////////////////////////
            vtx 
                = 
                (  
                    vtxid [ vtx.vtx = arg1 ]
                    >> 
                    (!(ufeat[vtx.lbl = arg1 ])) 
                    >> 
                    feats[ vtx.feats = arg1 ]
                )
                [
                    new_vertex_( d.val
                               , vtx.vtx
                               , vtx.lbl
                               , cbegin_(vtx.feats)
                               , cend_(vtx.feats)
                               )
                ]
                ;

            ////////////////////////////////////////////////////////////////////
            //
            // edge ::= vtxid_pair ufeat? feats
            //
            ////////////////////////////////////////////////////////////////////
            edge 
                =
                ( 
                    vtxid_pair [ edge.vtxpair = arg1 ]
                    >> ( !(ufeat[ edge.lbl = arg1 ]) )
                    >> feats[ edge.feats = arg1 ]
                )
                [
                    new_edge_( d.val
                             , edge.vtxpair
                             , edge.lbl
                             , cbegin_(edge.feats)
                             , cend_(edge.feats)
                             )
                ]
                ;

            ////////////////////////////////////////////////////////////////////
            //
            // feats ::= feat*
            //
            ////////////////////////////////////////////////////////////////////
            feats = *( feat[ push_back_(feats.val,arg1) ] );

            ////////////////////////////////////////////////////////////////////
            //
            // vtxid ::= '[' uint_p ']'
            //
            ////////////////////////////////////////////////////////////////////
            vtxid = ('[' >> uint_p[vtxid.val = arg1] >> ']');

            ////////////////////////////////////////////////////////////////////
            //
            // vtxid_pair ::= '[' uint_p ',' uint_p ']'
            //
            ////////////////////////////////////////////////////////////////////
            vtxid_pair 
                = 
                (
                    '[' >> uint_p[ vtxid_pair._0 = arg1 ] >>
                    ',' >> uint_p[ vtxid_pair._1 = arg1 ] >> 
                    ']'
                )
                [
                    vtxid_pair.val 
                        = construct_<boost::tuple<unsigned,unsigned> >
                          (
                              vtxid_pair._0, vtxid_pair._1
                          )
                ]
                ;

            ////////////////////////////////////////////////////////////////////
            //
            // ufeat ::= cstr
            //
            ////////////////////////////////////////////////////////////////////
            ufeat = lexeme_d[ cstr[ ufeat.val = arg1 ] ];

            ////////////////////////////////////////////////////////////////////
            //
            // feat ::= featname "=" cstr
            //
            ////////////////////////////////////////////////////////////////////
            feat 
                = lexeme_d[ 
                    (
                      featname[ feat._0 = arg1 ] 
                      >> '=' 
                      >> cstr[ feat._1 = arg1 ] 
                    )
                    [
                      feat.val 
                          = construct_< boost::tuple<string,string> >
                            (
                              feat._0, feat._1
                            )
                    ]
                  ]
                ;

            ////////////////////////////////////////////////////////////////////
            //
            //  cstr ::= '"' c_escape_ch '"'
            //
            ////////////////////////////////////////////////////////////////////
            cstr 
                = confix_p('"',*(c_escape_ch_p[cstr.val += arg1]),'"')
                [
                     cstr.val = construct_<std::string>(
                                    cstr.val
                                  , 0
                                  , size_(cstr.val) - 1
                                )
                ]
                ;

            ////////////////////////////////////////////////////////////////////
            //
            //  featname ::= alpha (alphanum | "-" | "_")*
            //
            ////////////////////////////////////////////////////////////////////
            featname 
                = 
                (
                    alpha_p 
                    >> (*(alnum_p | '-' | '_'))
                )
                [
                    featname.val = construct_<std::string>(arg1,arg2)
                ]
                ;

            ////////////////////////////////////////////////////////////////////
        }

        block_rule_t const& start() const { return lat; }
    };
};

} // namespace gusc

# endif //     GUSC__LATTICE__GRAMMAR_HPP

