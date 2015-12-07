# include <gusc/lattice/ast.hpp>
# include <set>
# include <string>
# include <iostream>
# include <boost/foreach.hpp>

struct terminal_set {
    typedef std::set<std::string> terminal_set_;
    terminal_set_ terminals;
    template <class I> void new_vertex(size_t v, std::string wd, I, I)
    {
        terminals.insert(wd);
    }
    template <class P, class I> void new_edge(P p, std::string wd, I, I)
    {
        terminals.insert(wd);
    }
    template <class I> void properties(I, I) {}
    template <class I> void begin_block(I, I) {}
    void end_block() {}
};

std::ostream& operator << (std::ostream& out, terminal_set const& ts)
{
    using namespace std;
    copy( ts.terminals.begin()
        , ts.terminals.end()
        , ostream_iterator<string>(out, " ")
        );
    return out;
}

void termset(gusc::latnode_interface const& nd, std::set<std::string>& tset)
{
    BOOST_FOREACH(gusc::lattice_line const& ln, nd.lines()) {
        if (ln.is_block()) termset(ln,tset);
        else if (ln.label() != "<foreign-sentence>" and ln.label() != "</foreign-sentence>") {
            tset.insert(ln.label());
        }
    }
}

int main(int argc, char** argv)
{
    using namespace boost;
    using namespace std;
    using namespace gusc;

    gusc::lattice_ast lat;
    
    while (cin >> lat) {
        std::set<std::string> tset;
        termset(lat,tset);
        bool first = true;
        BOOST_FOREACH(std::string str, tset) {
	    if (not first) cout << ' ';
            cout << str;
            first = false;
        }
        cout << '\n';
    }
    

    return 0;
}
