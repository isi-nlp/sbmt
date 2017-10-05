//creating the structure of the nn in a graph that will help in performing backpropagation and forward propagation
#pragma once

#include <cstdlib>
#include "neuralClasses.h"
#include <Eigen/Dense>
#include <vector>

using namespace std;
namespace nplm
{

template <class X>
class Node {
    public:
        X * param; //what parameter is this
        //vector <Node *> children;
        //vector <void *> parents;
	Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic> fProp_matrix;
	Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic> bProp_matrix;
	Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic> cumul_input_matrix;
	Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic> cumul_grad_matrix;
	int minibatch_size;

    public:
        Node() : param(NULL), minibatch_size(0) { }

        Node(X *input_param, int minibatch_size)
	  : param(input_param),
	    minibatch_size(minibatch_size)
        {
	    	resize(minibatch_size);
        }

	void resize(int minibatch_size)
	{
	    this->minibatch_size = minibatch_size;
	    if (param->n_outputs() != -1)
	    {
	        fProp_matrix.setZero(param->n_outputs(), minibatch_size);
			//cumul_fProp_matrix.setZero(param->n_inputs(),minibatch_size);
	    }
            if (param->n_inputs() != -1)
            {
	        bProp_matrix.setZero(param->n_inputs(), minibatch_size);
            }
	}

	void resize() { resize(minibatch_size); }
	
	 
	//void addChild(Node *child){
	//	children.push_back(child);
};

template <class X>
class Node {
    public:
	Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic> fProp_matrix;
	Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic> bProp_matrix;
	Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic> cumul_input_matrix;
	Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic> cumul_grad_matrix;
	int minibatch_size;
	vector<Node *> chidren;
	vector<Node *> parents;
    public:
        Node() :minibatch_size(0) { }

        Node(int minibatch_size)
	  	  : minibatch_size(minibatch_size)
        {
	    	resize(minibatch_size);
        }

	void resize(int minibatch_size)
	{
	    this->minibatch_size = minibatch_size;
	    if (param->n_outputs() != -1)
	    {
	        fProp_matrix.setZero(param->n_outputs(), minibatch_size);
			//cumul_fProp_matrix.setZero(param->n_inputs(),minibatch_size);
	    }
            if (param->n_inputs() != -1)
            {
	        bProp_matrix.setZero(param->n_inputs(), minibatch_size);
            }
	}

	void resize() { resize(minibatch_size); }
	
	void accumulateInput() {
		this->cumul_input_matrix.setZero();
		for (int i=0; i<children.size(); i++){
			this->cumul_input_matrix +=  children[i]->fProp_matrix;
		}
	}
	
	void accumulateGrad() {
		this->cumul_grad_matrix.setZero();
		for (int i=0; i<parents.size(); i++){
			this->cumul_grad_matrix +=  parents[i]->bProp_matrix;
		}
	}	 
	//void addChild(Node *child){
	//	children.push_back(child);
};


class HiddenNode public Node{
};
template <class X,class Y>
class AdderNode {
    public:
        X * param; //what parameter is this
        vector <Node<Y> *> children;
        //vector <void *> parents;
	Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic> fProp_matrix;
	Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic> bProp_matrix;
	Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic> cumul_input_matrix	;
	int minibatch_size;

    public:
        AdderNode() : param(NULL), minibatch_size(0) { }

        AdderNode(X *input_param, int minibatch_size)
	  : param(input_param),
	    minibatch_size(minibatch_size)
        {
	    	resize(minibatch_size);
        }

	void resize(int minibatch_size)
	{
	    this->minibatch_size = minibatch_size;
	    if (param->n_outputs() != -1)
	    {
	        fProp_matrix.setZero(param->n_outputs(), minibatch_size);
			cumul_input_matrix.setZero(param->n_inputs(),minibatch_size);
	    }
            if (param->n_inputs() != -1)
            {
	        	bProp_matrix.setZero(param->n_inputs(), minibatch_size);
				cumul_grad_matrix.setZero(param->n_outputs(), minibatch_size);
            }
	}

	void resize() { resize(minibatch_size); }
	
	 
	
	
	void accumulateInput() {
		this->cumul_input_matrix.setZero();
		for (int i=0; i<children.size(); i++){
			this->cumul_input_matrix +=  children[i]->fProp_matrix;
		}
	}
	
	void accumulateGrad() {
		this->cumul_grad_matrix.setZero();
		for (int i=0; i<children.size(); i++){
			this->cumul_grad_matrix +=  parents[i]->bProp_matrix;
		}
	}

};

class Accumulator Node
} // namespace nplm
