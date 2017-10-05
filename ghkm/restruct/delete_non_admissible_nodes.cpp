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

void delete_non_admissible_nodes(mtrule::TreeNode* node) {
	// then, delete me.
	if(!node->is_extraction_node() && node->is_internal()){

		// delete myself from the parent.
		bool is_head = false;
		mtrule::TreeNodes& children= node->get_subtrees();
		mtrule::TreeNodes& siblings = node->parent()->get_subtrees();
		mtrule::TreeNodes::iterator it;

		if(siblings[node->parent()->get_head_index()] == node){
			is_head = true;
		}

		int offset = 0;
		it = siblings.begin();
	    while(it != siblings.end()){
			offset ++;
			if(*it == node){
				it = siblings.erase(it);
				break;
			} else {
				it++;
			}
		}


		// move my chldren to the parent
		for(int i = (int)children.size() -1;  i >= 0; --i){
			it=siblings.insert(it, children[i]);
			children[i]->parent() = node->parent();
		}
		// fix the head indexes and other position information.
		if(is_head){
			node->parent()->set_head_index(offset + node->get_head_index() -1);
		} else {
			if(offset -1 < node->parent()->get_head_index()) {
				node->parent()->set_head_index(node->parent()->get_head_index()+ node->get_nb_subtrees() -1);
			}
		}

	}
	// first delete those non-admissible in children.
	vector<mtrule::TreeNode*> children_nodes;
	for(int i  = 0; i < node->get_nb_subtrees(); ++i){
		children_nodes.push_back(node->get_subtree(i));
	}
	for(int i  = 0; i < children_nodes.size(); ++i){
		delete_non_admissible_nodes(children_nodes[i]);
	}
		//cout<<"#CHILDREN"<<siblings.size()<<endl;
	if(!node->is_extraction_node() && node->is_internal() ){
		node->get_subtrees().resize(0);
		delete node;
	}

}

int main(int argc, char **argv)
{
    options_description general("General options");
    general.add_options() 
		("help,h",    bool_switch(&help), "show usage/documentation")
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
				cout<<"\%\%\% DELETING NON-ADMISSIBLE NODES ON "<<lineno<<endl;
			//if(lineno % 1000 == 0){
			//	cerr<<"\%\%\% COMPUTING"<<lineno<<endl;
			//}
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
			delete_non_admissible_nodes(t.get_root());
			t.get_root()->dump_radu_tree( cout, false);
			cout<<endl;

		}
}


