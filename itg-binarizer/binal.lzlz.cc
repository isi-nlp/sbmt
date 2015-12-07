/*
ITG BINARIZER
Hao Zhang and Liang Huang
August 2005  

what is it?

e.g.,

e3
x2         *
e2
x1               *
e1
x0   *
e0
  c0 x0 c1 x2 c2 x1

we binarize the synchrouns cfg rule,
garanteeing continuity on both sides
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <ext/hash_map>
#include <math.h>

using namespace std;

//the rule reader 
#include "Rule.h"
#include "RuleNode.h"

//sanity check
#define SANITY 1


//if just filtering, uncomment
//#define NOOUTPUT 1

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

//according to rule_file_line_number
//might have two identical bin rules from one xrs rule


#define TEMPSTR "/tmp/binaltemp"
ofstream ctmp(TEMPSTR);

//#define ov ctmp
#define ow cout

string ovstr;


vector <string> LHS_symbols, RHS_symbols;
vector <int> covered_mask;
vector <int> LHS_idx, RHS_idx;
vector <int> L2Rpermu;

//#ifdef NOPACKIING
//double curprb;
//#endif

int lhsptr;
int rhsptr;

int rhsStates;
int rhsSize;

int vRules;

string line;
int totalRules;
int totalXrs;

vector <int> stack;

int top;


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

template <class T>
short sgn(T x1)
{
    if (x1 > 0) return 1;
    else if x1 < 0 return -1;
    else return 0;
}

struct node_e_cmp {
    bool operator()(node const& n1, node const& n2) const
    {
        // no-overlap check
        assert(sgn(n1.left - n2.right) == sgn(n1.right - n2.left));
        return n1.left < n2.left;
    }
};

typedef node *nodeptr;

nodeptr prev_leaf;//, prev_binode;

vector <int> alignment;
vector <node> nodes;

string nonterminal[4] = {"", "A", "B", "C"};



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

  symbol_t(char t, int i):
    tg(t), id(i)
  {};
  symbol_t(){};
}symbol_t;

typedef struct vrule_t{
  symbol_t lhs;
  symbol_t kids[2];//yes, binary
  vector<symbol_t> lm_seq;//tells us how to synchronize the two kids' english translations 
  vector<int> xrs_seq;//tells us what xrs rules share this bin rule
  int inversion;
  vrule_t(){};
  //#ifdef NOPACKING
  //  double prb;
  //#endif
}vrule_t;


struct eqstr
{
  bool operator()(const char* s1, const char* s2) const
  {
    return strcmp(s1, s2) == 0;
  }
};


//hash_map<const char*, vector<int>, hash<const char*>, eqstr > vtbl;
//hash_map<const char*, hash_map<int, char>, hash<const char*>, eqstr > vtbl;
hash_map<int, vrule_t> vrul_tbl;

#ifdef SANITY
vector<vrule_t> topvrul_tbl;
#endif

hash_map<const char*, int, hash<const char*>, eqstr> vsym2id_tbl;
//hash_map<int, char *> vid2sym_tbl;
vector<char *> vid2sym_tbl;

hash_map<const char*, int, hash<const char*>, eqstr> rsym2id_tbl;
//hash_map<int, char *> rid2sym_tbl;
vector<char *> rid2sym_tbl;

hash_map<const char*, int, hash<const char*>, eqstr> esym2id_tbl;
//hash_map<int, char *> eid2sym_tbl;
vector<char *> eid2sym_tbl;

hash_map<const char*, int, hash<const char*>, eqstr> csym2id_tbl;
//hash_map<int, char *> cid2sym_tbl;
vector<char *> cid2sym_tbl;

string emp="";

//covering one nonterminal
#define ISLEAF(n) (n.left==n.right)

//covering one nonterminal and zero english word
#define ISVIRTUAL(n) (n.virt)

//old version
//void push(node &nd) {

  //  for (int i=0; i <top i++)
  //  cerr<<"   ";
  //  cerr <<"pushing span "<<nd.left<<"-"<<nd.right<<" "<<nonterminal[nd.tag]<<endl;
  //  nodes.push_back(nd);
  //  stack[top++]=nodes.size()-1; // index
  //  cerr<<"stack top ("<<top<<") = "<<stack[top-1]<<endl;
//}

//void push(int a) {
//  push(node(a, a, -1, -1, 3));
//}

#define stacking(lft, rht, aidx, bidx, t, v){\
nodes.push_back(node(lft, rht, aidx, bidx, t, v));\
stack[top++]=nodes.size()-1;\
}

#define straight(na, nb) (na.right==nb.left-1)
#define inverted(na, nb) (na.left==nb.right+1)

//try to combine two top nodes
inline int combine(){
  bool can=true;

  while (top>1 && can){
    int aidx=stack[top-2];
    int bidx=stack[top-1];

    node &na=nodes[stack[top-2]];
    node &nb=nodes[stack[top-1]];
  
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

//predicate, straight can-combine or inverted can-combine two cases
//old version. mem leak
/*
bool can_combine(int a, int b, nodeptr & newnode) {  
  node na = nodes[a], nb = nodes[b];
  //cerr<<"trying to combine "<<na.left<<"-"<<na.right<<" with "<<nb.left<<"-"<<nb.right<<endl;
  if (na.right == nb.left - 1) {
    // straight
    newnode = new node (na.left, nb.right, a, b, 1);
    newnode->virt=1;
    return true;
  }
  if (na.left == nb.right + 1) {
    // inverted
    newnode = new node (nb.left, na.right, a, b, 2);
    newnode->virt=1;
    return true;
  }  
  return false;
}
*/

//1 based index into the english symbols
#define EINDEX(node) (node.left)

#define ERINDEX(node) (node.right)

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

inline char *getsym(char thetype,
		    int theid){

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
  
  return (*theinvmap)[theid];
}

inline int putsym(char thetype,
		  string &thestring
		  ){

  hash_map<const char*, int, hash<const char*>, eqstr> *themap=NULL;
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
  hash_map<const char*, int, hash<const char*>, eqstr>::iterator sitr;

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
      thelmv.push_back(symbol_t(ew, putsym(ew, LHS_symbols[i])));
    }
    else{
      thelmv.push_back(symbol_t(ix, ord));
    }
  }
  else{
    thelmv.push_back(symbol_t(ix, ord));
  }
}

//print the lm string
void lmleaf(ostream &o, node &nd, int ord){
  if (!ISLEAF(nd)){
    o<<ord;
  }
  else
  for (int i=nd.e_left_idx; i<=nd.e_right_idx; i++){
    if (i!=LHS_idx[EINDEX(nd)-1]){//lexical
      o<<"\"";
      o<<LHS_symbols[i];
      o<<"\"";
      if (i<nd.e_right_idx)
	o<<" ";
    }
    else{
      o<<ord;
      if (i<nd.e_right_idx)
	o<<" ";
    }
  }
}

//print all the other attributes inherited from xrs
inline void otherattr(ostream &o){
  for (hash_map<string, string, ns_RuleReader::stringHash>::iterator it=myRule->getAttributes()->begin();
       it!=myRule->getAttributes()->end();
       it++) { // for each attribute value                                                                   
    o << (*it).first << "=" << (*it).second << " "; // print                                              
  } // end for each attribute
  return;
}

//out put the virtual rules in the hash table
// void voutput(ostream& o){
//   //  hash_map<const char*, hash_map<int, char>, hash<const char*>, eqstr > ::iterator vitr;
//   hash_map<const char*, vector<int>, hash<const char*>, eqstr > ::iterator vitr;

//   for (vitr=vtbl.begin(); vitr!=vtbl.end(); vitr++){
//     const char *vstring=(*vitr).first;
//     o<<vstring;

// #ifndef NOPACKING 
//     o<<" rule_file_line_number={{{";
    
//     //hash table version
//     //    int nbr=(*vitr).second.size();
//     //    for (hash_map<int, char>::iterator it=(*vitr).second.begin(); it!=(*vitr).second.end();++it){
//     //      o<<(*it).first;
//       //      if ((*it).second>1){
//       //	cerr<<vstring<<endl;
//       //	cerr<<"repetition: "<<(*it).first<<":"<<(int)((*it).second)<<endl;
//     //      }
//     //      if (--nbr>0)
//     //	o<<" ";
//     //    }

//     hash_map<int, int> exist;

//     int rfln=(*vitr).second.size();//must >=1
//     if (rfln>0){
//       o<<((*vitr).second)[0];
//       exist[(*vitr).second[0]]++;
//     }
//     for (int i=1; i<rfln; i++){
//       if (!exist[(*vitr).second[i]]){
// 	o<<" ";
// 	o<<((*vitr).second)[i];
//       }
//       else{
// 	//	cerr<<vstring<<endl;
// 	//	cerr<<"repetion: "<<(*vitr).second[i]<<":"<<(int)exist[(*vitr).second[i]]<<endl;
//       }
//       exist[(*vitr).second[i]]++;
//     }
//     o<<"}}}";
//     o<<endl;
// #endif
//   }

//   return;
// }


void vrul_output(ostream& o){
  hash_map<int, vrule_t>::iterator vritr;

  for (vritr=vrul_tbl.begin(); vritr!=vrul_tbl.end(); ++vritr){
    vrule_t &thevrule=(*vritr).second;
    char *lhsstr=getsym(vt, thevrule.lhs.id);

    o<<lhsstr;
    o<<RTRDELIM;
    o<<lhsstr;

    o<<"(";

    int totalvkids=0; //total variables
    int thekid; //iterator
    int ekid;//english order

    int variables[2]={-1, -1};

    for (thekid=0; thekid<2; thekid++){

      if (thekid==0)
	ekid=thevrule.inversion;
      else
	ekid=1-thevrule.inversion;


      if (thevrule.kids[ekid].tg!=cw){//rt or vt
	if (totalvkids==1)
	  o<<" ";
	o<<"x"<<totalvkids<<":"
	 <<getsym(thevrule.kids[ekid].tg, thevrule.kids[ekid].id);
	variables[ekid]=totalvkids;
	totalvkids++;
      }
      
    }

    if (!totalvkids){//pure lexical
      o<<"x0:NULL";
    }
    o<<")";


    o<<" -> ";

    
    for (thekid=0; thekid<2; thekid++){
      if (thekid==1)
	o<<" ";
      if (variables[thekid]>=0){
	o<<"x"<<variables[thekid];
      }
      else{//cw
	o<<"\"";
	o<<getsym(cw, thevrule.kids[thekid].id);
	o<<"\"";
      }
    }

    o<<" ### ";

    o<<"complete_subtree=no sblm=no ";
    int lmstrlen=thevrule.lm_seq.size();
    if (lmstrlen)
      o<<"lm=yes ";
    else
      o<<"lm=no ";

    o<<"lm_string={{{";
    for (int lmitr=0; lmitr<lmstrlen; lmitr++){
      symbol_t &thesymbol=thevrule.lm_seq[lmitr];

      //      cerr<<"lmseq "<<lmitr<<":"<<thesymbol.tg<<endl;

      if (thesymbol.tg==ix)
	o<<thesymbol.id;
      else{//english words;
	o<<"\"";
	o<<getsym(ew, thesymbol.id);
	o<<"\"";
      }

      if (lmitr<lmstrlen-1)
	o<<" ";
    }
    o<<"}}} ";

    o<<"sblm_string={{{}}} virtual_label=yes ";


    //rulefile line number
    o<<"rule_file_line_number={{{";
    
    hash_map<int, int> exist;

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


    o<<endl;
  }
}

inline void rrul_construct(char kid1type, string &kid1str, char kid2type, string &kid2str, node *kid1, node *kid2, int inversion){
  //  cerr<<kid1type<<":"<<kid1str<<","<<kid2type<<":"<<kid2str<<endl;

  int lftwidx, rhtwidx;

  lftwidx=rhtwidx=-1;

  ow<<curr->getString(false)
    <<RTRDELIM
    <<curr->treeString()
    <<" -> ";

#ifdef SANITY
  totalXrs++;
  topvrul_tbl.resize(totalXrs);
  vrule_t &thevrule=topvrul_tbl[totalXrs-1];

  thevrule.kids[0].tg=kid1type;
  thevrule.kids[1].tg=kid2type;
#endif

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
    ow<<kid1str.substr(0, lindex); //x?

#ifdef SANITY
    string realt=kid1str.substr(lindex+1, kid1str.size());
    thevrule.kids[0].id=putsym(rt, realt);
#endif
  }
  else if (kid1type==vt){
    ow<<kid1str;

#ifdef SANITY
    thevrule.kids[0].id=putsym(vt, kid1str);
#endif
  }


  if (strcmp(kid2str.c_str(), "")!=0){//unary cases
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
      ow<<kid2str.substr(0, lindex); //x?

#ifdef SANITY
      string realt=kid2str.substr(lindex+1, kid2str.size());
      thevrule.kids[1].id=putsym(rt, realt);
#endif
    }
    else if (kid2type==vt){
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

  ow<<" ### ";
  //all xrs attributes inherited 
#ifndef NOOTHERS
  otherattr(ow);
#endif
  
  ow<<"virtual_label=no complete_subtree=yes ";
  
  ow<<"lm_string={{{";

  //concatenate the english words
  if (kid1){
    if (kid1==kid2){
      lmleaf(ow, (*kid1), 0);

#ifdef SANITY
      lmvector(thevrule.lm_seq, kid1, 0);
#endif
    }
    else{
      if (inversion){
	lftwidx=LHS_idx[ERINDEX((*kid2))-1]+1;
	rhtwidx=LHS_idx[EINDEX((*kid1))-1]-1;
      }
      else{
	lftwidx=LHS_idx[ERINDEX((*kid1))-1]+1;
	rhtwidx=LHS_idx[EINDEX((*kid2))-1]-1;
      }

      if (inversion){
	int i;

	for (i=0; i<LHS_idx[EINDEX((*kid2))-1]; i++){
	  ow<<"\"";
	  ow<<LHS_symbols[i];
	  ow<<"\"";
	  ow<<" ";
	}
	//1
	ow<<1;
	ow<<" ";
	for (i=lftwidx; i<=rhtwidx; i++){
	  ow<<"\"";
	  ow<<LHS_symbols[i];
	  ow<<"\"";
	  ow<<" ";
	}
	//0
	ow<<0;

	for (i=LHS_idx[ERINDEX((*kid1))-1]+1; i<lhsptr; i++){
	  ow<<" ";
	  ow<<"\"";
	  ow<<LHS_symbols[i];
	  ow<<"\"";
	}
	
// 	lmleaf(ow, (*kid2), 1);
// 	ow<<" ";
// 	lmleaf(ow, (*kid1), 0);

#ifdef SANITY
	for (i=0; i<LHS_idx[EINDEX((*kid2))-1]; i++){
	  int eid=putsym(ew, LHS_symbols[i]);
	  thevrule.lm_seq.push_back(symbol_t(ew, eid));
	}
	//1
	thevrule.lm_seq.push_back(symbol_t(ix, 1));
	for (i=lftwidx; i<=rhtwidx; i++){
	  int eid=putsym(ew, LHS_symbols[i]);
	  thevrule.lm_seq.push_back(symbol_t(ew, eid));
	}
	//0
	thevrule.lm_seq.push_back(symbol_t(ix, 0));
	for (i=LHS_idx[ERINDEX((*kid1))-1]+1; i<lhsptr; i++){
	  int eid=putsym(ew, LHS_symbols[i]);
	  thevrule.lm_seq.push_back(symbol_t(ew, eid));
	}
// 	lmvector(thevrule.lm_seq, kid2, 1);
// 	lmvector(thevrule.lm_seq, kid1, 0);
#endif
      }
      else{
	int i;

	for (i=0; i<LHS_idx[EINDEX((*kid1))-1]; i++){
	  ow<<"\"";
	  ow<<LHS_symbols[i];
	  ow<<"\"";
	  ow<<" ";
	}
	//0
	ow<<0;
	ow<<" ";
	for (i=lftwidx; i<=rhtwidx; i++){
	  ow<<"\"";
	  ow<<LHS_symbols[i];
	  ow<<"\"";
	  ow<<" ";
	}
	//1
	ow<<1;
	for (i=LHS_idx[ERINDEX((*kid2))-1]+1; i<lhsptr; i++){
	  ow<<" ";
	  ow<<"\"";
	  ow<<LHS_symbols[i];
	  ow<<"\"";
	}
// 	lmleaf(ow, (*kid1), 0);
// 	ow<<" ";
// 	lmleaf(ow, (*kid2), 1);

#ifdef SANITY
	for (i=0; i<LHS_idx[EINDEX((*kid1))-1]; i++){
	  int eid=putsym(ew, LHS_symbols[i]);
	  thevrule.lm_seq.push_back(symbol_t(ew, eid));
	}
	//0
	thevrule.lm_seq.push_back(symbol_t(ix, 0));
	for (i=lftwidx; i<=rhtwidx; i++){
	  int eid=putsym(ew, LHS_symbols[i]);
	  thevrule.lm_seq.push_back(symbol_t(ew, eid));
	}
	//1
	thevrule.lm_seq.push_back(symbol_t(ix, 1));
	for (i=LHS_idx[ERINDEX((*kid2))-1]+1; i<lhsptr; i++){
	  int eid=putsym(ew, LHS_symbols[i]);
	  thevrule.lm_seq.push_back(symbol_t(ew, eid));
	}
// 	lmvector(thevrule.lm_seq, kid1, 0);
// 	lmvector(thevrule.lm_seq, kid2, 1);
#endif
      }
    }
  }
  else{//lexical

    if (kid1type==cw){//real lexical
      for (int i=0; i<lhsptr; i++){
	ow<<"\"";
	ow<<LHS_symbols[i];
	ow<<"\"";
	if (i<lhsptr-1)
	  ow<<" ";
#ifdef SANITY
	int eid=putsym(ew, LHS_symbols[i]);
	thevrule.lm_seq.push_back(symbol_t(ew, eid));
#endif
      }
    }
    else{
      //      lmleaf(ow, (*kid1), 0);
      ow<<0;
#ifdef SANITY
      thevrule.lm_seq.push_back(symbol_t(ix, 0));
#endif

    }

  }

  ow<<"}}} ";

  ow<<"sblm_string={{{";
  for (int i=0; i<rhsStates; i++){
    ow<<L2Rpermu[i]; 
    ow<<" ";
  }
  ow<<"}}} ";

  ow<<"lm=yes "<<"sblm=yes";

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
  ow<<endl;
}

inline string vrul_construct(char kid1type, string &kid1str, char kid2type, string &kid2str, node *kid1, node *kid2, int inversion, vrule_t *thenewrule, bool lmable, bool firstlexical){
  //construct a virtual nonterminal
  string vlhs;
  int lftwidx=-1;
  int rhtwidx=-1;

  if (kid1==kid2)
    lftwidx=rhtwidx=-1;//no gap

  if (!kid1){//leaf, concatenating a rt or vt or cw with a cw
    vlhs="V";
    vlhs+="[";

    if (kid1type==cw){
      vlhs+="\"";
      vlhs+=conv(kid1str);
      vlhs+="\"";
    }
    else{
      vlhs+=kid1str;
    }

    vlhs+=WORDELIM;

    if (kid2type==cw){
      vlhs+="\"";
      vlhs+=conv(kid2str);
      vlhs+="\"";
    }
    else{
      vlhs+=kid2str;
    }

    if (firstlexical){
      vlhs+=ECDELIM;
      for (int i=0; i<lhsptr; i++){
	vlhs+="\"";
	vlhs+=conv(LHS_symbols[i]);
	vlhs+="\"";
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

//     if (ISLEAF((*kid1))){
//       for (int itr=kid1->e_left_idx; itr<=kid1->e_right_idx; itr++){
// 	if (itr!=LHS_idx[EINDEX((*kid1))-1]){//english word
// 	  vlhs+="\"";
// 	  vlhs+=conv(LHS_symbols[itr]);
// 	  vlhs+="\"";
// 	}
// 	else{
// 	  vlhs+=kid1->label;
// 	}
// 	vlhs+=WORDELIM;
//       }
//     }
//     else{
//       vlhs+=kid1->label;
//       vlhs+=WORDELIM;
//     }
    
//    if (inversion){
//      vlhs+=kid2->label;
//    }
//    else{
      vlhs+=kid1->label;
      //    }

    //two parts
    vlhs+=BINDELIM;
    
    //middle words
    if (inversion){
      lftwidx=LHS_idx[ERINDEX((*kid2))-1]+1;
      rhtwidx=LHS_idx[EINDEX((*kid1))-1]-1;
    }
    else{
      lftwidx=LHS_idx[ERINDEX((*kid1))-1]+1;
      rhtwidx=LHS_idx[EINDEX((*kid2))-1]-1;
    }

    for (int itr=lftwidx; itr<=rhtwidx; itr++){
      vlhs+="\"";
      vlhs+=conv(LHS_symbols[itr]);
      vlhs+="\"";
      vlhs+=WORDELIM;
    }

    //two parts
    vlhs+=BINDELIM;

    //2nd part
    //    if (inversion){
    //      vlhs+=kid1->label;
    //    }
    //    else{
      vlhs+=kid2->label;
      //    }



//     if (ISLEAF((*kid2))){
//       for (int itr=kid2->e_left_idx; itr<=kid2->e_right_idx; itr++){
// 	if (itr!=LHS_idx[EINDEX((*kid2))-1]){//english word                                              
// 	  vlhs+="\"";
// 	  vlhs+=conv(LHS_symbols[itr]);
// 	  vlhs+="\"";
// 	}
// 	else{
// 	  vlhs+=kid2->label;
// 	}
// 	vlhs+=WORDELIM;
//       }
//     }
//     else{
//       vlhs+=kid2->label;
//       vlhs+=WORDELIM;
//     }

    if (!inversion){
      vlhs+="]";
    }
    else{
      vlhs+=">";
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

  if (!newvrule.xrs_seq.size()>0){//nonexisting
    //the fields
    newvrule.inversion=inversion;
    newvrule.lhs.tg=vt;
    newvrule.lhs.id=newrid;
    newvrule.kids[0].tg=kid1type;
    newvrule.kids[0].id=putsym(kid1type, kid1str);
    newvrule.kids[1].tg=kid2type;
    newvrule.kids[1].id=putsym(kid2type, kid2str);

    if (lmable && kid1){//having lm 
      //      newvrule.lm_seq.clear();
      if (!inversion){
	newvrule.lm_seq.push_back(symbol_t(ix, 0));
	for (int i=lftwidx; i<=rhtwidx; i++){
	  int eid=putsym(ew, LHS_symbols[i]);
	  newvrule.lm_seq.push_back(symbol_t(ew, eid));
	}
	newvrule.lm_seq.push_back(symbol_t(ix, 1));
	//	lmvector(newvrule.lm_seq, kid1, 0);
	//	lmvector(newvrule.lm_seq, kid2, 1);
      }
      else{
	newvrule.lm_seq.push_back(symbol_t(ix, 1));
	for (int i=lftwidx; i<=rhtwidx; i++){
	  int eid=putsym(ew, LHS_symbols[i]);
	  newvrule.lm_seq.push_back(symbol_t(ew, eid));
	}
	newvrule.lm_seq.push_back(symbol_t(ix, 0));
	//	lmvector(newvrule.lm_seq, kid2, 1);
	//	lmvector(newvrule.lm_seq, kid1, 0);
      }

    }
    else if (lmable){
      if (firstlexical){//do it early
	for (int i=0; i<lhsptr; i++){
	  int eid=putsym(ew, LHS_symbols[i]);
	  newvrule.lm_seq.push_back(symbol_t(ew, eid));
	}
      }
      else
      //      newvrul.lm_seq.clear();
      newvrule.lm_seq.push_back(symbol_t(ix, 0));//placeholdr
    }
  }

  newvrule.xrs_seq.push_back(totalRules);
  thenewrule=&newvrule;
  
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
  

  for (int lmi=0; lmi<(int)(thevrule->lm_seq.size()); lmi++){
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
    if (lmi<(int)(thevrule->lm_seq.size()-1))
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
//inline vrul_put(vrule_t &therule){

//}



//safety replacements

//binarize pure lexical rules
//pure lexical means no nonterminals on rhs
inline int lexbin(){
  string label;
  string lastlabel;
  //    label.clear();

  //label=conv(RHS_symbols[0]);
  label=RHS_symbols[0];
  lastlabel=label;

  //    cerr<<lastlabel<<endl;
  //  if (rhsSize>2){//more than 2 chinese words on rhs 
  for (int i=1; i<rhsSize-1; i++){

    vrule_t *therul=NULL;
    lastlabel=vrul_construct(
			     ((i==1)?cw:vt), lastlabel, 
			     cw, RHS_symbols[i], 
			     NULL, 
			     NULL,
			     0,
			     therul,
			     true,
			     (i==1)
			     );
    //    lastlabel=VLHS(therul);
    vRules++;

    //V["c1"_"c2"] or V[V[...]_"ci"]
//     label="V";
//     label+="[";
//     if (i==1)
//       label+="\"";
//     label+=lastlabel;
//     if (i==1)
//       label+="\"";
//     label+=WORDELIM;
//     label+="\"";
//     label+=conv(RHS_symbols[i]);
//     label+="\"";
//     label+="]";
        
    //    ovstr.clear();
    //    ostringstream ov(ovstr);

    //V[...]:VL: V[...](x0:NULL) -> "c1" "c2" | V[...] "ci"

//     ov<<label
//       <<RTRDELIM
//       <<label
//       <<"("
//       <<"x0:NULL"
//       <<")"	  
//      <<" -> ";


    //rhs
//     if (i==1)
//       ov<<"\"";
//     ov<<lastlabel;
//     if (i==1)
//       ov<<"\"";

//     ov<<" "
//       <<"\""
//       <<RHS_symbols[i]
//       <<"\"";

    //attributes
//     ov<<" ### "
//       <<"complete_subtree=no sblm=no lm=no lm_string={{{}}} sblm_string={{{}}} virtual_label=yes";


//#ifdef NOPACKING
//     ov<<" bin-prob="
//       <<1;
//     ov<<" original_rule_prob="
//       <<curprb;
//     ov<<" rule_file_line_number={{{"
//       <<totalRules
//       <<"}}}";
//     ov<<endl;
//#endif

    //put onto the hash table for virtual rules
    //    putv(ov.str(), totalRules);

    //    vrul_put(therul);
    //    lastlabel=label;

  }//end of expansion to the rightmost

  //    }//end if more than two chinese words on rhs
    
  //top-level bin rule or uni rule
  //Tree -> "c1"  | "c1" "c2" | V[...] "cn"


  rrul_construct(
		 (rhsSize<=2)?cw:vt, lastlabel,
		 cw, (rhsSize>1)?RHS_symbols[rhsSize-1]:emp,
		 NULL,
		 NULL,
		 0);

//   ow<<curr->getString(false)
//     <<RTRDELIM
//     <<curr->treeString()
//    <<" -> ";
    
//   if (rhsSize<=2) //two chinese words
//     ow<<"\"";
//   ow<<lastlabel;
//   if (rhsSize<=2)
//    ow<<"\"";

//  ow<<" ";

//   if (rhsSize>1){//more than one children
//     ow<<"\"";
//     ow<<RHS_symbols[rhsSize-1];
//     ow<<"\"";
//   }

//  ow<<" ### ";

  //all xrs attributes inherited 
  //#ifndef NOOTHERS
  //  otherattr(ow);
  //#endif
  
  //  ow<<"virtual_label=no complete_subtree=yes ";
  
//   ow<<"lm_string={{{";

//   //concatenate the english words
//   //  int LHSs=LHS_symbols.size();
//   for (int i=0; i<lhsptr; i++){
//     ow<<"\"";
//     ow<<LHS_symbols[i];
//     ow<<"\"";
//     if (i<lhsptr-1)
//       ow<<" ";
//   }

//   ow<<"}}} ";

//   ow<<"sblm_string={{{";
//   ow<<"}}} ";
//   ow<<"lm=yes "<<"sblm=yes";

// #ifdef NOPACKING
//   ow<<" bin-prob="
//     <<curprb;
//   ow<<" original_rule_prob="
//     <<curprb;
// #endif

//   ow<<" rule_file_line_number=";
//   //    ow<<"{{{";
//   ow<<totalRules;
//   //    ow<<"}}}";
//   ow<<endl;

  return vRules; //end of pure lexical rule
}



//binarize unary nonterminal rules
// ... -> ... x0 ...
//associating chinese words
inline int unibin(node &nd){
  //associate the bordering chinese words
  int lft_idx, lft, rht_idx, rht;
  string label;
  string lastlabel;

  int remain;

  remain=rhsSize-2; //remaining virtual rules needed, when nd is an intermediate node, we will exoughst remain

  label.clear();
  lft=(prev_leaf)?(prev_leaf->c_right_idx+1):0; //leaves are visited from left to right on chinese
    
  int lindex=LHS_symbols[LHS_idx[EINDEX(nd)-1]].find_first_of(":", 0);
  //cerr<<INDEX(nd)<<" "<<RHS_symbols[RHS_idx[INDEX(nd)-1]]<<" lindex:"<<lindex<<endl;
  label=LHS_symbols[LHS_idx[EINDEX(nd)-1]].substr(lindex+1, LHS_symbols[LHS_idx[EINDEX(nd)-1]].size());
    
  lastlabel=label;


  int isVirtual=0;
  for (lft_idx=RHS_idx[rhsptr]-1; remain>0 && lft_idx>=lft; lft_idx--){
    //iterate through the left neighboring chinese words
      
    vrule_t *therul=NULL;
    lastlabel=vrul_construct(
			     cw, RHS_symbols[lft_idx],
			     (isVirtual?vt:rt), lastlabel,
			     NULL, 
			     NULL,
			     0,
			     therul,
			     true,
			     false
			     );
    //    lastlabel=VLHS(therul);
    vRules++;
    remain--;
    isVirtual=1;

    //V["c"_V...]
//     label="V";
//     label+="[";
//     label+="\"";
//     label+=conv(RHS_symbols[lft_idx]);
//     label+="\"";
//     label+=WORDELIM;
//     label+=lastlabel;
//     label+="]";

//     ovstr.clear();
//     ostringstream ov(ovstr);

    //x0 is refering to the only nonterminal child 
//     ov<<label
//       <<RTRDELIM
//       <<label
//       <<"(x0:"
//       <<lastlabel
//      <<")";


//     ov<<" -> ";
//     ov<<"\"";
//     ov<<RHS_symbols[lft_idx];
//     ov<<"\"";

//     ov<<" ";
//     ov<<"x0";

    //we don't comput lm until bin rules or top uni rule
//     ov<<" ### ";
//     ov<<"complete_subtree=no sblm=no lm=no lm_string={{{}}} sblm_string={{{}}} virtual_label=yes";

//#ifdef NOPACKING
//     ov<<" bin-prob="
//       <<1;
//     ov<<" original_rule_prob="
//       <<curprb;

//     ov<<" rule_file_line_number={{{";
//     ov<<totalRules;
//     ov<<"}}}";
//     ov<<endl;
//#endif

    //    putv(ov.str(), totalRules);

    //    lastlabel=label;
  }//end of associating the left words


  nd.c_left_idx=lft; //remembering the boundary

  rht=(rhsptr<rhsStates-1)?(RHS_idx[rhsptr+1]-1):rhsSize-1;

  for (rht_idx=RHS_idx[rhsptr]+1; remain>0 && rht_idx<=rht; rht_idx++){
      //iterate through the right neighboring chinese words

    vrule_t *therul=NULL;
    lastlabel=vrul_construct(
			     (isVirtual?vt:rt), lastlabel, 
			     cw, RHS_symbols[rht_idx],
			     NULL, 
			     NULL,
			     0,
			     therul,
			     true,
			     false
			     );
    //    lastlabel=VLHS(therul);
    vRules++;
    remain--;
    isVirtual=1;

    //V[V..._"c"]
//     label="V";
//     label+="[";
//     label+=lastlabel;
//     label+=WORDELIM;
//     label+="\"";
//     label+=conv(RHS_symbols[rht_idx]);
//     label+="\"";
//     label+="]";

// //     ovstr.clear();
// //     ostringstream ov(ovstr);

//     ov<<label
//       <<RTRDELIM
//       <<label
//       <<"(x0:"
//       <<lastlabel
//       <<")";


//     ov<<" -> "
//       <<"x0"
//       <<" "
//       <<"\""
//       <<RHS_symbols[rht_idx]
//       <<"\"";

//     ov<<" ### ";
//     ov<<"complete_subtree=no sblm=no lm=no lm_string={{{}}} sblm_string={{{}}} virtual_label=yes";
    
//#ifdef NOPACKING
//     ov<<" bin-prob="
//       <<1;
//     ov<<" original_rule_prob="
//       <<curprb;

//     ov<<" rule_file_line_number={{{";
//     ov<<totalRules;
//     ov<<"}}}";
//     ov<<endl;
//#endif

    //    putv(ov.str(), totalRules);
      
    //    lastlabel=label;
  } //end of associating right words
  
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

    if (lft_idx==lft){
      rrul_construct(
		     cw, RHS_symbols[lft_idx],
		     (isVirtual)?vt:rt, (isVirtual)?lastlabel:LHS_symbols[LHS_idx[EINDEX(nd)-1]],
		     &nd,
		     &nd,
		     0);
    }
    else if (rht_idx==rht){
      rrul_construct(
		     (isVirtual)?vt:rt, (isVirtual)?lastlabel:LHS_symbols[LHS_idx[EINDEX(nd)-1]],
		     cw, RHS_symbols[rht_idx],
		     &nd,
		     &nd,
		     0);
    }
    else{
      rrul_construct(
		     rt, LHS_symbols[LHS_idx[EINDEX(nd)-1]],
		     cw, emp,
		     &nd,
		     &nd,
		     0);
    }


//     ow<<curr->getString(false)
//       <<RTRDELIM
//       <<curr->treeString()
//       <<" -> ";


    //virtual or x0
    
//     int lindex=LHS_symbols[LHS_idx[EINDEX(nd)-1]].find_first_of(":", 0);
      
//     //      if (isVirtual)
//     //	ow<<lastlabel;
//     //      else
//     //	ow<<RHS_symbols[RHS_idx[INDEX(nd)-1]].substr(0, lindex); //should be x0 

//     if (lft_idx==lft){
//       ow<<"\"";
//       ow<<RHS_symbols[lft_idx];
//       ow<<"\"";
//     }

//     ow<<" ";
    
//     if (isVirtual)
//       ow<<lastlabel;
//     else
//       ow<<LHS_symbols[LHS_idx[EINDEX(nd)-1]].substr(0, lindex); //should be x0

//     //    if (strcmp(RHS_symbols[RHS_idx[INDEX(nd)-1]].substr(0, lindex).c_str(), "x0")!=0){
//     //      cerr<<"WRONG VARIABLE INDEX"<<endl;
//     //    }

//     ow<<" ";

//     if (rht_idx==rht){
//       ow<<"\"";
//       ow<<RHS_symbols[rht_idx];
//       ow<<"\"";
//     }

//     ow<<" ";

//    ow<<" ### ";

// #ifndef NOOTHERS
//     otherattr(ow);
// #endif

//     ow<<"virtual_label=no complete_subtree=yes ";
//     ow<<"lm_string={{{";
//     //      lmleaf(ow, nd, 0);

//     //concatenate the english words
//     //    int LHSs=LHS_symbols.size();
//     for (int i=0; i<lhsptr; i++){
//       if (i!=LHS_idx[EINDEX(nd)-1]){//lexical
// 	ow<<"\"";
// 	ow<<LHS_symbols[i];
// 	ow<<"\"";
// 	if (i<lhsptr-1)
// 	  ow<<" ";
//       }
//       else{	  
// 	ow<<0;
// 	if (i<lhsptr-1)
// 	  ow<<" ";
//       }
//     }

//    ow<<"}}} ";
//     ow<<"sblm_string={{{";
      
//     for (int i=0; i<rhsStates; i++){
//       ow<<L2Rpermu[i]; //should be 0 just
//       ow<<" ";
//     }

//     ow<<"}}} ";
//     ow<<"lm=yes sblm=yes";

// #ifdef NOPACKING
//     ow<<" bin-prob="
//       <<curprb;
//     ow<<" original_rule_prob="
//       <<curprb;
// #endif
    
//     ow<<" rule_file_line_number=";
//     //      ow<<"{{{";
//     ow<<totalRules;
//     //      ow<<"}}}";
//     ow<<endl;
    
  }//end of unary variable rule

  return vRules;
}

//binarize two nonterminal rules
// ... -> x0 x1 | x1 x0
//associating english words
inline int binbin(node &nd, int index){
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
//     nd.label="V";
//     nd.label+="[";
//     if (ISLEAF(leftchild)){
//       for (int itr=leftchild.e_left_idx; itr<=leftchild.e_right_idx; itr++){
// 	if (itr!=LHS_idx[EINDEX(leftchild)-1]){//english word
// 	  nd.label+="\"";
// 	  nd.label+=conv(LHS_symbols[itr]);
// 	  nd.label+="\"";
// 	}
// 	else{
// 	  nd.label+=leftchild.label;
// 	}
// 	nd.label+=WORDELIM;
//       }
//     }
//     else{
//       nd.label+=leftchild.label;
//       nd.label+=WORDELIM;
//     }

//     nd.label+=BINDELIM;

//     if (ISLEAF(rightchild)){
//       for (int itr=rightchild.e_left_idx; itr<=rightchild.e_right_idx; itr++){
// 	if (itr!=LHS_idx[EINDEX(rightchild)-1]){//english word                                              
// 	  nd.label+="\"";
// 	  nd.label+=conv(LHS_symbols[itr]);
// 	  nd.label+="\"";
// 	}
// 	else{
// 	  nd.label+=rightchild.label;
// 	}
// 	nd.label+=WORDELIM;
//       }
//     }
//     else{
//       nd.label+=rightchild.label;
//       nd.label+=WORDELIM;
//     }

//     nd.label+="]";


    if (index==stack[0]){//root

      rrul_construct(
		     (!ISVIRTUAL(leftchild))?rt:vt, (ISVIRTUAL(leftchild))?leftchild.label:LHS_symbols[LHS_idx[EINDEX(leftchild)-1]],
		     (!ISVIRTUAL(rightchild))?rt:vt, (ISVIRTUAL(rightchild))?rightchild.label:LHS_symbols[LHS_idx[EINDEX(rightchild)-1]],
		     &leftchild,
		     &rightchild,		     
		     0);

//       ow<<curr->getString(false)
// 	<<RTRDELIM
// 	<<curr->treeString()
// 	<<" -> ";

// 	//virtual or x
// 	//virtual symbol or x allowed
// 	int lindex=LHS_symbols[LHS_idx[EINDEX(leftchild)-1]].find_first_of(":", 0);
// 	//	if (!ISVIRTUAL(leftchild) && lindex==string::npos)
// 	//	  cerr<<"Nonexistent"<<endl;
// 	if (!ISVIRTUAL(leftchild) && lindex!=string::npos)
// 	  ow<<LHS_symbols[LHS_idx[EINDEX(leftchild)-1]].substr(0, lindex);
// 	else
// 	  ow<<leftchild.label;

// 	ow<<" ";

// 	//virtual or x
// 	lindex=LHS_symbols[LHS_idx[EINDEX(rightchild)-1]].find_first_of(":", 0);
// 	//	if (!ISVIRTUAL(rightchild) && lindex==string::npos)
// 	//	  cerr<<"Nonexistent"<<endl;

//         if (!ISVIRTUAL(rightchild) && lindex!=string::npos)
//           ow<<LHS_symbols[LHS_idx[EINDEX(rightchild)-1]].substr(0, lindex);
//         else
//           ow<<rightchild.label;


//        ow<<" ### ";
	
// #ifndef NOOTHERS
// 	otherattr(ow);
// #endif
//        	ow<<"virtual_label=no complete_subtree=yes ";

// 	ow<<"lm_string={{{";
// 	lmleaf(ow, leftchild, 0);
// 	ow<<" ";
// 	lmleaf(ow, rightchild, 1);
// 	ow<<"}}} ";
// 	ow<<"sblm_string={{{";

// 	for (int i=0; i<rhsStates; i++){
// 	  ow<<L2Rpermu[i];
// 	  ow<<" ";
// 	}
// 	ow<<"}}} ";
// 	ow<<"lm=yes sblm=yes";

// #ifdef NOPACKING
// 	ow<<" bin-prob="
// 	  <<curprb;
// 	ow<<" original_rule_prob="
// 	  <<curprb;
// #endif

// 	ow<<" rule_file_line_number=";
// 	//	ow<<"{{{";
// 	ow<<totalRules;
// 	//	ow<<"}}}";
// 	ow<<endl;
      }//end of straight root rule
      else{
	//virtual bin straight rule
	
	vrule_t *therul=NULL;
	nd.label=vrul_construct(
				(leftchild.virt?vt:rt), leftchild.label,
				(rightchild.virt?vt:rt), rightchild.label,
				&leftchild,
				&rightchild,
				0,
				therul,
				true,
				false
				);
	//	nd.label=VLHS(therul);	
	vRules++;

//         ovstr.clear();
//         ostringstream ov(ovstr);

// 	ov<<nd.label
// 	  <<RTRDELIM
// 	  <<nd.label
// 	  <<"("
// 	  <<"x0:"
// 	  <<leftchild.label
// 	  <<" "
// 	  <<"x1:"
// 	  <<rightchild.label
// 	  <<")";

// 	ov<<" -> ";
	
// 	ov<<"x0";
// 	ov<<" ";
// 	ov<<"x1";


//         ov<<" ### "
//	  <<"complete_subtree=no sblm=no lm=yes "
//	  <<"lm_string={{{";
//         lmleaf(ov, leftchild, 0);
// 	ov<<" ";
//         lmleaf(ov, rightchild, 1);
//         ov<<"}}} ";
// 	ov<<"sblm_string={{{}}} virtual_label=yes";

	
//#ifdef NOPACKING
//         ov<<" bin-prob="
//           <<1;
//         ov<<" original_rule_prob="
//           <<curprb;

// 	ov<<" rule_file_line_number={{{";
// 	ov<<totalRules;
// 	ov<<"}}}";
// 	ov<<endl;
//#endif

	//	putv(ov.str(), totalRules);

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

//       nd.label="V";
//       nd.label+="<";
//       if (ISLEAF(leftchild)){
//         for (int itr=leftchild.e_left_idx; itr<=leftchild.e_right_idx; itr++){
//           if (itr!=LHS_idx[EINDEX(leftchild)-1]){//english word                                              
// 	    nd.label+="\"";
//             nd.label+=conv(LHS_symbols[itr]);
// 	    nd.label+="\"";
//           }
//           else{
//             nd.label+=leftchild.label;
//           }
//           nd.label+=WORDELIM;
//         }
//       }
//       else{
//         nd.label+=leftchild.label;
//         nd.label+=WORDELIM;
//       }

//       nd.label+=BINDELIM;

//       if (ISLEAF(rightchild)){
//         for (int itr=rightchild.e_left_idx; itr<=rightchild.e_right_idx; itr++){
//           if (itr!=LHS_idx[EINDEX(rightchild)-1]){//english word  
// 	    nd.label+="\"";
//             nd.label+=conv(LHS_symbols[itr]);
// 	    nd.label+="\"";
//           }
//           else{
//             nd.label+=rightchild.label;
//           }
//           nd.label+=WORDELIM;
//         }
//       }
//       else{
//         nd.label+=rightchild.label;
//         nd.label+=WORDELIM;
//       }

//       nd.label+=">";

      if (index==stack[0]){//root binary rule                                                              

	rrul_construct(
		       (!ISVIRTUAL(leftchild))?rt:vt, (ISVIRTUAL(leftchild))?leftchild.label:LHS_symbols[LHS_idx[EINDEX(leftchild)-1]],
		       (!ISVIRTUAL(rightchild))?rt:vt, (ISVIRTUAL(rightchild))?rightchild.label:LHS_symbols[LHS_idx[EINDEX(rightchild)-1]],
		       &leftchild,
		       &rightchild,		     
		       1);

// 	ow<<curr->getString(false);
//         ow<<RTRDELIM;

//         ow<<curr->treeString();
//         ow<<" -> ";

	//virtual or x
//         int lindex=LHS_symbols[LHS_idx[EINDEX(leftchild)-1]].find_first_of(":", 0);
// 	//	if (!ISVIRTUAL(leftchild) && lindex==string::npos)
// 	//	  cerr<<"Nonexistent"<<endl;
//         if (!ISVIRTUAL(leftchild) && lindex!=string::npos)
//           ow<<LHS_symbols[LHS_idx[EINDEX(leftchild)-1]].substr(0, lindex);
//         else
//           ow<<leftchild.label;

//         ow<<" ";

// 	//virtual or x
//         lindex=LHS_symbols[LHS_idx[EINDEX(rightchild)-1]].find_first_of(":", 0);
// 	//	if (!ISVIRTUAL(rightchild) && lindex==string::npos)
// 	//	  cerr<<"Nonexistent"<<endl;
//         if (!ISVIRTUAL(rightchild) && lindex!=string::npos)
//           ow<<LHS_symbols[LHS_idx[EINDEX(rightchild)-1]].substr(0, lindex);
//         else
//           ow<<rightchild.label;


//        ow<<" ### ";
// #ifndef NOOTHERS
// 	otherattr(ow);
// #endif
//         ow<<"virtual_label=no complete_subtree=yes ";

//         ow<<"lm_string={{{";
//         lmleaf(ow, rightchild, 1);
// 	ow<<" ";
//         lmleaf(ow, leftchild, 0);
//         ow<<"}}} ";

//         ow<<"sblm_string={{{";
//         for (int i=0; i<rhsStates; i++){
//           ow<<L2Rpermu[i];
// 	  ow<<" ";
// 	}
//         ow<<"}}} ";
//         ow<<"lm=yes sblm=yes";

// #ifdef NOPACKING
// 	ow<<" bin-prob="
// 	  <<curprb;
// 	ow<<" original_rule_prob="
// 	  <<curprb;
// #endif


// 	ow<<" rule_file_line_number=";
// 	//	ow<<"{{{";
// 	ow<<totalRules;
// 	//	ow<<"}}}";
//         ow<<endl;

      }//end of invertedbinary root rule
      else{
	//virtual bin inverted rule

	vrule_t *therul=NULL;
	nd.label=vrul_construct(
				(leftchild.virt?vt:rt), leftchild.label,
				(rightchild.virt?vt:rt), rightchild.label,
				&leftchild,
				&rightchild,
				1,
				therul,
				true,
				false
				);
	//	nd.label=VLHS(therul);
	vRules++;

//         ovstr.clear();
//         ostringstream ov(ovstr);

// 	ov<<nd.label
// 	  <<RTRDELIM
// 	  <<nd.label
// 	  <<"("
// 	  <<"x0:"
// 	  <<rightchild.label
// 	  <<" "
// 	  <<"x1:"
// 	  <<leftchild.label
//	  <<")";

// 	ov<<" -> ";
	
// 	ov<<"x1";
// 	ov<<" ";
// 	ov<<"x0";


// 	ov<<" ### ";
//         ov<<"complete_subtree=no sblm=no lm=yes ";
//         ov<<"lm_string={{{";
//         lmleaf(ov, rightchild, 1);
// 	ov<<" ";
//         lmleaf(ov, leftchild, 0);
//         ov<<"}}} ";
//         ov<<"sblm_string={{{}}} virtual_label=yes";

//#ifdef NOPACKING
//         ov<<" bin-prob="
//           <<1;
//         ov<<" original_rule_prob="
//           <<curprb;
// 	ov<<" rule_file_line_number={{{";
// 	ov<<totalRules;
// 	ov<<"}}}";
// 	ov<<endl;
//#endif

	//	putv(ov.str(), totalRules);
	
      }//end of virtual bin rule

    }//end of inverted rule
    
  return vRules;
}

//post order visiting the itg parse tree
//generating bin rules 
void output (int index) {
  
  if (index<0){//lexical rules
    lexbin();
    return;
  }

  //for rules having at least one variable

  node &nd = nodes[index];
  if (nd.tag == 3){//a leaf of the tree
    unibin(nd);
  }
  else {
    //binary rules
    
    //recurse on leftchild
    output(nd.leftchild);
    //recurse on rightchild
    output(nd.rightchild);

    binbin(nd, index);


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
int bracketing(){  
  int n=alignment.size();

  stack.resize(n);
  top=0;
  nodes.clear();

  //shift-reduce, reduce preferred
  for (int i = 0; i < n; i++) {
    //    push(alignment[i]);
    //    nodes.push_back(node(alignment[i], alignment[i], -1, -1, 3, 0));
    stacking(alignment[i], alignment[i], -1, -1, 3, 0);
    // nodeptr newnode;
    combine();
    //    while (top>1 && combine()){
      //      && can_combine( stack[top-2 ], stack[top-1], newnode ) ){
      //cerr<<"combined --> "<<newnode->left<<"-"<<newnode->right<<endl;
      //      top -= 2;
      //      push(*newnode);
    //    }
  }

  if (top > 1) {
    //cerr<<"unbinarizable!"<<endl;
  }
  else {
    rhsptr=0;
    prev_leaf=NULL;
    //    prev_binode=NULL;

    //so we output the binarizable cases
#ifndef NOOUTPUT
    vRules=0;
    if (top==1)
      output(stack[0]);
    else{
      output(-1);
    }
    //    cerr<<"vRules: "<<vRules<<" ";
    //    cerr<<"rhsSize: "<<rhsSize<<endl;
#else
    cerr<<line<<endl;
#endif

#ifdef SANITY
    sanity_xrs();
#endif
  }
  
  return 0;
}

//traverse a treenode on lhs of xrs
void traverseNode(ns_RuleReader::RuleNode *curr, int depth) // traverse a RuleNode tree
{

  if (curr->isNonTerminal()) { 	// if this is a nonterminal node
    //cerr << "NT: " << curr->getString(false) << endl; 
    for (vector<ns_RuleReader::RuleNode *>::iterator it=curr->getChildren()->begin();it!=curr->getChildren()->end(); ++it) { // recurse on children
      traverseNode((*it), depth+1);
    } // end for each child
  }
  else{ 
    //if this is a terminal node
    LHS_symbols.push_back(curr->getString());
    //bit map for binarizer to know which english words have been covered
    covered_mask.push_back(0);

    //    if (curr->isLexical()){
      // is this a lexical leaf node?
      //cerr << "LEX: " << curr->getString() << endl;
    //    }
    //    else
    if (curr->isPointer()) { // is this a pointer leaf node?
      //cerr << "POINTER: " << curr->getRHSIndex() << " " << curr->getString() << endl;
      //fill in the RHS symbols table
      RHS_symbols[curr->getRHSIndex()]=curr->getString();
      LHS_idx.push_back(lhsptr);
    }
    lhsptr++;
  }
}


//wrapper
void traverse(ns_RuleReader::RuleNode *curr){
  lhsptr=0;
  LHS_symbols.clear();
  LHS_idx.clear();
  //  RHS_symbols.resize(MAXSTATES);
  //  RHS_symbols.clear();
  covered_mask.clear();
  L2Rpermu.resize(rhsSize);
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
int main(int argc, char *argv[]){
  //  if (argc!=2){
  //    cerr<<argv[0]<<"<rule-file>"<<endl;
  //    return 1;
  //  }

  //  ifstream from(argv[1]);  

  //  if (!from){ 
  //    cerr<<"Couldn't open "<<argv[1]<<endl;
  //    exit(-1);
  //  }

  putsym(cw, emp);

  line.clear();
  totalRules=0;
  totalXrs=0;
  
  while (getline(cin, line)) { // for each line in example rules file
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
    } catch (string s) { 
      cerr << "Caught exception: " << s << endl;
      exit(-1);//      continue;
    }

    //myRule parsed

    curr = myRule->getLHSRoot(); // get the LHS root
    
    rhsSize = myRule->getRHSStates()->size(); // side of RHS of the rule
    RHS_symbols.clear();
    RHS_symbols.resize(rhsSize);

    traverse(curr);		// traverse the LHS root

    rhsStates=0;
    alignment.clear();
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
	alignment.push_back(lid+1);
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

    delete myRule;

  }//end for all rules

  //output all the virtual rules
  //  voutput(ow);
  vrul_output(ow);

#ifdef SANITY
  sanitycheck();
#endif
}
