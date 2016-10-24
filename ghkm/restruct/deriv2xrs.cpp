#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include "Restructurer.h"
#include "db_access.h"
#include "IntDer.h"

const char *CVSID="$Id: deriv2xrs.cpp,v 1.1 2006/12/04 22:22:05 wang11 Exp $";
const char *VERSION="v1.0";

using namespace boost;
using namespace std;
using namespace boost::program_options;

bool help;
string derivFile;
string dbFile;


int main(int argc, char **argv)
{
    options_description general("General options");
    general.add_options() 
		("help,h",    bool_switch(&help), "show usage/documentation")
        ("deriv-file,d",  value<string>(&derivFile)->default_value(""),
	         "the integerized deriv file. one deriv per line.")
        ("db,D",  value<string>(&dbFile)->default_value(""),
	         "the DB file.");

    variables_map vm;
    store(parse_command_line(argc, argv, general), vm);
    notify(vm);


	cerr<<"# "<<CVSID<<" "<<VERSION<<endl;
    if(help || argc == 1){ cerr<<general<<endl; exit(1); }

	/* open the db */
    DB *dbp;
    db_init(dbp);
	dbp->set_errfile(dbp, stderr); 
    db_open(dbp, dbFile.c_str(), true);



	ifstream derivF(derivFile.c_str());
	if(!derivF) {
		cerr<<"Cannot open "<<derivFile<<" for reading\n";
		exit(1);
	}

	string line;
	while(getline(derivF, line)){
		cout<<"<deriv> "<<line<<" </deriv>\n";
		cout<<"<XRS>\n";
		istringstream ist(line);
		IntDer der;
		ist >>  der;

		IntDer::iterator it;
		for(it = der.begin(); it != der.end(); ++it){
			ostringstream ost;
			ost<<*it;
			const char* ruleStr = 
			   db_lookup(dbp,const_cast<char*>(ost.str().c_str()));
		    if(ruleStr){
				   cout<<ruleStr<<endl;
			} else {
			   cerr<<"CANNOT FIND XRS for "<<*it<<endl;
			}
		}
		cout<<"</XRS>\n";
	}

    db_close(dbp);
	derivF.close();
}
