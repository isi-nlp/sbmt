#include <boost/program_options.hpp>
#include "TreeProjector.h"
#include "Tree.h"

using namespace boost;
using namespace mtrule;
using namespace boost::program_options;

bool help;
string eparse_file;
string alignment_file;
string foreign_file;
bool project_rule_size;

unsigned int startsent = 1;
unsigned int endsent = (unsigned int ) -1;


int main(int argc, char **argv) {

    options_description general("General options");
    general.add_options() 
		("help,h",    bool_switch(&help), "show usage/documentation")
        ("parses,p",  value<string>(&eparse_file)->default_value(""),
	         "the parses.")
        ("alignment,a",  value<string>(&alignment_file)->default_value(""),
	         "the word alignments.")
        ("foreign,f",  value<string>(&foreign_file)->default_value(""),
	         "the foreign text.")
		("project-rule-size,S",    bool_switch(&project_rule_size)->default_value(false), 
		      "print the size of rules to the foreign side.")
        ("start,s",  value<unsigned int>(&startsent)->default_value(1),
	         "the start sentence (1-indexed).")
        ("end,e",  value<unsigned int>(&endsent)->default_value(-1),
	         "the end sentence (1-indexed).");



    variables_map vm;
    store(parse_command_line(argc, argv, general), vm);
    notify(vm);

    if(help || argc == 1){ cerr<<general<<endl; exit(0); }


	ifstream eparse_fin(eparse_file.c_str());
	if(!eparse_fin) {
		cerr<<"Cannot open e-parse file for eading";
		exit(1);
	}

	ifstream alignment_fin(alignment_file.c_str());
	if(!alignment_fin) {
		cerr<<"Cannot open aligment file for eading";
		exit(1);
	}

	ifstream foreign_fin(foreign_file.c_str());
	if(!foreign_fin) {
		cerr<<"Cannot open foreign file for eading";
		exit(1);
	}

    string eparse_line, alignment_line, foreign_line;

	unsigned int i = 0;
	while(!eparse_fin.eof()){ 
	   i++;
	   if(i == endsent+1) break;
	   if(i < startsent) { 
		   if(!getline(eparse_fin, eparse_line) || 
			   ! getline(alignment_fin, alignment_line ) ||
			   ! getline(foreign_fin, foreign_line)) {
			   break;
		   }
		 continue; 
	   }

	   if(!getline(eparse_fin, eparse_line) || 
		   ! getline(alignment_fin, alignment_line ) ||
		   ! getline(foreign_fin, foreign_line)) {
		   break;
	   }


		if(i % 10000 == 0){ cerr<<i<<endl; }

	    Tree t(eparse_line, "radu-new");
		if(t.get_leaves() == NULL) { cout<<"0"<<endl; continue;}

	    TreeNode *root = t.get_root();
		if(!root){ cout<<"0"<<endl; continue;}


	    std::string e_string = t.get_string();

        if(alignment_line == "" || foreign_line == "" || e_string == ""){
            cout<<"0"<<endl;
            continue;
        }
		//cout<<alignment_line<<" \n"
		//	<<foreign_line<<endl
		//	<<eparse_line<<endl;
	    Alignment a(alignment_line,e_string,foreign_line);

	  if(!a.consistency_check(false) || a.is_bad()) {
		 cerr << "%%% ===== e-parse/c-string/alignment are NOT consistent!" << std::endl;
		 cout<<"0"<<endl;
		 continue;
	  }

	// Determine the target language span for each syntactic 
   // constituent (called on the root, and the rest is done recursively):
   TreeNode::assign_alignment_spans(root,&a);
   TreeNode::assign_complement_alignment_spans(root,&a);
   TreeNode::assign_frontier_node(root);
		DB* dbp = NULL;
		TreeProjector tprj (&a, dbp, project_rule_size);
		tprj.extract_derivations(root,0, a.get_target_len()-1);

	    cout<<tprj.project(t)<<endl;

	}

}




