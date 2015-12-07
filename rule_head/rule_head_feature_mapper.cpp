# include <xrsparse/xrs.hpp>
# include <boost/tuple/tuple_io.hpp>
# include <boost/tuple/tuple.hpp>
# include <boost/bind.hpp>
# include <boost/algorithm/string/trim.hpp>
# include <boost/lexical_cast.hpp>
# include <boost/program_options.hpp>
# include <iostream>
# include <iterator>
# include <sstream>
# include <gusc/iterator/ostream_iterator.hpp>


using namespace std;
using namespace boost;

std::string at_replace(std::string str);

typedef std::vector<feature>::iterator feature_pos_t;

string::const_iterator next( string::const_iterator pos
                           , string::const_iterator end )
{
    ++pos;
    while ( pos != end and 
            *pos != 'H' and 
            *pos != '(' and 
            *pos != ')' and 
            *pos != 'D'
          ) { ++pos; }
    return pos;
}

string::const_iterator next_tree( string::const_iterator pos
                                , string::const_iterator end
                                , int x = 0 )
{
    pos = next(pos,end);
    if (*pos == '(') return next_tree(pos,end,++x);
    if (*pos == ')') return next_tree(pos,end,--x);
    if (x == 0) return pos;
    return next_tree(pos,end,x);
}

boost::tuple<lhs_pos, string::const_iterator, string::const_iterator>
hpos( lhs_pos lpos
    , string::const_iterator pos
    , string::const_iterator end
    )
{
    if (pos == end) return boost::make_tuple(lpos,pos,end);
    else if (*pos == '(') {
        lhs_pos llpos(*lpos.vec, lpos.pos + 1);
        pos = next(pos,end);
        while (llpos.pos != llpos.vec->end()) {
            if (*pos == 'H') {
                return hpos(llpos,pos,end);
            } else {
                lhs_pos oldpos = llpos;
                llpos.pos = llpos.pos->next 
                          ? llpos.vec->begin() +  llpos.pos->next
                          : llpos.vec->end()
                          ;
                if (llpos.pos != llpos.vec->end()) {
                    pos = next_tree(pos,end);
                }
                else { 
                    // bad headmarker: no child is head.   
                    // assume rightmost is head
                    return hpos(oldpos,pos,end);
                }
            }
        }
        throw std::runtime_error("loop ended before return");
    }
    else if (*pos == 'D') {
        string::const_iterator npos = next(pos,end);
        if (*npos == '(') {
            return hpos(lpos,npos,end);
        } else {
            return boost::make_tuple(lpos,pos,end);
        }
    } else if (*pos == 'H') {
        string::const_iterator npos = next(pos,end);
        if (*npos == '(') {
            return hpos(lpos,npos,end);
        } else {
            return boost::make_tuple(lpos,pos,end);
        }
    } else {
        return hpos(lpos,next(pos,end),end);
    }
}

ptrdiff_t get_feature(rule_data& rd, string const& name)
{
    feature_pos_t pos = rd.features.begin();
    for (; pos != rd.features.end(); ++pos) {
        if (pos->key == name) return pos - rd.features.begin();
    }
    return pos - rd.features.begin();
}

ptrdiff_t get_set_feature(rule_data& rd, string const& name)
{
    feature_pos_t pos = rd.features.begin() + get_feature(rd,name);
    if (pos == rd.features.end()) {
        rd.features.push_back(feature());
        rd.features.back().compound = true;
        rd.features.back().key = name;
        rd.features.back().number = false;
        rd.features.back().str_value = "";
        pos = rd.features.end() - 1;
    }
    return pos - rd.features.begin();
}

size_t lpos2rpos(size_t lpos, rule_data const& rd)
{
    size_t rpos = 0;
    BOOST_FOREACH(rhs_node const& rhs, rd.rhs) {
        if (rhs.indexed) {
            if (rhs.index == lpos) return rpos;
            ++rpos;
        }
    }
}

ptrdiff_t add_hwpos(rule_data& rd)
{
    feature_pos_t pos = rd.features.begin() + get_set_feature(rd,"hwpos");

    feature_pos_t rpos = rd.features.begin() + get_feature(rd,"headmarker");
    if (rpos != rd.features.end()) {
        lhs_pos lpos(rd.lhs, rd.lhs.begin());
        boost::tie(
          lpos
        , boost::tuples::ignore
        , boost::tuples::ignore
        ) = hpos(lpos, rpos->str_value.begin(), rpos->str_value.end());
        if (lpos.pos->indexed) {
            pos->str_value = boost::lexical_cast<string>(lpos2rpos(lpos.pos->index,rd));
        } else {
            std::string arp = "\"" + at_replace((lpos.pos + 1)->label) + "\"";
            pos->str_value = std::string(arp.c_str());
        }
    }
    
    return pos - rd.features.begin();
}




void add_hwdh_hwidx(rule_data& rd) 
{
    ptrdiff_t dhpos = get_set_feature(rd,"hwdh");
    ptrdiff_t idxpos = get_set_feature(rd,"hwidx");
    ptrdiff_t hpos = add_hwpos(rd);
    if ( rd.features[dhpos].str_value != "" and
         rd.features[idxpos].str_value != "" and
         rd.features[hpos].str_value != "" ) return;
    int idx = 0;
    int hidx = -1;
    try {
        if (rd.features[hpos].str_value[0] != '"') {
            hidx = lexical_cast<int>(rd.features[hpos].str_value);
        }        
    } catch(std::exception const& e) {
        cerr << "warning: " << e.what() << '\n';
    }
    stringstream idxsstr, dhsstr;
    vector<int> dhv; vector< tuple<int,int> > idxv;
    BOOST_FOREACH(rhs_node const& rhs, rd.rhs) {
        if (rhs.indexed) {
            dhv.push_back(hidx == int(rhs.index) ? 1 : 0);
            idxv.push_back( make_tuple(rd.id, idx) );
            ++idx;
        }
    }
    
    idxsstr << boost::tuples::set_delimiter(',');
    copy(dhv.begin(),dhv.end(),gusc::ostream_iterator(dhsstr," "));
    copy(idxv.begin(),idxv.end(),gusc::ostream_iterator(idxsstr," "));
    rd.features[dhpos].str_value = dhsstr.str();
    rd.features[idxpos].str_value = idxsstr.str();
}    

struct options {
    bool append;
    options() 
     : append(false) {}
};

options parse_options(int argc, char** argv)
{
    using namespace boost::program_options;
    bool help = false;
    options opts;
    options_description desc;
    desc.add_options()
      ( "help,h"
      , bool_switch(&help)->default_value(false)
      , "display this message"
      )
      ( "append,a"
      , bool_switch(&opts.append)->default_value(false)
      , "append features to rules"
      )
      ;
      
    try {
        variables_map vm;
        store(parse_command_line(argc,argv,desc), vm);
        notify(vm);
        if (help) {
            cout << desc << '\n';
            exit(0);
        } 
    } catch(...) {
        cout << desc << '\n';
        exit(1);
    }
    return opts;
}

int main(int argc, char** argv) 
{
    ios::sync_with_stdio(false);
    
    options opts = parse_options(argc,argv);

    string line;
    while (getline(cin,line)) {
        rule_data rd;
        try {
            rd = parse_xrs(line);
            add_hwdh_hwidx(rd);
        } catch(std::exception const& e) { 
            cerr << line << " error={{{" << e.what() << "}}}" << endl;
            continue; 
        }

        if (opts.append) {
            cout << rd << '\n';
        } else {
            feature_pos_t 
                dhpos = rd.features.begin() + get_feature(rd,"hwdh"), 
                hwpos = rd.features.begin() + get_feature(rd,"hwpos"),
                idxpos = rd.features.begin() + get_feature(rd,"hwidx");
            
            cout <<rd.id << '\t' << *dhpos << ' ' << *idxpos << ' ' << *hwpos << '\n';
        }
    }
    return 0;
}

