//#define OLD_ARGS
/*
ITG BINARIZER
Hao Zhang and Liang Huang
August 2005

what is it?

e.g.,

e3
x2         *
e2v
x1               *
e1
x0   *
e0
  c0 x0 c1 x2 c2 x1

we binarize the synchronous cfg rule,
guaranteeing continuity on both sides
 */



char const* usage_str=
"what is it?\n\n"
"e.g.,"
"\n\n"
"e3\n"
"x2         *\n"
"e2\n"
"x1               *\n"
"e1\n"
"x0   *\n"
"e0\n"
"  c0 x0 c1 x2 c2 x1\n"
"\n"
"we binarize the synchronous cfg rule,\n"
"garanteeing continuity on both sides\n"
    ;

# include <boost/regex.hpp>
# include <boost/functional/hash.hpp>

#ifndef OLD_ARGS
# define GRAEHL__SINGLE_MAIN
# include <graehl/shared/program_options.hpp>
# include <graehl/shared/command_line.hpp>
# include <graehl/shared/fileargs.hpp>
#endif

#include <sbmt/grammar/syntax_rule.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#ifdef _WIN32
#include <hash_map>
#ifndef stlext_ns_alias_defined
#define stlext_ns_alias_defined
namespace stlext = ::std;
namespace stlextp = ::std;
#endif
#else
#include <ext/hash_map>
namespace stlext = ::__gnu_cxx;
namespace stlextp = ::std;
#endif
#include <math.h>
#include <set>

//the rule reader
#include <RuleReader/Rule.h>
#include <RuleReader/RuleNode.h>

#include <boost/algorithm/string.hpp>
#include <boost/array.hpp>

using namespace std;
using namespace sbmt;

bool flatten=false;
bool etree_string=false;

bool dependency_tree_string=false;
bool use_stupid_dependency_lm_string=false;
string stupid_dependency_lm_string = "";
bool binarize_length_features=true;
bool preterminal_string=false;
bool state_etree_string=false;
bool left_right_backoff=false;
int vrule_counter = 0;
bool nosharing = false;
bool save_binarized = false;
string subgrammar_id="";
//(if flatten) flatten top unary rules
#define FLATTEN

//sanity check
//#define SANITY 1


//if just filtering, uncomment
//#define filteringMode 1
bool filteringMode=false;
bool exitOnErrorMode=false;

//if not packing sharing virtual rules, uncomment
//only for the old version, where a 'constraint'
//program is used to do the weight pushing and
//rules packing

//#define NOPACKING 1

//and the prob_string is used to extract original rule prob
//#define prob_string "unit_root_prob_it1"

//for debugging, will output succinct attributes
//#define NOOTHERS 1

//#define BUFSIZE 100
//max # of rhs variables
//#define MAXSTATES 200

//word deliminer, used when creating virtual nts that contain actual words
#define WORDELIM "_"
//subpart deliminer, used when creating virtual nts that are either straight or inverted
#define BINDELIM "__"
#define ECDELIM "___"
//root - tree seperator
#define RTRDELIM ":VL: "

//replaceing ( and ). we cannot use ( and ) in virtual nonterminals
#define LBR "--LBR--"
#define RBR "--RBR--"
#define FEATSEP "###"
#define RHSSEP "->"
//according to rule_file_line_number
//might have two identical bin rules from one xrs rule


#define TEMPSTR "/tmp/binaltemp"
ofstream ctmp(TEMPSTR);

//#define ov ctmp
#define ow (*o_stream)

ostream *o_stream=&cout;
ostream *non_itg_out=&cerr;
ostream *itg_out=&cout;

string ovstr;

vector< vector<string> > pointer_features;
vector< vector<string> > or_pointer_features;
map<string,int> pointer_feature_name_map;
vector<string> pointer_feature_id_map;
map<string,int> or_pointer_feature_name_map;
vector<string> or_pointer_feature_id_map;
vector <string> LHS_symbols, RHS_symbols;
vector <int> covered_mask;
vector <int> LHS_idx, RHS_idx;
vector <int> L2Rpermu;

// liang
//#ifdef stat


//#ifdef NOPACKIING
//double curprb;
//#endif

int lhsptr;
int rhsptr;

int rhsStates;
int rhsSize;

int vRules;

string line;
//int totalRules;
int totalXrs;

vector <int> stck;

int top;

//liang
long long totalRules=0;
long long bin = 0, nonbin = 0, badrules = 0;

set<long> used_ids;
long ruleID;

ns_RuleReader::RuleNode *curr;

ns_RuleReader::Rule *myRule;

struct node {
  int left;
  int right;
  int c_left_idx;
  int c_right_idx;

  int e_left_idx;
  int e_right_idx;


  int leftchild, rightchild;  // backpointers

  int tag; // A or B or C
  int virt;

  string label;

  node (int l, int r, int lc, int rc, int t, int v) :
    left(l), right(r), leftchild(lc), rightchild(rc), tag(t), virt(v)
  {}
};

typedef node *nodeptr;

nodeptr prev_leaf;

vector <int> Alignment;
vector <node> nodes;

int cleft(node const& nd)
{
    if (nd.tag != 1) return nd.c_left_idx;
    else return std::min(nd.c_left_idx,cleft(nodes[stck[nd.leftchild]]));
}

int cright(node const& nd)
{
    if (nd.tag != 1) return nd.c_right_idx;
    else return std::max(nd.c_right_idx,cright(nodes[stck[nd.rightchild]]));
}

std::pair<int,int> cspan(node const& nd)
{
    return std::make_pair(cleft(nd),cright(nd));
}

std::pair<int,int> espan(node const& nd)
{
    return std::make_pair(nd.e_left_idx,nd.e_right_idx);
}

std::ostream& operator << (std::ostream& out, std::pair<int,int> const& p)
{
    return out << "(" << p.first << "," << p.second << ")";
}

std::ostream& operator << (std::ostream& out, node const& nd)
{
    out << "(";
    out << "left=" << nd.left << ",";
    out << "right=" << nd.right << ",";
    //std::pair<int,int> c = cspan(nd);
    out << "cspan=" << cspan(nd) << ",";
    out << "espan=" << espan(nd) << ",";
    out << "tag=" << nd.tag << ",";
    out << "virt=" << nd.virt << ",";
    out << "label=" << nd.label;
    out << ")";
    return out;
}

const string nonterminal[4] = {"", "A", "B", "C"};



//different types of symbols

#define rt 'r'
#define vt 'v'
#define ew 'e'
#define cw 'c'
#define ix 'x'
#define nu '\0'

typedef struct symbol_t{
  char tg;
  int id;
  int pos;
  symbol_t(char t, int i, int p):
    tg(t), id(i), pos(p)
  {};
  symbol_t(){};
}symbol_t;

typedef struct vrule_t{
  symbol_t lhs;
  vector< boost::array<int,2> > pointer_feature;
  vector< vector< boost::array<int,2> > > or_pointer_feature;
  symbol_t kids[2];//yes, binary
  vector<symbol_t> lm_seq;//tells us how to synchronize the two kids' english translations
  vector<int> xrs_seq;//tells us what xrs rules share this bin rule
  int inversion;
  vrule_t(){specified=false; active=false;};
  //#ifdef NOPACKING
  //  double prb;
  //#endif
  bool specified;
  bool active;
}vrule_t;


struct eqstr
{
  bool operator()(const char* s1, const char* s2) const
  {
    return strcmp(s1, s2) == 0;
  }
};

void print_lm_strings(ostream& os, vector<string> const& vs)
{
    string pre = "";
    vector<string> pt_lm_string;
    size_t text_length = 0;

    os << "lm_string={{{ ";
    for (size_t x = 0; x != vs.size(); ++x) {
        if (preterminal_string) {
            if (vs[x][0] == '"') {
                ++text_length;
                pt_lm_string.push_back("\"" + pre.substr(1,pre.size() - 2) + "\"");
                os << vs[x] << ' ';
            } else if (vs[x][0] == '(') {
                pre = vs[x];
                //if (etree_string) {
                //    os << vs[x] << ' ';
                //}
            } else {
                pt_lm_string.push_back(vs[x]);
                os << vs[x] << ' ';
            }
        } else {
            if (vs[x][0] == '"') ++text_length;
            if (vs[x][0] != '(') {
                os << vs[x] << ' ';
            }
        }
    }
    os << "}}}";

    if (etree_string) {
        os << " etree_string={{{ ";
        for (size_t x = 0; x != vs.size(); ++x) {
            os << vs[x] << ' ';
        }
        os << "}}}";
    }

    if (preterminal_string) {
        os << " pt_lm_string={{{ ";
        for (size_t x = 0; x != pt_lm_string.size(); ++x) {
            os << pt_lm_string[x] << ' ';
        }
        os << "}}}";
    }

    if (binarize_length_features and text_length > 0) {
        os << " text-length=" << text_length << ' ';
    }
}


//hash_map<const char*, vector<int>, hash<const char*>, eqstr > vtbl;
//hash_map<const char*, hash_map<int, char>, hash<const char*>, eqstr > vtbl;
stlext::hash_map<int, vrule_t> vrul_tbl;

typedef stlext::hash_map<std::string,std::vector<size_t>, boost::hash<std::string> > backoff_symbols_t;
backoff_symbols_t backoff_symbols;

void insert_left_right_backoff(std::string rule,size_t id)
{
    backoff_symbols_t::iterator pos = backoff_symbols.find(rule);
    if (pos == backoff_symbols.end()) {
        std::vector<size_t> v;
        v.push_back(id);
        backoff_symbols.insert(std::make_pair(rule,v));
    } else {
        pos->second.push_back(id);
    }
}

void output_left_right_backoffs_virt(std::ostream& out)
{
    backoff_symbols_t::iterator i = backoff_symbols.begin(),
                                e = backoff_symbols.end();
    for (; i != e; ++i) {
        out << i->first << " id={{{ ";
        copy( i->second.begin()
            , i->second.end()
            , std::ostream_iterator<size_t>(out," ")
            );
        out << "}}}" << std::endl;
    }
}

#ifdef SANITY
vector<vrule_t> topvrul_tbl;
#endif

stlext::hash_map<const char*, int, stlext::hash<const char*>, eqstr> vsym2id_tbl;
//hash_map<int, char *> vid2sym_tbl;
vector<char *> vid2sym_tbl;

stlext::hash_map<const char*, int, stlext::hash<const char*>, eqstr> rsym2id_tbl;
//hash_map<int, char *> rid2sym_tbl;
vector<char *> rid2sym_tbl;

stlext::hash_map<const char*, int, stlext::hash<const char*>, eqstr> esym2id_tbl;
//hash_map<int, char *> eid2sym_tbl;
vector<char *> eid2sym_tbl;

stlext::hash_map<const char*, int, stlext::hash<const char*>, eqstr> csym2id_tbl;
//hash_map<int, char *> cid2sym_tbl;
vector<char *> cid2sym_tbl;

void
clear_maps()
{
	vector<char*>::iterator i;
#define CV(v) \
	for (i = v.begin(); i != v.end(); ++i) \
		delete [] *i; \
	v.clear();

	CV(vid2sym_tbl)
	CV(rid2sym_tbl)
	CV(eid2sym_tbl)
	CV(cid2sym_tbl)
#undef CV

	vsym2id_tbl.clear();
	rsym2id_tbl.clear();
	esym2id_tbl.clear();
	csym2id_tbl.clear();

	vrul_tbl.clear();
}


void throw_stateful_etree()
{
    throw std::runtime_error("--stateful-etree-string requires EVERY xrs rule to have a unary state lhs root");
}


string emp="";

//covering one nonterminal
#define ISLEAF(n) (n.left==n.right)

//covering one nonterminal and zero english word
#define ISVIRTUAL(n) (n.virt)

//1 based index into the english symbols
#define EINDEX(node) (node.left)


inline string conv(string &str){
  if (strcmp(str.c_str(), "(")==0){
    return LBR;
  }
  else
    if (strcmp(str.c_str(), ")")==0){
      return RBR;
    }
    else
      return str;
}

inline char *getsym(char thetype,int theid){

  //  hash_map<int, char *> *theinvmap;
  vector<char *> *theinvmap=NULL;

  if (thetype==rt){
    theinvmap=&rid2sym_tbl;
  }
  else if (thetype==vt){
    theinvmap=&vid2sym_tbl;
  }
  else if (thetype==cw){
    theinvmap=&cid2sym_tbl;
  }
  else if (thetype==ew){
    theinvmap=&eid2sym_tbl;
  }

  return (*theinvmap).at(theid);
}

boost::regex whitespace("\\s");
boost::regex pointer("x\\d+:");

std::string orig_decorate(std::string const& s, bool deco)
{
  if (deco) return s + "_orig";
  else return s;
}

//print all the other attributes inherited from xrs
inline void print_other_attr(ns_RuleReader::Rule* R, ostream &o){
  for (ns_RuleReader::Rule::attribute_map::iterator it=R->getAttributes()->begin();
       it!=R->getAttributes()->end();
       it++) { // for each attribute value
           if (binarize_length_features)  {
               if (it->first == "text-length") {
                   continue;
               }
           }
           bool orig=false;
           if (pointer_feature_name_map.find(it->first) != pointer_feature_name_map.end()) {
	       orig=true;
           }
           if (or_pointer_feature_name_map.find(it->first) != or_pointer_feature_name_map.end()) {
	       orig=true;
           }
           if (it->second.bracketed or boost::regex_search(it->second.value,whitespace)) {
	     o << orig_decorate(it->first,orig) << "={{{" << it->second.value << "}}} ";
           } else {
	     o << orig_decorate(it->first,orig) << '=' << it->second.value << ' ';
           }
  } // end for each attribute
  return;
}

string to_string(fat_token const& f)
{
    if (is_lexical(f)) return "\"" + f.label() + "\"";
    else return f.label();
}

template <class T>
std::map<int,int> rhs_map_ids(syntax_rule<T> const& rule)
{
    std::map<int,int> id_map;
    typename syntax_rule<T>::rhs_iterator i = rule.rhs_begin(),
                                          e = rule.rhs_end();
    int id_count = 0;
    for (; i != e; ++i) if (i->indexed()) id_map[rule.index(*i)] = id_count++;

    return id_map;
}

string left_right_rhs_construct(fat_token const& r1, fat_token const& r2)
{
    if (is_lexical(r1) and is_lexical(r2)) {
        return "(x0:NULL) -> " + to_string(r1) + " " + to_string(r2);
    } else if (is_lexical(r1)) {
        return "(x0:" + to_string(r2) + ") -> " + to_string(r1) + " x0";
    } else if (is_lexical(r2)) {
        return "(x0:" + to_string(r1) + ") -> x0 " + to_string(r2);
    } else {
        return "(x0:" + to_string(r1) + " x1:" + to_string(r2) + ") -> x0 x1";
    }
}

string lm_record( fat_syntax_rule const& rule
                , map<int,int>& id_map
                , fat_syntax_rule::rhs_iterator end
                , char sep = ' ' )
{
    fat_syntax_rule::rhs_iterator itr = rule.rhs_begin();
    stringstream sstr;
    map<int,int> inverse;
    int idx = 0;
    for (; itr != end; ++itr) {
        if (itr->indexed()) {
            inverse.insert(stlextp::make_pair(id_map[rule.index(*itr)],idx));
            ++idx;
            //sstr << id_map[rule.index(*itr)] << ' ';
        }
    }
    map<int,int>::iterator j = inverse.begin(), e = inverse.end();
    for (; j != e; ++j) {
        sstr << j->second << sep;
    }
    return sstr.str();
}


string make_lm_string( fat_syntax_rule const& rule
                     , map<int,int>& id_map )
{
    stringstream sstr;

    fat_syntax_rule::lhs_preorder_iterator litr = rule.lhs_begin();

    for (; litr != rule.lhs_end(); ++litr) {
        if (is_lexical(litr->get_token()))
            sstr << '"' << litr->get_token().label() << "\" ";
        else if (litr->indexed()) sstr << id_map[litr->index()] << ' ';
    }
    return sstr.str();
}

string make_etree_string( fat_syntax_rule::tree_node const& root
                          , map<int,int>& id_map, bool skip_root=false )
{
    if (root.indexed()) {
        if (skip_root) throw_stateful_etree();
        return boost::lexical_cast<string>(id_map[root.index()]);
    } else if (is_lexical(root.get_token())) {
        if (skip_root) throw_stateful_etree();
        return to_string(root.get_token());
    } else {
        if (skip_root)
            return make_etree_string(*root.children_begin(),id_map);
        stringstream sstr;
        sstr << '(' << root.get_token().label() << ')';
        fat_syntax_rule::lhs_children_iterator itr = root.children_begin(),
                                               end = root.children_end();
        for (;itr != end; ++itr)
            sstr << ' ' << make_etree_string(*itr,id_map);
        sstr << " (/" << root.get_token().label() << ')';
        return sstr.str();
    }
}



vector<string> left_right_binarize(string const& s)
{
    string arrow = RHSSEP " ";
    string prelm = " " FEATSEP " lm_string={{{";
    string postlm = "}}}";
    ns_RuleReader::Rule r(s);
    fat_syntax_rule rule(r,fat_tf);
    map<int,int> lhs_id_map = map_ids_rhs(rule);
    map<int,int> id_map = map_ids_lhs(rule);
    size_t left = 0;
    size_t right = 1;
    size_t sz = rule.rhs_size();
    size_t id = rule.id();
    vector<string> retval(1);
    stringstream sstr;
    stringstream nstr;
    stringstream lhs, rhs1, rhs2;
    rhs1 << to_string((rule.rhs_begin() + left)->get_token());
    rhs2 << to_string((rule.rhs_begin() + right)->get_token());
    lhs << "V["
        << lm_record(rule,lhs_id_map,rule.rhs_begin() + right + 1,'_')
        << ","
        << rhs1.str()
        << ","
        << rhs2.str()
        << "]";
    sstr << lhs.str() << RTRDELIM << lhs.str()
         << left_right_rhs_construct( (rule.rhs_begin() + left)->get_token()
                                   , (rule.rhs_begin() + right)->get_token() )
         << prelm
         << lm_record(rule,lhs_id_map,rule.rhs_begin() + right + 1)
         << postlm;
    insert_left_right_backoff(sstr.str(), id);
    //retval[sz - right - 1] = sstr.str();
    ++left; ++right;
    while (right < sz - 1) {
        rhs1.str(lhs.str());
        lhs.str("");
        rhs2.str(to_string((rule.rhs_begin() + right)->get_token()));
        sstr.str("");
        lhs << "V["
            << lm_record(rule,lhs_id_map,rule.rhs_begin() + right + 1,'_')
            << ","
            << rhs1.str()
            << ","
            << rhs2.str()
            << "]";
        sstr << lhs.str() << RTRDELIM << lhs.str()
             << left_right_rhs_construct( virtual_tag(rhs1.str())
                                        , (rule.rhs_begin() + right)->get_token() )
             << prelm
             << lm_record(rule,lhs_id_map,rule.rhs_begin() + right + 1)
             << postlm;
        insert_left_right_backoff(sstr.str(), id);
        //retval[sz - right - 1] = sstr.str();
        ++left; ++right;
    }

    rhs1.str(lhs.str());
    lhs.str("");
    rhs2.str(to_string((rule.rhs_begin() + right)->get_token()));
    sstr.str("");
    lhs << print(rule.lhs_root()->get_token(),fat_tf);
    sstr << lhs.str() << ' ';
    print_lhs(sstr,rule,fat_tf);
    sstr << 'f' << rhs1.str() << ' ';
    if (is_lexical((rule.rhs_begin() + right)->get_token())) sstr << rhs2.str();
    else sstr << 'x' << lhs_id_map[rule.index(*(rule.rhs_begin() + right))];
    sstr << " " << FEATSEP << " "
         << "rhs={{{";
    print_rhs(sstr,rule,fat_tf);
    sstr << "}}} lm_string={{{";
    if (!etree_string) sstr << make_lm_string(rule,id_map);
    else sstr << make_etree_string(*rule.lhs_root(),id_map,state_etree_string);
    sstr << "}}} ";
    print_other_attr(&r,sstr);

    retval[0] = sstr.str();
    return retval;
}

inline int putsym(char thetype,string const& thestring){

  stlext::hash_map<const char*, int, stlext::hash<const char*>, eqstr> *themap=NULL;
  //  hash_map<int, char *> *theinvmap;
  vector<char *> *theinvmap=NULL;

  if (thetype==rt){
    themap=&rsym2id_tbl;
    theinvmap=&rid2sym_tbl;
  }
  else if (thetype==vt){
    themap=&vsym2id_tbl;
    theinvmap=&vid2sym_tbl;
  }
  else if (thetype==cw){
    themap=&csym2id_tbl;
    theinvmap=&cid2sym_tbl;
  }
  else if (thetype==ew){
    themap=&esym2id_tbl;
    theinvmap=&eid2sym_tbl;
  }

  int theid;
  stlext::hash_map<const char*, int, stlext::hash<const char*>, eqstr>::iterator sitr;

  if ((sitr=themap->find(thestring.c_str()))==themap->end()){
    char *newstr=new char[thestring.size()+1];
    strcpy(newstr, thestring.c_str());
    theid=(*themap)[newstr]=themap->size()+1;//depends on the complexity of size()
    //    (*theinvmap)[theid]=newstr;
    theinvmap->resize(theid+1);
    (*theinvmap)[theid]=newstr;
  }
  else{
    theid=(*sitr).second;
  }
  return theid;
}


/*
#define putv(s, n){\
char *newstr=new char[s.size()+1];\
strcpy(newstr, s.c_str());\
vtbl[newstr].push_back(n);\
//vtbl[newstr][n]++;\
}
*/


//create the lm vector
void lmvector(vector<symbol_t> &thelmv, node *thenode, int ord){
  if (ISLEAF((*thenode)))
  for (int i=thenode->e_left_idx; i<=thenode->e_right_idx; i++){
    if (i!=LHS_idx[EINDEX((*thenode))-1]){//lexical
      thelmv.push_back(symbol_t(ew, putsym(ew, LHS_symbols[i]),-1));
    }
    else{
      thelmv.push_back(symbol_t(ix, ord,-1));
    }
  }
  else{
    thelmv.push_back(symbol_t(ix, ord,-1));
  }
}

//print the lm string
void lmleaf(vector<string> &vs, node &nd, int ord){
  if (!ISLEAF(nd)){
      vs.push_back(boost::lexical_cast<string>(ord));
  }
  else
  for (int i=nd.e_left_idx; i<=nd.e_right_idx; i++){
    if (i!=LHS_idx[EINDEX(nd)-1]){//lexical
      //o<<"\"";
      vs.push_back(LHS_symbols[i]);
      //o<<"\"";
      //if (i<nd.e_right_idx) o<<" ";
    }
    else{
        vs.push_back(boost::lexical_cast<string>(ord));
      //o<<ord;
      //if (i<nd.e_right_idx) o<<" ";
    }
  }
}

void linearize(vrule_t *thevrule, string & esum, string  & csum, int *rt_id, string idstr){
  string esub[2];
  string csub[2];

  int maxrlen=(thevrule->kids[1].tg!=nu)?2:1; //actually, we only linearize unary virtual rules

  for (int k=0; k<maxrlen; k++){
    if (thevrule->kids[k].tg==rt){
      *rt_id=thevrule->kids[k].id;
      esub[k]=idstr;
      csub[k]="x"+idstr;
    }else if (thevrule->kids[k].tg==cw){
      csub[k]="\"";
      csub[k]+=getsym(cw, thevrule->kids[k].id);
      csub[k]+="\"";
    }else if (thevrule->kids[k].tg==vt){
      //if (vrul_tbl.find(thevrule->kids[k].id) == vrul_tbl.end()) cerr << "woops made a new entry to vrul_tbl in linearize()" << endl;
      vrule_t *kidvrul=&vrul_tbl.find(thevrule->kids[k].id)->second;
      string sube, subc;
      linearize(kidvrul, sube, subc, rt_id, idstr);
      esub[k]=sube;
      csub[k]=subc;
    }
  }

  //putting up chinese string
  for (int k=0; k<maxrlen; k++){
    csum+=csub[k];
    if (k<maxrlen-1)
      csum+=" ";
  }

  //putting up english string
  for (unsigned lmi=0; lmi<thevrule->lm_seq.size(); lmi++){
    if (thevrule->lm_seq[lmi].tg==ew){
      string eword=getsym(ew, thevrule->lm_seq[lmi].id);
      esum+="\""+eword+"\"";
    }
    else{//idx
      if (thevrule->kids[0].tg==cw || thevrule->kids[1].tg==cw){
	esum+=esub[0];
	esum+=esub[1];
      }
      else{
	//	cerr<<"id:"<<thevrule->lm_seq[lmi].id<<esub[thevrule->lm_seq[lmi].id]<<endl;
	esum+=esub[thevrule->lm_seq[lmi].id];
      }
    }

    //    cerr<<"here"<<esum<<" "<<endl;
    if (lmi<thevrule->lm_seq.size()-1)
      esum+=" ";
  }

}


bool is_single_rt_vrule(vrule_t const& vrul)
{
    if (vrul.kids[0].tg == cw) {
        if (vrul.kids[1].tg == rt) return true;
        else if (vrul.kids[1].tg == cw) return false;
        else return is_single_rt_vrule(vrul_tbl[vrul.kids[1].id]);
    } else if (vrul.kids[1].tg == cw) {
        if (vrul.kids[0].tg == rt) return true;
        else return is_single_rt_vrule(vrul_tbl[vrul.kids[0].id]);
    } else {
        return false;
    }
}

bool is_single_rt(char kidtype,int id)
{
    bool f;
    if (kidtype == rt) f = true;
    else if (kidtype == cw) f = false;
    else if (kidtype == vt) {
        f = is_single_rt_vrule(vrul_tbl[id]);
    } else throw "unexpected binarization in is_single_rt";
    //std::cerr << "is_single_rt(" << getsym(kidtype,id) << ") = " << f << '\n';
    return f;
}

bool is_single_rt(char kidtype,string& kidstr)
{
    return is_single_rt(kidtype,putsym(kidtype,kidstr));
}

symbol_t const& get_single_rt_sym(vrule_t const& vrul)
{
    if (vrul.kids[0].tg == cw) {
        if (vrul.kids[1].tg == rt) return vrul.kids[1];
        else if (vrul.kids[1].tg == cw) throw "unexpected binarization in get_single_rt_sym";
        else return get_single_rt_sym(vrul_tbl[vrul.kids[1].id]);
    } else if (vrul.kids[1].tg == cw) {
        if (vrul.kids[0].tg == rt) return vrul.kids[0];
        else return get_single_rt_sym(vrul_tbl[vrul.kids[0].id]);
    }
    throw "vrule is not single-rt";
}

symbol_t get_single_rt_sym(char kidtype,string& kidstr, int kidpos)
{
    if (kidtype == rt or kidtype == cw) {
        string stripped = boost::regex_replace(kidstr,pointer,"");
        return symbol_t(kidtype,putsym(kidtype,stripped),kidpos);
    }
    else if (kidtype == vt) return get_single_rt_sym(vrul_tbl[putsym(kidtype,kidstr)]);
    else throw "unexpected binarization in get_single_rt_pos";
}


void vrul_output(ostream& o){
  stlext::hash_map<int, vrule_t>::iterator vritr;
  for (vritr=vrul_tbl.begin(); vritr!=vrul_tbl.end(); ++vritr){//for each virtual rule
    vrule_t &thevrule=(*vritr).second;
    char *lhsstr=getsym(vt, thevrule.lhs.id);

#ifdef FLATTEN
    string estr[2], cstr[2]; //surface strings
    int thertid[2];//to extract the real nts
    bool notactive[2]; //flags
    if (flatten){
      if (!thevrule.active) continue; //simply do not output non-active virtual rules

    for (int thekid=0; thekid<2; thekid++){
      estr[thekid].clear();
      cstr[thekid].clear();
      thertid[thekid]=-1;
      notactive[thekid]=(thevrule.kids[thekid].tg==vt) && (!vrul_tbl.find(thevrule.kids[thekid].id)->second.active);
      if (notactive[thekid]){
          int csymid=thevrule.inversion?(1-thekid):thekid;//we must have two variables in this situation
          linearize(&vrul_tbl.find(thevrule.kids[thekid].id)->second, estr[thekid], cstr[thekid], &thertid[thekid], (csymid==0)?"0":"1");
      }
      //assuming each nonactive virual nonterminals contain one real nonterminal
    }
    }

#endif

    //LHS
    o<<"V: ";
    o<<lhsstr;
    //o<<RTRDELIM;
    //o<<lhsstr;

    //variables
    //o<<"(";

    int totalvkids=0; //total variables
    int thekid; //iterator
    int ekid;//english order

    int variables[2]={-1, -1};

    for (thekid=0; thekid<2; thekid++){

      //english side indexes
      if (thekid==0)
	ekid=thevrule.inversion;
      else
	ekid=1-thevrule.inversion;


      if (thevrule.kids[ekid].tg!=cw){//rt or vt
//	if (totalvkids==1)  o<<" ";

//	o<<"x"<<totalvkids<<":";
#ifdef FLATTEN
	if (flatten && notactive[ekid]){
//	    o<<
        //getsym(rt, thertid[ekid]); //the variable then would be the real nonterminal inside the virtual nonterminal
	}
	else
#endif
//	  o<<
      //getsym(thevrule.kids[ekid].tg, thevrule.kids[ekid].id);
//
	  variables[ekid]=totalvkids;
	  totalvkids++;
      }
    }

    //if (!totalvkids){//pure lexical
    //  o<<"x0:NULL";
    //}

    //o<<")";


    o<<" " RHSSEP " ";

    //rhs surface string
    for (thekid=0; thekid<2; thekid++){
      if (thekid==1)
	o<<" ";
#ifdef FLATTEN
      if (flatten && notactive[thekid])
	o<<cstr[thekid];//then we flatten the chinese side
      else{
#endif
	if (variables[thekid]>=0){
        o << getsym(thevrule.kids[thekid].tg, thevrule.kids[thekid].id);
	  //o<<"x"<<variables[thekid];
	}
	else{//cw
	  o<<"\"";
	  o<<getsym(cw, thevrule.kids[thekid].id);
	  o<<"\"";
	}
#ifdef FLATTEN
      }
#endif
    }

    //attributes
    o<<" "<< FEATSEP <<" ";

    for (size_t dx = 0; dx != thevrule.or_pointer_feature.size(); ++dx) {
        bool printit=false;
        bool tabit=false;
        if ( thevrule.or_pointer_feature[dx].size() > 1 ) printit=true;
        if (printit) {
        o << or_pointer_feature_id_map[dx] << "={{{";

        o << getsym(rt,thevrule.or_pointer_feature[dx][0][0]) << ' '
          << getsym(rt,thevrule.or_pointer_feature[dx][0][1]);

        for (size_t ods = 1; ods != thevrule.or_pointer_feature[dx].size(); ++ods) {
            o << '\t';
            bool spaceit=false;
            if (is_single_rt(thevrule.kids[0].tg,thevrule.kids[0].id)) {
                o << getsym(rt,thevrule.or_pointer_feature[dx][ods][0]);
                spaceit=true;
            }
            if (is_single_rt(thevrule.kids[1].tg,thevrule.kids[1].id)) {
                if (spaceit) o << ' ';
                o << getsym(rt,thevrule.or_pointer_feature[dx][ods][1]);
            }
            tabit=true;
        }
        o << "}}} ";
        }
    }

    if (thevrule.kids[0].tg == rt or thevrule.kids[1].tg == rt) {
        for (size_t dx = 0; dx != thevrule.pointer_feature.size(); ++dx) {
            bool printit=false;
            bool spaceit=false;
            if ( thevrule.kids[0].tg == rt and
                 strcmp(getsym(rt,thevrule.pointer_feature[dx][0]),"") != 0 )
                 printit=true;
            if ( thevrule.kids[1].tg == rt and
                 strcmp(getsym(rt,thevrule.pointer_feature[dx][1]),"") != 0 )
                 printit=true;
            if (printit) {
            o << pointer_feature_id_map[dx] << "={{{";
            if (thevrule.kids[0].tg == rt) {
                o << getsym(rt,thevrule.pointer_feature[dx][0]);
                spaceit = true;
            }
            if (thevrule.kids[1].tg == rt) {
                if (spaceit) o << ' ';
                o << getsym(rt,thevrule.pointer_feature[dx][1]);
            }
            o << "}}} ";
            }
        }
    }

    int lmstrlen=thevrule.lm_seq.size();
    if(use_stupid_dependency_lm_string){
        o<<"dep_lm=no ";
    }

    vector<string> lmstr;

    for (int lmitr=0; lmitr<lmstrlen; lmitr++){
      symbol_t &thesymbol=thevrule.lm_seq[lmitr];

      //      cerr<<"lmseq "<<lmitr<<":"<<thesymbol.tg<<endl;

      if (thesymbol.tg==ix){
          int csymid=thevrule.inversion?(1-thesymbol.id):thesymbol.id;//back to indexes on rhs
#ifdef FLATTEN
          if (flatten && notactive[csymid]){
              lmstr.push_back(estr[csymid]);
              //o<<estr[csymid];//then we flatten the english side
          } else
#endif
              lmstr.push_back(boost::lexical_cast<string>(thesymbol.id));
              //o<<thesymbol.id; //csymid;
      }
      else{//english words;
	//o<<"\"";
          lmstr.push_back(getsym(ew, thesymbol.id));
	//o<<getsym(ew, thesymbol.id);
	//o<<"\"";
      }

      //if (lmitr<lmstrlen-1) o<<" ";
    }

    print_lm_strings(o,lmstr);
    o<<" ";

    //if (binarize_length_features) o << "text-length=10^-"<<ecount<<" ";

/*
    //id(s) of xrs rules using this vrule.
    o<<"id={{{";

    stlext::hash_map<int, int> exist;

    int rfln=thevrule.xrs_seq.size();//must >=1
    if (rfln>0){
      o<<thevrule.xrs_seq[0];
      exist[thevrule.xrs_seq[0]]++;
    }
    for (int i=1; i<rfln; i++){
      if (!exist[thevrule.xrs_seq[i]]){
 	o<<" ";
 	o<<thevrule.xrs_seq[i];
      }
      else{
// 	//	cerr<<vstring<<endl;
// 	//	cerr<<"repetion: "<<(*vitr).second[i]<<":"<<(int)exist[(*vitr).second[i]]<<endl;
      }
      exist[thevrule.xrs_seq[i]]++;
    }
    o<<"}}}";
*/

    o<<endl;
  }//end for all virtual rules
}



// binary, unary, or zero-ary nonterminal, any number of chinese word top-level rules
inline void flatten_rrul_construct(){
  //root nonterminal on the lhs
  ow<<curr->getString(false)
    <<RTRDELIM
    <<curr->treeString()
    <<" " RHSSEP " ";

  //rhs is not changed
  ow<<myRule->rhs_string();


  //attributes
  ow<<" "<< FEATSEP << " ";
  //all xrs attributes inherited
#ifndef NOOTHERS
  print_other_attr(myRule,ow);
#endif

  ow<<"virtual_label=no complete_subtree=yes ";

  //lm

  //concatenate the english words
  vector<string> lmstr;

  for (int i=0; i<lhsptr; i++){
    //    if ((!thenode) || (i!=LHS_idx[EINDEX((*thenode))-1])){//lexical
    //ow<<"\"";
      lmstr.push_back(LHS_symbols[i]);
    //ow<<LHS_symbols[i];
    //ow<<"\"";
    //if (i<lhsptr-1) ow<<" ";

  }
  print_lm_strings(ow,lmstr);
  ow<<" ";

  //other attributes
  ow<<"sblm_string={{{";
  for (int i=0; i<rhsStates; i++){
    ow<<L2Rpermu[i];
    ow<<" ";
  }
  ow<<"}}} ";

  ow<<"lm=yes "<<"sblm=yes";

  if(use_stupid_dependency_lm_string){
      ow<<" dep_lm=yes "<<"dep_lm_string={{{ "<<stupid_dependency_lm_string<<"}}} ";
  }


// #ifdef NOPACKING
//   ow<<" bin-prob="
//     <<curprb;
//   ow<<" original_rule_prob="
//     <<curprb;
// #endif

  ow<<" rule_file_line_number=";
  //  ow<<"{{{";
  ow<<totalRules;
  //    ow<<"}}}";

  //for reconstruction
  ow<<" rhs={{{";
  ow<<myRule->rhs_string();
  ow<<"}}}";

  ow<<endl;

}



inline void rrul_construct(char kid1type, int kid1pos, string &kid1str, char kid2type, int kid2pos, string &kid2str, node *kid1, node *kid2, int inversion){
  //cerr<<kid1type<<":"<<kid1str<<"@"<< kid1pos <<","<<kid2type<<":"<<kid2str<<"@"<<kid2pos<<endl;
  vrule_counter=0;
  ow << "X: "<< myRule->toString(false) << " ### id=" << ruleID << " " << endl;
  //print_other_attr(myRule,ow);
  //ow << endl;
  ow << "V: ";
#ifdef FLATTEN
    string estr[2], cstr[2]; //surface strings
    int thertid[2];//to extract the real nts
    bool notactive[2]; //flags

    if (flatten) {
      //linearize the non-active child states
      for (int thekid=0; thekid<2; thekid++){
	estr[thekid].clear();
	cstr[thekid].clear();
	thertid[thekid]=-1;
	int thekidtype=(thekid==0?kid1type:kid2type);
	string &thekidstring=(thekid==0)?kid1str:kid2str;
	int thekidid=(thekidtype==vt)?putsym(thekidtype, thekidstring):0;
	notactive[thekid]=(thekidtype==vt)?(!vrul_tbl[thekidid].active):false;

	if (notactive[thekid]){
	  node &thekidnode=(thekid==0)?(*kid1):(*kid2);//look into the unary rule's leaf
	  string thekidrealstring=LHS_symbols[LHS_idx[EINDEX(thekidnode)-1]];
	  int lindex=thekidrealstring.find_first_of(":", 0);
	  linearize(&vrul_tbl[thekidid], estr[thekid], cstr[thekid], &thertid[thekid], thekidrealstring.substr(1, lindex-1));
	}
      }
    }
#endif

  //root nonterminal on the lhs
  ow<<curr->getString(false)
    //<<RTRDELIM
    //<<curr->treeString()
    <<" " RHSSEP " ";

#ifdef SANITY
  totalXrs++;
  topvrul_tbl.resize(totalXrs);
  vrule_t &thevrule=topvrul_tbl[totalXrs-1];

  thevrule.kids[0].tg=kid1type;
  thevrule.kids[1].tg=kid2type;
#endif

  //output kid1
  if (kid1type==cw){
    ow<<"\"";
    ow<<kid1str;
    ow<<"\"";

#ifdef SANITY
    thevrule.kids[0].id=putsym(cw, kid1str);
#endif
  }
  else if (kid1type==rt){
    int lindex=kid1str.find_first_of(":", 0);
    //ow<<kid1str.substr(0, lindex); //x?
    ow<<kid1str.substr(lindex + 1,std::string::npos);

#ifdef SANITY
    string realt=kid1str.substr(lindex+1, kid1str.size());
    thevrule.kids[0].id=putsym(rt, realt);
#endif
  }
  else if (kid1type==vt){
#ifdef FLATTEN
    if (flatten && notactive[0])
      ow<<cstr[0];//then we flatten the chinese side
    else
#endif
      ow<<kid1str;

#ifdef SANITY
      thevrule.kids[0].id=putsym(vt, kid1str);
#endif
  }

  //output kid2
  if (strcmp(kid2str.c_str(), "")!=0){//non-unary cases
    ow<<" ";

    if (kid2type==cw){
      ow<<"\"";
      ow<<kid2str;
      ow<<"\"";

#ifdef SANITY
    thevrule.kids[1].id=putsym(cw, kid2str);
#endif
    }
    else if (kid2type==rt){
      int lindex=kid2str.find_first_of(":", 0);
      ow<<kid2str.substr(lindex + 1,std::string::npos);
      //ow<<kid2str.substr(0, lindex); //x?

#ifdef SANITY
      string realt=kid2str.substr(lindex+1, kid2str.size());
      thevrule.kids[1].id=putsym(rt, realt);
#endif
    }
    else if (kid2type==vt){
#ifdef FLATTEN
      if (flatten && notactive[1])
	ow<<cstr[1];//then we flatten the chinese side
      else
#endif
	ow<<kid2str;

#ifdef SANITY
    thevrule.kids[1].id=putsym(vt, kid2str);
#endif
    }
  }
  else{
#ifdef SANITY
    thevrule.kids[1].tg=nu;
#endif
  }

  //attributes
  ow<<" " << FEATSEP <<" ";

  for (size_t dx = 0; dx != or_pointer_features.size(); ++dx) if (or_pointer_features[dx].size() > 0){

      bool printit=false;
      bool spaceit=false;
      bool kid1is = is_single_rt(kid1type,kid1str);
      bool kid2is = is_single_rt(kid2type,kid2str);
      symbol_t kid1; symbol_t kid2;
      if (kid1is) {
          kid1 = get_single_rt_sym(kid1type,kid1str,kid1pos);
      }
      if (kid2is) {
          kid2 = get_single_rt_sym(kid2type,kid2str,kid2pos);
      }
      if (kid1is and or_pointer_features[dx][kid1.pos] != "" ) printit=true;
      if (kid2is and or_pointer_features[dx][kid2.pos] != "" ) printit=true;
      if (printit) {
          ow << or_pointer_feature_id_map[dx] << "={{{";

          if (kid1is) ow << getsym(kid1.tg,kid1.id);
          else ow << "--";
          if (kid2is) ow << ' ' << getsym(kid2.tg,kid2.id);
          else if (kid2str != "") ow << " --";
          ow << '\t';

          if (kid1is) {
            ow << or_pointer_features[dx][kid1.pos];
            spaceit=true;
          }

          if (kid2is) {
            if (spaceit) ow << ' ';
            ow << or_pointer_features[dx][kid2.pos];
          }
          ow << "}}} ";
      }
  }

  if (kid1type == rt or kid2type == rt) {
      for (size_t dx = 0; dx != pointer_features.size(); ++dx) {
          bool printit=false;
          bool spaceit=false;
          if (kid1type == rt and pointer_features[dx][kid1pos] != "" ) printit=true;
          if (kid2type == rt and pointer_features[dx][kid2pos] != "" ) printit=true;
          if (printit) {
          ow << pointer_feature_id_map[dx] << "={{{";
          if (kid1type == rt) {
              ow << pointer_features[dx][kid1pos] ;
              spaceit=true;
          }

          if (kid2type == rt) {
              if (spaceit) ow << ' ';
              ow << pointer_features[dx][kid2pos];
          }
          ow << "}}} ";
          }
      }
      //X(A("a") x0:X0 B("b") x1:X1 C("c") x2:X2 D("d") x3:X3 E("e")) -> "A" x0 "B" x1 "C" x2 "D" x3 "E" ### id=1 xc={{{1,0 1,1 1,2 1,3}}}
  }

  //all xrs attributes inherited
#ifndef NOOTHERS
  print_other_attr(myRule,ow);
#endif

  //ow<<"virtual_label=no complete_subtree=yes ";

  //lm
  vector<string> lmstr;

  //concatenate the english words
  if (kid1){
    if (kid1==kid2){
      lmleaf(lmstr, (*kid1), 0);

#ifdef SANITY
      lmvector(thevrule.lm_seq, kid1, 0);
#endif
    }
    else{
      if (inversion){
	lmleaf(lmstr, (*kid2), 1);
	//ow<<" ";
	lmleaf(lmstr, (*kid1), 0);

#ifdef SANITY
	lmvector(thevrule.lm_seq, kid2, 1);
	lmvector(thevrule.lm_seq, kid1, 0);
#endif
      }
      else{
	lmleaf(lmstr, (*kid1), 0);
    //ow<<" ";
    lmleaf(lmstr, (*kid2), 1);

#ifdef SANITY
	lmvector(thevrule.lm_seq, kid1, 0);
	lmvector(thevrule.lm_seq, kid2, 1);
#endif
      }
    }
  }
  else{//lexical

    if (kid1type==cw){//real lexical
      for (int i=0; i<lhsptr; i++){
	//ow<<"\"";
	//ow<<LHS_symbols[i];
          lmstr.push_back(LHS_symbols[i]);
	//ow<<"\"";
	//if (i<lhsptr-1)ow<<" ";
#ifdef SANITY
	int eid=putsym(ew, LHS_symbols[i]);
	thevrule.lm_seq.push_back(symbol_t(ew, eid));
#endif
      }
    }
    else{
      //      lmleaf(ow, (*kid1), 0);
      //ow<<0;
        lmstr.push_back(boost::lexical_cast<string>(0));
#ifdef SANITY
      thevrule.lm_seq.push_back(symbol_t(ix, 0));
#endif

    }

  }
  print_lm_strings(ow,lmstr);
  ow<<" ";

  //other attributes
//  ow<<"sblm_string={{{";
//  for (int i=0; i<rhsStates; i++){
//    ow<<L2Rpermu[i];
//    ow<<" ";
//  }
//  ow<<"}}} ";

//  ow<<"lm=yes "<<"sblm=yes";

  if(use_stupid_dependency_lm_string){
      ow<<" dep_lm={{{yes}}} "<<"dep_lm_string={{{ "<<stupid_dependency_lm_string<<"}}} ";
  }


// #ifdef NOPACKING
//   ow<<" bin-prob="
//     <<curprb;
//   ow<<" original_rule_prob="
//     <<curprb;
// #endif

//  ow<<" rule_file_line_number=";
  //  ow<<"{{{";
//  ow<<totalRules;
  //    ow<<"}}}";

  //for reconstruction
  //ow<<" rhs={{{";
  //ow<<myRule->rhs_string();
  //ow<<"}}}";

  ow<<endl;
}

inline string vrul_construct(char kid1type, int kid1pos, string &kid1str, char kid2type, int kid2pos, string &kid2str, node *kid1, node *kid2, int inversion, vrule_t **thenewrule, bool lmable, bool firstlexical, bool active){
  //construct a virtual nonterminal
  string vlhs;

  //cerr << "vrule construction from\n";
  //cerr << "kid1: " << kid1str << " at " << kid1pos << "\n" ;
  //if (kid1) cerr << "\t"<<*kid1<<"\n";
  //cerr << "kid2: " << kid2str << " at " << kid2pos << "\n";
  //if (kid2) cerr << "\t"<<*kid2<<"\n";
  if (nosharing) {
      vlhs="V[" + boost::lexical_cast<std::string>(ruleID) + ":" + boost::lexical_cast<std::string>(vrule_counter) + "]";
      ++vrule_counter;
  } else {

  if (!kid1){//leaf, concatenating a rt or vt or cw with a cw
    vlhs="V";
    vlhs+="[";
    if (subgrammar_id != "") vlhs += "[" + subgrammar_id + "]";

    if (kid1type==cw){
      vlhs+="\"";
      vlhs+=conv(kid1str);
      vlhs+="\"";
    }
    else if (kid1type==rt){
      vlhs+=kid1str;
      for (size_t dx = 0; dx != pointer_features.size(); ++dx) {
          vlhs += "{";
          vlhs += pointer_features[dx][kid1pos];
          vlhs += "}";
      }
    } else {
        vlhs+=kid1str;
    }

    vlhs+=WORDELIM;

    if (kid2type==cw){
      vlhs+="\"";
      vlhs+=conv(kid2str);
      vlhs+="\"";
    }
    else if (kid2type==rt) {
      vlhs+=kid2str;
      for (size_t dx = 0; dx != pointer_features.size(); ++dx) {
          vlhs += "{";
          vlhs += pointer_features[dx][kid2pos];
          vlhs += "}";
      }
    } else {
        vlhs+=kid2str;
    }

    if (firstlexical){
      vlhs+=ECDELIM;
      for (int i=0; i<lhsptr; i++){
        //vlhs+="\"";
          vlhs+=conv(LHS_symbols[i]);
        //vlhs+="\"";
          vlhs+=WORDELIM;
      }
    }

    vlhs+="]";
  }
  else{

    vlhs="V";
    if (!inversion){
      vlhs+="[";
    }
    else{
      vlhs+="<";
    }
    if (subgrammar_id != "") vlhs += "[" + subgrammar_id + "]";

    if (ISLEAF((*kid1))){
      for (int itr=kid1->e_left_idx; itr<=kid1->e_right_idx; itr++){
	if (itr!=LHS_idx[EINDEX((*kid1))-1]){//english word
	  //vlhs+="\"";
	  vlhs+=conv(LHS_symbols[itr]);
	  //vlhs+="\"";
	}
	else if(kid1type==rt){
	  vlhs+=kid1->label;
	  for (size_t dx = 0; dx != pointer_features.size(); ++dx) {
            vlhs += "{";
            vlhs += pointer_features[dx][kid1pos];
            vlhs += "}";
        }
	} else vlhs+=kid1->label;
	vlhs+=WORDELIM;
      }
    }
    else{
      vlhs+=kid1->label;
      vlhs+=WORDELIM;
    }

    vlhs+=BINDELIM;

    if (ISLEAF((*kid2))){
      for (int itr=kid2->e_left_idx; itr<=kid2->e_right_idx; itr++){
	if (itr!=LHS_idx[EINDEX((*kid2))-1]){//english word
	  //vlhs+="\"";
	  vlhs+=conv(LHS_symbols[itr]);
	  //vlhs+="\"";
	}
	else{
	  vlhs+=kid2->label;
	  if (kid2type == rt) {
	      for (size_t dx = 0; dx != pointer_features.size(); ++dx) {
                  vlhs += "{";
                  vlhs += pointer_features[dx][kid2pos];
                  vlhs += "}";
              }
	  }
	}
	vlhs+=WORDELIM;
      }
    }
    else{
      vlhs+=kid2->label;
      vlhs+=WORDELIM;
    }

    if (!inversion){
      vlhs+="]";
    }
    else{
      vlhs+=">";
    }

  }
  }

  //put the new virtual rule into table
  int newrid=putsym(vt, vlhs);
  vrule_t &newvrule=vrul_tbl[newrid];
  //: if existing, we can check the consitency here...

  //  if (newvrule.xrs_seq.size()>0){//existing
//     if (newvrule.kids[0].tg!=kid1type ||
// 	newvrule.kids[0].id!=putsym(kid1type, kid1str))
//       cerr<<"WARNING: "<<endl;
//     if (newvrule.kids[1].tg!=kid2type ||
// 	newvrule.kids[1].id!=putsym(kid2type, kid2str))
//       cerr<<"WARNING: "<<endl;
//  }

//  if (!newvrule.xrs_seq.size()>0){//nonexisting
  if (newvrule.specified) {
      //if (kid1type == rt or kid2type == rt) {
      if (kid1type == rt) {
            newvrule.kids[0].pos = kid1pos;
      }
      if (kid2type == rt) {
            newvrule.kids[1].pos = kid2pos;
      }
      if ((is_single_rt(kid1type,kid1str) or is_single_rt(kid2type,kid2str)) and
           kid1type != cw and kid2type != cw) {
          for (size_t dx = 0; dx != or_pointer_features.size(); ++dx) {
              if (or_pointer_features[dx].size() > 0) {
              newvrule.or_pointer_feature[dx].push_back(boost::array<int,2>());

              if (is_single_rt(kid1type,kid1str)) {
                  symbol_t s = get_single_rt_sym(kid1type,kid1str,kid1pos);
                  int pos = s.pos;
                  string m = or_pointer_features[dx][pos];
                  newvrule.or_pointer_feature[dx].back()[0] = putsym(rt,m);
              }

              if (is_single_rt(kid2type,kid2str)) {
                  symbol_t s = get_single_rt_sym(kid2type,kid2str,kid2pos);
                  int pos = s.pos;
                  string m = or_pointer_features[dx][pos];
                  newvrule.or_pointer_feature[dx].back()[1] = putsym(rt,m);
              }
          }
          }
      }
  } else {
    //the fields
    if ((is_single_rt(kid1type,kid1str) or is_single_rt(kid2type,kid2str)) and
         kid1type != cw and kid2type != cw) {
        //std::cerr << vlhs << " is or_pointer location\n";
        newvrule.or_pointer_feature.resize(or_pointer_features.size());

        for (size_t dx = 0; dx != or_pointer_features.size(); ++dx)  {
            newvrule.or_pointer_feature[dx].push_back(boost::array<int,2>());
            if (or_pointer_features[dx].size() > 0) newvrule.or_pointer_feature[dx].push_back(boost::array<int,2>());
        }

        if (is_single_rt(kid1type,kid1str)) {
            for (size_t dx = 0; dx != or_pointer_features.size(); ++dx)  {
                symbol_t s = get_single_rt_sym(kid1type,kid1str,kid1pos);
                int pos = s.pos;
                newvrule.or_pointer_feature[dx][0][0] = putsym(rt,string(getsym(rt,s.id)));
                if (or_pointer_features[dx].size() > 0)newvrule.or_pointer_feature[dx][1][0] = putsym(rt,or_pointer_features[dx][pos]);
                //int skpos = get_single_rt_sym(kid1type,kid1str,kid1pos).pos;
                //newvrule.or_pointer_feature[dx].back()[0] = putsym(rt,or_pointer_features[dx][skpos]);
            }
        } else {
            for (size_t dx = 0; dx != or_pointer_features.size(); ++dx) {
                newvrule.or_pointer_feature[dx][0][0] = putsym(rt,"--");
            }
        }
        if (is_single_rt(kid2type,kid2str)) {
            for (size_t dx = 0; dx != or_pointer_features.size(); ++dx) {
                symbol_t s = get_single_rt_sym(kid2type,kid2str,kid2pos);
                int pos = s.pos;
                newvrule.or_pointer_feature[dx][0][1] = putsym(rt,string(getsym(rt,s.id)));
                if (or_pointer_features[dx].size() > 0) newvrule.or_pointer_feature[dx][1][1] = putsym(rt,or_pointer_features[dx][pos]);
                //int skpos = get_single_rt_sym(kid2type,kid2str,kid2pos).pos;
                //newvrule.or_pointer_feature[dx].back()[1] = putsym(rt,or_pointer_features[dx][skpos]);
            }
        } else {
            for (size_t dx = 0; dx != or_pointer_features.size(); ++dx) {
                newvrule.or_pointer_feature[dx][0][1] = putsym(rt,"--");
            }
        }
    }
    if (kid1type == rt or kid2type == rt) {
        newvrule.pointer_feature.resize(pointer_features.size());
        if (kid1type == rt) {
            for (size_t dx = 0; dx != pointer_features.size(); ++dx) {
                newvrule.pointer_feature[dx][0] = putsym(rt,pointer_features[dx][kid1pos]);
            }
        }
        if (kid2type == rt) {
            for (size_t dx = 0; dx != pointer_features.size(); ++dx) {
                newvrule.pointer_feature[dx][1] = putsym(rt,pointer_features[dx][kid2pos]);
            }
        }
    }

    newvrule.specified=true;
    newvrule.inversion=inversion;
    newvrule.lhs.tg=vt;
    newvrule.lhs.id=newrid;
    newvrule.lhs.pos=-1;
    newvrule.kids[0].tg=kid1type;
    newvrule.kids[0].id=putsym(kid1type, kid1str);
    newvrule.kids[0].pos=kid1pos;
    newvrule.kids[1].tg=kid2type;
    newvrule.kids[1].id=putsym(kid2type, kid2str);
    newvrule.kids[1].pos=kid2pos;

    if (lmable && kid1){//having lm
      //      newvrule.lm_seq.clear();
      if (!inversion){
	lmvector(newvrule.lm_seq, kid1, 0);
	lmvector(newvrule.lm_seq, kid2, 1);
      }
      else{
	lmvector(newvrule.lm_seq, kid2, 1);
	lmvector(newvrule.lm_seq, kid1, 0);
      }

    }
    else if (lmable){
      if (firstlexical){//do it early
	for (int i=0; i<lhsptr; i++){
	  int eid=putsym(ew, LHS_symbols[i]);
	  newvrule.lm_seq.push_back(symbol_t(ew, eid,-1));
	}
      }
      else
      //      newvrul.lm_seq.clear();
      newvrule.lm_seq.push_back(symbol_t(ix, 0,-1));//placeholdr
    }
  }

#ifdef FLATTEN
  if (!flatten || active)
#endif
    newvrule.xrs_seq.push_back(ruleID);

#ifdef FLATTEN
  if (flatten && active)
    newvrule.active=true;
#endif

  *thenewrule=&newvrule;

  return vlhs;
}




#ifdef SANITY

void rule_check_sum(vrule_t *thevrule, string & esum, string  & csum){
  string esub[2];
  string csub[2];

  int maxrlen=(thevrule->kids[1].tg!=nu)?2:1;

  for (int k=0; k<maxrlen; k++){
    if (thevrule->kids[k].tg==rt){
      esub[k]+=getsym(rt, thevrule->kids[k].id);
      csub[k]+=esub[k];
    }else if (thevrule->kids[k].tg==cw){
      csub[k]=getsym(cw, thevrule->kids[k].id);
    }else if (thevrule->kids[k].tg==vt){
      vrule_t *kidvrul=&vrul_tbl[thevrule->kids[k].id];
      string sube, subc;
      rule_check_sum(kidvrul, sube, subc);
      esub[k]=sube;
      csub[k]=subc;

    }
  }

  for (int k=0; k<maxrlen; k++){
    csum+=csub[k];
    if (k<maxrlen-1)
      csum+=" ";
  }


  for (int lmi=0; lmi<thevrule->lm_seq.size(); lmi++){
    if (thevrule->lm_seq[lmi].tg==ew){
      string eword=getsym(ew, thevrule->lm_seq[lmi].id);
      esum+=eword;
    }
    else{//idx
      if (thevrule->kids[0].tg==cw || thevrule->kids[1].tg==cw){
	esum+=esub[0];
	esum+=esub[1];
      }
      else{
	//	cerr<<"id:"<<thevrule->lm_seq[lmi].id<<esub[thevrule->lm_seq[lmi].id]<<endl;
	esum+=esub[thevrule->lm_seq[lmi].id];
      }
    }

    //    cerr<<"here"<<esum<<" "<<endl;
    if (lmi<thevrule->lm_seq.size()-1)
      esum+=" ";
  }

}

void sanitycheck(){
  for (int i=0; i<totalXrs; i++){
    vrule_t *thevrule=&topvrul_tbl[i];
    string e_string, c_string;

    rule_check_sum(thevrule, e_string, c_string);
    cerr<<e_string<<endl;
    cerr<<c_string<<endl;

  }
}
#endif

//#define VLHS(r) (vid2sym_tbl[r->lhs.id])


//safety replacements

//binarize pure lexical rules
//pure lexical means no nonterminals on rhs
inline int lexbin(bool complete = true){
  string label;
  string lastlabel;
  //    label.clear();

  //label=conv(RHS_symbols[0]);
  label=RHS_symbols[0];
  lastlabel=label;

  //    cerr<<lastlabel<<endl;
  for (int i=1; i<rhsSize-1; i++){

    vrule_t *therul=NULL;
    //cerr <<"lexbin:\n";
    lastlabel=vrul_construct( ((i==1)?cw:vt), -1, lastlabel
                            , cw, -1, RHS_symbols[i]
                            , NULL
                            , NULL
                            , 0
                            , &therul
                            , true
                            , (i==1)
                            , false
                            ); //not going to be flattened
    //    lastlabel=VLHS(therul);
    vRules++;

    //V["c1"_"c2"] or V[V[...]_"ci"]
    //V[...]:VL: V[...](x0:NULL) -> "c1" "c2" | V[...] "ci"

  }//end of expansion to the rightmost

  //top-level bin rule or uni rule
  //Tree -> "c1"  | "c1" "c2" | V[...] "cn"

#ifdef FLATTEN
  if (flatten) {
    flatten_rrul_construct();
  } else
#endif
  //if(complete)
      rrul_construct( (rhsSize<=2)?cw:vt, -1, lastlabel
                    , cw, -1, (rhsSize>1)?RHS_symbols[rhsSize-1]:emp
                    , NULL
                    , NULL
                    , 0
                    );//need to flatten

  return vRules; //end of pure lexical rule
}



//binarize unary nonterminal rules
// ... -> ... x0 ...
//associating chinese words
inline int unibin(node &nd, bool cplt = true){
  //associate the bordering chinese words
  int lft_idx, lft, rht_idx, rht;
  lft_idx = RHS_idx[rhsptr]-1;
  string label;
  string lastlabel;
  int rhs_pos = RHS_idx[rhsptr];
  int remain;

  remain=rhsSize-2; //remaining virtual rules needed, when nd is an intermediate node, we will exhaust remaining

  label.clear();
  lft=(prev_leaf)?(prev_leaf->c_right_idx+1):0; //leaves are visited from left to right on chinese

  int lindex=LHS_symbols[LHS_idx[EINDEX(nd)-1]].find_first_of(":", 0);
  //cerr<<INDEX(nd)<<" "<<RHS_symbols[RHS_idx[INDEX(nd)-1]]<<" lindex:"<<lindex<<endl;
  label=LHS_symbols[LHS_idx[EINDEX(nd)-1]].substr(lindex+1, LHS_symbols[LHS_idx[EINDEX(nd)-1]].size());

  lastlabel=label;//the real nonterminal

  vrule_t *therul=NULL;
  int isVirtual=0;
  for (; remain>0 && lft_idx>=lft; lft_idx--){
    //iterate through the left neighboring chinese words
    //cerr << "unibin left:\n";
    lastlabel=vrul_construct( cw, lft_idx, RHS_symbols[lft_idx]
                            , (isVirtual?vt:rt), (isVirtual?-1:rhs_pos), lastlabel
                            , NULL
                            , NULL
                            , 0
                            , &therul
                            , true
                            , false
                            , false
                            );//assume the rule is not active temporarily
    //    lastlabel=VLHS(therul);
    vRules++;
    remain--;
    isVirtual=1;

    //V["c"_V...]
    //x0 is refering to the only nonterminal child

    //    lastlabel=label;
  }//end of associating the left words

  nd.c_left_idx=lft; //remembering the boundary

  rht=(rhsptr<rhsStates-1)?(RHS_idx[rhsptr+1]-1):rhsSize-1;

  for (rht_idx=RHS_idx[rhsptr]+1; remain>0 && rht_idx<=rht; rht_idx++){
      //iterate through the right neighboring chinese words

    //    vrule_t *therul=NULL;
    //cerr << "unibin right:\n";
    lastlabel=vrul_construct( (isVirtual?vt:rt), (isVirtual?-1:rhs_pos), lastlabel
                            , cw, rht_idx, RHS_symbols[rht_idx]
                            , NULL
                            , NULL
                            , 0
                            , &therul
                            , true
                            , false
                            , false
                            );//assume the rule is not active temporarily
    //    lastlabel=VLHS(therul);
    vRules++;
    remain--;
    isVirtual=1;

    //V[V..._"c"]
    //    lastlabel=label;
  } //end of associating right words

// #ifdef FLATTEN
//   //post processing the last vrule
//   if (flatten && rhsStates>1 && therul){//we do have a virtual rule that is to be flattened
//     therul->xrs_seq.push_back(ruleID);
//     therul->active=true;
//   }
// #endif

  nd.c_right_idx=rht;//remembering the right boundary

  rhsptr++;//keepiing track of visited rhs variables

  nd.label=lastlabel; //this virtual (or non-virtual) label is for node nd

  prev_leaf=&nd; //going right to next leaf

  if (isVirtual)
    nd.virt=1; //flag of virtual

  //we don't create virtual unary rules
  //unary variable rule
  if (rhsStates==1){

    nd.e_left_idx=0;
    nd.e_right_idx=lhsptr-1;

// #ifdef FLATTEN
//     if (flatten)
//         flatten_rrul_construct();
//     else {
// #endif
    if (lft_idx==lft){
      //if (cplt)
      rrul_construct( cw, -1, RHS_symbols[lft_idx]
                    , (isVirtual)?vt:rt, (isVirtual)?-1: lft_idx + 1, (isVirtual)?lastlabel:LHS_symbols[LHS_idx[EINDEX(nd)-1]]
                    , &nd
                    , &nd
                    , 0
                    );
    }
    else if (rht_idx==rht){
      //if (cplt)
      rrul_construct( (isVirtual)?vt:rt, isVirtual?-1:rht_idx-1, (isVirtual)?lastlabel:LHS_symbols[LHS_idx[EINDEX(nd)-1]]
                    , cw, -1, RHS_symbols[rht_idx]
                    , &nd
                    , &nd
                    , 0
                    );
    }
    else{
      //if (cplt)
      rrul_construct( rt, rhs_pos, LHS_symbols[LHS_idx[EINDEX(nd)-1]]
                    , cw, -1, emp
                    , &nd
                    , &nd
                    , 0
                    );
    }
// #ifdef FLATTEN
//     }
// #endif
  }//end of unary variable rule

  return vRules;
}

//binarize two nonterminal rules
// ... -> x0 x1 | x1 x0
//associating english words
inline int binbin(node &nd, int index,bool cplt=true){
  node & leftchild=nodes[nd.leftchild];
  node & rightchild=nodes[nd.rightchild];

  int prev_idx;

  //associate the bordering english words
  if (nd.tag == 1){

    //straight rule
    //computing the covered english words
    if (ISLEAF(leftchild)){
      leftchild.e_left_idx=
	(EINDEX(leftchild)>1)?
	(
	 (covered_mask[prev_idx=LHS_idx[EINDEX(leftchild)-2]])?//is last anchor covered?
	 LHS_idx[EINDEX(leftchild)-1]: //stop invading
	 prev_idx+1 //invade
	 ):
	0;//grow to the previous anchor or the left boundary

      leftchild.e_right_idx=
	(ISLEAF(rightchild))?
	(LHS_idx[EINDEX(rightchild)-1]-1)://invade
	(LHS_idx[EINDEX(leftchild)-1]);//grow to the next anchor or stop

      covered_mask[LHS_idx[EINDEX(leftchild)-1]]=1;//i'm visited
    }

    if (ISLEAF(rightchild)){
      rightchild.e_right_idx=
	(EINDEX(rightchild)<rhsStates)?
	(
	 (covered_mask[prev_idx=LHS_idx[EINDEX(rightchild)]])?//is next ancchor covered?
	 LHS_idx[EINDEX(rightchild)-1]://stop invading
	 prev_idx-1//invade
	 ):
	lhsptr-1;//grow to the next anchor or the right boundary

      rightchild.e_left_idx=(LHS_idx[EINDEX(rightchild)-1]);//stop at this anchor

      covered_mask[LHS_idx[EINDEX(rightchild)-1]]=1;//i'm visited
    }

    //cerr<<"leftchild left idx"<<leftchild.e_left_idx<<endl;
    //cerr<<"leftchild right idx"<<leftchild.e_right_idx<<endl;

    //cerr<<"rightchild left idx"<<rightchild.e_left_idx<<endl;
    //cerr<<"rightchild right idx"<<rightchild.e_right_idx<<endl;

    nd.e_left_idx=leftchild.e_left_idx;
    nd.e_right_idx=rightchild.e_right_idx;

    //cerr<<nd.e_left_idx<<"!!"<<endl;
    //cerr<<nd.e_right_idx<<"!!"<<endl;
    //cerr<<"straight"<<endl;

    //creating the parent label

    if (index==stck[0] and cplt){//root
      //if (cplt)
      rrul_construct( (!ISVIRTUAL(leftchild))?rt:vt, ISVIRTUAL(leftchild)?-1:cright(leftchild), (ISVIRTUAL(leftchild))?leftchild.label:LHS_symbols[LHS_idx[EINDEX(leftchild)-1]]
                    , (!ISVIRTUAL(rightchild))?rt:vt, ISVIRTUAL(rightchild)?-1:cright(rightchild), (ISVIRTUAL(rightchild))?rightchild.label:LHS_symbols[LHS_idx[EINDEX(rightchild)-1]]
                    , &leftchild
                    , &rightchild
                    , 0
                    );

    }//end of straight root rule
    else{
    //virtual bin straight rule

        vrule_t *therul=NULL;
        //cerr << "binbin straight:\n";
        nd.label=vrul_construct( (leftchild.virt?vt:rt), leftchild.virt?-1:cright(leftchild), leftchild.label
                               , (rightchild.virt?vt:rt), rightchild.virt?-1:cright(rightchild), rightchild.label
                               , &leftchild
                               , &rightchild
                               , 0
                               , &therul
                               , true
                               , false
                               , true
                               );
	//	nd.label=VLHS(therul);
	vRules++;

      }//end of virtual bin rule

    }//end of straight rule
    else{
      //inverted rule


      if (ISLEAF(rightchild)){
	rightchild.e_left_idx=
	  (EINDEX(rightchild)>1)?
	  (
           (covered_mask[prev_idx=LHS_idx[EINDEX(rightchild)-2]])?
           LHS_idx[EINDEX(rightchild)-1]:
           prev_idx+1
           ):
	  0;//grow to the previous anchor or the left boundary

        rightchild.e_right_idx=
	  (ISLEAF(leftchild))?
	  (LHS_idx[EINDEX(leftchild)-1]-1):
          (LHS_idx[EINDEX(rightchild)-1]);//grow to the next anchor or stop at this anchor

	covered_mask[LHS_idx[EINDEX(rightchild)-1]]=1;

      }

      if (ISLEAF(leftchild)){
        leftchild.e_right_idx=
	  (EINDEX(leftchild)<rhsStates)?
	  (
           (covered_mask[prev_idx=LHS_idx[EINDEX(leftchild)]])?
           LHS_idx[EINDEX(leftchild)-1]:
           prev_idx-1
           ):
	  lhsptr-1;//grow to the next anchor or the right boundary

        leftchild.e_left_idx=(LHS_idx[EINDEX(leftchild)-1]);//stop at this anchor

	covered_mask[LHS_idx[EINDEX(leftchild)-1]]=1;

      }


      //cerr<<"leftchild left idx"<<leftchild.e_left_idx<<endl;
      //cerr<<"leftchild right idx"<<leftchild.e_right_idx<<endl;

      //cerr<<"rightchild left idx"<<rightchild.e_left_idx<<endl;
      //cerr<<"rightchild right idx"<<rightchild.e_right_idx<<endl;

      nd.e_left_idx=rightchild.e_left_idx;
      nd.e_right_idx=leftchild.e_right_idx;

      //cerr<<nd.e_left_idx<<"!!"<<endl;
      //cerr<<nd.e_right_idx<<"!!"<<endl;
      //cerr<<"inversion"<<endl;

      if (index==stck[0] and cplt){//root binary rule
    //if (cplt)
	rrul_construct( (!ISVIRTUAL(leftchild))?rt:vt, leftchild.virt?-1:cright(leftchild), (ISVIRTUAL(leftchild))?leftchild.label:LHS_symbols[LHS_idx[EINDEX(leftchild)-1]]
                  , (!ISVIRTUAL(rightchild))?rt:vt, rightchild.virt?-1:cright(rightchild), (ISVIRTUAL(rightchild))?rightchild.label:LHS_symbols[LHS_idx[EINDEX(rightchild)-1]]
                  , &leftchild
                  , &rightchild
                  , 1
                  );

      }//end of invertedbinary root rule
      else{
	//virtual bin inverted rule

          vrule_t *therul=NULL;
          //cerr << "binbin inverted:\n";
          nd.label=vrul_construct( (leftchild.virt?vt:rt), leftchild.virt?-1:cright(leftchild), leftchild.label
                                 , (rightchild.virt?vt:rt), rightchild.virt?-1:cright(rightchild), rightchild.label
                                 , &leftchild
                                 , &rightchild
                                 , 1
                                 , &therul
                                 , true
                                 , false
                                 , true
                                 );
        //nd.label=VLHS(therul);
          vRules++;

      }//end of virtual bin rule

    }//end of inverted rule

  return vRules;
}

//post order visiting the itg parse tree
//generating bin rules
void output (int index,bool cplt = true) {

  if (index<0){//lexical rules
    lexbin(cplt);
    return;
  }

  //for rules having at least one variable

  node &nd = nodes[index];
  if (nd.tag == 3){//a leaf of the tree
    unibin(nd,cplt);
  }
  else {
    //binary rules

    //recurse on leftchild
    output(nd.leftchild,cplt);
    //recurse on rightchild
    output(nd.rightchild,cplt);

    binbin(nd, index, cplt);
    //std::clog << "(((" << nodes[nd.leftchild] << "," << nodes[nd.rightchild] << ")))";

  }//end of straight rule or inverted rule

  return;
}


#ifdef SANITY
void sanity_xrs(){
  int curlhsptr=0;
  int curlhsStateptr=0;

  for (curlhsptr=0; curlhsptr<lhsptr;){
    if (curlhsStateptr<rhsStates && curlhsptr==LHS_idx[curlhsStateptr]){//real nt
      int lindex=LHS_symbols[curlhsptr].find_first_of(":", 0);
      int len=LHS_symbols[curlhsptr].size();
      string nt=LHS_symbols[curlhsptr].substr(lindex+1, len);
      ctmp<<nt;
      curlhsStateptr++;
    }
    else
      ctmp<<LHS_symbols[curlhsptr];
    if (curlhsptr<lhsptr-1)
      ctmp<<" ";
    curlhsptr++;
  }

  ctmp<<endl;

  int currhsptr=0;
  int currhsStateptr=0;

  for (currhsptr=0; currhsptr<rhsSize;){
    if (currhsStateptr<rhsStates && currhsptr==RHS_idx[currhsStateptr]){//real nt
      int lindex=RHS_symbols[currhsptr].find_first_of(":", 0);
      int len=RHS_symbols[currhsptr].size();
      string nt=RHS_symbols[currhsptr].substr(lindex+1, len);
      ctmp<<nt;
      currhsStateptr++;
    }
    else
      ctmp<<RHS_symbols[currhsptr];
    if (currhsptr<rhsSize-1)
      ctmp<<" ";
    currhsptr++;
  }
  ctmp<<endl;
}
#endif

//linear time itg bracketer

#define stacking(lft, rht, aidx, bidx, t, v){\
nodes.push_back(node(lft, rht, aidx, bidx, t, v));\
stck[top++]=nodes.size()-1;\
}

#define straight(na, nb) (na.right==nb.left-1)
#define inverted(na, nb) (na.left==nb.right+1)

//try to combine two top nodes
inline int combine(){
  bool can=true;

  while (top>1 && can){
    int aidx=stck[top-2];
    int bidx=stck[top-1];

    node &na=nodes[stck[top-2]];
    node &nb=nodes[stck[top-1]];

    if (straight(na, nb)){
      top-=2;
      stacking(na.left, nb.right, aidx, bidx, 1, 1);
    }
    else
      if (inverted(na, nb)){
	top-=2;
	stacking(nb.left, na.right, aidx, bidx, 2, 1);
      }
      else{
	//non contigous
	can=false;
      }
  }//til we cannot combining any two adjacent spans

  return top;
}

int bracketing() {
  int n=Alignment.size();

  stck.resize(n);
  top=0;
  nodes.clear();

  //shift-reduce, reduce preferred
  for (int i = 0; i < n; i++) {
    stacking(Alignment[i], Alignment[i], -1, -1, 3, 0);
    // nodeptr newnode;
    combine();
  }

  if (top > 1) {
    nonbin++;
    //cerr<<"unbinarizable!"<<endl;
    if (filteringMode) {
      // non-binarizable cases
      *non_itg_out<<line<<endl;
    }
    rhsptr=0;
    prev_leaf=NULL;
    vRules=0;
    //std::clog << "**********************************************************" << std::endl;
    //std::clog << line << std::endl;
    //for (int x = 0; x != top; ++x) {
    //    output(stck[x],false);
    //    std::clog << nodes[stck[x]] << " \n";
    //}
    //std::clog << "**********************************************************" << std::endl;
    if (left_right_backoff) {
        //for (int x = 0; x != top; ++x) output(stck[x]);
        vector<string> vec = left_right_binarize(line);
        for (vector<string>::iterator i = vec.begin(); i != vec.end(); ++i) {
            ow << *i << endl;
        }
    }

  }
  else {
    bin++;
    rhsptr=0;
    prev_leaf=NULL;

    //so we output the binarizable cases
    //#ifndef filteringMode
    if (!filteringMode) {
      vRules=0;
      if (top==1)
	output(stck[0]);
      else
	output(-1);//purel lexical rule
    }
    else {
      // filtering mode, output binarizable cases
      *itg_out << line <<endl;
    }
  }

#ifdef SANITY
    sanity_xrs();
#endif

  return 0;
}

//! Returns true if a node dominates any other node.
bool dominate_any_other(ns_RuleReader::RuleNode*curr){
    if(!curr){ return false;}

    if(curr->isHeadNode()){
        // if the parent of this node has other children.
        if( curr->getParent() && curr->getParent()->getChildren()->size() > 1){
            return true;
        }  else {
            return dominate_any_other(curr->getParent());
        }
    } else {
        return false;
    }
}

//! Output dependency LM string.
void get_dependency_lm_string(ostream& ost, ns_RuleReader::RuleNode *curr, ns_RuleReader::Rule* rule)
{

  if (curr->isNonTerminal()) {  // if this is a nonterminal node
    //cerr << "NT: " << curr->getString(false) << endl;
    // We dont print out internal nodes when we are interested only in dependency tree.
    //ost<<"(" << curr->getString(false) << ")";

    // when it comes to dependency tree, we need to print out the phrase boundaries.
    if(!curr->isHeadNode() ) {
        ost<<"<D> ";
    }


    if(!curr->isLeaf()){
        for (vector<ns_RuleReader::RuleNode *>::iterator it=curr->getChildren()->begin();
                                   it!=curr->getChildren()->end(); ++it) { // recurse on children
          get_dependency_lm_string(ost, (*it), rule);
        }
    }

    // when it comes to dependency tree, we need to print out the phrase boundaries.
    if(!curr->isHeadNode() ) {
        ost<<"</D> ";
    }

    // We dont print out internal nodes when we are interested only in dependency tree.
            // ost<<"(/" << curr->getString(false) << ")";
  }
  else{
 // mark the head.
    if(curr->isHeadNode()){
        if(dominate_any_other(curr) || curr->isPointer()){
            ost<<"<H> ";
        }
    }else if(!curr->isHeadNode()){ // this will only be for pointer.
        ost<<"<D> ";
    }


    //if this is a english terminal node
    if (curr->isPointer()) { // is this a pointer leaf node?
        string var = curr->getString();
        int index=var.find_first_of(":", 0);
//        int len=var.size(); //FIXME: this is unused.
        string v=var.substr(1, index-1); //FIXME: what if : not found?  impossible?
        ost<<rule->rhsVarIndex(atoi(v.c_str()))<<" ";
    } else {
        ost<<"\""<<curr->getString()<<"\" ";
    }

    // mark the head.
    if(curr->isHeadNode()){ // lexical items will always go to this branch.
        if(dominate_any_other(curr) || curr->isPointer()){
            ost<<"</H> ";
        }
    }else if(!curr->isHeadNode()){
        ost<<"</D> ";
    }
  }
}

//traverse a treenode on lhs of xrs
void traverseNode(ns_RuleReader::RuleNode *curr, int depth) // traverse a RuleNode tree
{
    bool paren_xml_tag=etree_string && (!state_etree_string || depth > 0);

  if (curr->isNonTerminal()) { 	// if this is a nonterminal node
    //cerr << "NT: " << curr->getString(false) << endl;
    if (paren_xml_tag && ! dependency_tree_string) {
        LHS_symbols.push_back("(" + curr->getString(false) + ")");
        covered_mask.push_back(0);
        ++lhsptr;
    } else if (preterminal_string and curr->isPreTerminal()) {
        LHS_symbols.push_back("(" + curr->getString(false) + ")");
        covered_mask.push_back(0);
        ++lhsptr;
    }

    // when it comes to dependency tree, we need to print out the phrase boundaries.
    if(!curr->isHeadNode() && dependency_tree_string ) {
            LHS_symbols.push_back("<D>");
            covered_mask.push_back(0);
            ++lhsptr;
    }


    if(!curr->isLeaf()){
        for (vector<ns_RuleReader::RuleNode *>::iterator it=curr->getChildren()->begin();
                it!=curr->getChildren()->end(); ++it) { // recurse on children
          traverseNode((*it), depth+1);
        } // end for each child
    }

    // when it comes to dependency tree, we need to print out the phrase boundaries.
    if(!curr->isHeadNode() && dependency_tree_string ) {
            LHS_symbols.push_back("</D>");
            covered_mask.push_back(0);
            ++lhsptr;
    }

    if (paren_xml_tag && !dependency_tree_string) {
        LHS_symbols.push_back("(/" + curr->getString(false) + ")");
        covered_mask.push_back(0);
        ++lhsptr;
    }
  }
  else{
    // mark the head.
    if(dependency_tree_string && curr->isHeadNode()){
        LHS_symbols.push_back("<H>");
        covered_mask.push_back(0);
        ++lhsptr;
    }else if(dependency_tree_string && !curr->isHeadNode()){ // this will only be for pointer.
        LHS_symbols.push_back("<D>");
        covered_mask.push_back(0);
        ++lhsptr;
    }

    //if this is a english terminal node
    if (curr->isPointer()) { // is this a pointer leaf node?
      LHS_symbols.push_back(curr->getString());
      RHS_symbols[curr->getRHSIndex()]=curr->getString();
      LHS_idx.push_back(lhsptr);
    } else {
      LHS_symbols.push_back("\"" + curr->getString() + "\"");
    }
    //bit map for binarizer to know which english words have been covered
    covered_mask.push_back(0);
    lhsptr++;

    // mark the head.
    if(dependency_tree_string && curr->isHeadNode()){ // lexical items will always go to this branch.
        LHS_symbols.push_back("</H>");
        covered_mask.push_back(0);
        ++lhsptr;
    }else if(dependency_tree_string && !curr->isHeadNode()){
        LHS_symbols.push_back("</D>");
        covered_mask.push_back(0);
        ++lhsptr;
    }
  }
}



//wrapper for traverseNode
void traverse(ns_RuleReader::RuleNode *curr){
  lhsptr=0;
  LHS_symbols.clear();
  LHS_idx.clear();
  //  RHS_symbols.resize(MAXSTATES);
  //  RHS_symbols.clear();
  covered_mask.clear();
  L2Rpermu.resize(rhsSize);
  if (state_etree_string) {
      if (!curr->isNonTerminal() || curr->getChildren()->size() != 1)
          throw_stateful_etree();
//      curr=curr->getChildren()->front();
  }

  traverseNode(curr, 0);
}

//#ifdef NOPACKING
//retrieve rule probability
//inline void getprob(){
//  string probval=myRule->getAttributeValue(prob_string);
//  const char *prbuf=probval.c_str();
//  curprb=exp(atof(prbuf+2));//the prob string looks like e^...
//  curprb<0?curprb=0:0;
//  //  cerr<<curprb;
//}
//#endif

//loop over all the xrs rules in the file of arg[1]
//bracketing() does everything
int main(int argc, char** argv){
  //  if (argc!=2){
  //    cerr<<argv[0]<<"<rule-file>"<<endl;
  //    return 1;
  //  }

  //  ifstream from(argv[1]);

  //  if (!from){
  //    cerr<<"Couldn't open "<<argv[1]<<endl;
  //    exit(-1);
  //  }

#ifdef OLD_ARGS
    istream &in=cin;
  if (argc>1){
    filteringMode=true;
  }
  else
    filteringMode=false;
#else
  using namespace boost::program_options;
  using namespace graehl;
  typedef boost::program_options::options_description OD;
  istream_arg input_file("-");
  ostream_arg output_file("-");
  ostream_arg non_itg_output_file("-2");
  bool help;
  bool keep_duplicates;
  bool ignoreme=false;
  OD all(general_options_desc());
  ostream &log=cerr;
  string pointer_feature_names_str;
  string or_pointer_feature_names_str;
  all.add_options()
      ( "help,h"
      , bool_switch(&help)
      , "show usage/documentation"
      )
      ( "keep-duplicates"
      , bool_switch(&keep_duplicates)
      , "do not eliminate duplicate virtual rules"
      )
      ( "no-sharing"
      , bool_switch(&nosharing)
      , "do not share virtual rules among xrs rules"
      )
      ( "left-right-backoff"
      , bool_switch(&left_right_backoff)
      , "generate left-right binarization if no itg-binarization exists"
      )
      ( "input,i"
      , defaulted_value(&input_file)
      , "input xrs rules"
      )
      ( "output,o"
      , defaulted_value(&output_file)
      , "output brf rules"
      )
      ( "non-itg-output,n"
      , defaulted_value(&non_itg_output_file)
      )
      ( "etree-string,e"
      , bool_switch(&etree_string)
      , "generate binarized etree string"
      )
      ( "preterminal-string,P"
      , bool_switch(&preterminal_string)
      , "generate a binarized string feature of the preterminals"
      )
      ( "binarize-length-features,l"
      , bool_switch(&ignoreme)
      , "treat text-length as brf (not xrs) feature"
      )
      ( "dependency-tree-string,d"
      , bool_switch(&dependency_tree_string)
      , "generate binarized dependency tree string"
      )
      ( "stupid-dependency-tree-string"
      , bool_switch(&use_stupid_dependency_lm_string)
      , "generate stupid binarized dependency tree string"
      )
      ( "pointer-features,p"
      , defaulted_value(&pointer_feature_names_str)
      , "comma separated list of features.  features are a list of values intended "
        "to match up with the rhs indices of corresponding rules, and they "
        "will be binarized accordingly"
      )
      ( "or-pointer-features"
      , defaulted_value(&or_pointer_feature_names_str)
      )
      ( "subgrammar-id"
      , defaulted_value(&subgrammar_id)
      )
      ( "filter-mode,f"
      , defaulted_value(&filteringMode)
      , "filtering mode (output lm-scoreable when binarized rules only)"
      )
      ( "exit-on-error"
      , bool_switch(&exitOnErrorMode)
      , "exit-the-program if a malformed rule is encountered "
        "(otherwise, report and continue)"
      )
      ( "stateful-etree-string,s"
        , bool_switch(&state_etree_string)
        ,  "implies --etree-string; generate binarized etree string based not on the root of the etree, but on the first child - lhs starts with state, like 'qNP_1(NP(x0:DT x1:NN)', but the state shouldn't appear in the etree-string"
          )
# ifdef FLATTEN
      ( "quasi-binarize,q"
      , defaulted_value(&flatten)
      , "unary rules are maximally lexicalized: (c1 c2 X1 c3 c4 X2; X1 e X2) "
        "will produce (c1 c2 X1 c3; X1) in one-step; the virtual  bin rules "
        "combining two nonterminals are not changed so far."
      )
# endif
      ;

  positional_options_description po;
  po.add("input",1);
  po.add("output",1);
  try {
      basic_command_line_parser<char> cmd(argc,argv);
      variables_map vm;
      store(cmd.options(all).positional(po).run(),vm);
      notify(vm);
      
      if (help) {
          log << "\n" << argv[0]<<"\n\n"
                    << usage_str << "\n"
                    << all << "\n";
          return 1;
      }
  } catch (std::exception &e) {
      log << "ERROR:"<<e.what() << "\nTry '" << argv[0] << " -h' for help\n\n";
      throw;
  }

  set<string> pointer_feature_names;
  set<string> or_pointer_feature_names;
  boost::split(pointer_feature_names,pointer_feature_names_str,boost::is_any_of(", \t"));
  boost::split(or_pointer_feature_names,or_pointer_feature_names_str,boost::is_any_of(", \t"));
  int iodx=0;
  for ( set<string>::iterator i = pointer_feature_names.begin()
      ; i != pointer_feature_names.end()
      ; ++i, ++iodx ) if (*i != ""){
          pointer_feature_name_map.insert(make_pair(*i,iodx));
          pointer_feature_id_map.push_back(*i);
  }
  iodx=0;
  for ( set<string>::iterator i = or_pointer_feature_names.begin()
      ; i != or_pointer_feature_names.end()
      ; ++i, ++iodx ) if (*i != ""){
          or_pointer_feature_name_map.insert(make_pair(*i,iodx));
          or_pointer_feature_id_map.push_back(*i);
  }
  pointer_features.resize(pointer_feature_name_map.size());
  or_pointer_features.resize(or_pointer_feature_name_map.size());

  istream &in=input_file.stream();
  o_stream=itg_out=&output_file.stream();
  non_itg_out=&non_itg_output_file.stream();
  if (state_etree_string) etree_string=true;
#endif


  // so that we can re-use utilities for the existing etree_string printing.
  if(dependency_tree_string){
      etree_string = true;
  }

  if(dependency_tree_string && use_stupid_dependency_lm_string){
      std::cerr<<"-d and -s cannot be set at the same time!\n";
      exit(1);
  }


  putsym(cw, emp);

  line.clear();
  totalRules=0;
  totalXrs=0;

  ruleID = 0;


  if (not filteringMode) ow << "BRF version 2\n";
  while (getline(in, line)) { // for each line in example rules file
    totalRules++;//so, actually we are counting # of lines

    if (line.find("$$$", 0) == 0 ||
	line == "") {// ignore comment and empty line
      /*ostringstream os;
	os << "Ignoring comment: " << str;
	dbg->info("Grammar", os.str(), __FILE__, __LINE__);*/
      continue;
    }

    //cerr << "Rule: " << line << endl;

    try {
      myRule = new ns_RuleReader::Rule(line); // this constructor throws a string exception
      sbmt::fat_syntax_rule frule(*myRule,sbmt::fat_tf); // this throws if a rule cant fit in a syntax rule
    } catch (std::exception &e) {
        cerr << "Caught exception: " << e.what() << endl;
        goto badrule;
    } catch (...) {
      goto badrule;
    }//end of exeption
    goto goodrule;

  badrule:
    cerr << "WARNING: bad rule: " << line << endl;
    badrules ++;
      if (filteringMode or not exitOnErrorMode) {
	// liang
	continue;
      }
      else {
	// binarization mode -- can't handle wrong rules, so just skip. should be filtered out in the filtering mode already.
	exit(-1);
	//continue;
      }

  goodrule:
    if (!myRule->existsAttribute("id")) {
        cerr << "this version of the binarizer requires a unique \"id\" field for each xrs rule." << endl;
        badrules ++;

        if (filteringMode) continue;
        else {
	  exit(-1);
	}
    }

    ruleID = atoi(myRule->getAttributeValue("id").c_str());

    if (used_ids.find(ruleID) != used_ids.end()) {
        cerr << "this version of the binarizer requires ids to be unique. duplicate tossed at line "<<totalRules << endl;
        badrules ++;

        continue;
    }

    for (size_t dx = 0; dx != pointer_features.size(); ++dx) {
        vector<string>(myRule->getRHSStates()->size(),"").swap(pointer_features[dx]);
    }
    for (size_t dx = 0; dx != or_pointer_features.size(); ++dx) {
        if (myRule->getAttributes()->find(or_pointer_feature_id_map[dx]) != myRule->getAttributes()->end())
            vector<string>(myRule->getRHSStates()->size(),"").swap(or_pointer_features[dx]);
        else vector<string>().swap(or_pointer_features[dx]);
    }

    for ( ns_RuleReader::Rule::attribute_map::iterator itr = myRule->getAttributes()->begin()
        ; itr != myRule->getAttributes()->end()
        ; itr++ ) {
        if (pointer_feature_names.find(itr->first) != pointer_feature_names.end()) {
            vector<string> pcons;
            string vec = boost::trim_copy(itr->second.value);
            if (vec != "") boost::split(pcons,vec,boost::is_any_of(" \t"));
            vector<string> states = *(myRule->getRHSStates());
            vector<string>::iterator si = states.begin(), se = states.end(), pi = pcons.begin();
            for (; si != se; ++si) {
                if (*si != "") {
                    if (pi == pcons.end()) {
                        stringstream msg;
                        msg << "rhs attribute " << itr->first <<"={{{" << vec << "}}} "
                            << " has too few variables for rule " << line;
                        throw runtime_error(msg.str());
                    }
                    *si = *pi;
                    ++pi;
                }
            } if (pi != pcons.end()) {
                stringstream msg;
                msg << "rhs attribute " << itr->first <<"={{{" << vec << "}}} "
                    << " has too many variables for rule " << line;
                throw runtime_error(msg.str());
            }
            pointer_features[pointer_feature_name_map[itr->first]].swap(states);
        }
        if (or_pointer_feature_names.find(itr->first) != or_pointer_feature_names.end()) {
            vector<string> pcons;
            string vec = boost::trim_copy(itr->second.value);
            if (vec != "") boost::split(pcons,vec,boost::is_any_of(" \t"));
            vector<string> states = *(myRule->getRHSStates());
            vector<string>::iterator si = states.begin(), se = states.end(), pi = pcons.begin();
            for (; si != se; ++si) {
                if (*si != "") {
                    if (pi == pcons.end()) {
                        stringstream msg;
                        msg << "rhs attribute " << itr->first <<"={{{" << vec << "}}} "
                            << " has too few variables for rule " << line;
                        throw runtime_error(msg.str());
                    }
                    *si = *pi;
                    ++pi;
                }
            } if (pi != pcons.end()) {
                stringstream msg;
                msg << "rhs attribute " << itr->first <<"={{{" << vec << "}}} "
                    << " has too many variables for rule " << line;
                throw runtime_error(msg.str());
            }
            or_pointer_features[or_pointer_feature_name_map[itr->first]].swap(states);
        }
    }

    used_ids.insert(ruleID);

    //myRule parsed

    curr = myRule->getLHSRoot(); // get the LHS root

    // Mark head.
    if(dependency_tree_string){
        string headmarker = myRule->getAttributeValue("headmarker");
        if(headmarker != ""){
            myRule->mark_head(headmarker);
        }
    }

    if(use_stupid_dependency_lm_string){
        ostringstream ost;
        string headmarker = myRule->getAttributeValue("headmarker");
        myRule->mark_head(headmarker);
        get_dependency_lm_string(ost, myRule->getLHSRoot(), myRule) ;
        stupid_dependency_lm_string = ost.str();
    }




    rhsSize = myRule->getRHSStates()->size(); // side of RHS of the rule
    RHS_symbols.clear();
    RHS_symbols.resize(rhsSize);

    traverse(curr);		// traverse the LHS root

    rhsStates=0;
    Alignment.clear();
    RHS_idx.clear();

    //#ifdef NOPACKING
    //    getprob();
    //#endif

    for (int counter=0; counter<rhsSize; counter++) { // for each item on the RHS of the rule
      if((myRule->getRHSLexicalItems())->at(counter) != ""){
	// if its not a RHS state
	//cerr << " LEX: " << (myRule->getRHSLexicalItems())->at(counter) << " "; // then its a lexical item

	//	RHS_symbols.push_back((myRule->getRHSLexicalItems())->at(counter));
	RHS_symbols[counter]=(myRule->getRHSLexicalItems())->at(counter);

      } else if((myRule->getRHSStates())->at(counter) != ""){
	// its a RHS state
	//cerr << " xRs: " << (myRule->getRHSStates())->at(counter) << " "; // print it out as a state
	const char* chbuf;

	chbuf=(myRule->getRHSStates())->at(counter).c_str();
	int lid=atoi(chbuf+1);

	//permutation from view of left and right
	Alignment.push_back(lid+1);
	L2Rpermu[lid]=rhsStates;

	rhsStates++;
	//	RHS_symbols.push_back((myRule->getRHSStates())->at(counter));
	//the variables are anchors
	RHS_idx.push_back(counter);

      }else{
	// it's a virtual constituent
	//cerr << " VC: " << (myRule->getRHSConstituents())->at(counter) << " "; // print it out as a constituent
      } // end if its a RHS state
    }// end for each item on the RHS of the rule


    if (rhsStates>=0){
      bracketing(); //do the itg binarization, and associated interpretation into bin rules
    }
    else{
    }


    // liang

#ifdef stat
    if (totalRules % 100 == 0) {
      cerr <<totalRules <<"\t"<< bin<<"\t"<<nonbin<<"\t"<<badrules<<endl;
    }
#endif

	if (keep_duplicates) {
		vrul_output(ow);
        output_left_right_backoffs_virt(ow);
        backoff_symbols.clear();
		clear_maps();
	}

	delete myRule;

  }//end for all rules

  //output all the virtual rules
  //  voutput(ow);
  if (!keep_duplicates) {
      vrul_output(ow);
      output_left_right_backoffs_virt(ow);
  }

#ifdef SANITY
  sanitycheck();
#endif

  //liang
  cerr << "finished"<<endl<<totalRules <<"\t"<< bin<<"\t"<<nonbin<<"\t"<<badrules<<endl;

}
