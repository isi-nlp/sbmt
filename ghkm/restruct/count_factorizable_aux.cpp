#include "Tree.h"
#include "Alignment.h"
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>

using namespace boost;
using namespace std;
using namespace boost::program_options;

bool help;
bool aux;
string parsesFile;
string alnFile;
string foreignFile;

map<int, size_t> fanout_to_node_count;

void count(mtrule::TreeNode* node, 
		   int& nbAuxNodes, int& nbFactorizableAuxNodes) {
	string cat = node->get_cat();
	if(cat != "ROOT") {
		bool isAux = false;
		if(aux){
		if(cat.length() > 4 && cat.substr(cat.length() - 4 , 4) == "-BAR" ){
			nbAuxNodes ++;
			isAux = true;
		}
		} else {
			//cout<<cat<<endl;
			if(node->is_internal()){
				nbAuxNodes ++;
			}
		}
		if(aux){
		if(node->is_extraction_node() && isAux ){
			nbFactorizableAuxNodes++;
		}
		} else {
		//	fanout_to_node_count[node->get_nb_c_spans()] ++;
	//		fanout_to_node_count[node->size_cross()] ++;
		
	//		cout<<node->get_c_span(0).first<<"-"<<node->get_c_span(0).second<<endl;
			if(node->is_extraction_node() && node->is_internal()){
				nbFactorizableAuxNodes ++;
			}
		}
	}
	for(int i  = 0; i < node->get_nb_subtrees(); ++i){
		count(node->get_subtree(i), nbAuxNodes, nbFactorizableAuxNodes);
	}
}

int main(int argc, char **argv)
{
    options_description general("General options");
    general.add_options() 
		("help,h",    bool_switch(&help), "show usage/documentation")
		("aux,u",    bool_switch(&aux)->default_value(false), 
		             "if set, count only among aux nodes.")
        ("parses,p",  value<string>(&parsesFile)->default_value(""),
	         "the parses.")
        ("alignment,a",  value<string>(&alnFile)->default_value(""),
	         "the word alignments.")
        ("foreign,f",  value<string>(&foreignFile)->default_value(""),
	         "the foreign text.");

    variables_map vm;
    store(parse_command_line(argc, argv, general), vm);
    notify(vm);

    if(help || argc == 1){ cerr<<general<<endl; exit(0); }

		ifstream alnfile(alnFile.c_str());
		if(!alnfile){
			cerr<<"Can not open file "<<alnFile<<endl;
			exit(1);
		}

		ifstream foreignfile(foreignFile.c_str());
		if(!foreignfile){
			cerr<<"Can not open file "<<foreignFile<<endl;
			exit(1);
		}

		string parse_string;
		string a_string;
		string f_string;
		size_t lineno = 0;

		int nbAuxNodes = 0;
		int nbFactAuxNodes = 0;
    	ifstream infile(parsesFile.c_str());
    	if(!infile){
			cerr<<"Can not open file "<<parsesFile<<endl;
			exit(1);
    	}

		// mtrule::TreeNode::find_bad_align  = true;
		while(getline(infile, parse_string)){
			++lineno;
			if(lineno % 1000 == 0){
				cerr<<"\%\%\% COMPUTING"<<lineno<<endl;
			}
			if(!getline(alnfile, a_string)){
				cerr<<"no word alignments any more!\n";
				exit(1);
			}
			if(!getline(foreignfile, f_string)){
				cerr<<"no foreign lines any more!\n";
				exit(1);
			}

			//mtrule::Tree t;
			string line_bk = parse_string;
			mtrule::Tree t(parse_string, "radu-new");
			if(t.get_leaves() == NULL){ cout<<line_bk<<endl; continue; }

			std::string e_string = t.get_string();
			mtrule::Alignment a(a_string, e_string, f_string);

		    mtrule::TreeNode *root = t.get_root();
			mtrule::TreeNode::assign_alignment_spans(root,&a);
			mtrule::TreeNode::assign_complement_alignment_spans(root,&a);
			mtrule::TreeNode::assign_frontier_node(root);

			count(t.get_root(), nbAuxNodes, nbFactAuxNodes );

		}

		cout<<nbAuxNodes<<" " <<nbFactAuxNodes<<" " <<((float)nbFactAuxNodes)/((float)nbAuxNodes)<<endl;

#if 0
		for(map<int, size_t>::iterator it = fanout_to_node_count.begin();
				it != fanout_to_node_count.end(); ++it){
			cout<<it->first <<"    "<<it->second<<endl;
		}
#endif

}


