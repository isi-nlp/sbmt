#define GRAEHL__SINGLE_MAIN
#define SINGLE_PRECISION
//#define HINT_SWAPBATCH_BASE
#include <graehl/shared/config.h>
#include <graehl/shared/main.hpp>

#define VERSION "0.5"

#ifndef GRAEHL_TEST
#include <iostream>
#include <graehl/shared/weight.h>
#include <algorithm>
#include <boost/program_options.hpp>
#include <graehl/shared/fileargs.hpp>
#include <graehl/shared/filelines.hpp>
#include <graehl/shared/debugprint.hpp>
#include <graehl/shared/backtrace.hpp>
#include <graehl/shared/randomreader.hpp>
#include <graehl/shared/statistics.hpp>

//using namespace boost;
using namespace std;
using namespace boost::program_options;
using namespace graehl;

MAIN_BEGIN
{
    DBP_INC_VERBOSE;
#ifdef DEBUG
    DBP::set_logstream(&cerr);
#endif
//DBP_OFF;

    istream_arg infile;
    ostream_arg outfile;
    bool help,version,human_probs,reverse;

    int log_level;
    size_t partial,reserve=1024*1024;
    double quantile,sample_prob;
    bool number_lines,quantile_lines;
    bool minmax;

    options_description general("General options");
    general.add_options()
        ("help,h", bool_switch(&help), "show usage/documentation")
        ("version,v", bool_switch(&version), "print the version number")
        ("reverse,R", bool_switch(&reverse), "reverse the order of output (from greatest to least)")
        ("infile,i",value<istream_arg>(&infile)->default_value(stdin_arg(),"STDIN"),"Array or list of weights from here")
        ("outfile,o",value<ostream_arg>(&outfile)->default_value(stdout_arg(),"STDOUT"),"Write result here")
        ("reserve,r",value<size_t>(&reserve)->default_value(reserve,"1M"),"Hint: how many items to be sorted (k, m, g suffix: 2^10, 2^20, 2^30)")
        ("nbest,n",value<size_t>(&partial)->default_value(0),"Only output the n highest values - 0 means all")
        ("quantile,q",value<double>(&quantile)->default_value(0),"Only output values at or above the Pth quantile (the top P of them)")
        ("prob-sample,p",value<double>(&sample_prob)->default_value(1),"Accept an input with probability p - if sorting takes too long or the input is too large, try p<1")
        ("minmax,m",bool_switch(&minmax), "replace the sample minimum and maximum values with the true minimum and maximum")
        ;
    options_description cosmetic("Cosmetic options");
    cosmetic.add_options()
        ("log-level,L",value<int>(&log_level)->default_value(1),"Higher log-level => more status messages to logfile (0 means absolutely quiet except for final statistics)")
        ("human-probs,H",bool_switch(&human_probs),"Display probabilities as plain real numbers instead of logs")
        ("number-lines,N",bool_switch(&number_lines),"Prepend line numbers to output (starting with 1)")
        ("quantile-lines,Q",bool_switch(&quantile_lines),"Prepend quantile ([0.0-1.0] as a portion of total input) to output")
        ;


    options_description all("Allowed options");

    positional_options_description positional; positional.add("infile", 1);
    
    all.add(general).add(cosmetic);

    const char *progname=argv[0];
    try {
        variables_map vm;
#if 0
        command_line_parser cl(argc,argv);
        cl.options(all);
        cl.positional(positional);        
        store(cl.run(),vm);
#else     
        store(parse_command_line(argc, argv, all), vm);
#endif
        notify(vm);
        
#ifdef DEBUG
        DBP::set_logstream(&cerr);
        DBP::set_loglevel(log_level);
#endif

        if (help) {
            cerr << progname << ":\n";
            cerr << all << "\n";
            return 0;
        }
        if (version) {
            cerr << progname << ' ' << "version " << VERSION << std::endl;
            return 0;
        }

        if (human_probs)
            Weight::default_never_log();
        else
            Weight::default_always_log();
        Weight::default_exp();

        dynamic_array<Weight> weights(reserve);

        typedef min_max_accum<Weight> Accum;
        Accum exact_minmax;

#if 0
        if (sample_prob < 1)
            weights.read(*infile,RandomReaderTerm<Weight>(sample_prob));
        else
            *infile >> weights;
#else
        if (minmax) {
            typedef boost::reference_wrapper<Accum> RefAccum;
            RandomReaderTermCallback<Weight,RefAccum> reader(boost::ref(exact_minmax),sample_prob);
            weights.read(*infile,reader);
        } else {
            if (sample_prob < 1)
                weights.read(*infile,RandomReaderTerm<Weight>(sample_prob));
            else
                *infile >> weights;
        }
#endif
        if (!*infile && !infile->eof()) {
            throw runtime_error("couldn't read weights.");
        }
        size_t N=weights.size();

        size_t ntile=1+(size_t)((N-1) *(1- quantile));
        if (!partial || partial > ntile)
            partial=ntile;
        if (partial > N)
            partial=N;

        dynamic_array<Weight>::iterator b=weights.begin(),
            m=weights.begin()+partial,
            e=weights.end();

        if (m!=e)
            partial_sort(b,m,e,std::greater<Weight>());
        else
            sort(b,e,std::greater<Weight>());

        cerr << "Top " << partial << " out of " << N << " weights ";
        Weight min,max;


        size_t i=0;
        float oon=N>1 ? 1./(N-1) : 1;

        if (minmax) {
            *b=exact_minmax.maximum;
            *(m-1)=exact_minmax.minimum;
        }

#define LINEPRE                 if (number_lines) *outfile << i+1 << ' '; if (quantile_lines) *outfile << i*oon << ' '
        if (reverse) {
            cerr<<" in reverse (best to worst) order:\n";
            for (;b<m;++b,++i) {
                LINEPRE;
                *outfile << *b << '\n';
            }
        } else {
            cerr<< ":\n";
            for (--m;m>=b;--m,++i) {
                LINEPRE;
                *outfile << *m << '\n';
            }
        }

    }
    catch(std::exception& e) {
        cerr << "ERROR: " << e.what() << "\n\n";
        cerr << "Try '" << progname << " -h' for documentation\n";
        BackTrace::print(cerr);
        return 1;
    }
    catch(...) {
        cerr << "FATAL: Exception of unknown type!\n\n";
        return 2;
    }
    return 0;
}
MAIN_END

#endif

