#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include "Restructurer.h"

const char *CVSID="$Id: restruct.cpp,v 1.7 2006/12/04 22:22:05 wang11 Exp $";
const char *VERSION="v4.0";

using namespace boost;
using namespace std;
using namespace boost::program_options;

bool help;
string parsesFile;
string alnFile;
string foreignFile;
string restructType;

int factorLimit;
string exceptList;

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
	         "the foreign text.")
        ("factor-limit,l",  value<int>(&factorLimit)->default_value(5),
	         "the maximum number of children that can factored out (only applicable to parallel binarization.")
        ("except-list,e",  value<string>(&exceptList)->default_value(""),
	         "list of non-terminals for which we dont apply the factor limit (-l).")
        ("restr-type,s",  
			  value<string>(&restructType)->default_value("left-branching"),
	         "the type of restructuring") ;

    variables_map vm;
    store(parse_command_line(argc, argv, general), vm);
    notify(vm);


	cerr<<"# "<<CVSID<<" "<<VERSION<<endl;
    if(help || argc == 1){ cerr<<general<<endl; exit(1); }



	Binarizer* binarizer = NULL;
	if(restructType == "left-branching"){
		binarizer = new LeftBranchBinarizer;
	} else if (restructType == "right-branching"){
		binarizer = new RightBranchBinarizer;
	} else if (restructType == "head-outwards"){
		binarizer = new HeadOutwardsBinarizer;
	} else if(restructType == "parallel"){
		binarizer = new ParallelBinarizer;
	}
	
    ifstream infile(parsesFile.c_str());
    if(!infile){
		cerr<<"Can not open file "<<parsesFile<<endl;
		exit(1);
    }

	if(restructType != "parallel"){
		string line;
		while(getline(infile, line)){
			::Tree tr;
			string line_bk = line;
			int ret = tr.read(line);
			if(ret){
				cout<<line_bk<<endl;
				continue;
			} else if(tr.begin() == tr.end()){
				cout<<"0\n";
				continue;
			}

			binarizer->restruct(tr);
			print(tr,  tr.begin());
			cout<<endl;
		   // cout<<line_bk<<endl;
		}
	}else{ 
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


		static_cast<ParallelBinarizer*>(binarizer)->setFactorLimit(factorLimit);

		if(exceptList != ""){
			static_cast<ParallelBinarizer*>(binarizer)->
				                                readExceptCategs(exceptList);
		}

		string parse_string;
		string a_string;
		string f_string;
		size_t lineno = 0;
		while(getline(infile, parse_string)){
			++lineno;
			cerr<<"\%\%\% BINARIZING "<<lineno<<endl;
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
			Alignment a(a_string, e_string, f_string);

			static_cast<ParallelBinarizer*>(binarizer)->setAln(&a);

		    mtrule::TreeNode *root = t.get_root();
			mtrule::TreeNode::assign_alignment_spans(root,&a);
			mtrule::TreeNode::assign_complement_alignment_spans(root,&a);
			mtrule::TreeNode::assign_frontier_node(root);

            SimplePackedForest forest;
			binarizer->restruct(t, forest);
			// we dont dispaly the ROOT.
			forest.changeToNextRoot();
            forest.put(cout);
			cout<<endl;
		}
	}

	delete binarizer;
}

