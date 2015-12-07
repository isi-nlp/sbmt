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
#include "forestMethods.h"
#include "node.h"

using namespace std;
//this function will read the forest string , which is passed as iterators in a recursive descent fashion

// regular expressions for extracting the forest string
const boost::regex feature_and_feature_vector("(.*)<(.*)>");
const boost::regex comma(",");
const boost::regex colon("(.*):(.*)");

//this function will return true if the forest has not been declared and will return true if the forest is declared
bool inline forestNotDeclared(map<int,Node *> * forest_references_dictionary,int forest_reference_id)
{
	map<int,Node *>::iterator find_forest = (*forest_references_dictionary).find(forest_reference_id);
	if (find_forest == (*forest_references_dictionary).end())
	{
		return(true);
	}
	else
	{
		return(false);
	}
}	

struct Node * readForest(boost::sregex_token_iterator * i,boost::sregex_token_iterator* end,vector<Node *> * forest_references,int * num_nodes,map<string,float> * final_weight_vector, map<int,string> *node_feature_vector_dictionary,map<int,Node *> * forest_references_dictionary)
{	
//	cout << " we are in teh function "<<endl;
	(*i)++;				
	string label(**i);
//	cout << "the label in the function was " << label <<endl;
	struct Node * curr_root;
	if (label == "OR")
	{
//		cout <<"we saw an or node "<<endl;
		curr_root = createNode(-1,true,false,"",final_weight_vector);
	}
	else if (label[0] == '#')
	{
		int id = getForestReferenceId(label);
		//int id = atoi(&str_id);
//		cout << "the reference id is "<<id<<endl; 
		//Node * true_child = (*forest_references)[id -1];
		map<int,Node*>::iterator find_child = (*forest_references_dictionary).find(id);
		if (find_child == (*forest_references_dictionary).end())
		{
			cout<<"the forest reference "<<id<<"shoud have been there in the forest referneces dictionary but it wasnt"<<endl;
			exit(0);
		}
		Node * true_child = find_child->second;
		curr_root = createNode(id,false,true,"",final_weight_vector);
		curr_root->true_node_ref = true_child;

	}
	else //it is a regular rule and we have to update the feature vector
	{
		*num_nodes += 1;
		//cout<<"the label is"<<endl;
		//cout<<label<<endl;
		//getchar();

		curr_root = processRegularRule(label,final_weight_vector,node_feature_vector_dictionary);
		//curr_root->dot_product = calcDotProduct(final_weight_vector,& (curr_root->feature_vector));
		
	}
			
	(*i)++;
	
	//cout<<"curr root id is "<<curr_root->id<<endl;
	//now the loop that will go over its children and recursively process the forest
	while(true)
	{
		Node * child;
		string label(**i);
//		cout << " the label in the loop is "<<label<<endl;
		if (label == "(")
		{
			child = readForest(i,end,forest_references,num_nodes,final_weight_vector,node_feature_vector_dictionary,forest_references_dictionary);
//			cout << "inserting node for label "<<curr_root->id<<endl;
		//		cout<<"creating dummy or node "<<endl;
			//if (curr_root->is_or_node == false && child->is_or_node == false) //then we need to create a dummy or node
		//	{
			//Node * temp_node = createNode(-1,true,false,"",final_weight_vector);
			//temp_node->children.push_back(child);
			curr_root->children.push_back(child);
			//child->parents.push_back(curr_root);
			child->parents.insert ( pair<Node*,int>(curr_root,1));
				
		//	}
		//	else
		//	{
		//		curr_root->children.push_back(child);
		//	}

		//		cout<<"adding a dummy node to "<<curr_root->id<<endl;
		}
		else if (label == ")")
		{
			break;
		}
		else if (label == "OR")
		{
			child = createNode(-1,true,false,"",final_weight_vector);
			curr_root->children.push_back(child);
			//child->parents.push_back(curr_root);

			child->parents.insert ( pair<Node*,int>(curr_root,1));
		}
		else if (label[0] == '#')
		{
			int forest_reference_id = getForestReferenceId(label);
			
//			cout <<"it was a forest reference" <<endl;
			boost::sregex_token_iterator temp_iterator = (*i);
			(*i)++;
			//if (**i == "(" && (*forest_references).size() <= forest_reference_id -1 ) //this means a new forest is being declared
			if (**i == "(" && forestNotDeclared(forest_references_dictionary,forest_reference_id))
			{
		
				forest_references->push_back(NULL); //since we know a forest is going to be defined, we need to take up that space
				child = readForest(i,end,forest_references,num_nodes,final_weight_vector,node_feature_vector_dictionary,forest_references_dictionary);
				/*
				if (curr_root->is_or_node == false && child->is_or_node == false) //then we need to create a dummy or node
				{
					Node * temp_node = createNode(-1,true,false,"",final_weight_vector);
					temp_node->children.push_back(child);
					curr_root->children.push_back(temp_node);
					(*forest_references)[forest_reference_id-1] = child;
				}
				else
				{
				*/
				child->forest_reference_number = forest_reference_id;
				curr_root->children.push_back(child);
			//	child->parents.push_back(curr_root);

				child->parents.insert ( pair<Node*,int>(curr_root,1));
						//forest_references->push_back(child);
				//(*forest_references)[forest_reference_id-1] = child;
				(*forest_references_dictionary).insert(pair<int,Node*>(forest_reference_id,child));
				
						//cout<<"adding the id "<<child->id<<" to the forest references and the size of forest references is "<<forest_references->size()<<endl;
					//curr_root->children.push_back(child);	
				//}
			}
			else 
			{
				(*i) = temp_iterator; //decrementing it because we incremented it before to check if the next symbol was ( or not
				//int id = getForestReferenceId(label);
				child = createNode(forest_reference_id,false,true,"",final_weight_vector);
			//	cout<<"we are gettingt eh true child"<<endl;
			//	cout<<"the forest reference id is "<<forest_reference_id<<endl;
			//	cout<<"the size of forest references is "<<(*forest_references).size()<<endl;
				map<int,Node*>::iterator find_child = (*forest_references_dictionary).find(forest_reference_id);
				if (find_child == (*forest_references_dictionary).end())
				{
					cout<<"the forest reference "<<forest_reference_id<<"shoud have been there in the forest referneces dictionary but it wasnt"<<endl;
					exit(0);
				}
				Node * true_child = find_child->second;
				//Node * true_child = (*forest_references)[forest_reference_id-1]; //checking if we need to add a dummy or node or not
				child->true_node_ref = true_child;
				//true_child->parents.push_back(curr_root);

				true_child->parents.insert ( pair<Node*,int>(curr_root,1));
				//child->parents.push_back(curr_root);

				child->parents.insert ( pair<Node*,int>(curr_root,1));
			//	cout<<"we got the true child"<<endl;
				/*
				if (curr_root->is_or_node == false && true_child->is_or_node==false) //then you need to add a dummy or node
				{
			//		cout<<"we are in teh if statement"<<endl;
					Node * temp_node = createNode(-1,true,false,"",final_weight_vector);
					temp_node->children.push_back(child);
					curr_root->children.push_back(temp_node);
				}
				else
				{
				*/
				curr_root->children.push_back(child);
				//}

//				curr_root->children.push_back(child);
			}
//			cout<<"inserting node for label"<<curr_root->id<<endl;
		}
		else //it is a regular rule and we need to process the regular rule
		{
			*num_nodes += 1 ;
			//cout<<"the label is "<<endl;
			//cout<<label<<endl;
			//getchar();

			child = processRegularRule(label,final_weight_vector,node_feature_vector_dictionary);
//			cout<<"inserting node for label"<<curr_root->id<<endl;
			/*
			if (curr_root->is_or_node == false) //then we need to insert a dummy or node
			{
				Node * temp_node = createNode(-1,true,false,"",final_weight_vector);
				temp_node->children.push_back(child);
				curr_root->children.push_back(temp_node);
			}
			else
			{
			*/
			curr_root->children.push_back(child);
			//child->parents.push_back(curr_root);

			child->parents.insert ( pair<Node*,int>(curr_root,1));
			//}
			//child->dot_product = calcDotProduct(final_weight_vector,&(child->feature_vector));
		}
		(*i)++;	
	}

	return (curr_root);
}

inline int getForestReferenceId(string label)
{
	string str_id = label.substr(1,(label.length()-1));
	std::istringstream s(str_id);
	int id;
	if (!(s >> id)) 
	{
		cout << "could not convert from string " << str_id <<" to integer "<<endl;
		exit(0);
	}
//	cout <<"the forest reference id is " << id <<endl;
	return(id);
}

inline Node * processRegularRule(string label,map<string,float> * final_weight_vector,map<int,string> * node_feature_vector_dictionary)
{
	Node *node;
	string str_id;
	string feature_vector;
	boost::smatch what;
	if (boost::regex_match(label, what , feature_and_feature_vector,boost::match_extra))
	{
//		cout <<"the regular expression matched"<<endl;
		//what[1] is the id and what[2] is the feature vector
		str_id = what[1];
		feature_vector = what[2];	
	}
	else 
	{
		cout <<" the regular expression for getting the id and feature vector of a regular rule did not match for the label "<<label <<endl;
		exit(0);
	}			
	std::istringstream s(str_id);
	int id;
	if (!(s >> id)) 
	{
		cout << "could not convert from sting " << label << " to integer"<<endl;		
		exit(0);
	}
	(*node_feature_vector_dictionary).insert(pair<int,string>(id,feature_vector));
	//int id = atoi(&str_id);
	//cout << "the reference id is "<<id<<endl; 
	node = createNode(id,false,false,feature_vector,final_weight_vector);
	return(node);
}


//
struct Node * createNode(int id,bool is_or_node,bool is_forest_reference,const string feature_vector_string,map<string,float> * final_weight_vector)
{
	Node * node  = new Node;
	node->id = id;
	node->rtg_symbol = -1; //this will actually indicate if this node has been visited or not
	node->forest_reference_number = -1;
	node->is_or_node = is_or_node;
	node->is_forest_reference = is_forest_reference;
	//node->feature_vector_string = feature_vector_string;
	//node->pst_probability = 0.0;
	map<string,float> feature_vector;

	/*this is not true. if the node does not have a gt prob, that is when the node is an unknown node' 
	
	if (is_or_node == false && is_forest_reference == false && (id <=0 || id > MAX_NODE_ID))
	{
		//cout<<"the node was declared as an unknown node"<<endl;
		//getchar();
		//cout<<"the id is "<<id<<endl;
		node->is_unknown_node = true;
	}
	else
	{
		node->is_unknown_node = false;
	}
	*/

	bool unknown_node_flag = true;
	node->is_unknown_node = true;
	if (feature_vector_string != "")//we have a feature vector and we need to separate the items in it
	{
		//cout<<"the feature vector string is "<<endl;
		//cout<<feature_vector_string<<endl;
		//getchar();

		boost::sregex_token_iterator  i ( feature_vector_string.begin(), feature_vector_string.end(), comma, -1 ), j;
		while (i!=j)
		{
			string feature_value_pair (*i);
			//now to get the feature and the value from the pair
			boost::smatch what;
			if (boost::regex_match(feature_value_pair, what , colon,boost::match_extra))
			{
//				cout <<"the regular expression for feature value pair  matched"<<endl;
				//what[1] is the id and what[2] is the feature vector
				string  feature = what[1];
				string value_string = what[2];	
				istringstream s(value_string);
				float feature_value;
				if (!(s >> feature_value))
				{
					cout << " could not convert from string "<<value_string<<" to float "<<endl;
					exit(0);
				}
				//so, now we have the feature value and string and we just need to put it into the feature vector map
				node->feature_vector.insert(pair<string,float>(feature,feature_value));
				if (feature == "gt_prob")
				{
					unknown_node_flag = false;
					node->is_unknown_node = false;
				}
			}
			else 
			{
				cout << "the regular expression for feature value pair did not match fore the feature value string " << feature_vector_string<<endl;
				exit(0);
			}
		
			i++;
		}
		//node->dot_product = calcDotProduct(final_weight_vector,&node->feature_vector);
		//feature_vector.~map();//deleting the feature vector that was just created because we dont need it
	}
	else
	{
		//node->dot_product = 0;
	}	
	return(node);
}



void printForest(Node * node)
{
	cout << "the node is "<<endl;
	printNode(node);
	if (node->children.size() > 0)
	{
		cout <<"children"<<endl;
		Node::children_collection::iterator itr;
		itr = node->children.begin();
		while(itr != node->children.end())
		{
			printNode((*itr));
			//cout << "the child is "<< (*itr)->id <<endl;
			itr++;
		}
		//now calling the same function on the children
		itr = node->children.begin();
		while(itr != node->children.end())
		{
			printForest(*itr);
			itr++;
		}
	
	}

	else 
	{
		cout <<"The node does not have any children"<<endl;
	}


}

inline void printNode(Node * node)
{
	//cout <<"the node is"<<endl;
	if (node->is_or_node == true)
	{
		cout << "OR" <<endl;
	}
	else if(node->is_forest_reference == true)
	{
		Node * true_node = node->true_node_ref;
		cout <<"#"<<node->id<<" forest reference"<<endl;
		printNode(true_node);
	}
	else {
		cout <<node->id<<endl;
		//printFeatureVector(node->feature_vector);
	}

}

inline void printFeatureVector(map<string,float> feature_vector)
{
	map<string,float>::const_iterator itr;
	itr = feature_vector.begin();
	while(itr != feature_vector.end())
	{
		cout <<itr->first<<":"<<itr->second<<endl;
		itr++;
	}	
}

//this will free up the memory that is being used up by the forest



void readFinalWeightVector(map<string,float> * final_weight_vector,char * weight_vector_file_name)
{
	ifstream weight_vector_file(weight_vector_file_name);
	//forest_file.open(argv[1]);
	if (!weight_vector_file.is_open())
	{
		cout <<  "could not open final weight vector file \n";
		exit(0);
	}
	string line;
	getline(weight_vector_file,line);
	extractFeatureVectorString(line,final_weight_vector);
	weight_vector_file.close();
}

//this takes a feature vector string ane extracts teh feature value pairs and inserts it in a map
void extractFeatureVectorString(string feature_vector_string,map<string,float> * feature_vector)
{

	boost::sregex_token_iterator  i ( feature_vector_string.begin(), feature_vector_string.end(), comma, -1 ), j;
	while (i!=j)
	{
		string feature_value_pair (*i);
		//now to get the feature and the value from the pair
		boost::smatch what;
		if (boost::regex_match(feature_value_pair, what , colon,boost::match_extra))
		{
//			cout <<"the regular expression for feature value pair  matched"<<endl;
			//what[1] is the id and what[2] is the feature vector
			string  feature = what[1];
			string value_string = what[2];	
			istringstream s(value_string);
			float feature_value;
			if (!(s >> feature_value))
			{
				cout << " could not convert from string "<<value_string<<" to float "<<endl;
				exit(0);
			}
			//so, now we have the feature value and string and we just need to put it into the feature vector map
			feature_vector->insert(pair<string,float>(feature,feature_value));
		}
		else 
		{
			cout << "the regular expression for feature value pair did not match fore the feature value string " << feature_vector_string<<endl;
			exit(0);
		}
	
		i++;
	}
}

//returns the dot product between two vectors
float calcDotProduct(map<string,float> * final_weight_vector, map<string,float> * feature_vector)
{
	float dot_product = 0;
	map<string,float>::iterator final_weight_vector_itr,find_item;
	final_weight_vector_itr = final_weight_vector->begin();
	for (/*final_weight_vector_itr = final_weight_vector->begin()*/;final_weight_vector_itr != final_weight_vector->end();final_weight_vector_itr++)
	{
		find_item = feature_vector->find((*final_weight_vector_itr).first);
		if (find_item != feature_vector->end()) //if the item has been found
		{
			dot_product = dot_product + (*final_weight_vector_itr).second * (*find_item).second;
		}	
	}
	return(dot_product);
}

inline string trim(const string& o) {
  string ret = o;
  const char* chars = "\n\t\v\f\r ";
  ret.erase(ret.find_last_not_of(chars)+1);
  ret.erase(0, ret.find_first_not_of(chars));
  return ret;
}

void countNodes (Node * node, long int *count)
{
	if (node->children.size() ==0)
	{
		*count += 1;
	}
	else
	{
		Node::children_collection::iterator itr;
		for (itr = node->children.begin(); itr != node->children.end();itr++)
		{
			countNodes (*itr,count);
		}	
	
		*count += 1;
	}
}


//we are removing the forest references that were created because we don't need them
void removeForestReferences(Node * node) 
{
	//cout<<"we are in remove forest references"<<endl;
	Node::children_collection temp_children;
	Node::children_collection::iterator child_itr;
	bool reference_found_flag = false;
	for (child_itr = node->children.begin();child_itr != node->children.end();child_itr++)
	{
		if ((*child_itr)->is_forest_reference == true)
		{
			reference_found_flag = true;
			//temp_children.push_back((*child_itr)->true_node_ref);
			//cout<<" we found a forest reference"<<endl;
			//replacing the reference with the true child
			(*child_itr)->parents.clear();
			(*child_itr)->children.clear();
			//Node *true_node_ref = (*child_itr)->true_node_ref;

			*child_itr = (*child_itr)->true_node_ref;
			/*
			if (true_node_ref->id == 4261863)
			{
				cout<<"The node was 42 blah and the parent was "<<node->id<<endl;
				cout<<"the node address was "<<true_node_ref<<endl;
				cout<<"the node address in the parent is "<<*child_itr<<endl;
				cout<<"the address of the parent is "<<node<<endl;
				getchar();
			}
			*/
		//	delete *child_itr;
			//cout<<" we deleted the forest reference"<<endl;
		}
		else 
		{

			removeForestReferences(*child_itr);
			//temp_children.push_back(*child_itr);
		}
	}
	/*
	if (reference_found_flag == true)
	{
		//so, we have to empty the children list of the node 
		node->children.clear();
		node->children.insert(node->children.begin(),temp_children.begin(),temp_children.end());
	}
	
	for (child_itr = node->children.begin();child_itr != node->children.end();child_itr++)
	{
		removeForestReferences(*child_itr);
	}
	*/
}


