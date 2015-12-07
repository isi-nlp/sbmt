#include <iostream>
//#include <istringstream>
#include<sstream>
#include <fstream>
#include <iterator>
#include <algorithm>
//#include "node.h"
#include <list>
#include <string>
#include <vector>
#include <boost/regex.hpp>
#include <map>

//creating the functions here that will work on the forest
//
//function declaratinos
//
/*
///CHANGE : THE DATA STRUCTURE THAT WILL HOLD THE PARENTS WILL BE A MAP AND NOT A LIST. THIS WILL ALLOW FOR EASY DELETION
//struct Node * readForest(boost::sregex_token_iterator *,boost::sregex_token_iterator*,vector<Node *> * forest_references,int * ,map<string,float> *,map<int,string> *,map<int,Node *>* );
struct Node * createNode(int,bool,bool,const string,map<string,float> *);
inline Node * processRegularRule(string,map<string,float> *,map<int,string> * node_feature_vector_dictionary);
inline int getForestReferenceId(string);
void printForest(Node *);
void printForestContents(Node);
void countDerivations(Node*);
void countDerivationsAfterSplitting(Node );
inline void printNode(Node*);
inline void printFeatureVector(map<string,float>);
inline string trim(const string & );
inline void deleteParent(Node *,Node *);
void deleteSubtree(Node *);
void freeMemory(Node *);
void createParentLists(Node * , map<int,Node>*);
void removeForestReferences(Node *);
//void getOrNodeHyperarcs(Node *, vector<Node *>, int, list<list<Node *> > *);
//void getRegularNodeHyperarcs(Node *, vector<Node *>, int, list<list<Node *> > *);
//inline void initHyperarcs(Node *,vector<Node *>, list< list<Node *> > *);
//void printHyperarcs(Node *);
void createHyperarcArrays(Node *,const vector<Node *> *);
//declaring constants that i will be using

void readFinalWeightVector(map<string,float> *,char *);
void extractFeatureVectorString(string,map<string,float> *);
float inline calcDotProduct(map<string,float> * , map<string,float> *);
void getTotalHyperarcs(Node *, int *, const vector<Node *> *);
void printForest (Node * , ostream * , map<int,int> * ,int * );
//function definitions
//
//this function will read the forest string , which is passed as iterators in a recursive descent fashion

//this function will return true if the forest has not been declared and will return true if the forest is declared
*/
struct Node;

bool forestNotDeclared(std::map<int,Node *> * forest_references_dictionary,int forest_reference_id);

Node* readForest( boost::sregex_token_iterator * i
                , boost::sregex_token_iterator* end
                , std::vector<Node*>* forest_references
                , int* num_nodes
                , std::map<std::string,float>* final_weight_vector
                , std::map<int,std::string>* node_feature_vector_dictionary
                , std::map<int,Node*>* forest_references_dictionary
                );


int getForestReferenceId(std::string label);

Node* processRegularRule( std::string label
                        , std::map<std::string,float>* final_weight_vector
                        , std::map<int,std::string>* node_feature_vector_dictionary
                        );

//
Node* createNode( int id
                , bool is_or_node
                , bool is_forest_reference
                , const std::string feature_vector_string
                , std::map<std::string,float> * final_weight_vector
                );


void printForest(Node * node);
void printNode(Node * node);
void printFeatureVector(std::map<std::string,float> feature_vector);

//this will free up the memory that is being used up by the forest
void readFinalWeightVector(std::map<std::string,float> * final_weight_vector,char * weight_vector_file_name);

//this takes a feature vector string ane extracts teh feature value pairs and inserts it in a map
void extractFeatureVectorString(std::string feature_vector_string,std::map<std::string,float> * feature_vector);

//returns the dot product between two vectors
float calcDotProduct(std::map<std::string,float> * final_weight_vector, std::map<std::string,float> * feature_vector);

std::string trim(const std::string& o);

void countNodes (Node* node, long int* count);


//we are removing the forest references that were created because we don't need them
void removeForestReferences(Node * node);


