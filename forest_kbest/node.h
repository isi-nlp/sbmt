/*
 * node.h
 *
 *  Created on: Jan 5, 2009
 *      Author: turing
 */
#ifndef FORESTREADER__NODE_H
#define FORESTREADER__NODE_H
#include <sbmt/forest/implicit_xrs_forest.hpp>
#include <list>
#include <map>
#include <vector>
#include <string>
//#include "nodeContext.h"

//const int MAX_NODE_ID=5095460;
//const int MAX_POSSIBLE_CONTEXT = 2;


struct Node 
{
	Node * original_node_ref; 
	int id;
    int rtg_symbol;
	bool is_or_node; //this flag will go up if the node is an or node 
	bool is_forest_reference; //this flag will go up is the node is a forest reference. What i mean by forest reference is #N where N is a number. In the packed forest format, when you declare a subforest that might be reused, you give it an id #N. If a previously declared forest is being referenced here, then I will create a node with this flag up.
	bool is_unknown_node; //this flag will go up if the node is an unknown node
    typedef std::map<std::string,float> feature_vector_map;
	feature_vector_map feature_vector; //this will hold the components of the feature vector. 
	Node * true_node_ref; //if the node was a forest reference, then this will hold the address of the root of the subforest being referenced (in this case, is_forest_reference will be 'true'
	//double dot_product;
	int forest_reference_number; //you don't need this
    typedef std::vector<Node*> children_collection;
	children_collection children; //a list of node addresses of all the children of a node
	double best_cost; //this stores the inner product of the feature vector and the weight vector. you probably don't need htis
	std::map<Node*,int> parents; //a list of the addresses of all the parents of a node
};
# endif // FORESTREADER__NODE_H

