//FIXME: why does any whitespace after force-decode { } (incl. newline) give parse error (!parse.full?)
//#define  BOOST_SPIRIT_DEBUG
#define BOOST_SPIRIT_THREADSAFE 1
#include "decode_sequence_reader.hpp"

#ifdef _WIN32
#include <iso646.h>
#endif

#include <iostream>
#include <fstream>
#include <sstream>

#include <sbmt/sentence.hpp>
#include <sbmt/feature/feature_vector.hpp>
#include <sbmt/edge/any_info.hpp>
#include "lattice_grammar.hpp"
#include "any_info_grammar.hpp"
#include <gusc/phoenix_helpers.hpp>
#include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/classic_assert.hpp>
#include <boost/spirit/include/classic_actor.hpp>
#include <boost/spirit/include/phoenix1.hpp>
/*
#include <boost/spirit/core.hpp>
#include <boost/spirit/error_handling/exceptions.hpp>
#include <boost/spirit/utility/distinct.hpp>
#include <boost/spirit/utility/confix.hpp>
#include <boost/spirit/utility/escape_char.hpp>
#include <boost/spirit/actor/assign_actor.hpp>
#include <boost/spirit/actor/clear_actor.hpp>
#include <boost/spirit/actor/push_back_actor.hpp>
#include <boost/spirit/actor/erase_actor.hpp>
#include <boost/spirit/iterator/multi_pass.hpp>
#include <boost/spirit/iterator/position_iterator.hpp>
#include <boost/spirit/phoenix/statements.hpp>
*/
#include <boost/algorithm/string/trim.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/regex.hpp>
#include <graehl/shared/fileargs.hpp>

using namespace boost::filesystem;
using namespace std;
using namespace boost;
using namespace boost::spirit::classic;
using namespace phoenix;

struct decode_sequence_data
{
    sbmt::fat_weight_vector weights;
    sbmt::archive_type archive_fmt;
    string filename;
    string lex_string;
    string weight_string;
    string sentence;
    string target_sentence;
    string dynamic_ngram_init;
    string change_options_cmdline;
    std::vector<std::string> passes;
    std::vector<std::string> forest_data;
    
    size_t sentid;
    gusc::lattice_ast ltree;
    
    boost::filesystem::path root_directory;
    
    decode_sequence_data(std::size_t sentid = 1)
    : sentid(sentid) {}
    
    decode_sequence_reader::decode_forest_cb_type decode_forest;
    decode_sequence_reader::set_weights_cb_type set_weights;
    decode_sequence_reader::load_dynamic_ngram_cb_type load_dynamic_ngram;
    decode_sequence_reader::change_options_cb_type change_options;
    decode_sequence_reader::push_inline_rules_cb_type push_inline_rules;
    decode_sequence_reader::load_grammar_cb_type load_grammar;        
    decode_sequence_reader::push_grammar_cb_type push_grammar;
    decode_sequence_reader::pop_grammar_cb_type pop_grammar;     
    decode_sequence_reader::force_decode_cb_type force_decode;
    decode_sequence_reader::decode_cb_type decode;
    decode_sequence_reader::use_info_cb_type use_info;
    decode_sequence_reader::set_info_option_cb_type set_info_option;

    void trigger_push_inline_rules()
    {
        push_inline_rules(passes);
    }
    
    void trigger_load_grammar()
    {
        load_grammar(filename,weight_string,archive_fmt);
    }
    
    void trigger_push_grammar() 
    { 
        push_grammar(filename,archive_fmt); 
    }
    
    void trigger_pop_grammar() 
    { 
        pop_grammar(); 
    }
    
    void trigger_set_weights(sbmt::fat_weight_vector const& w)
    {
        set_weights(w);
    }

    void trigger_load_dynamic_ngram()
    {
        load_dynamic_ngram(boost::trim_copy(dynamic_ngram_init),root_directory);
    }

    void trigger_change_options()
    {
        change_options(change_options_cmdline);
    }
    
    void trigger_decode()
    {
        ltree = sent_to_lattice(sentence, sentid);
        trigger_decode_lattice();
    }
    
    gusc::lattice_ast sent_to_lattice(std::string sent, size_t id)
    {
        gusc::lattice_ast lat;
        lat.insert_property("id",boost::lexical_cast<std::string>(id));
        sbmt::fat_sentence fsent = sbmt::foreign_sentence(sent);
		sbmt::fat_sentence::iterator itr = fsent.begin(), end = fsent.end();
        size_t i = 0;
        for (; itr != end; ++itr, ++i) {
            lat.insert_edge(boost::make_tuple(i,i+1),itr->label());
        }
        
        return lat;
    }
    
    void trigger_force_decode()
    {
        force_decode(boost::trim_copy(sentence),boost::trim_copy(target_sentence),sentid);
        ++sentid;
    }
    
    void trigger_decode_lattice()
    {
        
        BOOST_FOREACH(gusc::lattice_ast::key_value_pair p, ltree.properties()) {
            if (p.key() == "id") sentid = boost::lexical_cast<size_t>(p.value());
        }
        decode(ltree,sentid);
        ++sentid;
    }
    
    void trigger_decode_forest()
    {
        decode_forest(forest_data);
    }
    
    void clear();
};

decode_sequence_parse_error::decode_sequence_parse_error(string const& err)
: std::runtime_error(err) {}

void throw_decode_sequence_parse_error(string const& err)
{
    decode_sequence_parse_error e(err);
    throw e;
}

struct handler
{
    template <typename ScannerT, typename ErrorT>
    error_status<>
    operator()(ScannerT const&, ErrorT const&) const
    {
        return error_status<>(error_status<>::fail);
    }
};

struct trigger_decode
{
    trigger_decode(decode_sequence_data& act)
    : act(act) {}

    template <class IT>
    void operator()(IT begin, IT end) const
    {
        act.trigger_decode();
    }
    
    decode_sequence_data& act;    
};

struct trigger_force_decode
{
    trigger_force_decode(decode_sequence_data& act)
    : act(act) {}

    template <class IT>
    void operator()(IT begin, IT end) const
    {
        act.trigger_force_decode();
    }
    
    decode_sequence_data& act;    
};

struct trigger_decode_forest 
{
    trigger_decode_forest(decode_sequence_data& act) : act(act) {}
    template <class IT>
    void operator()(IT begin, IT end) const
    {
        act.trigger_decode_forest();
    }
    
    decode_sequence_data& act; 
};

struct trigger_decode_lattice
{
    trigger_decode_lattice(decode_sequence_data& act)
    : act(act) {}
    
    template <class IT>
    void operator()(IT begin, IT end) const
    {
        act.trigger_decode_lattice();
    }
    
    decode_sequence_data& act;
};

struct trigger_load_grammar
{
    trigger_load_grammar(decode_sequence_data& act)
    : act(act) {}

    template <class IT>
    void operator()(IT begin, IT end) const
    {
        path grampath = absolute(path(act.filename),act.root_directory);
        act.filename = grampath.native();
        act.trigger_load_grammar();
    }
    
    decode_sequence_data& act;    
};

struct trigger_push_grammar
{
    trigger_push_grammar(decode_sequence_data& act)
    : act(act) {}

    template <class IT>
    void operator()(IT begin, IT end) const
    {
        path grampath = absolute(path(act.filename),act.root_directory);
        act.filename = grampath.native();
        act.trigger_push_grammar();
    }
    
    decode_sequence_data& act;    
};

struct trigger_pop_grammar
{
    trigger_pop_grammar(decode_sequence_data& act)
    : act(act) {}

    template <class IT>
    void operator()(IT begin, IT end) const
    {
        path grampath = absolute(path(act.filename),act.root_directory);
        act.filename = grampath.native();
        act.trigger_pop_grammar();
    }
    
    decode_sequence_data& act;    
};



struct trigger_load_dynamic_ngram
{
    trigger_load_dynamic_ngram(decode_sequence_data& act)
    : act(act) {}

    template <class IT>
    void operator()(IT begin, IT end) const
    {
        act.trigger_load_dynamic_ngram();
    }
    
    decode_sequence_data& act;    
};

struct trigger_change_options
{
    trigger_change_options(decode_sequence_data& act)
    : act(act) {}

    template <class IT>
    void operator()(IT begin, IT end) const
    {
        act.trigger_change_options();
    }
    
    decode_sequence_data& act;    
};

struct trigger_push_inline_rules
{
    trigger_push_inline_rules(decode_sequence_data& act)
    : act(act) {}

    template <class IT>
    void operator()(IT begin, IT end) const
    {
        act.trigger_push_inline_rules();
    }
    
    decode_sequence_data& act;    
};


struct set_archive_format
{
    set_archive_format(decode_sequence_data& act, sbmt::archive_type fmt)
    : archive_fmt(fmt)
    , act(act) {}

    template <class IT>
    void operator()(IT begin, IT end) const
    {
        act.archive_fmt = archive_fmt;
    }
    sbmt::archive_type archive_fmt;
    decode_sequence_data& act;    
};

struct trim_lex_string 
{
    trim_lex_string(decode_sequence_data& act)
    : act(act) {}

    template <class IT>
    void operator()(IT begin, IT end) const
    {
        if (act.lex_string.size() > 0 and 
            act.lex_string[act.lex_string.size()-1] == '"')
            act.lex_string.erase(act.lex_string.size()-1);
    }

    decode_sequence_data& act;
};

struct do_print
{
    do_print(decode_sequence_data& act)
    : act(act) {}
    
    template <class IT>
    void operator()(IT begin, IT end) const
    {
        cout << "archive_format:"<< act.archive_fmt << endl;
        cout << "filename:" << act.filename << endl;
        cout << "weight_string:" << act.weight_string << endl;
        cout << "sentence:" << act.sentence << endl;
    }
    
    decode_sequence_data& act;
};

sbmt::fat_weight_vector from_file(std::string const& filename)
{
    sbmt::fat_weight_vector weights;
    string line;
    graehl::istream_arg filearg(filename);
    std::istream& file = *filearg;
    while(file and not file.eof()) {
        getline(file,line);
        sbmt::fat_weight_vector w;
        read(w,line);
        weights += w;
    }
    return weights;
}

sbmt::fat_weight_vector
set_combiner(bool file, std::string const& data)
{
    sbmt::fat_weight_vector weights;
    if (file) {
        weights = from_file(data);
    } else {
        read(weights,data);
    }
    return weights;
}

struct weights_closure : boost::spirit::classic::closure<weights_closure, sbmt::fat_weight_vector>
{
    weights_closure::member1 val;
};

struct set_weights_grammar 
: public grammar<set_weights_grammar, weights_closure::context_t> 
{
    struct string_closure
    : boost::spirit::classic::closure<string_closure,std::string> { member1 val; };
    
    struct state_closure
    : public boost::spirit::classic::closure<state_closure, sbmt::fat_weight_vector, bool, bool>{
        member1 val;
        member2 is_file;
        member3 is_diff;
    };
    
    template <class Scanner>
    struct definition {
        rule<Scanner,weights_closure::context_t> root;
        rule<Scanner,state_closure::context_t> expr_;
        rule<Scanner,string_closure::context_t> cstr;
        definition(set_weights_grammar const& self)
        {
            using namespace gusc;
            using namespace phoenix;
            cstr 
                = confix_p('"',*(c_escape_ch_p[cstr.val += arg1]),'"')
                [
                     cstr.val = construct_<std::string>(
                                    cstr.val
                                  , 0
                                  , size_(cstr.val) - 1
                                )
                ]
                ;
            
            expr_
                = 
                (str_p("weights")[expr_.is_diff = false, expr_.is_file = false])
                >>
                 ( 
                  (   (str_p("diff")[expr_.is_diff = true]) 
                  >> !(str_p("file")[expr_.is_file = true])
                  )
                 | 
                  (  !(str_p("file")[expr_.is_file = true])
                  >> !(str_p("diff")[expr_.is_diff = true])
                  )
                 )
                >> (cstr[
                    if_(expr_.is_diff) [
                        self.val += phoenix::bind(set_combiner)(expr_.is_file,arg1)
                    ].else_ [
                        self.val = phoenix::bind(set_combiner)(expr_.is_file,arg1)
                    ]
                 ])
                 >> ch_p(';')
                ;
        }
        
        rule<Scanner,state_closure::context_t> const& start() const
        {
            return expr_;
        }
    };
};

struct decode_sequence_grammar : public grammar<decode_sequence_grammar>
{
    decode_sequence_grammar(decode_sequence_data& sem)
    : sem(sem) {}
    
    template <typename ST>
    struct definition
    {
        rule<ST> sequence_p;
        rule<ST> load_ngram_p;
        rule<ST> load_grammar_p;
        rule<ST> push_grammar_p;
        rule<ST> pop_grammar_p;
        rule<ST> decode_p;
        rule<ST> force_decode_p;
        rule<ST> decode_lattice_p;
        rule<ST> decode_forest_p;
        rule<ST> set_info_opt_p;
        rule<ST> use_info_p;
        rule<ST> target_p;
        rule<ST> source_p;
        rule<ST> file_p;
        rule<ST> weight_string_p;
        rule<ST> lex_string_p;
        rule<ST> until_newline_p;
        rule<ST> until_empty_line_p;
        rule<ST> not_semicolon_line_p;

        rule<ST> push_inline_rules_p;
        rule<ST> newline_p;
        
        rule<ST> archive_file_p;
        rule<ST> load_dynamic_ngram_p;
        rule<ST> change_options_p;
        
        distinct_parser<> keyword_p;
        boost::spirit::classic::assertion<int> expect;
        guard<int> grd;
        
        sg::use_info_grammar use_info_gram;
        sg::set_info_option_grammar set_info_opt_gram;
        block_lattice_grammar lattice_gram;
        set_weights_grammar set_weights;
        
        definition(decode_sequence_grammar const& self)
        : keyword_p("a-zA-Z0-9_\\-")
        , expect(0)
        , set_info_opt_gram(self.sem.set_info_option)
        {
            using namespace phoenix;
            ////////////////////////////////////////////////////////////////////
            //
            // $sequence := ($load_ngram | $load_grammar | $decode)+
            // $load_ngram := load-ngram $file ;
            // $load_grammar := load-grammar ( $weight_string ($file|$archive) |
            //                  ($file|$archive) [$weight_string]) ;
            // $decode := decode " $escape_seq " ;
            //
            ////////////////////////////////////////////////////////////////////
            sequence_p 
                =   *(
                        ( load_ngram_p [do_print(self.sem)]
                        | load_dynamic_ngram_p [trigger_load_dynamic_ngram(self.sem)]
                        | change_options_p [trigger_change_options(self.sem)]                           
                        | load_grammar_p [trigger_load_grammar(self.sem)]
                        | push_grammar_p [trigger_push_grammar(self.sem)]
                        | pop_grammar_p [trigger_pop_grammar(self.sem)] 
                        | decode_p [trigger_decode(self.sem)]
                        | force_decode_p [trigger_force_decode(self.sem)]
                        | decode_lattice_p [trigger_decode_lattice(self.sem)]
                        | decode_forest_p [trigger_decode_forest(self.sem)]
                        | push_inline_rules_p [trigger_push_inline_rules(self.sem)]
                        | set_info_opt_p 
                        | use_info_p 
                        | set_weights(var(self.sem.weights)) [ phoenix::bind(&decode_sequence_data::trigger_set_weights)(var(self.sem), var(self.sem.weights) = arg1)]
                        )
                     )
                ;

            
            load_ngram_p 
                =   keyword_p("load-ngram") 
                    >> grd(expect(file_p))[handler()]
                    >> ch_p(';')
                ;

            load_dynamic_ngram_p
                = keyword_p("load-dynamic-ngram")
                >> (until_newline_p [ assign_a(self.sem.dynamic_ngram_init) ])
                // >> newline_p
                ;
            
                         
            change_options_p
                = keyword_p("change-options")
                >> (until_newline_p [ assign_a(self.sem.change_options_cmdline) ])
                // >> newline_p
                ;

            load_grammar_p 
                =   keyword_p("load-grammar") 
                    >> grd(expect(    
                            (weight_string_p >> grd(expect(archive_file_p))[handler()])
                            |
                            (archive_file_p >> (!weight_string_p))
                       ))[handler()]
                    >> ch_p(';')
                ;
    
            push_grammar_p 
                =   keyword_p("push-grammar") >> archive_file_p >> ';' ;
                
            pop_grammar_p = keyword_p("pop-grammar") >> ';' ;
            
            decode_p 
                =   ( str_p("decode[") >> (uint_p[assign_a(self.sem.sentid)])
                                       >> str_p("]") 
                    | keyword_p("decode")
                    )
                >> (until_newline_p [ assign_a(self.sem.sentence) ])
                // >> newline_p
                ;

            push_inline_rules_p
                = keyword_p("push-inline-rules") [clear_a(self.sem.passes)]
                >> *(not_semicolon_line_p [push_back_a(self.sem.passes)])
                >> ch_p(';')
                ;
                
            source_p
                = keyword_p("source") >> ch_p(':')
                                      >> (until_newline_p [ assign_a(self.sem.sentence) ])
                                      // >> newline_p
                
                ;
                
            target_p
                = keyword_p("target") >> ch_p(':')
                                      >> (until_newline_p [ assign_a(self.sem.target_sentence) ])
                                      // >> newline_p
                ; 
                
            force_decode_p
                =   ( str_p("force-decode[") >> (uint_p[assign_a(self.sem.sentid)])
                                       >> str_p("]") 
                    | keyword_p("force-decode")
                    ) 
                    >> ch_p('{')
                    >> ( (target_p >> source_p) | (source_p >> target_p ) )
                    >> ch_p('}') >> !ch_p(';')
                ;
                
            set_info_opt_p
                = set_info_opt_gram >> ch_p(';')
                ;
            
            use_info_p
                =  use_info_gram [ phoenix::bind(self.sem.use_info)(arg1) ]
                >> ch_p(';')
                ;
            
            decode_lattice_p
                =     lattice_gram[var(self.sem.ltree) = arg1] 
                   >> !ch_p(';')
                ;    
                     
            lex_string_p 
                =   lexeme_d[
                        epsilon_p [clear_a(self.sem.lex_string)]
                        >>
                        confix_p( 
                            "\""
                          , +(lex_escape_ch_p [push_back_a(self.sem.lex_string)])
                          , "\""
                        )
                        >> epsilon_p [ trim_lex_string(self.sem) ]
                    ]
                ;
            decode_forest_p = lexeme_d[str_p("forest\n")[clear_a(self.sem.forest_data)] 
                            >> (+(anychar_p - (eol_p|end_p)))[push_back_a(self.sem.forest_data)] >> (eol_p|end_p)
                            >> +((+(anychar_p - (eol_p|end_p)))[push_back_a(self.sem.forest_data)] >> (eol_p|end_p))
                            >> (eol_p|end_p) ] >> ';';
            not_semicolon_line_p
                = lexeme_d[
                    (anychar_p-ch_p(';')-ch_p('\n')) >> *(anychar_p - ch_p('\n')) >> ch_p('\n')
                    ]
                ;
            
            until_empty_line_p 
	      =   lexeme_d[*(anychar_p - ch_p('\n')) >> ch_p('\n') >> ch_p('\n')];
            until_newline_p 
                =   lexeme_d[
                    *(anychar_p - ch_p('\n')) >> ch_p('\n')
                    ]
                ;
            
            file_p 
                =   keyword_p("file") 
                    >> 
                    grd(expect(lex_string_p [ assign_a( self.sem.filename
                                           , self.sem.lex_string ) ]))[handler()]
                ;
                
            archive_file_p
                =   ( keyword_p("brf") [set_archive_format(self.sem,sbmt::brf)]
                    | keyword_p("archive") [set_archive_format(self.sem,sbmt::gar)]
                    | keyword_p("fat-archive") [set_archive_format(self.sem,sbmt::fat_gar)]
                    | keyword_p("text-archive") [set_archive_format(self.sem,sbmt::text_gar)]
                    | keyword_p("fat-text-archive") [set_archive_format(self.sem,sbmt::fat_text_gar)]
                    )
                    >>
                    grd(expect(lex_string_p [ assign_a( self.sem.filename
                                           , self.sem.lex_string ) ]))[handler()]
                ;
            
            weight_string_p 
                =   (keyword_p("weight-string") | keyword_p("weights"))
                    >> 
                    grd(expect(lex_string_p[ assign_a( self.sem.weight_string
                                          , self.sem.lex_string ) ]))[handler()]
                ;
            
            #ifdef   BOOST_SPIRIT_DEBUG
            BOOST_SPIRIT_DEBUG_RULE( sequence_p );
            BOOST_SPIRIT_DEBUG_RULE( load_ngram_p );
            BOOST_SPIRIT_DEBUG_RULE( load_dynamic_ngram_p );
            BOOST_SPIRIT_DEBUG_RULE( push_inline_rules_p );
            BOOST_SPIRIT_DEBUG_RULE( change_options_p );
            BOOST_SPIRIT_DEBUG_RULE( load_grammar_p );
            BOOST_SPIRIT_DEBUG_RULE( decode_p );
            BOOST_SPIRIT_DEBUG_RULE( lex_string_p );
            BOOST_SPIRIT_DEBUG_RULE( weight_string_p );
            BOOST_SPIRIT_DEBUG_RULE( until_newline_p );
	    BOOST_SPIRIT_DEBUG_RULE( until_empty_line_p );
            #endif
            
        }
        rule<ST> const& start() const { return sequence_p; }
    };
    decode_sequence_data& sem;
    
};

void decode_sequence_reader::read(istream& in)
{
    if (!in) {
        throw_decode_sequence_parse_error(
            "Unable to open instruction-file input stream!"
        );
    }
    data->root_directory = initial_path();
    read_impl(in);
}

void decode_sequence_reader::read(string filename)
{
    path filepath = system_complete(path(filename));
    ifstream in(filepath.native().c_str());
    if (!in) {
        throw_decode_sequence_parse_error(
            "Unable to open instruction file " + filepath.native()
        );
    }
    data->root_directory = filepath.branch_path();
    read_impl(in);
}

void decode_sequence_reader::read_impl(istream& in)
{
    

    decode_sequence_grammar gram(*data);    //  Our parser

    typedef multi_pass< istreambuf_iterator<char> > mp_iterator_t;
    typedef position_iterator<mp_iterator_t> iterator_t;

    mp_iterator_t mp_first = make_multi_pass(istreambuf_iterator<char>(in));
    mp_iterator_t mp_last  = make_multi_pass(istreambuf_iterator<char>());
    
    iterator_t first(mp_first,mp_last);
    iterator_t last;

    parse_info<iterator_t> info = parse( first
                                       , last
                                       , gram
                                       , space_p | 
                                         comment_p("//") | 
                                         comment_p("/*","*/")
                                       );

    if (not info.full) {
        stringstream sstr;
        string line;
        getline(in,line);
        if (line != "") {
            sstr << "Parsing failed.  line: "<< info.stop.get_position().line
                 << " column: " << info.stop.get_position().column << endl
                 << "at: \"" << line << "\"";
            throw_decode_sequence_parse_error(sstr.str());
        }
    }
}

void dummy_use_info(std::vector<std::string> const& infos)
{
    cerr << "unsupported operation: " << endl;
    cerr << "use-info ";
    copy(infos.begin(),infos.end(),std::ostream_iterator<std::string>(cerr,","));
    cerr << endl;
}

void dummy_decode(gusc::lattice_ast const& ltree, std::size_t id)
{
    cerr << "unsupported operation: " << endl;
    cerr << ltree << endl;
}

void dummy_load_grammar(string const& fname, string const& wt, sbmt::archive_type fmt)
{
    cerr << "unsupported operation: " << endl ;
    cerr << "load-grammar " << fmt << ' '
         << '"' << fname << '"'
         << " weights " << wt << " ;" << endl ;
}

void dummy_push_grammar(string const& fname, sbmt::archive_type fmt)
{
    cerr << "unsupported operation: " << endl ;
    cerr << "push-grammar " << '"' << fname << '"'<< " ;" << endl ;
}

void dummy_pop_grammar()
{
    cerr << "unsupported operation: " << endl ;
    cerr << "pop-grammar "<< " ;" << endl ;
}

void dummy_set_weights(sbmt::fat_weight_vector const& w)
{
    std::cout << "weights coulda been set to: " << w << std::endl;
}

void dummy_force_decode(string const& source, string const& target, size_t id)
{
    cerr << "unsupported operation: " << endl ;
    cerr << "force-decode[" << id << "] {" << endl ;
    cerr << "    source " << source << endl ;
    cerr << "    target " << target << endl ;
    cerr << "}" << endl ;
}

decode_sequence_reader::decode_sequence_reader(std::size_t sentid)
: data(new decode_sequence_data())
{ 
    data->sentid = sentid;
    data->archive_fmt = sbmt::gar;
    data->load_grammar = dummy_load_grammar;
    data->push_grammar = dummy_push_grammar;
    data->pop_grammar = dummy_pop_grammar;
    data->decode = dummy_decode; 
    data->force_decode = dummy_force_decode;
    data->use_info = dummy_use_info;
    data->set_weights = dummy_set_weights;
    data->set_info_option = sbmt::info_registry_set_option;
}

void decode_sequence_reader::set_info_option_cb(set_info_option_cb_type cb)
{ data->set_info_option=cb; }

void decode_sequence_reader::set_weights_cb(set_weights_cb_type cb)
{ data->set_weights=cb; }

void decode_sequence_reader::set_load_dynamic_ngram_cb(load_dynamic_ngram_cb_type cb)
{ data->load_dynamic_ngram=cb; }

void decode_sequence_reader::set_change_options_cb(change_options_cb_type cb)
{ data->change_options=cb; }

void decode_sequence_reader::set_push_inline_rules_cb(push_inline_rules_cb_type cb)
{ data->push_inline_rules=cb; }

void decode_sequence_reader::set_load_grammar_cb(load_grammar_cb_type cb)
{ data->load_grammar = cb; }

void decode_sequence_reader::set_push_grammar_cb(push_grammar_cb_type cb)
{ data->push_grammar = cb; }

void decode_sequence_reader::set_pop_grammar_cb(pop_grammar_cb_type cb)
{ data->pop_grammar = cb; }

void decode_sequence_reader::set_decode_cb(decode_cb_type cb)
{ data->decode = cb; }

void decode_sequence_reader::set_decode_forest_cb(decode_forest_cb_type cb)
{ data->decode_forest = cb; }

void decode_sequence_reader::set_use_info_cb(use_info_cb_type cb)
{ data->use_info = cb; }

void decode_sequence_reader::set_force_decode_cb(force_decode_cb_type cb)
{ data->force_decode = cb; }
    
void decode_sequence_data::clear()
{
    archive_fmt = sbmt::gar;
    filename = "";
    sentence = "";
    target_sentence = "";
    lex_string = "";
}

/*
int main ( )
{
    decode_sequence_reader actor;
    try {
        actor.read(cin);
    } catch (exception const& e) { 
        cout << "exception caught:" << e.what() << endl; 
    }
    return 0;
}
*/
