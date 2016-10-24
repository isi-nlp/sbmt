#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include "Restructurer.h"

using namespace boost;
using namespace std;
using namespace boost::program_options;

int main(int argc, char **argv)
{
	    Tree2ForestConverter converter;
	
		string parse_string;
		size_t lineno = 0;
		while(getline(cin, parse_string)){
			++lineno;
			if(lineno % 10 == 0){
				cerr<<"\%\%\% transforming "<<lineno<<endl;
			}
			//mtrule::Tree t;
			string line_bk = parse_string;
			mtrule::Tree t(parse_string, "radu-new");
			if(t.get_leaves() == NULL){ cout<<line_bk<<endl; continue; }

            SimplePackedForest forest;
			converter.restruct(t, forest);
			// we dont dispaly the ROOT.
			forest.changeToNextRoot();
            forest.put(cout);
			cout<<endl;
		}

}

