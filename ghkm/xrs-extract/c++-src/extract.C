#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <sys/resource.h>

#include "Tree.h"
#include "Alignment.h"
#include "Derivation.h"
#include "GenDer.h"
#include "RuleSet.h"
#include "ATS.h"
#include "MyErr.h"
#include "defs.h"
#include "db_access.h"
#include "PackedForest.h"

using namespace mtrule;

/**
 * 'extract' is a program that parses source-language trees aligned
 * with target language strings and extracts cross-lingual syntactic
 * xRs rules.
  * The -H option extracts rules of the following types:

 vXRS - XRS like rules that have only variables and non-aligned words:
 Examples: VP(x0:VB x1:NP-C) -> x0 x1 ### type=vXRS fc=0.5
           VP(MD("will") x1:VP-C) -> x1 ### type=vXRS fc=0.3333
	   VP(x0:VBD NP-C(x1:NPB PP(IN("of") NP-C(x2:NPB x3:PP)))) -> x3 x0 x1 x2 ### type=vXRS fc=1
	   VP(x0:VP-C) -> c1 x0 ### type=vXRS fc=1

lcvXRS - XRS like rules that have lexicalizations on the source and target sides and vars
  Examples: S(NP(DT("these") CD("7") NNS("people")) VP(VBP("include") x0:NP) x1:.) -> "THESE" "7PEOPLE" "INCLUDE" x0 x1 ### type=lcvXRS
            VP(VBG("coming") PP(IN("from") x0:NP)) -> "COMINGFROM" x0 ### type=lcvXRS

lcXRS - XRS like rules that have contiguous lexicalizations on the source and target sides and no variables
Examples: NP(DT("these") CD("7") NNS("people")) -> "THESE" "7PEOPLE" ### type=lcXRS
          NP(NP(NNP("france")) CC("and") NP(NNP("russia"))) -> "FRANCE" "AND" "RUSSIA" ### type=lcXRS

lcvXRS-C - composed lcvXRS rules that are produced by the WSD rule extraction procedure in a bottom-up fashion

 */

void usage() {
  //  *mtrule::MyErr::err  (for some reason using cerr is crashing on hpc)
  std::cout  // not a permanent solution
  << "\n"
  << "Usage: extract ...\n" 
  << "MAIN ARGUMENTS:\n"
  << "          -s <n> : specify index of start line.\n"
  << "          -e <n> : specify index of end line.\n"
  << "   -r <basename> : basename of three files that are read by\n"
  << "                   the extraction program:\n"
  << "                   <basename>.e-parse : file of English parse trees\n"
  << "                   <basename>.a       : word alignment\n"
  << "                   <basename>.f       : foreign sentences\n"
  << "                   (if no basename is provided, data is read from\n"
  << "                    stdin, 3 lines at a time (etree, fstring, alignment)).\n"
  << "    -D <DB file> : read/write rules from/to a Berkeley DB file.\n"
  << "              -C : only perform consistency checking of the data\n"
  << "                   (do not print rules).\n"
  << "EXTRACTION ARGUMENTS:\n"
  << "[-x|-d|-c] <out> : GHKM extraction in one of the following modes:\n"
  << "                    x : extraction mode. Print rules to stdout\n"
  << "                        (or save them into a DB if -D is used).\n"
  << "                    d : derivation mode. Print derivations to\n"
  << "                        stdout, followed by rules that are used in\n"
  << "                        the derivation. If -D is specified, then\n"
  << "                        global rule indexes are used, and rules\n"
  << "                        are not appended after each derivation.\n"
  << "                    c : count mode. Print counts to stdout in\n"
  << "                        csv format, i.e. each line has the following\n"
  << "                        format: count,fraccount (note: line number\n"
  << "                        == rule ID)\n"
  << "                        This mode only works in conjunction with -D.\n"
  << "                    Note that 'd' and 'c' can be combined, e.g.\n"
  << "                     -d set1.deriv -c set1.counts\n"
  << "                    You can use '-' as a substitute to stdout.\n"
  << "   -a <ATS file> : extract alignment template rules (ATS).\n"
  << "                   (this overrides modes [-x|-d|-c]).\n"
  << "        -l <n:i> : Various constraints on rules to extract:\n"
  << "                   n : maximum number of rules per tree node\n"
  << "                   i : maximum number of internal nodes in LHS (not including pre-terminals)\n"
  << "                   (default: " << MAX_TO_EXTRACT << ":" << MAX_RULE_EXPANSIONS << ")\n"
  << "              -m <n> : Extract n rules from each node at maximum. \n"
  << "                       in forest-based extraction, n includes minimal, \n"
  << "                       the ordinary extraction doesnt. \n"
  << "                       The -l restricts the maximum number of LHS's extracted for each node.\n"
  << "        -h : mark the head constituents in the rules via generating headmarker={{{...}}}\n"
  << "              -S : do not skip lines that have inconsistent alignments\n"
  << "              -I : print @IMPOSSIBLE@ rules; used in conjunction with -G to OUT file\n"
  << "              -K : extract composed lexicalized rules; used in conjunction with -G\n"
  << "          -G OUT : WSD-phrase extraction mode. Print WSD info and impossible rules\n"
  << "                   to OUT (- can be used as a substitute for STDERR)\n"
  << "            -g N : max source phrase size: used in conjunction with -G\n"
  << " Note that it is required to use either -m or -a (exactly one of them).\n"
  << "          -U <n> : if number of unaligned foreign words per RHS is <= n,\n"
  << "                   search all possible attachments (as explained in \"GHKM2\")\n"
  << "                   Otherwise, default to highest attachment (as in GHKM).\n"
  << "                   Default is "<< MAX_NB_UNALIGNED <<".\n"
  << "              -O : unaligned foreign words can be assigned to POS rules.\n"
  << "              -M : Mixed case in the tree side.\n"
  << "PRE-PROCESSING ARGUMENTS:\n"
  << "       -A <file> : list of bad alignments that should be discarded\n"
  << "     -t <format> : specify tree format (currently supported:\n"
  << "                   \"radu\", \"radu-new\", and \"collins\"; default: radu-new)\n"
  << "              -f : fix punctuation in parse trees (collins only)\n"
  << "              -B : delete maximum-crossing alignment before extracting rules\n"
  << "MISC. ARGUMENTS:\n"
  << "              -i : ignore any expansion rooted at non-terminal *-BAR\n"
  << "              -F : forest-based rule extraction \n"
  << "              -E : erase auxilary nodes when print rules. \n"
  << "              -H : skip header lines\n"
  << "  -p <prob file> : load probability file and print most-probable\n"
  << "                   derivations instead of dforests (this option\n"
  << "                   must be used in conjunction with \"-d\".)\n"
  << "              -P : print rules after derivations\n"
  << "              -1 : return the 1-best derivation instead of the entire\n"
  << "                   dforest (assumes that \"-P\" and \"-d\" are used).\n"
  << "       -R <rule> : print e-tree/c-string/align to standard output if the\n"
  << "                   tuple can be used to extract the given rule.\n"
  << "        -L <LHS> : same as -R, but tries to match a given LHS.\n"
  << " This program has two modes of operation: extraction according to GHKM&WSD (-x|-d|-c) and\n"
  << " sATS rule extraction (-a).\n"
  << " For WSD functionality use the G,g,K,I arguments. To print alignments while running the\n"
  << " GHKM extraction only procedure, use the option -G - -g 1\n"
  << std::endl;
  std::exit(1);
}

int main(int argc, char *argv[]) {

 int opt;
 char *basename=NULL;
 char *tree_format="radu-new", *ATSfile=NULL, *stoplist=NULL,
      *limits=NULL, *n_rules_limits = NULL, *dbfile=NULL, *extract_outfile=NULL, *deriv_outfile=NULL,
      *count_outfile=NULL, *prob_file=NULL, *match_rule=NULL, *match_lhs=NULL,
   *hierarchical_outfile=NULL; // Daniel
 bool verbose=false, fix_trees=false, consistency_only=false, no_skip=false,
      extract_mode=false, derivation_mode=false, count_mode=false,
      print_rules=false, one_best=false, header=true, 
      print_impossible=false, print_lexicalized_composed=false, hierarchical_mode=false, // Daniel
   ignore_aux_expansion=false, // Wei
   erase_aux_nodes_when_print=false; // Wei
   bool forest_based_extraction = false;

 int startsent=-1, 
     endsent=-1;
 int max_rule_expansions = MAX_RULE_EXPANSIONS,
   max_to_extract      = MAX_TO_EXTRACT,
   max_phrase_source_size = MAX_PHRASE_SOURCE_SIZE; // Daniel
 ATS ats;

 // Prevent core file creation:
 // JSV: Use "ulimit -c 0" -- don't put it in the app!
#ifndef DEBUG
 struct rlimit    core_limits;
 core_limits.rlim_cur = 0;
 core_limits.rlim_max = 0;
 setrlimit(RLIMIT_CORE, &core_limits);
#endif

 double vXRS=0, lcvXRS=0, lcXRS=0, lcATS=0, hlcXRS=0, sIMPOSSIBLE=0, tIMPOSSIBLE=0, LC=0; // Daniel

 // JSV: gain speed by NOT sync'ing streams with stdio
 std::ios_base::sync_with_stdio(false);

 ////////////////////////////////////////////////////////////////////////
 // Parse command-line parameters:
 ////////////////////////////////////////////////////////////////////////
 while((opt = getopt(argc,argv,"r:x:d:c:t:s:e:a:p:A:G:g:l:m:D:fCvMSP1R:L:U:TOBHhoiFEIK")) != -1) 
   switch(opt) {
   case 'r': basename = optarg;        break;
   case 'x': extract_outfile = optarg; 
     extract_mode = true;      break;
   case 'd': deriv_outfile = optarg; 
     derivation_mode = true;   break;
   case 'c': count_outfile = optarg; 
     count_mode = true;        break;
   case 't': tree_format = optarg;     break;
   case 's': startsent = atoi(optarg); break;
   case 'e': endsent = atoi(optarg);   break;
   case 'a': ATSfile = optarg;         break;
   case 'p': prob_file = optarg;       break;
   case 'P': print_rules= true;        break;
   case 'l': limits = optarg;          break;
   case 'm': n_rules_limits = optarg;  break;
   case 'D': dbfile = optarg;          break;
   case 'A': stoplist = optarg;        break;
   case 'f': fix_trees = true;         break;
   case 'C': consistency_only = true;  break;
   case 'v': verbose = true;
     Derivation::set_verbose();break;
   case 'M': TreeNode::lowercasing = false; break; 
			               // dont lowercase tree leaves. Wei
   case 'S': no_skip = true;           break;
   case '1': one_best = true;          break;
   case 'R': match_rule = optarg;      
     *myerr << "Trying to match: " << match_rule << std::endl;
     break;
   case 'L': match_lhs = optarg;
     *myerr << "Trying to match: " << match_lhs << std::endl;
     break;
   case 'U': RuleInst::set_max_nb_unaligned(atoi(optarg)); break;
   case 'T': State::print_all_rules(true); break;
   case 'O': RuleInst::set_can_assign_unaligned_c_to_preterm(); break;
   case 'B': TreeNode::find_bad_align = true; break;
   case 'H': header = false; break;
   case 'h': State::mark_const_head_in_rule = true; break;
   case 'o': MyErr::err = &std::cout; break;
   case 'G': hierarchical_mode=true;  hierarchical_outfile = optarg; break; // Daniel
   case 'K': print_lexicalized_composed=true; break; // Daniel
   case 'I': print_impossible=true;  break;           // Daniel
   case 'g': max_phrase_source_size = atoi(optarg); break;     // Daniel 
   case 'i': ignore_aux_expansion = true; break; // Wei
   case 'F': forest_based_extraction = true; break; // Wei
   case 'E': erase_aux_nodes_when_print=true; break; // Wei
   default: usage();
   }

 // Parse limits argument:
 if(limits != NULL) {
   std::string str(limits);
	size_t pos1 = str.find(":");
	max_to_extract      = atoi(str.substr(0, pos1).c_str());
	max_rule_expansions = atoi(str.substr(pos1+1).c_str());
 }

 if(n_rules_limits != NULL){
	 Derivation::set_max_rules_per_node (atoi(n_rules_limits));
 } 

 /*
 // Daniel: If extract rules in hierarchical_mode, set max_to_extract and
 //         max_rule_expansions to 1 and 10 respectively
 if(hierarchical_mode == true){
   max_to_extract = 1;
   max_rule_expansions = 10;
   if(limits != NULL){
     std::string str(limits);
     size_t pos1 = str.find(":");
     max_to_extract      = atoi(str.substr(0, pos1).c_str());
     max_rule_expansions = atoi(str.substr(pos1+1).c_str());
   }
 }
 */


 Derivation::set_limits(max_rule_expansions, max_to_extract);

 // Check that command-line parameters are correct:
 if(!extract_mode && !derivation_mode && !count_mode &&
    ATSfile==NULL && !match_rule && !match_lhs &&
    // !hierarchical_mode && // Daniel
    !consistency_only) 
   usage();
 if(prob_file != NULL && !derivation_mode)
   usage();
 if(one_best && (prob_file == NULL || !derivation_mode))
   usage();

 // Create a RuleSet object to store counts:
 RuleSet *rs = NULL;
 if(count_mode) {
	rs = new RuleSet(true,0);
 }

 ////////////////////////////////////////////////////////////////////////
 // Setup I/O stuff
 ////////////////////////////////////////////////////////////////////////

 // Input streams:
 std::istream *parse_in;
 std::ifstream parse_fin;
 std::istream *forest_in;
 std::ifstream forest_fin;
 std::istream *chinese_in;
 std::ifstream chinese_fin;
 std::istream *align_in;
 std::ifstream align_fin;
 
 // We have a basename specified by '-r'. 
 // Read data from 3 different files:
 if ( basename != NULL ) {
   // File names:
   std::string parsefile(basename), 
     forestfile(basename),
     alignmentfile(basename),
     chinesefile(basename);

   // Add default extensions: (no way to modify this...)
   parsefile += ".e-parse"; 
   forestfile += ".e-forest";
   alignmentfile += ".a"; 
   chinesefile += ".f";

   // Open parse file:
   size_t bufsize = 8 << 20;
   char* parse_buf = new char[bufsize];
   if ( parse_fin.rdbuf()->pubsetbuf( parse_buf, bufsize ) == NULL ) {
     *myerr << "Error setting buffer on "
	    << parsefile << std::endl;
     exit(1);
   }
   parse_fin.open(parsefile.c_str());
   if(parse_fin.fail()) {
     *myerr << "Error opening file : " 
	    << parsefile << std::endl;
     exit(1);
   }

   // Open forest file:
   if ( forest_based_extraction ) {
     char* forest_buf = new char[bufsize];
     if ( forest_fin.rdbuf()->pubsetbuf( forest_buf, bufsize ) == NULL ) {
       *myerr << "Error setting buffer on "
	      << forestfile << std::endl;
       exit(1);
     }
     forest_fin.open(forestfile.c_str());
     if(forest_fin.fail()) {
       *myerr << "Error opening file : " 
	      << forestfile << std::endl;
       exit(1);
     }
   }

   // Open alignment file:
   char* align_buf = new char[bufsize];
   if ( align_fin.rdbuf()->pubsetbuf( align_buf, bufsize ) == NULL ) {
     *myerr << "Error setting buffer on "
	    << alignmentfile << std::endl;
     exit(1);
   }
   align_fin.open(alignmentfile.c_str());
   if(align_fin.fail()) {
     *myerr << "Error opening file : " 
	    << alignmentfile << std::endl;
     exit(1);
   }

   // Open chinese file:
   char* chinese_buf = new char[bufsize];
   if ( chinese_fin.rdbuf()->pubsetbuf( chinese_buf, bufsize ) == NULL ) {
     *myerr << "Error setting buffer on "
	    << chinesefile << std::endl;
     exit(1);
   }
   chinese_fin.open(chinesefile.c_str());
   if(chinese_fin.fail()) {
     *myerr << "Error opening file : " 
	    << chinesefile << std::endl;
     exit(1);
   }

   parse_in   = &parse_fin;
   forest_in  = &forest_fin;
   align_in   = &align_fin;
   chinese_in = &chinese_fin;

 } else {
   // Otherwise, read everything from stdin:
   parse_in   = &std::cin;
   forest_in   = &std::cin;
   align_in   = &std::cin;
   chinese_in = &std::cin;
 }

 // Open ATS file if program is set to extract AT rules:
 std::ifstream ATS_in;
 if(ATSfile != NULL) {
	ATS_in.open(ATSfile);
	if(ATS_in.fail()) {
	  *myerr << "Error opening file : " 
					<< ATSfile << std::endl;
	  exit(1);
	}
	*myerr << "%%% Loading ATS file..." << std::endl;
	ats.read_ATS_file(ATS_in);
	*myerr << "%%% done: "<< ats.get_entries().size() 
	          << " entries." << std::endl;
 }

 // Open stoplist of bad alignments, if provided:
 if(stoplist != NULL) {
	std::ifstream stop_in;
	stop_in.open(stoplist);
   if(stop_in.fail()) {
	  *myerr << "Error opening file : " 
					<< stoplist << std::endl;
	  exit(1);
	}
   Alignment::load_stoplist(stop_in);
 }

 // Open probability file:
 if(prob_file) {
	rs = new RuleSet(false,0);
	rs->load_probs(prob_file);
 }

 // Open output files:
 std::ostream *extract_out=NULL;
 std::ofstream extract_fout;
 std::ostream *deriv_out=NULL;
 std::ofstream deriv_fout;
 std::ostream *count_out=NULL;
 std::ofstream count_fout;

 std::ostream *hierarchical_out=NULL; // Daniel
 std::ofstream hierarchical_fout; // Daniel

 if(extract_outfile != NULL) {
	if(strcmp(extract_outfile,"-") == 0) {
	  extract_out = &std::cout;
	} else {
	  extract_out = &extract_fout;
	  extract_fout.open(extract_outfile);
	  if(extract_fout.fail()) {
		 *myerr << "Error opening file : " 
					  << extract_outfile << std::endl;
		 exit(1);
	  }
	}
 }

 // Daniel
 if(hierarchical_outfile != NULL) {
     if(strcmp(hierarchical_outfile,"-") == 0) {
       hierarchical_out = myerr;
     } else {
       hierarchical_out = &hierarchical_fout;
       hierarchical_fout.open(hierarchical_outfile);
       if(hierarchical_fout.fail()) {
	 *myerr << "Error opening file : " 
		<< hierarchical_outfile << std::endl;
	 exit(1);
       }
     }
   }
 
 if(deriv_outfile != NULL) {
	if(strcmp(deriv_outfile,"-") == 0) {
	  deriv_out = &std::cout;
	} else {
     std::ifstream is;
     // Make sure that the deriv file doesn't already exist:
     is.open(deriv_outfile);
     if(is.is_open()) {
       *myerr << "File exists: "
                 << deriv_outfile << std::endl;
       exit(1);
     }
     is.close();
	  deriv_out = &deriv_fout;
	  deriv_fout.open(deriv_outfile);
	  if(deriv_fout.fail()) {
		 *myerr << "Error opening file : " 
					  << deriv_outfile << std::endl;
		 exit(1);
	  }
	}
 }
 if(count_outfile != NULL) {
	if(strcmp(count_outfile,"-") == 0) {
	  count_out = &std::cout;
	} else {
     std::ifstream is;
     is.open(count_outfile);
     if(is.is_open()) {
       *myerr << "File exists: "
                 << count_outfile << std::endl;
       exit(1);
     }
     is.close();
	  count_out = &count_fout;
	  count_fout.open(count_outfile);
	  if(count_fout.fail()) {
		 *myerr << "Error opening file : " 
					  << count_outfile << std::endl;
		 exit(1);
	  }
	}
 }

 ////////////////////////////////////////////////////////////////////////
 // Setup DB access:
 ////////////////////////////////////////////////////////////////////////

 DB* dbp=NULL;
 if(dbfile) {
   *myerr << "commented out 2016-05-02\n";
   //*myerr << "%%% opening DB: " << dbfile << std::endl;
   //db_init(dbp);
   //set_db_cache_size(dbp,DB_CACHE_SIZE);
   //db_open(dbp,dbfile,true); // read-only
   //*myerr << "%%% done." << std::endl;
 }

 ////////////////////////////////////////////////////////////////////////
 // Print header lines:
 ////////////////////////////////////////////////////////////////////////
 
 std::stringstream start_ss, end_ss;
 if(header) {
	if(startsent >= 0)
	  start_ss << " start=" << startsent; 
	if(endsent >= 0)   
	  end_ss << " end=" << endsent;
	if(derivation_mode) {
	  *deriv_out << "$$$ filetype=derivation version=1.0" << std::endl
	             << "%%% data=" << basename 
				    << start_ss.str() << end_ss.str() << std::endl;
	}
	if(extract_mode) {
	  *extract_out << "$$$ filetype=rule version=1.0" << std::endl
	               << "%%% data=" << basename 
				 	   << start_ss.str() << end_ss.str();
	  if(ATSfile != NULL) 
		 *extract_out << " ATS=" << ATSfile;
	  *extract_out << std::endl;
	  *extract_out << "$$$ ";
	  int noArgs = 0;
	  while(noArgs < argc)
	    *extract_out << argv[noArgs++] << ' ';
	  *extract_out << std::endl;
	}
 }
 if(count_mode) {
	  *count_out << "$$$ filetype=weight order=ruleIDs fields=";
#ifdef COLLECT_COUNTS
	  *count_out << "count"; 
#ifdef COLLECT_FRAC_COUNTS
	  *count_out << ",";
#endif
#endif
#ifdef COLLECT_FRAC_COUNTS
	  *count_out << "fraccount";
#endif
	  *count_out << " version=0.1" << std::endl
	             << "%%% data=" << basename 
			 	    << start_ss.str() << end_ss.str() << std::endl;
 }

 // Do we count or ignore the aux expansion when computing the big N?
 State::ignore_aux_expansion(ignore_aux_expansion);
 State::erase_aux_nodes_when_print(erase_aux_nodes_when_print);

 ////////////////////////////////////////////////////////////////////////
 // Loop through each sentence pair (with alignment) and extract R rules:
 ////////////////////////////////////////////////////////////////////////
 
 int i=0;
 std::string tmp;
 int good_trees=0, bad_trees=0;
 while(!parse_in->eof()) {
   ++i;
   // Skip line i if not satisfying : startsent <= i <= endsent
   if(i == endsent+1) break;
   if(i < startsent) { 
     // Skip one line:
     getline(*parse_in,tmp);
     if(tmp.substr(0,3) == "@@@")
       getline(*parse_in,tmp);
	 if(forest_based_extraction){ getline(*forest_in,tmp);}
     getline(*chinese_in,tmp);
     getline(*align_in,tmp);
     continue; 
   }

   // Read parse tree from file:
	std::string parse_str, forest_str, c_string, align_string;
	getline(*parse_in,parse_str);
	if(forest_based_extraction){ getline(*forest_in, forest_str); }
   getline(*chinese_in,c_string);
   getline(*align_in,align_string);
   if(parse_str == "" || c_string == "" || align_string == "" || (forest_based_extraction && forest_str == "") )
     continue;
   if(parse_str.substr(0,3) == "@@@") {
     // Parse control sequence after the @@@:
     std::stringstream ss(parse_str.substr(4));
     std::string AVpair;
     while(ss >> AVpair) {
       *myerr << "%%% parsing options: " << AVpair << std::endl;
       size_t pos1 = AVpair.find("=");
       assert(pos1 != std::string::npos);
       std::string attrname = AVpair.substr(0, pos1).c_str();
       std::string attrval  = AVpair.substr(pos1+1).c_str();
       *myerr << "%%% attr: " << attrname << std::endl;
       if(attrname == "l") {
         size_t pos2 = attrval.find(":");
         assert(pos2 != std::string::npos);
         max_to_extract      = atoi(attrval.substr(0, pos2).c_str());
         max_rule_expansions = atoi(attrval.substr(pos2+1).c_str());
         Derivation::set_limits(max_rule_expansions, max_to_extract);
       }
     }
     getline(*parse_in,parse_str); // IN
   }

   // Start processing a sentence pair:
   *myerr << "%%% ===== LINE : " << i << " ";

#if 0
   std::map<int, bool>::iterator it;
   for(it = State::__map.begin(); it != State::__map.end(); ++it){
	   if(it->second){
		   *myerr<<it->first<<endl;
	   }
   }

   State::__map.clear();
#endif

#ifdef DEBUG
   *myerr << "(memory allocation: treenod=" 
				 << TreeNode::get_nb_nodes_in_mem() 
             << " rules=" 
				 << State::get_nb_states_in_mem() 
             << " rulenod=" 
				 << RuleNode::get_nb_nodes_in_mem() 
             << " dernod=" 
				 << DerivationNode::get_nb_nodes_in_mem() 
				 << " rhsels="
				 << RuleRHS_el::get_nb_els_in_mem()
				 << " rhs="
				 << RuleRHS::get_nb_obj_in_mem()
				 << ")";
#endif
   *myerr << std::endl;

   // Create tree structure, and fix punctuation (if in collins format):
   Tree t(parse_str,tree_format);
	if(t.get_leaves() == NULL)
	  continue;

   TreeNode *root = t.get_root();

	SimplePackedForest forest;
	if(forest_based_extraction){
		istringstream ist(forest_str);
		forest.get(ist);
		forest.cutLeaves();
		//forest.put(cout);
        root = forest.getRoot();
	}


   if(fix_trees) {
	  *myerr << "%%% collins: fixed/raised punctuation " << std::endl;
     t.fix_collins_punc();
	}

   // Read e-string, c-string, and the alignment, and make 
   // sure the 3 elements are consistent, i.e. that no alignment
   // connects to an unexisting word:
   std::string e_string = t.get_string();
   Alignment a(align_string,e_string,c_string);
	if(stoplist != NULL)
	  a.delete_badalignments();
	if(!no_skip)
	  if(!a.consistency_check(false) || a.is_bad()) {
		 *myerr << "%%% ===== e-parse/c-string/alignment are NOT consistent!" << std::endl;
		 continue;
	  }

	if(!root) {
		 *myerr << "%%% ===== NULL root." << std::endl;
		 continue;
	}

	// Determine the target language span for each syntactic 
   // constituent (called on the root, and the rest is done recursively):
   TreeNode::assign_alignment_spans(root,&a);
   TreeNode::assign_complement_alignment_spans(root,&a);
   TreeNode::assign_frontier_node(root);

   // It is very important to reset the following to maintain
   // the priority of the States generated for current sentence
   // tuple.
   State::idd = 0;

   //if(forest_based_extraction){ forest.put(cout);}

   // Extra info sent to standard error for debug purposes:
	// (alignment spans, complement spans, etc)
   if(verbose) {
      *myerr << "%%% ===== e-string: "  << e_string << std::endl;
      *myerr << "%%% ===== e-forest: "  << forest_str<< std::endl;
      *myerr << "%%% ===== f-string: "  << c_string << std::endl;
      *myerr << "%%% ===== alignment: " << align_string << std::endl;
      *myerr << "%%% ===== G=(e-parse,f-string,alignment): " << std::endl; 
      t.get_root()->dump_pretty_tree(*myerr,0);
	}

   // Skip to the next line if we just want to check consistency of the 
	// data:
   if(consistency_only) {
	  if(verbose)
	    *myerr << "%%% ===== e-parse/f-string/alignment are consistent." << std::endl;
	  continue;
	}

   // Create a derivation object, make the alignment used in the search 
	// available to the Derivation object.
   Derivation *der;
   // Wei: if it is the e-forest based rule extraction, we allocate a
   // generalized derivation, GenDer, that can hanle AND and OR nodes in the
   // e-forest.  Otherwise, we still use the e-tree based Derivation class.
	if(forest_based_extraction) { der = new GenDer(&a,dbp,rs); } 
	else { der = new Derivation(&a,dbp,rs); }	
	 //der = new Derivation(&a,dbp,rs); 



	//if(dbp != NULL)
	//  der->set_global_indexing();

	// Different behaviors depending on mode: 
	if(ATSfile)
	  ////////////////////////
	  // AT rule extraction (don't extract "normal" rules):
	  der->extract_ATRules(t.get_leaves(), &ats);
	else {
	  ////////////////////////
	  // GHKM rule extraction
	  // Recursively extract rules, starting from the root:
	  der->extract_derivations(root,0,a.get_target_len()-1);
	  if(hierarchical_mode)
          std::cerr << "hier mode\n";
	    der->extract_wsd_rules_as_derivations(root, 0, a.get_target_len()-1, t.get_leaves(), 
			 hierarchical_out, dbp, max_phrase_source_size, &a, 
                         print_impossible, print_lexicalized_composed, 
			 vXRS, lcvXRS, lcXRS, lcATS, hlcXRS,
			 sIMPOSSIBLE, tIMPOSSIBLE, LC, i); 
	  if(der->is_good()) {
	    // This is a good derivation:
		 ++good_trees;
		 // Rule extraction mode:
		 if(match_rule || match_lhs) {
			if((match_rule && der->has_rule(std::string(match_rule),false)) ||
				(match_lhs  && der->has_rule(std::string(match_lhs),true))) {
			  *myerr << "%%% match at line : " << i << std::endl;
			  std::cout << parse_str << "\n" 
							<< e_string << "\n" 
							<< c_string << "\n" 
							<< align_string << std::endl;
		   }
		 } else if(extract_mode) {
			if(dbp != NULL) {
			  der->add_rules_to_db();
			} else {
			  der->print_derivation_rules(*extract_out,true,true);
			}
		 } else {
			if(derivation_mode || count_mode)
			  if(dbp != NULL)
				 der->retrieve_global_rule_indices();
			// If we need to only keep the 1-best:
			if(one_best) {
			  der->one_best(NULL);
			  der->keep_only_one_best(NULL);
			  if(der->non_zero_prob()) {
			    der->print_derivation_forest(*deriv_out,i);
				 if(print_rules)
					der->print_derivation_rules(std::cout,true,false);
			  }
			}
			// Derivation mode: (without extracting the viterbi derivation)
			else if(derivation_mode) {
			  // der->print_derivation_rules(*deriv_out,true,true);
			  der->print_derivation_forest(*deriv_out,i);
			  if(print_rules)
				 der->print_derivation_rules(std::cout,true);
			}
			// Count mode:
			if(count_mode)
			  der->add_derivation_counts();
		 }
	  } else {
	    // Not a good parse:
	    ++bad_trees;
	  }
	}

	delete der;
 }


 // Daniel
 if(hierarchical_mode){
   *hierarchical_out << "%%% Total vXRS rules (only vars): " << vXRS << std::endl;
   *hierarchical_out << "%%% Total lcvXRS rules (lexicalized contiguous, with vars): " << lcvXRS << std::endl;
   *hierarchical_out << "%%% Total lcXRS rules (lexicalized contiguous, no vars): " << lcXRS << std::endl;
   *hierarchical_out << "%%% Total lcATS/vATS rule pairs (lexicalized contiguous, syntax-incompatible): " << lcATS << std::endl;
   *hierarchical_out << "%%% Total hlcXRS/hvXRS rule pairs (lexicalized contiguous, hierarchical, syntax-incompatible, with no vars): " << hlcXRS << std::endl;
   *hierarchical_out << "%%% Total source->IMPOSSIBLE rules: " << sIMPOSSIBLE << std::endl;
   *hierarchical_out << "%%% Total target->IMPOSSIBLE rules: " << tIMPOSSIBLE << std::endl;
   *hierarchical_out << "%%% Total lexicalized composed rules: " << LC << std::endl;
 }


 ////////////////////////////////////////////////////////////////////////
 // Close database:
 ////////////////////////////////////////////////////////////////////////
 
 if(dbfile) {
   *myerr << "commented out 2016-05-02\n";
   //*myerr << "%%% closing DB: " << dbfile << std::endl;
   //db_close(dbp);
 }

 ////////////////////////////////////////////////////////////////////////
 // Print counts:
 ////////////////////////////////////////////////////////////////////////

 if(count_mode) {
   rs->print_counts(*count_out); 
 }

 ////////////////////////////////////////////////////////////////////////
 // Display statistics: (used in the GHKM paper)
 ////////////////////////////////////////////////////////////////////////
 
 if(!consistency_only) {
	*myerr    << "%%% Parses: admitted: " << good_trees
	          << " total: " << good_trees+bad_trees
				 << " (" << good_trees/(double)(good_trees+bad_trees) 
				 << ")" << std::endl;
 }

 ////////////////////////////////////////////////////////////////////////
 // Close file handles:
 ////////////////////////////////////////////////////////////////////////
 
 extract_fout.close();
 deriv_fout.close();
 count_fout.close();

 if(basename != NULL) {
   parse_fin.close();
   align_fin.close();
   chinese_fin.close();
 }

 if(hierarchical_mode)
   hierarchical_fout.close();

 return(0);
}
