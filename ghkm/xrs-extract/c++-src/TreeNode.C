#include <sstream>
#include <vector>
#include <algorithm>
#include <treelib/Tree.h>

#include "TreeNode.h"
#include "Features.h"
#include "MyErr.h"

namespace mtrule {
	using namespace std;

/***************************************************************************
 * Static initialization:
 ***************************************************************************/

int  TreeNode::nodes_in_mem=0;          //!< Value to keep track how many 
                                        //!< TreeNode objects have been created
bool TreeNode::find_bad_align = false;

bool TreeNode::lowercasing = true;

/***************************************************************************
 * Constructor and initialization functions 
 ***************************************************************************/

// Daniel 
void TreeNode::set_c_spans(spans cs)
{
   _c_spans.resize(0);
   for(unsigned int i=0; i < cs.size(); i++)
     _c_spans.push_back(std::make_pair(cs[i].first, cs[i].second));
   return;
}

// Daniel
TreeNode::TreeNode(TreeNode* node)
{
    cerr<<"FFF\n";
  ++nodes_in_mem;
  for(TreeNodes_it it=node->get_subtrees().begin(), 
	it_end = node->get_subtrees().end();
      it != it_end; ++it){
    TreeNode *st = *it; // get pointer to the child
    TreeNode *tn = new TreeNode(st); // make copy of the child
    tn->_parent = this;
    _sub_trees.push_back(tn);
  }
  _cat = node->get_cat();
  _word = node->get_word();
  _tag= node->get_tag();
  _head_index = node->get_head_index();
  _head_string_pos = node->get_head_string_pos();
  _e_span = node->get_span();
  _c_spans = node->_c_spans;
  _c_comp_spans = node->_c_comp_spans;
  _extraction_node = node->is_extraction_node();
  _frontier_node = node->is_frontier_node();
  _is_virtual = node->is_virtual();
  m_projected_size =  node->get_projected_size();
  if(node->_parent == NULL)
    _parent = NULL;
}



TreeNode::~TreeNode()
{
#if 1
  TreeNode  *sub_tree;
  TreeNodes_it  sub_treeIter = _sub_trees.begin();
  for( ; sub_treeIter != _sub_trees.end() ; sub_treeIter++ ) {
		sub_tree = *sub_treeIter;
		delete sub_tree;
  }
  if(get_nb_subtrees() == 0) { // Node leaf:
	 for(alignp_set::iterator it=_span_set.begin(); it!=_span_set.end(); ++it)
		delete *it;
	 for(alignp_set::iterator it=_comp_span_set.begin(); it!=_comp_span_set.end(); ++it)
		delete *it;
  }
#endif
  --nodes_in_mem;

}
 
/***************************************************************************
 * Functions to read Collins/Radu parse trees from file
 * (or that use the treelib library to parse trees)
 ***************************************************************************/

// Use treelib to load a parse from a stream:
Leaves* 
TreeNode::read_parse( std::string& treestr, const std::string& format ) {

  bool convert = true;

  // Get format:
  treelib::ParserFormat intformat;
  if(format == "collins")   { intformat = treelib::COLLINS; } 
  // Daniel: added convert=false; below. This should always be false when working with LW tokenization
  else if(format == "radu") { intformat = treelib::RADU; convert = false;} // Daniel
  else if(format == "radu-new") { intformat = treelib::RADU; convert = false; }
  else { 
	 *myerr << "Unknown format: " << format << std::endl;
	 exit(1);
  }

  treelib::Tree tree(intformat);

  //! tell not to lower case the E-tree leaves if necessary.
  if(!lowercasing) {tree.enableLowercasing(false);}

  tree.read(treestr);
  if(tree.begin() == tree.end()) {
  	 myerr->flush();
    *myerr << "WARNING: bad parse tree\n";
  	 return NULL;
  }
  if (convert)
    tree.convertPTBToMT();

  //////////////////////////////////////////////////////////
  // Create a tree node data structure by first creating a
  // root (mtrule::TreeNode):
  //////////////////////////////////////////////////////////

  int pos = 0;

  // Create the Leaves data structure:
  Leaves *lv = new Leaves();
  
  // Start reading tree from 'is':
  //treelib::TreeNode *root = &*tr.begin();
  treelib::Tree::iterator it = tree.begin();
  TreeNode* mainTree = new_child(&tree, it, pos, this, lv);
  _parent = NULL;
  _sub_trees.push_back(mainTree);
  _e_span.first = 0;
  if(mainTree != NULL)
	 _e_span.second = mainTree->get_end();
  else 
	 _e_span.second  = -1;
  _cat = "ROOT";
  _word = "";
  _tag="";
  return lv;
}

// Create a new child in the tree representation from a node 'par':
TreeNode*
TreeNode::new_child(treelib::Tree* tree, treelib::Tree::iterator it, 
						  int& current, TreeNode* par, Leaves* lv)
{

  treelib::TreeNode* curNode = &*it;
  std::string label = curNode->getLabel();
  std::string headword = curNode->getHeadword();
  std::string headtag= curNode->getHeadPOS();
  TreeNodes tmp;

  int start = current;
  int num_cat = it.number_of_children();
  // Wei: in the tree lib, the headPosition is 1-indexed, and we here
  // need the 0-indexed.
  int head_index = curNode->getHeadPosition() - 1;
  int head_string_pos = start; // position of the head

  // Check if the current node is not a leaf node:
  if(num_cat != 0) { 
	 // Dealing with a non-terminal node:

	 // Create parse of subtree:
	 TreeNode* ans = new TreeNode
		(par, label, headword, headtag, head_index, head_string_pos, tmp, start, -1);
	 TreeNodes* subTrs = &ans->get_subtrees();
	 // Recursive calls:
	 for(int childIndex = 0; childIndex < num_cat; ++childIndex) {
		TreeNode* child 
		  = new_child(tree, tree->child(it,childIndex), current, ans, lv);
		if(child != NULL)
		  subTrs->push_back(child);
	 }
	 // Find the real offset of the head word: (collins ignores
	 // punctuation)
	 int i = 0;
	 if(subTrs->size() != 0) {
		TreeNodes_it it = subTrs->begin();
		for(; it != subTrs->end(); ++it, ++i) {
		  TreeNode* st = *it;
		  if(st->get_word() == headword) {
			 head_index = i;
			 head_string_pos = st->get_head_string_pos();
		  }
		}
	 } 
	 // Manage leaves:
	 for(TreeNodes_it it = subTrs->begin(); 
		  it != subTrs->end() ; ++it) {
		TreeNode* st = *it;
		st->parent() = ans;
	 }
	 // If ans is a leave (preterminal), add it to lv:
	 if(curNode->getIsPreterminal()) {
		// Deal with virtual nodes:
		if(ans->get_word() == "") {
		  ans->set_word(ans->get_cat());
		  ans->_is_virtual = true;
		}
		lv->push_back(ans);
	 }

	 // Determine the end of the source-language span:
	 if(curNode->getIsPreterminal()) {
		current++;
	 }
	 if(curNode->getIsVirtual())
		ans->set_end(current);
	 else
		ans->set_end(current-1);

	 return ans;

  } else {

	 // Dealing with a terminal node:
	 // update parent and return:
	 par->set_word(label);
	 return NULL;

  }
}

/***************************************************************************
 * Functions to create or print strings representing English strings, 
 * tree fragments, etc
 ***************************************************************************/

// Return the left-hand side of the CFG rule that can be read off the
// current node:
std::string
TreeNode::lhs_str() const {
  std::string tmp(_cat.c_str());
  return tmp;
} 

// Return the right-hand side of the CFG rule that can be read off the
// current node:
std::string
TreeNode::rhs_str() const {
  std::string right;
  for(TreeNodes_cit it = _sub_trees.begin(), 
		it_end = _sub_trees.end();  
		it != it_end; ++it) {
	 TreeNode &t = **it;
	 if(t.get_cat() != "") {
		right += t._cat.c_str();
		right += " ";
	 }
  }
  if(right != "")
	 right.erase(right.length()-1,1);
  return right;
}

// Read the English string from the leaves of a tree:
std::string 
TreeNode::get_string( Leaves* lv ) const {
  //int i=0;
  std::string tmp;
  for(Leaves_cit it = lv->begin(), it_end = lv->end(); 
		it != it_end; ++it) {
	 tmp +=  (*it)->_word.c_str();
	 tmp += " ";
  }
  if(tmp.length() > 0)
	 tmp.erase(tmp.length()-1,1);
  return tmp;
}

// Dump a tree structure that shows: spans, complement spans,
// and some other helpful info:
void
TreeNode::dump_pretty_tree( std::ostream& os, int level ) const {
  if(_cat == "ROOT") {
    os << "%%% format1: syn-cat      (e-span)[f-span]{f-complement-span}\n";
    os << "%%% format2: pos-tag/word (e-span)[f-span]{f-complement-span}\n";
    os << "%%% #-nodes are in the frontier set.\n";
  }
  if(_cat != "ROOT" && _cat != "MULTI") {
	 os << "%%% ";
	 for(int i=0; i<level*3; ++i)
		os << " ";
	 // Print syntactic category:
	 os << _cat;
	 // Print word if it is a leaf:
	 if(is_preterm()) 
		os << "/" << _word;
	 // Can the current node be part of valid frontier graph fragment:
	 if(_frontier_node)  
	  os << " (frontier)";
	  //os << "#";
	 os << "  ";
	 if(!find_bad_align) {
		// Print english span:
		//os << "(" << _e_span.first << "-" << _e_span.second << ")";
		// Print chinese span:
		os << "span=(";
		for(size_t i=0; i<_c_spans.size(); ++i) {
		  if(i>0) os << ",";
		  os << _c_spans[i].first;
		  if(_c_spans[i].first != _c_spans[i].second)
			 os << "-" << _c_spans[i].second;
		}
		os << ") ";
		// Print chinese complement span:
		os << "cspan=(";
		for(size_t i=0; i<_c_comp_spans.size(); ++i) {
		  if(i>0) os << ",";
		  os << _c_comp_spans[i].first;
		  if(_c_comp_spans[i].first != _c_comp_spans[i].second)
			 os << "-" << _c_comp_spans[i].second;
		}
		os << ")";
	 // Print span and complement span sets:
	 } else {
	    os << " ss(";
		 for(alignp_set::iterator it=_span_set.begin(); it!=_span_set.end(); ++it) {
		   if(it != _span_set.begin())
			  os << " ";
		 	os << (*it)->first << "-" << (*it)->second;;	
		 }
	    os << ") css(";
		 for(alignp_set::iterator it=_comp_span_set.begin(); it!=_comp_span_set.end(); ++it) {
		   if(it != _comp_span_set.begin())
			  os << " ";
		 	os << (*it)->first << "-" << (*it)->second;;	
		 }
	    os << ") cs(";
		 for(alignp_set::iterator it=_cross_set.begin(); it!=_cross_set.end(); ++it) {
		   if(it != _cross_set.begin())
			  os << " ";
		 	os << (*it)->first << "-" << (*it)->second;;	
		 }
	    os << ") ccs(";
		 for(alignp_set::iterator it=_comp_cross_set.begin(); it!=_comp_cross_set.end(); ++it) {
		   if(it != _comp_cross_set.begin())
			  os << " ";
		 	os << (*it)->first << "-" << (*it)->second;;	
		 }
	    os << ")";

	 }
	 os << std::endl;
	 level++;
  }
  for(TreeNodes_cit it = _sub_trees.begin(); it != _sub_trees.end() ; ++it) {
	 TreeNode* st = *it;
	 st->dump_pretty_tree(os,level);
  }
}

// and some other helpful info:
void
TreeNode::dump_pretty_tree_old( std::ostream& os, int level ) const {
  if(_cat == "ROOT") {
    os << "%%% format1: syn-cat      (e-span)[f-span]{f-complement-span}\n";
    os << "%%% format2: pos-tag/word (e-span)[f-span]{f-complement-span}\n";
    os << "%%% #-nodes are in the frontier set.\n";
  }
  if(_cat != "ROOT" && _cat != "MULTI") {
	 os << "%%% ";
	 for(int i=0; i<level*3; ++i)
		os << " ";
	 // Print syntactic category:
	 os << _cat;
	 // Print word if it is a leaf:
	 if(is_preterm()) 
		os << "/" << _word;
	 // Can the current node be part of valid frontier graph fragment:
	 if(_frontier_node)  
	  os << "#";
	 os << "  ";
//	 // Print english span:
//	 os << "(" << _e_span.first << "-" << _e_span.second << ")";
//	 // Print chinese span:
//	 os << "[";
//	 for(size_t i=0; i<_c_spans.size(); ++i) {
//		if(i>0) os << ",";
//		os << _c_spans[i].first;
//		if(_c_spans[i].first != _c_spans[i].second)
//		  os << "-" << _c_spans[i].second;
//	 }
//	 os << "]";
//	 // Print chinese complement span:
//	 os << "{";
//	 for(size_t i=0; i<_c_comp_spans.size(); ++i) {
//		if(i>0) os << ",";
//		os << _c_comp_spans[i].first;
//		if(_c_comp_spans[i].first != _c_comp_spans[i].second)
//		  os << "-" << _c_comp_spans[i].second;
//	 }
//	 os << "}";
	 // Print span and complement span sets:
	 if(find_bad_align) {
	    os << " ss(";
		 for(alignp_set::iterator it=_span_set.begin(); it!=_span_set.end(); ++it) {
		   if(it != _span_set.begin())
			  os << " ";
		 	os << (*it)->first << "-" << (*it)->second;;	
		 }
	    os << ") css(";
		 for(alignp_set::iterator it=_comp_span_set.begin(); it!=_comp_span_set.end(); ++it) {
		   if(it != _comp_span_set.begin())
			  os << " ";
		 	os << (*it)->first << "-" << (*it)->second;;	
		 }
	    os << ") cs(";
		 for(alignp_set::iterator it=_cross_set.begin(); it!=_cross_set.end(); ++it) {
		   if(it != _cross_set.begin())
			  os << " ";
		 	os << (*it)->first << "-" << (*it)->second;;	
		 }
	    os << ") ccs(";
		 for(alignp_set::iterator it=_comp_cross_set.begin(); it!=_comp_cross_set.end(); ++it) {
		   if(it != _comp_cross_set.begin())
			  os << " ";
		 	os << (*it)->first << "-" << (*it)->second;;	
		 }
	    os << ")";

	 }
	 os << std::endl;
	 level++;
  }
  for(TreeNodes_cit it = _sub_trees.begin(); it != _sub_trees.end() ; ++it) {
	 TreeNode* st = *it;
	 st->dump_pretty_tree(os,level);
  }
}




// Dump a tree in Collins's parser format:
void
TreeNode::dump_collins_tree( std::ostream& os, bool root ) const {
  
  if(_sub_trees.size() > 0) {
	 if(_cat != "ROOT" || true) { 
		if(_word != "")
		  os << "(" << _cat << "~" 
			  << _word << "~" 
			  << _sub_trees.size() << "~"
			  << _head_index+1 << " ";
		else
		  os << "(" << _cat << " ";
	 } 
	 for(TreeNodes_cit it = _sub_trees.begin(); it != _sub_trees.end() ; ++it) {
		TreeNode* st = *it;
		st->dump_collins_tree(os,false);
	 }
	 if(_cat != "ROOT" || true) 
		os << ") ";
  } else {
	 os << _word << "/" << _cat << " "; 
  }
  if(root)
	 os << std::endl;
}

// Dump a tree in Collins's parser format:
void
TreeNode::dump_radu_tree( std::ostream& os, bool root ) const {
  
  if(_sub_trees.size() > 0) {
	 if(_cat != "ROOT" || root) { 
		if(_word != "")
		  os << "(" << _cat 
			  <<"~" 
			  << _sub_trees.size() << "~"
			  << _head_index +1<< " "
				  <<"-0 ";
		else
		  os << "(" << _cat << "~0~0 -0 ";
	 } 
	 for(TreeNodes_cit it = _sub_trees.begin(); it != _sub_trees.end() ; ++it) {
		TreeNode* st = *it;
		st->dump_radu_tree(os,false);
	 }
	 if(_cat != "ROOT" || root) 
		os << ") ";
  } else {
	 os <<"("<< _cat <<" " <<_word<< ") "; 
  }
  if(root)
	 os << std::endl;
}

/***************************************************************************
 * Function to manage spans:
 ***************************************************************************/

// Print spans provided as an argument:
void
TreeNode::print_spans(spans& s) {
  for(spans_it it = s.begin(), it_end = s.end(); it != it_end; ++it) {
	 int x = it->first, y = it->second;
	 std::cout << "[" << x << ":" << y << "]";
  }
  std::cout << std::endl;
}
	
// Merges chinese spans given an alignment. The argument 's' is a vector of 
// spans. The function sorts it by ascending order, then stores all spans 
// in a new vector of spans, but merges the contiguous ones.
// Example:  [1,2,3-5,7-9,11-12] --> [1-5,7-12]  (assuming 10 is unaligned)
void
TreeNode::merge_spans(spans& s, Alignment *a) {
  // Sort input vector of spans:
  stable_sort(s.begin(),s.end(),TreeNode::span_sort());
  spans new_s;
  span *last_span=NULL;
  for(spans_it it = s.begin(), it_end = s.end(); 
		it != it_end; ++it) {
	 if(last_span == NULL)
		last_span = &*it;
	 else {
		int x = last_span->second, y = it->first;
		bool gap = false;
		for(int i=x+1;i<y;++i) {
		  if(i< 0 || i >= a->get_target_alignment_len()) {
			 // either last_span or the current span was set to dummy values:
			 gap = true;
			 break;
		  }
		  if(a->target_to_source(i).size() != 0) {
			 // word of index i is aligned:
			 gap = true;
			 break;
		  }
		}
		if(!gap) { 
		  if(last_span->second < it->second) {
			 last_span->second = it->second;
		  }
		} else {
		  new_s.push_back(*last_span);
		  last_span = &*it;
		}
	 }
  }
  if(last_span != NULL)
	 new_s.push_back(*last_span);
  s = new_s; 
}

// Assigning alignment spans to each syntactic constituent,
// bottom-up.
// Modified by Wei on Mon Sep 18 21:36:54 PDT 2006 to handle
// e-forest, not only e-tree. An e-forest contains AND node
// and OR node. An e-tree contains UNDEF node.
void
TreeNode::assign_alignment_spans(TreeNode* n, Alignment *a ) {
	set<TreeNode*> memoize;
	TreeNode::assign_alignment_spans(n, a, memoize);
}

void
TreeNode::assign_alignment_spans(TreeNode* n, Alignment *a, 
		                         set<TreeNode*> & memoize) {
	if(memoize.find(n) != memoize.end()){
		return;
	}else{
		memoize.insert(n);
	}	

  
  // Treat leaves:
  // If we reach a leaf, get alignment of the leaf, 
  // set chinese spans, and return:
  n->_c_spans.clear();
  if(n->get_nb_subtrees() == 0) { // We have reached a leaf node:

	  assert(n->type() == AND || n->type() == UNDEF);

	 int start = n->get_start();
	 // Consistency check:
	 if(start < 0 || start >= a->get_source_alignment_len()) {
		*myerr << "===== CONSISTENCY CHECK FAILED: " << start << std::endl;
		exit(1);
	 }
	 align& al = a->source_to_target(start);
	 if(al.size() > 0) {
		for(align_it it = al.begin(), it_end = al.end(); it != it_end; ++it) {
		  if(*it >=0 && *it < a->get_target_alignment_len()) {
			 n->_c_spans.push_back(std::make_pair(*it,*it));
			 if(find_bad_align) {
			   assert(n->_e_span.first == n->_e_span.second);
				n->_span_set.insert(new alignp(n->_e_span.first,*it));
			 }
		  }
		}
	 }
	 merge_spans(n->_c_spans,a);
	 // Determine if there is a clean mapping from english to chinese:
	 if(clean_english_to_chinese_alignment(n,a))
		// Determine if there is a clean mapping from chinese to english:
		// (that is: no chinese word aligned to any english word outside 
		// the english span)
		if(clean_chinese_to_english_alignment(n,a))
		  n->_extraction_node = true;
	 return;
  }

  // Treat internal nodes:
  // For each syntactic constituent, determine the corresponding span
  // in the chinese sentence (_t_start and _t_end). 
  // If n is an OR node, we need to check only the first child --- all
  // children have the same spans.
  bool get_span_from_children = true;
  for(TreeNodes_it it = n->get_subtrees().begin(), 
		it_end = n->get_subtrees().end();
		it != it_end; ++it) {
	 TreeNode* st = *it;
	 assign_alignment_spans(st,a, memoize);

	 if(get_span_from_children){
		 for(int i=0; i<st->get_nb_c_spans(); ++i) {
			// Check the validity of the span:
			if(st->_c_spans[i].first >= 0  && 
				st->_c_spans[i].first < a->get_target_alignment_len() &&
				st->_c_spans[i].second >= 0 && 
				st->_c_spans[i].second < a->get_target_alignment_len()) {
			  n->_c_spans.push_back(st->_c_spans[i]);
			}
		 }

		if(find_bad_align) {
		  for(alignp_set::iterator it=st->_span_set.begin(); 
				                            it!=st->_span_set.end(); ++it) {
			 n->_span_set.insert(*it);
		  }
		}
	 }

	/* if n is an OR node, we only care about its first child.
	 */
	if(n->type() == OR){ get_span_from_children = false; }
  }
 /* if n is an OR node, we dont need to merge spans. if it is an AND
  * node, we need to merge the spans.
  */
 merge_spans(n->_c_spans,a); 

  // Correct english spans if needed:
  // TODO: it seems that the span of TOP is never computer correctly
  int new_start, new_end;
  if(n->type() == OR ){
	  new_start = (*n->get_subtrees().begin())->_e_span.first;
	  new_end= (*n->get_subtrees().begin())->_e_span.second;
  } else {
        new_start = (*n->get_subtrees().begin())->_e_span.first;
		new_end   = (*n->get_subtrees().rbegin())->_e_span.second;
  }
  if(new_start != n->_e_span.first) {
	 *myerr << "%%% ===== CORRECTING SPAN START : " 
				  << n->_e_span.first << " -> " << new_start << std::endl;
	 n->_e_span.first = new_start;
  }
  if(new_end != n->_e_span.second) {
	 *myerr << "%%% ===== CORRECTING SPAN END : " 
				  << n->_e_span.second << " -> " << new_end << std::endl;
	 n->_e_span.second = new_end;
  }

  // Check if any rule can be extracted from the current node:
  if(clean_english_to_chinese_alignment(n,a))
	 if(clean_chinese_to_english_alignment(n,a))
		n->_extraction_node = true;

}

// Assigning complement spans to each syntactic constituent 
// is performed in two steps:
// 1) adds the spans of each syntactic constituent
//    to the complement-spans of all its siblings.
// 2) propagate the complement span of each node
//    to all its children.
// Modified by Wei Wang to handle forest (including AND and OR nodes).
void TreeNode::assign_complement_alignment_spans(TreeNode* n, Alignment* a){
	set<TreeNode*> memoize;
	assign_complement_alignment_spans(n, a, memoize);
}

void TreeNode::assign_complement_alignment_spans(TreeNode* n, Alignment* a,
		set<TreeNode*> & memoize) {
	if(memoize.find(n) != memoize.end()){
		return;
	} else {
		memoize.insert(n);
	}

  // For each syntactic constituent, determine the corresponding
  // complement span in the chinese sentence (_t_start and _t_end):
  for(TreeNodes_it it = n->get_subtrees().begin(), 
			  it_end = n->get_subtrees().end();
			  it != it_end; ++it) {
	 TreeNode* st = *it;

	 /* 
	  * if n is an OR node, we dont need to care about the inside spans
	  * of the children, and we can directly pass the complement span
	  * of the OR-node to the AND node children.
	  */
	 if(n->type() != OR){
		 // Add spans to complement spans of siblings:
		 for(TreeNodes_it it2 = n->get_subtrees().begin(), 
					 it2_end = n->get_subtrees().end();
					 it2 != it2_end; ++it2) {
			TreeNode* st2 = *it2;
			if(st == st2)
			  continue;
			for(int i=0; i<st2->get_nb_c_spans(); ++i) {
			  st->_c_comp_spans.push_back(st2->_c_spans[i]);
			  if(st2->_c_spans[i].first > 1000) {
				 // If sth printing here, likely a bug:
				 std::cout << "[" 
							  << st2->_c_spans[i].first  << ":"  
							  << st2->_c_spans[i].second << "]"
							  << std::endl;
			  }
			}
			if(find_bad_align) {
			  for(alignp_set::iterator it=st2->_span_set.begin(); it!=st2->_span_set.end(); ++it)
				 st->_comp_span_set.insert(*it);
			}
		 }
	 }

	 // Add the complement spans of the node to its children: 
	 for(int i=0; i<n->get_nb_c_comp_spans(); ++i) {
		st->_c_comp_spans.push_back(n->_c_comp_spans[i]);
	 }
	 if(find_bad_align) {
		for(alignp_set::iterator it=n->_comp_span_set.begin(); it!=n->_comp_span_set.end(); ++it)
		  st->_comp_span_set.insert(*it);
	 }

	 // Merge/compact all spans of a given node:
	 merge_spans(st->_c_comp_spans,a);

	 // Recursive call (top-down)
	 assign_complement_alignment_spans(st,a, memoize);
  }
}

// Make sure that alignment spans are consisitents, otherwise
// exit:
void 
TreeNode::check_alignment_spans() {

  // If leaf, leave:
  if(get_subtrees().size() == 0)
	 return;

  // For each syntactic constituent, determine the corresponding
  // complement span in the chinese sentence (_t_start and _t_end):
  for(TreeNodes_it it = get_subtrees().begin(), 
			  it_end = get_subtrees().end();
			  it != it_end; ++it) {
	 TreeNode* st = *it;

	 // Recursive call: before assignment operations are done --> bottom-up
	 st->check_alignment_spans();
  }

  // Correct spans:
  TreeNode* first = *get_subtrees().begin();
  TreeNode* last  = *get_subtrees().rbegin();
  if(_e_span.first != first->_e_span.first) {
	 *myerr << "CORRECTING SPAN: " << first->_e_span.first  
				  << " --> " << _e_span.first << std::endl;
	 exit(1);
  }
  if(_e_span.second != last->_e_span.second) {
	 *myerr << "CORRECTING SPAN: " << last->_e_span.second 
				  << " --> " << _e_span.second << std::endl;
	 exit(1);
  }
  _e_span.first   = first->_e_span.first;
  _e_span.second  = last->_e_span.second;
}

// For each node, determine if the node is a potential frontier set node
// (top-down). If not, find crossings.
void 
TreeNode::assign_frontier_node(TreeNode* n) {
	set<TreeNode*> memoize;
	assign_frontier_node(n, memoize);
}
void 
TreeNode::assign_frontier_node(TreeNode* n, set<TreeNode*>&memoize) {
	if(memoize.find(n) != memoize.end()){
		return;
	} else {
		memoize.insert(n);
	}

  if(n->_extraction_node) { // To be a frontier node, a node must first 
									 // be a node from which a rule can be extracted
	 spans tmp;
	 for(int i=0; i<n->get_nb_c_spans(); ++i)
		tmp.push_back(n->_c_spans[i]);
	 for(int i=0; i<n->get_nb_c_comp_spans(); ++i)
		tmp.push_back(n->_c_comp_spans[i]);
	 std::stable_sort(tmp.begin(),tmp.end(),TreeNode::span_sort());
	 // If there is any overlap, this means that it isn't a pot frontier node:
	 n->_frontier_node = true;
	 for(int i=1, size=tmp.size(); i<size; ++i) {
		if(tmp[i-1].second >= tmp[i].first) {
		  n->_frontier_node = false;
		  break;
		}
	 }
  }

  // Find crossings:
  if(find_bad_align) {
	  for(alignp_set::iterator it=n->_span_set.begin(); it!=n->_span_set.end(); ++it) {
		 int x2= (*it)->second;
		 for(alignp_set::iterator it2=n->_comp_span_set.begin(); it2!=n->_comp_span_set.end(); ++it2) {
			int y2= (*it2)->second;
			if(x2 == y2) {
				n->_cross_set.insert(*it);
				n->_comp_cross_set.insert(*it2);
			}
		 }
	  }
  }

  // Recursively call assign_frontier_node():
  for(TreeNodes_it it = n->get_subtrees().begin(), 
			  it_end = n->get_subtrees().end();
			  it != it_end; ++it) {
	 TreeNode* st = *it;
	 assign_frontier_node(st, memoize);
  }
}

/***************************************************************************
 * Functions to test alignment quality:
 ***************************************************************************/

// Determine if the syntactic constituent n is aligned to a single
// and contiguous target language (e.g. Chinese) span:
bool
TreeNode::clean_english_to_chinese_alignment(TreeNode* n, Alignment *a) {
  return (n->_c_spans.size() == 1);
}

// Assuming the syntactic constituent n is aligned to a single
// and contiguous target language span, determine if all target-language
// (e.g. Chinese) are either unaligned or map to source-language words
// that are under the same constituent: (if the fuction returns false, 
// it means that there is a crossing)
bool
TreeNode::clean_chinese_to_english_alignment(TreeNode* n, Alignment *a) {
  int _c_start = n->_c_spans[0].first,
		_c_end = n->_c_spans[0].second;
  if(_c_end < 0 || _c_end >= a->get_target_alignment_len())
	 return false;
  for(int i=_c_start; i<=_c_end; ++i) {
	 align& al = a->target_to_source(i);
	 for(align::iterator it = al.begin(), it_end = al.end(); it != it_end; ++it)
		if(*it < n->get_start() || *it > n->get_end())
		  return false;
  }
  return true;
}

bool
TreeNode::clean_chinese_to_english_alignment(const span cspan, const span espan, Alignment *a) {
  int _c_start = cspan.first,
		_c_end = cspan.second;
  if(_c_end < 0 || _c_end >= a->get_target_alignment_len())
	 return false;
  for(int i=_c_start; i<=_c_end; ++i) {
	 align& al = a->target_to_source(i);
	 for(align::iterator it = al.begin(), it_end = al.end(); it != it_end; ++it)
		if(*it < espan.first || *it > espan.second)
		  return false;
  }
  return true;
}

}
