# include <boost/archive/binary_oarchive.hpp>
# include <boost/iostreams/device/file_descriptor.hpp>
# include <boost/filesystem/path.hpp>
# include <boost/filesystem/operations.hpp>
# include <boost/filesystem/fstream.hpp>
# include <boost/date_time/posix_time/posix_time.hpp>
# include <boost/cstdint.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/regex.hpp>

# include <word_cluster.hpp>
# include <filesystem.hpp>
# include <collapsed_signature_iterator.hpp>
# include <syntax_rule_util.hpp>

# include <iostream>
# include <stdexcept>
# include <iterator>
# include <cstdlib>

namespace sbmt {

std::istream& operator >> (std::istream& in, indexed_token& tok)
{
    boost::uint32_t idx;
    in >> idx;
    tok = indexed_token(idx);
    return in;
}

}

using namespace boost;
using namespace boost::posix_time;
using namespace boost::filesystem;
using namespace boost::serialization;
using namespace boost::archive;
using namespace sbmt;
using namespace xrsdb;

using std::vector;
using std::string;
using std::swap;
using std::stringstream;
using std::istream_iterator;
using std::ostream_iterator;
using std::exception;
using std::cin;
using std::cerr;
using std::clog;
using std::endl;

////////////////////////////////////////////////////////////////////////////////

tuple<path,indexed_token,vector<indexed_token>,string>
procline(string const& s)
{
    static regex splt("([^\\t]*)\\t([^\\t]*)\\t([^\\t]*)\\t(.*)\\s*");
    smatch what;
    regex_match(s,what,splt);

    tuple<path,indexed_token,vector<indexed_token>,string>
        tpl( lexical_cast<path>(what.str(1))
           , lexical_cast<indexed_token>(what.str(2))
           , vector<indexed_token>()
           , what.str(4)
           );

    stringstream sstr(what.str(3));
    copy( istream_iterator<indexed_token>(sstr)
        , istream_iterator<indexed_token>()
        , back_inserter(tpl.get<2>())
        );

    return tpl;
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
    std::ios_base::sync_with_stdio(false);
    bool debug = false;
    if (argc >= 3 and strcmp(argv[2],"-d") == 0) debug = true;
    path dbdir(argv[1]);

    string line;
    header h;
    load_header(h, dbdir);
    indexed_token wildcard = h.wildcard();
    indexed_token rarest;
    path groupid;
    vector<indexed_token> keys;
    string rule;
    
    boost::filesystem::ofstream ofs;
    const char* tmpstr = getenv("TMPDIR");
    if (not tmpstr) tmpstr = "/tmp";
    path tmp(tmpstr);
    path tmpfile;
    time_duration reading, writing, converting;
    word_cluster_construct dbc(rarest);
    size_t numlines=0;
    while (getline(cin,line)) {
        try {
            tie(groupid,rarest,keys,rule) = procline(line);
            //cerr << rarest << '\n';
            word_cluster_construct(rarest).swap_self(dbc);
            dbc.insert(keys, rule);
            ++numlines;
        }  catch (std::exception const& e) {
            cerr << "could not process line: " << line << ".  "
                 << "msg: " << e.what() << "\n" << endl;
            continue;
        }
        tmpfile = tmp / lexical_cast<std::string>(rarest);
        
        ofs.open(tmpfile);
        ptime start_read = microsec_clock::local_time();
        ptime start_write;
        ptime start_convert;

        while(getline(cin,line)) {
            try {
                path gid;
                tie(gid,rarest,keys,rule) = procline(line);
                indexed_token r = rarest;
                
                if (r != dbc.root_word()) {
                    //cerr << r << '\n';
                    start_convert = microsec_clock::local_time();
                    reading += start_convert - start_read;
                    word_cluster wc(dbc);

                    start_write = microsec_clock::local_time();
                    converting += start_write - start_convert;
                    std::cout << dbc.root_word() << '\t' 
                              << groupid << '\t' 
                              << uint64_t(ofs.tellp()) << std::endl; 
                    save_word_cluster(wc,ofs);
                    word_cluster_construct newdbc(r);
                    swap(dbc,newdbc);

                    start_read = microsec_clock::local_time();
                    writing += start_read - start_write;
                }
                if (gid != groupid) {
                    ofs.close();
                    copy_file(tmpfile,dbdir / groupid);
                    remove(tmpfile);
                    tmpfile = tmp / lexical_cast<std::string>(rarest);
                    ofs.open(tmpfile);
                    groupid = gid;
                }
                dbc.insert(keys,rule);
                ++numlines;
                
            } catch (std::exception const& e) {
                cerr << "could not process line: " << line << ".  \n"
                     << rarest << " &&& " << rule << endl
                     << "msg: " << e.what() << "\n" << endl;
            }
        }
        std::cout << dbc.root_word() << '\t' 
                  << groupid << '\t' << uint64_t(ofs.tellp()) << std::endl;
        save_word_cluster(dbc,ofs);
        ofs.close();
        copy_file(tmpfile, dbdir / groupid);
        remove(tmpfile);
    }

    clog << "populated database with "<< numlines << " lines" << endl;
    clog << "time spent writing:   " << writing << endl;
    clog << "           reading:   " << reading << endl;
    clog << "           converting:" << converting << endl;

    return 0;
}
