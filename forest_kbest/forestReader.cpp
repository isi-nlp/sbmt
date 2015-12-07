//#include <boost/lambda/lambda.hpp>
#include <iostream>
#include <fstream>
#include <iterator>
#include <algorithm>
//#include "node.h"
#include <list>
#include <string>
#include <vector>
#include <boost/regex.hpp>
#include <map>
#include <ostream>
//function definitions
#include "node.h"
#include "forestMethods.h"
#include "forest_connection.hpp"
//#include "D_parameters.h"
#include <sbmt/forest/forest_em_output.hpp>

using namespace std;

/* Here is the forest reader. the node structure is in node.h the arguments it takes in are i
 1. the packed forest file one packed forest per line . you can find a sample forest in forest.sample
 2. the weight vector file. You can find a sample weight vector file in final-weights.txt
 
 forest_root is the root of the parsed packed forest. If you look at node.h, I have described what you will find the node structure
*/ 
struct best_cost_sorter {
    bool operator()(Node* n1, Node* n2) const
    {
        return n1->best_cost < n2->best_cost;
    }
};

void sortscore(Node* nd, std::set<Node*>& ndset, map<string,float>& weight_vector)
{
    if (ndset.find(nd) == ndset.end()) {
        ndset.insert(nd);
        
        BOOST_FOREACH(Node* cnd, nd->children) {
            sortscore(cnd,ndset,weight_vector);
        }
        if (nd->is_or_node) {
            std::sort(nd->children.begin(),nd->children.end(),best_cost_sorter());
            nd->best_cost = nd->children.front()->best_cost;
        } else {
            nd->best_cost = calcDotProduct(&weight_vector,&nd->feature_vector);
            BOOST_FOREACH(Node* cnd, nd->children) {
                nd->best_cost += cnd->best_cost;
            }
        }
    }
}

int main(int argc, char * argv[])
{
    size_t N = boost::lexical_cast<int>(argv[4]);
    size_t C = boost::lexical_cast<int>(argv[5]);
    size_t M = boost::lexical_cast<int>(argv[6]);
    std::ifstream gramin(argv[3]);
    xrs_grammar* gram = read_xrs_grammar(gramin);
	string line;
	const boost::regex bracket_match("([\\(\\)])");
	const string replace_string(" $1 ");
	const boost::regex space_match(" +");
	const string space_replace(" ");
	const boost::regex leading_space("^ +");
	const boost::regex trailing_space(" +$");
	map<string,float> final_weight_vector;
//	vector<float> scores;
	readFinalWeightVector(&final_weight_vector,argv[2]);
    gram->weights = weights(gram,final_weight_vector); 
//	readScores(argv[3],&scores);
	
	ifstream forest_file(argv[1]);
	//forest_file.open(argv[1]);
	if (!forest_file.is_open())
	{
		cout <<  "could not open forest file \n";
		exit(0);
	}
	int line_counter = 0;
	while (!forest_file.eof())
	{

		map<int,string> node_feature_vector_dictionary;
		getline(forest_file,line);
		int num_nodes = 0;
		//cout<<"read the line"<<endl;
		string space_cleaned_line = regex_replace(line, bracket_match, replace_string, boost::match_default | boost::format_perl);
		//cout<<"finished doing the first space cleaning"<<endl;
		//removing leading and trailing spaces
		space_cleaned_line = regex_replace(space_cleaned_line,leading_space,"",boost::match_default | boost::format_perl);
		//cout<<"finished doing the second space cleaning"<<endl;
		space_cleaned_line = regex_replace(space_cleaned_line,trailing_space,"",boost::match_default | boost::format_perl);
		//cout<<"finished doing the third space cleaning"<<endl;
		
		map<int,int> forest_id_mapping;
		if (space_cleaned_line == "")
		{
//			cout<<"the line was empty"<<endl;
			continue;
		}
		map<int,Node*> forest_references_dictionary;
		space_cleaned_line = regex_replace(space_cleaned_line,space_match,space_replace,boost::match_default | boost::format_perl);
		boost::sregex_token_iterator  i ( space_cleaned_line.begin(), space_cleaned_line.end(), space_match, -1 ), j; 
		vector<Node *> forest_references;
		map<int,Node *> forest_dictionary;
		Node * forest_root = readForest(&i,&j,& forest_references,& num_nodes,&final_weight_vector,&node_feature_vector_dictionary,&forest_references_dictionary);
		for (int j = 0;j<= forest_references.size();j++)
		{
			forest_dictionary.insert(pair<int,Node*>(j+1,forest_references[j]));
		}

        removeForestReferences(forest_root);
        std::set<Node*> occmap;
        sortscore(forest_root,occmap,final_weight_vector);
        occmap.clear();
        
        sbmt::xforest xf(node_as_xforest(gram,forest_root));

        std::tr1::unordered_map<std::string,size_t> dupmap;
        xtree_generator gen = xtrees_from_xforest(xf,gram);
        int x=0;
        int y=0;
        while (gen and x <= N and y <= M) {
            xtree_ptr t = gen();
            std::string sent = hypsent(t,gram);
            std::tr1::unordered_map<std::string,size_t>::iterator p = dupmap.find(sent);
            if (p == dupmap.end()) dupmap.insert(std::make_pair(sent,0));
            int c = ++(dupmap[sent]);
            if (c <= C) {
                sbmt::feature_vector v = accum(t);
                sbmt::score_t scr = sbmt::geom(v,gram->weights);
                std::cout << "NBEST nbest="<< x++ 
                          << " totalcost=" << sbmt::logmath::neglog10_scale << scr
                          << " hyp={{{" << sent <<"}}}"
                          << " tree={{{" << hyptree(t,*(t->root.rule().lhs_root()),gram) << "}}}"
                          << " derivation={{{" << t << "}}}"
                          << " " << nbest_features(v,gram) << '\n';
            };
            ++y;
        }
	}
	
	forest_file.close();

}

