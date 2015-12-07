#define GRAEHL__SINGLE_MAIN
#include <graehl/shared/fileargs.hpp>
#include <graehl/shared/teestream.hpp>
#include <graehl/shared/io.hpp>
#include <graehl/shared/makestr.hpp>
#include <graehl/shared/backtrace.hpp>
#include <graehl/shared/dynarray.h>
#include <graehl/shared/fileheader.hpp>
#include <graehl/shared/debugprint.hpp>

#define TREETRIE_VERSION "v1"
#define TREETRIE_COMPILED " (compiled " MAKESTR_DATE ")"

using namespace boost;
using namespace std;
using namespace graehl;


/* TREETRIE_MAP **********/
#include <graehl/shared/io.hpp>
#include <graehl/shared/funcs.hpp>
#include <graehl/shared/treetrie.hpp>
#include <graehl/shared/dynarray.h>
#include <graehl/shared/command_line.hpp>
///FOR treetrie.hpp - anything starting with "x:" is an any-rank variable.
bool is_leaf_any_rank(const Symbol &s)
{
    char *str=s.c_str();
    return str[0]=='x' && str[1]==':';
}

Symbol get_symbol(const Symbol &s)
{
    if (is_leaf_any_rank(s))
        return s.c_str()+2;
    else
        return s;
}

rank_type get_leaf_rank(const Symbol &s)
{
    return 0; //TODO: define syntax like "x:" for specifying fixed-rank leaf
}

///END

/*reader:
typedef string value_type;
operator()(istream &in,value_type &read_into)
*/

template <class value_reader=getline_reader<std::string> >
struct treetrie_map
{
    typedef treetrie<treetrie_map> tt;
    typedef shared_tree<Symbol> key_type;
    typedef typename value_reader::value_type val_type;
    typedef key_type first_type;
    typedef val_type second_type;

    tt trie;
    template <class val_acceptor>
    void find(first_type tree,val_acceptor visit)  // e.g. visit=push_backer(some_container)
    {
//        visit.visit();
    }



};




/**************/


struct treetrie_param
{
    std::ostream *log_stream;
    std::string cmdline_str;
    std::auto_ptr<teebuf> teebufptr;
    std::auto_ptr<std::ostream> teestreamptr;
    int log_level;
    void validate_parameters();
    void run();
    inline std::ostream &log() {
        return log_stream ? *log_stream : std::cerr;
    }
    inline static std::string get_version() {
        return TREETRIE_VERSION TREETRIE_COMPILED;
    }
    inline void print_options(std::ostream &o) const {
        o << "treetrie-version={{{" << get_version() << "}}} treetrie-cmdline={{{"<<cmdline_str<<"}}}";
    }

    istream_arg treedb_file,query_file;
    ostream_arg answer_file,log_file;

};

inline std::ostream & operator << (std::ostream &o,const treetrie_param &params) {
    params.print_options(o);
    return o;
}


void treetrie_param::validate_parameters()
{
    log_stream=log_file.get();
    if (!is_default_log(log_file)) // tee to cerr
    {
        teebufptr.reset(new teebuf(log_stream->rdbuf(),std::cerr.rdbuf()));
        teestreamptr.reset(log_stream=new ostream(teebufptr.get()));
    }
#ifdef DEBUG
    DBP::set_logstream(log_stream);
    DBP::set_loglevel(log_level);
#endif
    log() << "Parsing command line: " << cmdline_str << "\n\n";
    if (!treedb_file)
        throw std::runtime_error("Must provide treedb-file.");

}

// ACTUAL MAIN:
void treetrie_param::run()
{

}

treetrie_param param;


#include <memory> //auto_ptr
#include <graehl/shared/main.hpp>

#ifndef GRAEHL_TEST
#include <boost/program_options.hpp>
#include "treetrie.README.hpp"
#include <stdexcept>

using namespace boost::program_options;


MAIN_BEGIN
{
    DBP_INC_VERBOSE;
#ifdef DEBUG
        DBP::set_logstream(&cerr);
#endif
//DBP_OFF;

   bool help;

    options_description general("General options");
    general.add_options()
        ("-treedb-file,f",value<istream_arg>(&param.treedb_file)->default_value(istream_arg(),"none"),
         "contains one per line of 'tree rest-of-line-value' e.g. 'NP(DET(the),NN(dog)) value goes here'")
        ("-query-file,q",value<istream_arg>(&param.query_file)->default_value(stdin_arg(),"STDIN"),
         "one per line trees to match against treedb-file")
        ("-answer-file,a",value<ostream_arg>(&param.answer_file)->default_value(stdout_arg(),"STDOUT"),
         "for each line in query-file with N matching lines in treedb-file, print a line: N, followed by the N lines of data")
        ;
    options_description cosmetic("Cosmetic options");
    cosmetic.add_options()
        ("log-level,L",value<int>(&param.log_level)->default_value(1),"Higher log-level => more status messages to logfile (0 means absolutely quiet except for final statistics)")
        ("log-file,l",value<ostream_arg>(&param.log_file)->default_value(stderr_arg(),"STDERR"),
         "Send logs messages here (as well as to STDERR)")
         ("help,h", bool_switch(&help), "show usage/documentation")
        ;
    options_description all("Allowed options");
    all.add(general).add(cosmetic);
    const char *progname=argv[0];
    try {
        MAKESTRS(param.cmdline_str,print_command_line(cerr,argc,argv,NULL));
        variables_map vm;
        store(parse_command_line(argc, argv, all), vm);
        notify(vm);

        if (help) {
            cout << "\n" << progname << ' ' << "version " << param.get_version() << "\n\n";
            cout << usage_str << "\n";
            cout << all << "\n";
            return 0;
        }

        param.validate_parameters();

        param.run();

    }
    catch(std::bad_alloc& e) {
        param.log() << "ERROR: ran out of memory\n\n";
        param.log() << "Try descreasing -m or -M, and setting an accurate -P if you're using initial parameters.\n";
        BackTrace::print(param.log());
        return 1;
    }
    catch(std::exception& e) {
        param.log() << "ERROR: " << e.what() << "\n\n";
        param.log() << "Try '" << progname << " -h' for documentation\n";
        BackTrace::print(param.log());
        return 1;
    }
    catch(const char * e) {
        param.log() << "ERROR: " << e << "\n\n";
        param.log() << "Try '" << progname << " -h' for documentation\n";
        BackTrace::print(param.log());
        return 1;
    }
    catch(...) {
        param.log() << "FATAL: Exception of unknown type!\n\n";
        return 2;
    }

}
MAIN_END

#endif
