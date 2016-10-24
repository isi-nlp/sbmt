#include "RegexRestructurer.h"
#include "Transformer.h"

using namespace std;


RegexRestructurer::RegexRestructurer(string regex, string newroot) : 
    m_regex(regex), m_newroot(newroot)
{ }

void RegexRestructurer::
restructure(Tree::pre_order_iterator it,
	    Tree& tree)
{
    int start_NT_posit;
    int end_NT_posit;
    for(Tree::sibling_iterator itsib = it.begin(); itsib != it.end(); 
		++itsib){
	    restructure(itsib, tree);
    }
    if(it != tree.end()){
	while(1){
	    string idstring = getIDString(it, tree);
	    if(m_regex.search(idstring) == true) {
		if(m_regex.matches() >=1) {
		    int start = m_regex.get_match_start(0);
	            int end = m_regex.get_match_end(0);

		    if(idstring[start] == ' '){start++;}

		    //cerr<<"start: "<<start<<endl;
		    // get the start NT position.
		    string ts = idstring.substr(0, start);
		    Pcre tmpregex("\\s+");
		    vector<string> vs = tmpregex.split(ts);
		    assert(vs.size());
		    start_NT_posit = vs.size() - 2;

		    // get the end NT position.
		    string te = idstring.substr(0, end);
		    Pcre tmpregex1("\\s+");
		    vs = tmpregex1.split(te);
		    end_NT_posit = vs.size() - 2;

		    //cerr<<idstring<<endl;
		    //cerr<<start_NT_posit<<":"<<end_NT_posit<<endl;

		    // insert a new node before it.begin() + start_NT_pos

		    Tree::pre_order_iterator it_new;
		    Tree::sibling_iterator it_from, it_end;
		    it_from = tree.child(it, start_NT_posit);
		    it_end = tree.child(it, end_NT_posit);
		    it_end++;
		    it_new = tree.insert(it_from, TreeNode(m_newroot, 0));   
                    tree.reparent(it_new, it_from, it_end);
	    } else {
		cerr<<"Cannot get to matches!\n";
		exit(0);
	    }
	} else {
	    break;
	}
	}

    }
}

// NPB\t\tNN\tNN or ""
string RegexRestructurer::
getIDString(Tree::pre_order_iterator it,
	    Tree& tree) const
{
    string ret = "";

    if(it != tree.end()){
	ret = ret + it->label +  " ";
	for(Tree::sibling_iterator itsib = it.begin(); itsib != it.end(); 
		++itsib){
	    ret = ret + " "  + itsib->label;
	}
    }
    return ret;
}

