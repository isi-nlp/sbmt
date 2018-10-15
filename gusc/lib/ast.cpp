# include <gusc/lattice/grammar.hpp>
# include <gusc/lattice/ast.hpp>
# include <gusc/lattice/ast_construct.hpp>
# include <gusc/lattice/grammar.hpp>
# include <boost/spirit/include/phoenix1.hpp>
# include <iostream>
# include <boost/tuple/tuple.hpp>

# include <boost/spirit/include/classic_core.hpp>    
# include <boost/spirit/include/classic_flush_multi_pass.hpp>
# include <boost/spirit/include/classic_multi_pass.hpp>
# include <boost/spirit/include/classic_position_iterator.hpp>

using boost::tie;
using boost::tuples::tuple;
using namespace phoenix;
using namespace std;

namespace gusc {

////////////////////////////////////////////////////////////////////////////////

size_t lattice_ast::vertex_info::id() const
{
    return id_;
}

////////////////////////////////////////////////////////////////////////////////

string const& lattice_ast::vertex_info::label() const
{
    return label_;
}

////////////////////////////////////////////////////////////////////////////////

property_container_interface::key_value_pair::key_value_pair( string const& k
                                                            , string const& v )
  : p(k,v) {}

////////////////////////////////////////////////////////////////////////////////

property_container_interface::key_value_pair::key_value_pair(
                                            boost::tuple<string,string> const& p
                                            )
  : p(p) {}

////////////////////////////////////////////////////////////////////////////////

string const& 
property_container_interface::key_value_pair::key() const
{
    return p.get<0>();
}

////////////////////////////////////////////////////////////////////////////////

string const& 
property_container_interface::key_value_pair::value() const
{
    return p.get<1>();
}

////////////////////////////////////////////////////////////////////////////////

lattice_vertex_pair::lattice_vertex_pair(size_t f, size_t t)
  : p(f,t) {}

////////////////////////////////////////////////////////////////////////////////
  
lattice_vertex_pair::lattice_vertex_pair(boost::tuple<size_t,size_t> const& p)
  : p(p) {}

////////////////////////////////////////////////////////////////////////////////

size_t lattice_vertex_pair::from() const
{
    return p.get<0>();
}

////////////////////////////////////////////////////////////////////////////////

size_t lattice_vertex_pair::to() const
{
    return p.get<1>();
}

////////////////////////////////////////////////////////////////////////////////

bool lattice_line::is_block() const { return impl->isblk; }

////////////////////////////////////////////////////////////////////////////////

lattice_vertex_pair lattice_line::span() const 
{ 
    if (!is_block()) return impl->spn; 
    else {
        size_t min = ULONG_MAX;
        size_t max = 0;
        const_line_iterator i, e;
        tie(i,e) = lines();
        for (;i != e; ++i) {
            lattice_vertex_pair p = i->span();
            min = std::min(p.from(),min);
            max = std::max(p.to(),max);
        }
        if (min > max) min = max;
        return vertex_pair(min,max);
    }
}

////////////////////////////////////////////////////////////////////////////////

string const& lattice_line::label() const 
{ 
    return impl->lbl; 
}

////////////////////////////////////////////////////////////////////////////////

pair< property_container_interface::const_property_iterator,
      property_container_interface::const_property_iterator > 
lattice_line::properties() const
{
    return impl->properties();
}

////////////////////////////////////////////////////////////////////////////////

pair< property_container_interface::property_iterator,
      property_container_interface::property_iterator > 
lattice_line::properties()
{
    return impl->properties();
}

////////////////////////////////////////////////////////////////////////////////

property_container_interface::property_iterator 
lattice_line::insert_property(string const& k, string const& v)
{
    return impl->insert_property(k,v);
}

////////////////////////////////////////////////////////////////////////////////

void 
lattice_line::erase_property(property_container_interface::property_iterator pos)
{
    impl->erase_property(pos);
}

////////////////////////////////////////////////////////////////////////////////

pair<line_container::const_line_iterator,line_container::const_line_iterator> 
lattice_line::lines() const 
{
    if (!is_block()) { throw runtime_error("notablock"); }
    return impl->lines();
}

////////////////////////////////////////////////////////////////////////////////

pair<line_container::line_iterator,line_container::line_iterator> 
lattice_line::lines()
{
    if (!is_block()) { throw runtime_error("notablock"); }
    return impl->lines();
}

void 
lattice_line::erase_line(line_container_interface::line_iterator pos)
{
    impl->erase_line(pos);
}

////////////////////////////////////////////////////////////////////////////////

line_container::line_iterator 
lattice_line::insert_edge(vertex_pair const& spn, std::string const& lbl)
{
    if (!is_block()) throw std::runtime_error("notablock");
    return impl->insert_edge(spn,lbl);
}

////////////////////////////////////////////////////////////////////////////////

line_container::line_iterator 
lattice_line::insert_block()
{
    if (!is_block()) throw std::runtime_error("notablock");
    return impl->insert_block();
}

////////////////////////////////////////////////////////////////////////////////

pair<line_container::const_line_iterator,line_container::const_line_iterator> 
line_container::lines() const
{
    return make_pair(lns.begin(), lns.end());
}

////////////////////////////////////////////////////////////////////////////////

pair<line_container::line_iterator,line_container::line_iterator> 
line_container::lines()
{
    return make_pair(lns.begin(), lns.end());
}

////////////////////////////////////////////////////////////////////////////////

line_container::line_iterator 
line_container::insert_edge(vertex_pair const& spn, std::string const& lbl)
{
    return lns.insert(lns.end(),lattice_line(spn,lbl));
}

////////////////////////////////////////////////////////////////////////////////

void
line_container::erase_line(line_container_interface::line_iterator pos)
{
    lns.erase(pos);
}

////////////////////////////////////////////////////////////////////////////////

line_container::line_iterator 
line_container::insert_block()
{
    return lns.insert(lns.end(),line());
}

////////////////////////////////////////////////////////////////////////////////

lattice_line_impl::lattice_line_impl(vertex_pair const& spn, string const& lbl)
  : isblk(false)
  , spn(spn)
  , lbl(lbl) {}

////////////////////////////////////////////////////////////////////////////////

lattice_line_impl::lattice_line_impl() 
  : isblk(true), spn(0,1) {}

////////////////////////////////////////////////////////////////////////////////

lattice_line::lattice_line(vertex_pair const& spn, string const& lbl)
  : impl(new lattice_line_impl(spn,lbl)) {}

////////////////////////////////////////////////////////////////////////////////

lattice_line::lattice_line()
  : impl(new lattice_line_impl()) {}

////////////////////////////////////////////////////////////////////////////////

lattice_ast::vertex_info::vertex_info(size_t id, std::string const& label)
  : id_(id)
  , label_(label) {}

////////////////////////////////////////////////////////////////////////////////

lattice_ast::vertex_info_iterator 
lattice_ast::insert_vertex_info(size_t id, std::string const& label)
{
    return vertices.insert(vertices.end(),vertex_info(id,label));
}

////////////////////////////////////////////////////////////////////////////////

void
lattice_ast::erase_vertex_info(vertex_info_iterator pos)
{
    vertices.erase(pos);
}

////////////////////////////////////////////////////////////////////////////////

pair< property_container::const_property_iterator
    , property_container::const_property_iterator >
property_container::properties() const 
{ 
    return make_pair(props.begin(),props.end()); 
}

////////////////////////////////////////////////////////////////////////////////

pair< property_container::property_iterator
    , property_container::property_iterator >
property_container::properties()
{ 
    return make_pair(props.begin(),props.end()); 
}

////////////////////////////////////////////////////////////////////////////////

property_container::property_iterator
property_container::insert_property(string const& k, string const& v)
{
    return props.insert(props.end(),key_value_pair(k,v));
}

////////////////////////////////////////////////////////////////////////////////

void property_container::erase_property(property_iterator pos)
{
    props.erase(pos);
}

////////////////////////////////////////////////////////////////////////////////

pair< lattice_ast::const_vertex_info_iterator
    , lattice_ast::const_vertex_info_iterator
    >
lattice_ast::vertex_infos() const
{
    return make_pair(vertices.begin(),vertices.end());
}

////////////////////////////////////////////////////////////////////////////////

pair< lattice_ast::vertex_info_iterator
    , lattice_ast::vertex_info_iterator
    >
lattice_ast::vertex_infos()
{
    return make_pair(vertices.begin(),vertices.end());
}

////////////////////////////////////////////////////////////////////////////////

string escape_c(string const& str)
{
    string::const_iterator i = str.begin(), e = str.end();
    string ret;
    
    for (; i != e; ++i) {
        string::value_type c = *i;
        switch (c) {
            case '\n': ret += "\\n"; break;
            case '\t': ret += "\\t"; break;
            case '\v': ret += "\\v"; break;
            case '\b': ret += "\\b"; break;
            case '\r': ret += "\\r"; break;
            case '\f': ret += "\\f"; break;
            case '\a': ret += "\\a"; break;
            case '\\': ret += "\\\\"; break;
            case '"':  ret += "\\\""; break;
            default: ret.push_back(c);
        }
    }
    return ret;
}

string unescape_c(string const& str)
{
    string::const_iterator i = str.begin(), e = str.end();
    string ret;
    for (; i != e; ++i) {
        if (*i == '\\') {
            ++i;
            if (i != e) {
                string::value_type c = *i;
                switch (c) {
                    case 'n': ret += "\n"; break;
                    case 't': ret += "\t"; break;
                    case 'v': ret += "\v"; break;
                    case 'b': ret += "\b"; break;
                    case 'r': ret += "\r"; break;
                    case 'f': ret += "\f"; break;
                    case 'a': ret += "\a"; break;
                    case '\\': ret += "\\"; break;
                    case '"':  ret += "\""; break;
                    default: ret.push_back('\\'); ret.push_back(c);
                }
            } else {
                ret.push_back('\\');
            }
        } else {
            ret.push_back(*i);
        }
    }
    return ret;
}

////////////////////////////////////////////////////////////////////////////////

ostream& 
operator << (ostream& out, lattice_ast::key_value_pair const& p)
{
    return out << p.key() << '=' << '"' << escape_c(p.value()) << '"';
}

////////////////////////////////////////////////////////////////////////////////

ostream& print_properties(ostream& os, property_container_interface const& pc)
{
    property_container_interface::const_property_iterator pi, pe;
    tie(pi,pe) = pc.properties();
    if (pi != pe) {
        os << *pi;
        ++pi;
    }
    for (; pi != pe; ++pi) {
        os << ' ' << *pi;    
    }
    return os;
}

////////////////////////////////////////////////////////////////////////////////

ostream&
operator << (ostream& out, lattice_ast::vertex_pair const& p)
{
    return out << '[' << p.from() << ',' << p.to() << ']';
}

////////////////////////////////////////////////////////////////////////////////

ostream&
operator << (ostream& out, lattice_ast::vertex_info const& v)
{
    out << '[' << v.id() << ']' << ' ';
    if (v.label().size() > 0) out << '"' << escape_c(v.label()) << '"' << ' ';
    print_properties(out,v);
    return out;
}

////////////////////////////////////////////////////////////////////////////////

ostream& operator << (ostream& out, lattice_ast::line const& bl)
{
    using namespace phoenix;
    using namespace std;

    bool isblock = bl.is_block();

    if (isblock) {
        out << "block ";
    } else {
        out << bl.span() << ' ' ;
        if (bl.label().size() > 0) {
            out << '"' << escape_c(bl.label()) << '"' << ' ';
        }
    }

    print_properties(out,bl);

    if (isblock) {
        out << " {" << endl;
        lattice_ast::const_line_iterator li, le; 
        tie(li,le) = bl.lines();
        for_each(li, le, out << arg1 << ";" << endl);
        out << "}";
    }
    return out;
}

////////////////////////////////////////////////////////////////////////////////

ostream& operator << (ostream& out, lattice_ast const& lat)
{
    using namespace phoenix;
    using boost::tie;
    using namespace std;
    out << "lattice ";
    
    print_properties(out,lat);
    
    out << " {" << endl;
    
    lattice_ast::const_vertex_info_iterator vi,ve;
    tie(vi,ve) = lat.vertex_infos();
    for_each(vi, ve, out << arg1 << ';' << endl);
    
    lattice_ast::const_line_iterator li,le;
    tie(li,le) = lat.lines();
    for_each(li, le, out << arg1 << ";" << endl);
            
    out << "}";
    return out;
}

////////////////////////////////////////////////////////////////////////////////

istream& operator>>(istream& in, lattice_ast& lat)
{
    getlattice(in,lat);
    return in;
}

istream& getlattice(istream& in, lattice_ast& lat)
{
    using namespace boost::spirit::classic;
    typedef multi_pass< istreambuf_iterator<char> > mp_iterator_t;
    typedef position_iterator<mp_iterator_t> iterator_t;

    mp_iterator_t mp_first = make_multi_pass(istreambuf_iterator<char>(in));
    mp_iterator_t mp_last  = make_multi_pass(istreambuf_iterator<char>());

    iterator_t first(mp_first,mp_last);
    iterator_t last;
    
    if (first == last) {
      in.setstate(ios::eofbit);
      //std::cerr << "eofbit set\n";
      in.setstate(ios::failbit);
      return in;
    }
    
    parse_info<iterator_t> preinfo
        = parse(
	    first
	  , last
	  , end_p >> flush_multi_pass_p
	  , space_p | comment_p("//") | comment_p("/*","*/") | comment_p("#")
	  )
          ;

    if (first == last) {
      in.setstate(ios::eofbit);
      //std::cerr << "eofbit set\n";                                                                                                                                                       
      in.setstate(ios::failbit);
      return in;
    }

    lattice_grammar<lattice_ast_construct> ast;
    parse_info<iterator_t> info 
        = parse( 
            first
          , last
          , ast[ var(lat) = get_ast_(arg1) ] 
            >> eps_p 
            >> flush_multi_pass_p
          , space_p | comment_p("//") | comment_p("/*","*/") | comment_p("#")
          )
          ;

    if (!info.hit) {
        in.setstate(ios::failbit);
	//std::clog << "failbit set\n";
    }
    
    return in;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc
