////////////////////////////////////////////////////////////////////////////////

# include <iostream>
# include <vector>
# include <map>
# include <utility>
# include <cstdlib>

namespace std {
    
template <class V, class A>
std::ostream& operator<<(std::ostream& out, std::vector<V,A> const& v)
{
    typename std::vector<V,A>::const_iterator itr = v.begin(), end = v.end();
    for (;itr != end; ++itr) { out << *itr << " "; }
    return out;
}

template <class K, class V, class A>
std::ostream& operator<<(std::ostream& out, std::map<K,V,A> const& m)
{
    typename std::map<K,V,A>::const_iterator itr = m.begin(), end = m.end();
    for (;itr != end; ++itr) { out << itr->first << "=" << itr->second << " "; }
    return out;
}

template <class K, class V>
std::ostream& operator<<(std::ostream& out, std::pair<K,V> const& p)
{
    return out << p.first << ":" << p.second;
}

}


////////////////////////////////////////////////////////////////////////////////
///
///  lattice_p   ::= "lattice" '{' line_p+ '}'
///  line_p      ::= block_p | edge_p
///  block_p     ::= "block" '{' line_p+ '}'
///  edge_p      ::= span_p source_p ( ':' score_vec_p ( ':' target_p )? )? ';'
///  span_p        = lexical-cast-of-span_t
///  source_p    ::= '"' escape_char_p '"'
///  target_p    ::= '"' escape_char_p '"'
///  score_vec_p ::= ( string_p=logprob_p )*
///  logprob_p     = lexical-cast-of-score_t
///
////////////////////////////////////////////////////////////////////////////////

# include <utilities/lattice_grammar.hpp>

int main(int argc, char** argv)
{
    using namespace std;
    using namespace boost::spirit::classic;
    using namespace phoenix;
    using namespace sbmt;
    
    if (argc != 2) {
        cerr << "usage: validate_lattice <filename>" << endl;
        return EXIT_FAILURE;
    }
    
    ifstream in(argv[1]); 

    block_lattice_grammar gram; 

    typedef multi_pass< istreambuf_iterator<char> > mp_iterator_t;
    typedef position_iterator<mp_iterator_t> iterator_t;

    mp_iterator_t mp_first = make_multi_pass(istreambuf_iterator<char>(in));
    mp_iterator_t mp_last  = make_multi_pass(istreambuf_iterator<char>());

    iterator_t first(mp_first,mp_last);
    iterator_t last;
    
    gusc::lattice_ast tree;

    parse_info<iterator_t> info = parse( first
                                       , last
                                       , "decode-lattice" >> gram[ var(tree) = arg1 ]
                                       , space_p );

    if (!info.full) {
        string line;
        getline(in,line);
        cerr << "Parsing failed.  line: "<< info.stop.get_position().line
             << " column: " << info.stop.get_position().column << endl
             << "at: \"" << line << "\"";
        return EXIT_FAILURE;
    } else {
        clog << "lattice_tree:" << endl;
        clog << tree << endl;
        return EXIT_SUCCESS;
    }
}
