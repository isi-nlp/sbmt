#define GRAEHL__SINGLE_MAIN
#include "main.hpp"
#include "makestr.hpp"

#define VERSION "0.1"


#ifndef GRAEHL_TEST
#include <iostream>
#include <algorithm>
#include <boost/program_options.hpp>
#include "fileargs.hpp"
#include "debugprint.hpp"
#include "backtrace.hpp"
#include "2hash.h"
#include "dynarray.h"
#include <cctype>
#include "fileheader.hpp"
#include "funcs.hpp"
#include <vector>

using namespace boost;
using namespace std;
using namespace boost::program_options;

inline ostream & write_my_header(ostream &o) {
    return write_header(o) << " count-id-freq version={{{" VERSION " " MAKESTR_DATE "}}} ";
}


// COUNT ONLY FIRST PER LINE:
#include "threadlocal.hpp"

typedef unsigned ItemId;
typedef size_t InCount; // may be larger than Count; in fact, recommended to allow callers to benefit from clamping to MaxCount
typedef unsigned char Count; // TODO: template these
static const Count MaxCount=(Count)-1;

struct counts_seen 
{
    typedef DynamicArray<ItemId> SeenList;
    typedef vector<bool> Seens; // because they usually implement a bit-vector specialization (I didn't for DynamicArray)
    static THREADLOCAL Seens *seens;
    static THREADLOCAL SeenList *seenlist;
    typedef DynamicArray<Count> Counts;
    struct local_seens {
        SetLocal<Seens *> setl;
        SetLocal<SeenList *> setl2;
        local_seens(Seens *new_seens, SeenList *new_seenlist) : setl(seens,new_seens),setl2(seenlist,new_seenlist) {}
    };
    void count(ItemId i)
    {
        Count &count=counts[i];
        if (!MaxCount || count != MaxCount)
            ++count;
    }
    
    //post: returns if i was already seen, and marks i as seen.
    bool first_sighting(ItemId i)
    {
        resize_up_for_index(*seens,i);
        if (!(*seens)[i]) {
            (*seens)[i]=true;
            seenlist->push_back(i);
            return true;
        }        
        return false;
    }    
    Counts counts;
    void count_first_sighting(ItemId i) {
        if (first_sighting(i)) {
            count(i);
        } else {
//            DBPC3("already seen",this,count);
        }
    }
    Count get_count(ItemId i) const {
        return counts[i];
    }
    void forget_seeing(ItemId i) {
        (*seens)[i]=false;
    }
    unsigned N;
    
    void forget_seeing_all() {
        ++N;
        for (Seens::iterator i=seens->begin(),e=seens->end();i!=e;++i)
            forget_seeing(*i);
        seens->clear();
    }
    counts_seen() : N(0) {}
};



struct freqfreq
{    
    size_t maxfreq;
    typedef FixedArray<Count> FreqFreq;
    FreqFreq ff;
    freqfreq(size_t maxfreq_) : maxfreq(maxfreq_), ff(maxfreq_)
    {        
    }
    void push_back(size_t freq) 
    {
        maybe_decrease_min(freq,maxfreq);
        ++ff[freq];
    }
    size_t operator[](size_t i)
    {
    }
    
};

    

struct freqbins 
{
    
    struct bin
    {
        Count 
    };
    
        
    typedef DynamicArray<unsigned> FreqHistPosts; // only the first (boundary) of FreqHistogram
    FreqHistPosts fhbounds;
//    typedef std::pair<unsigned,unsigned> FreqSince;
//    typedef DynamicArray<FreqSince> FreqHistogram; // (first,second): second total counts of events with freqs on (first-prev,first)
//    FreqHistogram fh;
    size_t binwidth;
    freqbins(const FreqFreq &ff,size_t bin_min_totalcount) : binwidth(bin_min_totalcount) 
    { // compute histograms of preferred mass binwidth (last may be less than binwidth - sorry - no search done here)
        unsigned inbin=0;
        unsigned freq=1;
        for (;freq <= maxff;++freq) {
            inbin += freq*ff[freq];
            if (inbin >= binwidth) {
            outbin:
                if (binfile.get())
                    *binfile << freq << "\t" << inbin << "\n";
//                        fh.push_back(make_pair(freq,inbin));
                fhbounds.push_back(freq);
                inbin=0;
            }
        }
        if (inbin) // if anything left, output final partial bin
            goto outbin;

    }
    
};

    

template <class Count=unsigned short,size_t MaxCount=0>
struct folds 
{
    typedef counts_seen<Count,MaxCount> CountsSeen;
    typedef FixedArray<CountsSeen> Folds;
    Folds folds;
    size_t maxi;
    typename CountsSeen::Seens s;
    typename CountsSeen::SeenList sl;
    typename CountsSeen::local_seens guard;
    
    folds(unsigned n,size_t reserve_=0)  : folds(n),maxi(0),guard(&s,&sl),totalN(0)
    {
        reserve(reserve_);
    }
    void reserve(size_t n) 
    {
        if (n)
            for (Folds::iterator i=folds.begin(),e=folds.end();i!=e;++i)            
                counts.reserve(reserve);
    }
    size_t get_maxi()
    {
        if (!maxi)
            for (Folds::iterator i=folds.begin(),e=folds.end();i!=e;++i)            
                maybe_increase_max(maxi,i->size());
        for (Folds::iterator i=folds.begin(),e=folds.end();i!=e;++i)
            (*i)(maxi); // maxi is now a valid index
        return maxi;
    }
    size_t get_count_in(unsigned i,unsigned fold) const
    {
        return folds[folds].get_count(i);
    }    
    size_t get_totalcount(unsigned i) const
    {
        size_t sum=0;
        for (Folds::const_iterator i=folds.begin(),e=folds.end();i!=e;++i)
            sum+=i->get_count(i);
        return sum;
    }
    size_t get_totalcount_except(unsigned i,unsigned exclude) const
    {
        return get_totalcount(i)-get_count_in(i,exclude);
    }

    size_t totalN;

    size_t get_totalN() 
    {
        if (!totalN)
            for (Folds::iterator i=folds.begin(),e=folds.end();i!=e;++i)
                totalN += i->N;
        return totalN;
    }
    
            

    
    
};

    

inline std::ostream & operator <<(std::ostream &o,const count_seen &cs) {
    return o << cs.get_count();
}


#ifdef MAIN
THREADLOCAL count_seen::Seens *count_seen::seens;
#endif

MAIN_BEGIN
{
    DBP_INC_VERBOSE;
#ifdef DEBUG
    DBP::set_logstream(&cerr);
#endif
//DBP_OFF;

    Infile infile;
    Outfile outfile,freqfile,binfile;
    bool help;

    int log_level;
    size_t reserve, linesin, nfolds, binwidth;

    bool sparse;

    string usage_str="Expects positive integers (ignoring other characters), counting the number of lines on which each occurs.\nOutput by default is one per line counts starting with id 1 on line 1 (change that with --sparse)\nYou may run out of memory if your IDs approach billions.";

    options_description general("General options");
    general.add_options()
        ("help,h", bool_switch(&help), "show usage/documentation")
        ("infile,i",value<Infile>(&infile)->default_value(default_in,"STDIN"),"Array or list of weights from here")
        ("count-out,c",value<Outfile>(&outfile)->default_value(default_out,"STDOUT"),"Write per-id counts here")
        ("freq-count-out,f",value<Outfile>(&freqfile)->default_value(default_out,"STDOUT"),"Write frequency of frequency (count freq(count) one per line) here")
        ("bin-freq-out,b",value<Outfile>(&binfile)->default_value(default_out,"STDOUT"),"Write bin histogram (count, sum of #items *count' for count'<=count and > last count)")
        ("linesin,l",value<size_t>(&linesin)->default_value(0),"Assume that infile has this many lines (otherwise, with the default of 0, the input file will have to be a disk file to allow two input passes)")
        ("nfolds,n",value<size_t>(&nfolds)->default_value(1),"If >1, report rules/counts for n equal-size chunks of the linesin input lines")
        ("binwidth,w",value<size_t>(&binwidth)->default_value(4),"If non-zero, collect histogram bin statistics based on frequencies such that sum(frequency*#events with frequency) is >= b")
        ("reserve,r",value<size_t>(&reserve)->default_value(1000*1000),"Hint: maximum integer expected (overestimate better than under...) (k, m, g suffix: 2^10, 2^20, 2^30)")
        ;
    options_description cosmetic("Cosmetic options");
    cosmetic.add_options()
        ("log-level,L",value<int>(&log_level)->default_value(1),"Higher log-level => more status messages to logfile (0 means absolutely quiet except for final statistics)")
        ("sparse,s",bool_switch(&sparse),"output nonzero counts as id count")
        ;


    options_description all("Allowed options");

    positional_options_description positional; positional.add("infile", 1);

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

        if (help) {
            cerr << progname << ' ' << "version " << VERSION << ":\n";
            cerr << all << "\n";
            cerr << usage_str << "\n";
            return 0;
        }

        DBP(reserve);

        if (nfolds<1)
            nfolds=1;
        
        typedef FixedArray<count_seen::Counts> Folds;
        Folds folds(nfolds);
        if (reserve) 

        count_seen::Seens s(10000);
        count_seen::local_seens guard(&s);

        // actual work!

        istream &in=*infile;
        remove_ws<'\n'>(in); //memory leak; don't care

        unsigned id;
        unsigned n=0,line=1;
        const char *reason="couldn't read a character";

        double ifold=0;
        double endline=HUGE_VAL;
        
        Folds::iterator b=folds.begin(),i,e=folds.end();
        i=b;
        
            count_seen::Counts &counts=*i;
            count_seen::forget_seeing_all();
            ++ifold;
            //INPUT: (lines of ids and other characters, '\n' terminated)
            {
                DBP_INC_VERBOSE;
                while(in) {
                    char c;
                    if (!(in >>c))
                        break;
                    if (c=='\n') {
                        ++line;
                        count_seen::forget_seeing_all();
                        ++i;
                        if (i==e)
                            i=b;
                    }
                    if (std::isdigit(c)) //FIXME: use in's char_traits
                        in.unget();
                    else
                        continue;
                    if (!(in>>id))
                        break;
                    DBP2(line,id);
                    if (!id)
                        throw_input_error(in,"read unexpected id=0","line",line);
                    counts(id).count_first_sighting();
                    ++n;
                }
                if (!in.eof())
                    throw_input_error(in,reason,"line",line);
                cerr << "Read " << n << " ids over " << line <<" lines"<<", with max id="<<counts.size()-1<<endl;
            }
        }
        
        //COMPUTE:
        // freqs of freqs: (compute by sorting, or by hash+count?)
        // let's do freq of freq hash because ... could do SGT in here later?
        {
            unsigned maxff;
            for (count_seen::Counts::iterator b=counts.begin(),i=b+1,e=counts.end();i!=e;++i)
                ff(i->get_count())++; // note: 0-init, and we'll get some value for # of rules with 0 counts (not terribly meaningful as it only includes those created during GHKM and not all potential unseen rules)
            maxff=ff.size()-1;
            if (freqfile.get()) {
                write_my_header(*freqfile) << " (suitable for Simple Good-Turing): count, frequency-of-count\n";
                for (unsigned freq=1;freq <= maxff;++freq) {
                    const unsigned nfreq=ff[freq];
                    if (nfreq)
                        *freqfile << freq << "\t" << nfreq << "\n";
                }
            }

            if (binfile.get())
                write_my_header(*binfile) << " (histogram of frequency of frequency): count, sum-of-counts-since-last-count\n";

            }
        }

        //OUTPUT id/line-count:
        if (outfile.get()) {
            if (sparse) {
                write_my_header(*outfile) << " (number of lines in which ids occur): id, count-lines-with-id\n";
                for (count_seen::Counts::iterator b=counts.begin(),i=b+1,e=counts.end();i!=e;++i)
                    if (i->get_count())
                        *outfile<<i-b<<"\t"<<i->get_count()<<"\n";
            } else {
                print_range(*outfile,counts.begin()+1,counts.end(),true,false); // print (first line is id 1)
            }
        }

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

#endif

