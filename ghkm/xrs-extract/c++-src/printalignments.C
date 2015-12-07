#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <sys/resource.h>

#include "Tree.h"
#include "Alignment.h"
#include "Derivation.h"
#include "PhrasalCohesion.h"
#include "ATS.h"
#include "defs.h"

using namespace mtrule;

void usage() {
  std::cerr 
  << "\n"
  << "Usage: extract -r <base name> [OPTIONS]\n\n" 
  << "     <basename>  : basename of the three files that are required by\n"
  << "                   the extraction program:\n"
  << "                   <basename>.eng : file of English parse trees\n"
  << "                   <basename>.a   : word alignment\n"
  << "                   <basename>.cgb : chinese sentences\n\n"
  << "Options:\n"
  << "            -s N : specify index of start line\n"
  << "            -e N : specify index of end line\n"
  << "       -t FORMAT : specify tree format (currently supported:\n"
  << "                   \"radu\" and \"collins\"; default: radu)\n"
  << "              -S : do not skip lines that have an inconsistent alignment\n"
  << "              -l : loose (not only 1-1)\n";
  std::exit(1);
}

int main(int argc, char *argv[]) {

 int opt;
 char *basename=NULL,
      *tree_format=NULL;
 char default_tree_format[] = "radu";
 bool verbose=false, 
      fix_trees=false,
		no_skip=false,
		loose=false;
 int startsent=-1, 
     endsent=-1;
 ATS ats;
     
 // Parse command-line parameters:
 while((opt = getopt(argc,argv,"r:t:s:e:lfvS")) != -1) 
   switch(opt) {
     case 'r': basename = optarg;        break;
	  case 't': tree_format = optarg;     break;
     case 's': startsent = atoi(optarg); break;
     case 'e': endsent = atoi(optarg);   break;
     case 'f': fix_trees = true;         break;
	  case 'l': loose = true;             break;
     case 'v': verbose = true;           break;
	  case 'S': no_skip = true;           break;
     default: usage();
   }

 // If no default tree format is specified, use Collins's:
 if(tree_format == NULL)
   tree_format = default_tree_format;
 
 // Check that command-line parameters are correct:
 if(!basename) usage();

 // Prevent core file creation:
#ifndef DEBUG
 struct rlimit    core_limits;
 core_limits.rlim_cur = 0;
 core_limits.rlim_max = 0;
 setrlimit(RLIMIT_CORE, &core_limits);
#endif

 // File names:
 std::string parsefile(basename), 
             alignmentfile(basename),
             chinesefile(basename);
 // Add default extensions: (no way to modify this...)
 parsefile += ".eng"; 
 alignmentfile += ".a"; 
 chinesefile += ".cgb";
 
 // Open parse file:
 std::ifstream p_in(parsefile.c_str());
 if(p_in.fail()) {
   std::cerr << "Error opening file : " 
             << parsefile << std::endl;
   exit(1);
 }

 // Open alignment file:
 std::ifstream a_in(alignmentfile.c_str());
 if(a_in.fail()) {
   std::cerr << "Error opening file : " 
             << alignmentfile << std::endl;
   exit(1);
 }

 // Open chinese file:
 std::ifstream c_in(chinesefile.c_str());
 if(c_in.fail()) {
   std::cerr << "Error opening file : " 
             << chinesefile << std::endl;
   exit(1);
 }

 // Loop through each sentence pair (with alignment) and extract R rules:
 int i=0;
 std::string tmp;
 while(!p_in.eof()) {
   ++i;
   // Skip line i if not satisfying : startsent <= i <= endsent
   if(i == endsent+1) break;
   if(i < startsent) { 
     // Skip one line:
     getline(p_in,tmp); 
     getline(a_in,tmp); 
     getline(c_in,tmp); 
     continue; 
   }

   std::cerr << "%%% ===== LINE : " << i << std::endl;

   // Read parse tree from file:
	std::string str;
	getline(p_in,str);
	if(str == "")
	  break;

   // Create tree structure, and fix punctuation (if in collins format):
   Tree t(str,tree_format);
   TreeNode *root = t.get_root();
   if(fix_trees) {
	  std::cerr << "%%% collins: fixed/raised punctuation " << std::endl;
     t.fix_collins_punc();
	}

   // Read e-string, c-string, and the alignment, and make 
   // sure the 3 elements are consistent, i.e. that no alignment
   // connects to an unexisting word:
   std::string c_string, e_string = t.get_string();
   getline(c_in,c_string);
   Alignment a(a_in,e_string,c_string);
	a.print_alignments(loose);

 }

 // Close file handles:
 p_in.close();
 a_in.close();
 c_in.close();

 return(0);
}
