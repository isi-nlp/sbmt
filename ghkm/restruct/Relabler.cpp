#include "Relabler.h"
#include <fstream>

using namespace std;


Relabeler::Relabeler(const string labelfile) 
{ 
    if(labelfile != ""){
	ifstream infile(labelfile.c_str());
	string line;
	while(getline(infile,  line)){
	    m_labels.insert(line);
	}
    }
}

void Relabeler::
relabel(Tree& tree)
{
    Tree::pre_order_iterator i, j;
    for(i = tree.begin(); i != tree.end(); ++i){
	string l = "";
	j = tree.parent(i);
	if(tree.is_valid(j)){
	    l = i->label + "^" + j->label;
	    if(m_labels.find(l) != m_labels.end()){
		i->relabel = l;
	    } else {
		i->relabel = i->label;
	    }
	} else {
		i->relabel = i->label;
	}
    }
    for(i = tree.begin(); i != tree.end(); ++i){
	i->label = i->relabel;
    }
}

void Relabeler::
relabel(string parseFile)
{
    ifstream infile(parseFile.c_str());
    if(!infile){
	cerr<<"Can not open file "<<parseFile<<endl;
	exit(1);
    }

    string line;
    while(getline(infile, line)){
	Tree tr = getTree(line);
	relabel(tr);
	print(tr,  tr.begin());
	cout<<endl;
    }
}


