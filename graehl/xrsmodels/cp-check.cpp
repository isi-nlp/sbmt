#define GRAEHL__SINGLE_MAIN
#include "main.hpp"
#include "makestr.hpp"

#define VERSION "0.1"

//#include "ttable.hpp"
//#include "lexicalfeature.hpp"

#include <boost/program_options.hpp>
#include "fileargs.hpp"
#include <stdexcept>
#include "fileheader.hpp"
#include "backtrace.hpp"

#include <set>

#define DEFAULT_BUFSIZE 1024*1024

using namespace boost;
using namespace std;
using namespace boost::program_options;


MAIN_BEGIN
{
    DBP_INC_VERBOSE;
#ifdef DEBUG
    DBP::set_logstream(&cerr);
#endif
//DBP_OFF;

    Infile in;
    Outfile out;
    string inpath,outpath;
    bool help=false;
    int log_level;
    string usage_str="Copies infile to outfile (respecting .gz = compressed) while checking for invalid bytes - returns 0 (true to shell) if no bad characters were found, 1 (false) otherwise";
    string excluderange,excludechars;
    bool warn=false,quiet=false;
    
    options_description general("General options");
    general.add_options()
        ("help,h", bool_switch(&help), "show usage/documentation")
        ("infile,i",value<Infile>(&in)->default_value(default_in,"STDIN"),"Input file")
        ("outfile,o",value<Outfile>(&out)->default_value(default_out_none,"none"),"Output file")
        ("inpath",value<string>(&inpath),"Input file")
        ("outpath",value<string>(&outpath),"Output file (or directory, like cp)")
        ("exclude-byterange,b",value<string>(&excluderange)->default_value("0-31,^10,^13"),"Comma separated (numeric bytes, e.g. 32,40,41 - also ranges e.g. 32-40, also ^50 or ^50-60 means remove from set")
        ("exclude-charrange,c",value<string>(&excludechars)->default_value(""),"As exclude-byterange, but with single letters and optional commas, e.g. 'a-zA-Z0-9_^x-z' - \\ escapes e.g. \\^ escapes ^ (note: this option is processed second and is additive with --exclude-byterange")
        ("warn,w",bool_switch(&warn),"don't terminate on excluded bytes - just print warning to STDERR")
        ("quiet,q",bool_switch(&quiet),"just give summary at end")
        ;
    options_description cosmetic("Cosmetic options");
    cosmetic.add_options()
        ("log-level,L",value<int>(&log_level)->default_value(1),"Higher log-level => more status messages to logfile (0 means absolutely quiet)")
        ;

    options_description all("Allowed options");
    
    positional_options_description positional; positional.add("inpath", 1); positional.add("outpath",1);

    all.add(general).add(cosmetic);

    const char *progname=argv[0];
    try {
        variables_map vm;
        command_line_parser cl(argc,argv);
        cl.options(all);
        cl.positional(positional);
        store(cl.run(),vm);
        notify(vm);
#ifdef DEBUG
        DBP::set_logstream(&cerr);
        DBP::set_loglevel(log_level);
#endif
        if (help || inpath.empty() ) { //vm.count("inpath") == 0
            cerr << progname << ' ' << "version " << VERSION << ":\n";
            cerr << all << "\n";
            cerr << usage_str << "\n";
            return 1;
        }

/////////////////////// ACTUAL PROGRAM:

        set<unsigned char> naughty;
        if (!parse_range_as<unsigned>(excluderange,naughty))
            throw runtime_error(excluderange.append(" - bad range format"));
        if (!parse_range_as<char>(excludechars,naughty))
            throw runtime_error(excludechars.append(" - bad range format"));
        const unsigned max_char=(unsigned((unsigned char)-1));
        bool isnaughty[max_char+1];
        for (int i=0;i<=max_char;++i)
            isnaughty[i]=false;
        for (set<unsigned char>::const_iterator i=naughty.begin(),e=naughty.end();i!=e;++i) {
//            if (*i > max_char) throw runtime_error("Impossible character value (greater than maximum representible!");
//            assert(*i <= max_char); // range of data type -> always true
            isnaughty[*i]=true;
        }

        in=infile(inpath);
        
        if (!outpath.empty())
            out=outfile(output_name_like_cp(inpath,outpath));        
        
        const int bufsize=DEFAULT_BUFSIZE;
        char buf[bufsize];
        streambuf * ibuf = in->rdbuf();
        streambuf * obuf = (out ? out->rdbuf() : NULL);
        streamsize n_read;
        unsigned line=1,col=0,byteoffset=0;
        unsigned nbad;
        
        while ((n_read=ibuf->sgetn(buf,bufsize))) {
            for (char *p=buf,*e=buf+n_read;p!=e;++p,++byteoffset) {
                unsigned char c=*p;
                if (c=='\n') {
                    ++line;
                    col=0;
                } else
                    ++col;
                if (isnaughty[c]) {
                    ++nbad;
                    if (!quiet || !warn)
                        cerr << "ERROR: Illegal byte " << (unsigned)c << " ('" << (char)c<<"') line="<<line<<", column="<<col<<", offset="<<byteoffset<<endl;                    
                    if (!warn)
                        return 1;                    
//                        throw runtime_error("Illegal input byte");                    
                }                
            }
            if (obuf)
                obuf->sputn(buf,n_read);
        }
        if (nbad)
            cerr << "ERROR: ";
        cerr << nbad <<" illegal bytes found (out of "<<byteoffset<<" total) in input " << endl;
        if (nbad)
            return 1;
/////////////////////            
    }
    catch(exception& e) {
        cerr << "ERROR: " << e.what() << "\n\n";
        cerr << "Try '" << progname << " -h' for documentation\n";
        BackTrace::print_on(cerr);
        return 1;
    }
    catch(...) {
        cerr << "FATAL: Exception of unknown type!\n\n";
        return 2;
    }
    return 0;
}
MAIN_END
