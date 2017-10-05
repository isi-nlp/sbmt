#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <iostream>
#include <fstream>

using namespace boost;
using namespace std;
using namespace boost::program_options;

int main(int argc, char **argv)
{
	boost::regex ruleRe("(.*)\\s\\->\\s.*\\s###");
	boost::regex varRe("\\bx(\\d+):\\S+\\b");

	string line;
	boost::match_results<std::string::const_iterator> what;
	while(getline(cin, line)) {
		//cout<<line<<endl;
		if(0 == boost::regex_match(line, what, ruleRe)) {
			continue;
		} else {
			cout<<what.size()<<endl;
			cout<<what.str(0)<<endl;
		}
	}

    //boost::sregex_token_iterator i(s.begin(), s.end(), re, -1);
	//boost::sregex_token_iterator j;

}


