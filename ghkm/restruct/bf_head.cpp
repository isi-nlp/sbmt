#include <iostream>
#include <fstream>
#include "SyntaxTree.h"
#include "SyntaxTree.h"

using namespace std;

int main(int argc, char **argv)
{
		string line;
		while(getline(cin, line)){
			::Tree ht;
			if(ht.read(line) < 0){
			  cout<<line<<endl; continue; 
			} 
			print(ht, ht.begin(), true);
			cout<<endl;
		}

}

