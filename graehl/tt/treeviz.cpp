#define BOOST_AUTO_TEST_MAIN
#define GRAEHL__SINGLE_MAIN
#define GRAPHVIZ_SYMBOL

//#define BENCH
#include <graehl/shared/config.h>
#include <graehl/tt/ttconfig.hpp>

#include <graehl/shared/symbol.hpp>
#include <graehl/shared/tree.hpp>
#include <iostream>
#include <boost/program_options.hpp>
#include <fstream>
#include <graehl/shared/fileargs.hpp>
#include <graehl/shared/genio.h>
#include <graehl/shared/input_error.hpp>

#ifndef GRAEHL_TEST
using namespace boost;
using namespace std;
using namespace boost::program_options;
using namespace graehl;

struct SymbolLabeler : public DefaultNodeLabeler<Symbol> {
    typedef DefaultNodeLabeler<Symbol> Parent;
    void print(ostream &o,const Label &l) {
/*        if (l.c_str()[0] == '\\')
            o << l.c_str();
        else
*/
        {
            Parent::print(o,l);
            if (l == Symbol("OR"))
                o << ",shape=box";
        }
    }
};


#define ABORT return 1
int
#ifdef _MSC_VER
__cdecl
#endif
main(int argc, char *argv[])
{
    INITLEAK;
    cin.tie(0);

    istream_arg tree_in,caption_in;
    ostream_arg graphviz_out;
    options_description options("General options");
    string prelude;
    bool same_level=false;
    bool any_order=false;
    bool verbose=false;

    options.add_options()
        ("help,h", "produce help message")
        ("graphviz-prelude,p", value<string>(&prelude), "graphviz .dot directives to be inserted in output graphs")
        ("in-tree-file,i",value<istream_arg>(&tree_in)->default_value(stdin_arg(),"STDIN"),"file containing one or more trees e.g. 1 root(apple,jacks) (A (2 3)) A(2,3)")
        ("out-graphviz-file,o",value<ostream_arg>(&graphviz_out)->default_value(stdout_arg(),"STDOUT"),"Output Graphviz .dot file (run dot -Tps out.dot -o out.ps")
        ("caption-file,c",value<istream_arg>(&caption_in)->default_value(istream_arg(),"none"),"file containing captions for each tree, one per line")
        ("any-order,a", bool_switch(&any_order), "Allow reordering of children to keep layout compact")
        ("same-level,s", bool_switch(&same_level), "Keep nodes of the same tree depth at the same vertical position (implies same-order)")
        ("verbose,v",bool_switch(&verbose))
        ;

    positional_options_description pos;
    pos.add("in-tree-file", -1);

    unsigned n=0;
    try {
        variables_map vm;
        store(command_line_parser(argc, argv).options(options).positional(pos).run(), vm);
        notify(vm);
#define RETURN(i) retval=i;goto done
        if (vm.count("help")) {
            cerr << argv[0] << ":\n";
            cerr << options << "\n";
            ABORT;
        }

        typedef shared_tree<Symbol> T;
        T t;
        TreeVizPrinter<Symbol,SymbolLabeler> tviz(*graphviz_out,(same_level ? 2 : (any_order ? 0 : 1)),prelude);
        bool first=true;
        std::string caption;
        while (*tree_in) {

            ++n;
            *tree_in >> t;
            if (!*tree_in && !tree_in->eof()) {
                cerr << "Error reading input tree #" << n << "\n";
                show_error_context(*tree_in,cerr);
                ABORT;
            }

            if (*tree_in) {
                if(first)
                    first=false;
                else {
                    tviz.next();
                }
                if (verbose) {
                    cerr << t << "\n";
                }
                tviz.print(t);
                if (caption_in && *caption_in)
                    if (getline(*caption_in,caption))
                        tviz.caption(caption);
            }

        }
    } catch(std::exception& e) {
        cerr << "ERROR: " << e.what() << "\n\n";
                cerr << "Error reading input tree #" << n << "\n";

        show_error_context(*tree_in,cerr);
        cerr << options << "\n";
        ABORT;
    }
    return 0;
}


#endif

