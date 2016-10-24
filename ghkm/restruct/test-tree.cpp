#include <iostream>
#include "SyntaxTree.h"

using namespace std;
#include <stdlib.h> 
#include <boost/regex.hpp> 
#include <string> 
#include <iostream> 

using namespace boost; 


int main () {


	Tree ht;
	string line;

	while(getline(cin, line)){
		getTree(line, ht);
		Tree::iterator it;
		for(it = ht.begin(); it != ht.end(); ++it){
			if(ht.isHead(it)){
				cout<<it->label<<endl;
			}else {
			}
	}
	}


	return 1;
}

