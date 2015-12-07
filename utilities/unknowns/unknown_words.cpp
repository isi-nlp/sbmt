// application unknown_words
//   given a set of source words, and an xrs file, output which words are 
//   unknown to the xrs file.  suitable for chaining when working with 
//   multiple grammars.

# include <iostream>
# include <string>
# include <set>

# include <boost/program_options.hpp>
# include <boost/filesystem/path.hpp>
# include <boost/regex.hpp>

# include <sbmt/grammar/syntax_rule.hpp>

# include <boost/iostreams/filtering_stream.hpp>
# include <boost/iostreams/device/file.hpp>
# include <boost/iostreams/filter/gzip.hpp>

# include <gusc/filesystem_io.hpp>

bool unary_lex_rule(sbmt::fat_syntax_rule const& rule)
{
    return rule.rhs_size() == 1 and is_lexical(rule.rhs_begin()->get_token());
}

void remove_from_set(std::set<std::string>& words, sbmt::fat_syntax_rule const& rule)
{
    if (unary_lex_rule(rule)) {
        std::string word = rule.rhs_begin()->get_token().label();
        words.erase(word);
    }
}

namespace fs = boost::filesystem;
namespace po = boost::program_options;
namespace io = boost::iostreams;

boost::regex gz("^.+\\.gz$");

struct options {
    fs::path gramfile;
    bool gzipped;
    options() : gzipped(false) {}
};

options parse_options(int argc, char** argv)
{
    options opts;
    po::options_description desc;
    po::variables_map vm;
    
    desc.add_options()
        ( "grammar,g"
        , po::value(&opts.gramfile)
        , "xrs grammar file"
        )
        ( "help,h"
        , "print usage and exit"
        );
    
    po::positional_options_description posdesc;
    posdesc.add("xrs-grammar",-1);
    
    po::basic_command_line_parser<char> cmd(argc,argv);
    po::store(cmd.options(desc).positional(posdesc).run(),vm);
    po::notify(vm);
    
    if (vm.count("help")) {
        std::cerr << desc << std::endl;
        exit(1);
    }
    if (regex_match(opts.gramfile.filename().native(),gz)) opts.gzipped = true;

    return opts;
}

int main(int argc, char** argv)
{
    std::ios_base::sync_with_stdio(false);
    options opts = parse_options(argc,argv);
    
    using namespace std;
    
    set<string> words;
    copy( istream_iterator<string>(cin)
        , istream_iterator<string>()
        , inserter(words,words.end())
        );
    
    io::filtering_stream<io::input> gramstr;
    if (opts.gzipped) gramstr.push(io::gzip_decompressor());
    gramstr.push(io::file_source(opts.gramfile.string()));
    
    string line;
    while (std::getline(gramstr,line)) {
        try {
            sbmt::fat_syntax_rule rule(line,sbmt::fat_tf);
            remove_from_set(words,rule);
        } catch (...) {
            clog << "skipping line: " << line << endl;
        }
    }
    
    copy(words.begin(), words.end(), ostream_iterator<string>(cout,"\n"));
    
    return 0;
}
