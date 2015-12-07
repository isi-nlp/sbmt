#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <algorithm>
#include <cassert>

#include "Alignment.h"
#include "MyErr.h"

/***************************************************************************
 * Alignment.C/h
 *
 * Class that encapsulates word-to-word alignments, that is: 
 *  - two alignment tables (target- to source-language and source- to target-
 *    language
 *  - a source-language string
 *  - a target-language string
 * 
 * TODO: count the number of Alignment objects that are created/deleted, 
 * and make sure there is no memory leak.
 * 
 * Author: Michel Galley, galley@cs.columbia.edu
 * $Id: Alignment.C,v 1.3 2006/09/12 21:51:22 marcu Exp $
 ***************************************************************************/

namespace mtrule {

  // Static definitions:
  StrHashM Alignment::_badalign; 

  /***************************************************************************
	* Functions to deal with bad alignments:
	***************************************************************************/

  // Use a stoplist for bad 1-1 alignments:
  void
  Alignment::load_stoplist(std::istream& is) {
	 while(!is.eof()) {
	   std::string str;
		getline(is,str);
		if(str == "")
		  continue;
		std::stringstream ss(str);
		std::string estr,cstr;
		ss >> estr;
		ss >> cstr;
		_badalign.insert(std::make_pair(estr,cstr));
      //*myerr << "%%% loading: " 
		//          << estr << "\t" << cstr << "<" << std::endl;
	 }
  }

  // Function to throw out bad alignments:
  void
  Alignment::delete_badalignments() {
    for(int i=0, size=_source_to_target.size(); i<size; ++i) {
	   align *a = _source_to_target[i];
		if(a != NULL)
		  if(a->size() >= 1) {
		    for(align_cit it = a->begin(), it_end = a->end(); 
			     it != it_end; ++it) {
				int j = *it;
				if(_target_to_source[j]->size() >= 1) {
					std::pair<StrHashM_it,StrHashM_it> p 
					  = _badalign.equal_range(get_source_word(i).c_str());
					for(StrHashM_it it2 = p.first; it2 != p.second; ++it2) {
					  if(get_target_word(j) == it2->second) {
						 // Delete alignment:
						 *myerr << "%%% deleting: " 
							<< get_source_word(i) 
							<< "(" << i << ")"
							<< " - "
							<< get_target_word(j)
							<< "(" << j << ")"
							<< std::endl;
						 _source_to_target[i]->erase(j);
						 _target_to_source[j]->erase(i);
						 --_nb_alignments;
					 }
				  }
				}
			 }
		  }
	 }    
	 assert(_nb_alignments >= 0);
	 if(_nb_alignments == 0)
	   _bad = true;
  }

  /***************************************************************************
	* Constructor and destructor:
	***************************************************************************/

  // Create a new alignment object that encapsulates an english
  // string, a foreign string (e.g. chinese), and an alignment:
  // Note: English is typically assumed to be the source language:
  Alignment::Alignment(std::string& align_str,
							  std::string& e_str,
							  std::string& c_str) : 
	 _source_to_target(0), _target_to_source(0), _bad(false),
	 _nb_alignments(0) {
	 
	 // Process source and target sentences:
	 //*myerr << "e string:";
	 std::stringstream sstr(e_str);
	 while(!sstr.eof()) { // source (english)
		std::string temp;
		sstr >> temp;
		if(temp == "")
		  continue;
		_source_words.push_back(temp);
		//*myerr << " " << temp;
	 }
	 //*myerr << "\nc string:";
	 std::stringstream tstr(c_str);
	 while(!tstr.eof()) { // target (e.g. chinese)
		std::string temp;
		tstr >> temp;
		if(temp == "")
		  continue;
		_target_words.push_back(temp);
		//*myerr << " " << temp;
	 }
	 //*myerr << "\n";
	 // Resize the two alignment tables (_source_to_target and 
	 // _target_to_source):
	 checkResize(_source_words.size(),_target_words.size());
	 
	 // Process an alignment, i.e. a string like:
	 // 0-0 1-1 1-2
	 // (0th english word aligns to 0th foreign word, 1st english word
	 //  alignes to 1st and 2nd foreign words)
	 std::stringstream ss(align_str);
	 while(!ss.eof()) {
		std::string temp;
		ss >> temp;
		size_t pos = temp.find("-");
		if(pos == std::string::npos)
		  break;
		int si = atoi(temp.substr(0,pos).c_str()); // 1st element of an x-y pair
		int ti = atoi(temp.substr(pos+1).c_str()); // 2nd element of an x-y pair
		// Check whether si or ti go beyond sentence boundaries:
		if(si >= static_cast<int>(_source_to_target.size())) { 
		  *myerr << "%%% beyond sentence boundary (source language): " << si 
						<< " >= " << _source_to_target.size() 
						<< std::endl;
		  _bad = true;
		  break;
		}
		if(ti >= static_cast<int>(_target_to_source.size())) { 
		  *myerr << "%%% beyond sentence boundary (target language): " << ti 
						<< " >= " << _target_to_source.size() 
						<< std::endl;
		  _bad = true;
		  break;
		}
		_source_to_target[si]->insert(ti);
		_target_to_source[ti]->insert(si);
		++_nb_alignments;
	 }
  }

  Alignment::~Alignment() {
	 for(int i=0,size=_source_to_target.size();i<size;++i)
		if(_source_to_target[i] != NULL)
		  delete _source_to_target[i];
	 for(int i=0,size=_target_to_source.size();i<size;++i)
		if(_target_to_source[i] != NULL)
		  delete _target_to_source[i];
  }

  /***************************************************************************
	* Stuff to print the alignment:
	***************************************************************************/

  void
  Alignment::print_alignments(bool loose) const {
    for(int i=0, size=_source_to_target.size(); i<size; ++i) {
	   align *a = _source_to_target[i];
		if(a != NULL) {
		  if(a->size() == 1 || loose)
			 for(align_cit it = a->begin(), it_end = a->end(); it != it_end; ++it) {
			   if(_target_to_source[*it] != NULL)
				  if(_target_to_source[*it]->size() == 1 || loose)
					 std::cout << get_source_word(i) << " ||| " 
								  << get_target_word(*it) << std::endl;
			 }
		}
	 }
  }

  /***************************************************************************
	* Memory management stuff:
	***************************************************************************/

  // Check length of source and target alignments (source and target vectors),
  // and if they are smaller than the specified sizes (s and t, respectively),
  // make them longer:
  inline void
  Alignment::checkResize(int st, int ts) {
	 int old_st = _source_to_target.size(),
		  old_ts = _target_to_source.size();
	 if(st >= old_st) _source_to_target.resize(st);
	 if(ts >= old_ts) _target_to_source.resize(ts);
	 for(int i=old_st;i<st;++i)
		if(_source_to_target[i] == NULL)
		  _source_to_target[i] = new align;
	 for(int i=old_ts;i<ts;++i)
		if(_target_to_source[i] == NULL)
		  _target_to_source[i] = new align;
  }

  /***************************************************************************
	* Function for consistency checking: 
	***************************************************************************/

  // Make sure that no word index specified in the alignment line
  // is out of boundary with respect to the target or source sentence:
  // (note: the constructor checks this, so this function is no
  // longer needed)
  bool Alignment::consistency_check(bool verbose) const {
	 int 
		len_source = _source_words.size(), // len of source sentence
		len_target = _target_words.size(), // len of target sentence
		align_len_source = _source_to_target.size(), // len of source in alignment
		align_len_target = _target_to_source.size(); // len of target in alignment

	 if(verbose) {
		*myerr << "SOURCE: a: " << len_source    << "\t"
								<< "d: " << align_len_source << "\t"
								<< "(diff: " << len_source-align_len_source << ")"
								<< std::endl
					 << "TARGET: a: " << len_target    << "\t" 
								<< "d: " <<  align_len_target << "\t"
								<< "(diff: " << len_target-align_len_target << ")"
								<< std::endl;
	 }
	 return (len_source >= align_len_source && 
				len_target >= align_len_target);
  }

  /***************************************************************************
	* String representation functions:
	***************************************************************************/

  std::string
  Alignment::str_source() const {
    std::string tmp;
    for(int i=0, size=_source_words.size(); i<size; ++i) {
	   if(i>0) 
	     tmp += " ";
	   tmp += _source_words[i].c_str();
	 }
	 return tmp;
  }

  std::string
  Alignment::str_target() const {
    std::string tmp;
    for(int i=0, size=_target_words.size(); i<size; ++i) {
	   if(i>0) 
	     tmp += " ";
	   tmp += _target_words[i].c_str();
	 }
	 return tmp;
  }

  // Daniel
   std::string 
   Alignment::get_source_words(int s, int e) const { 
     std::string s1; 
     for(int i=s; i<=e; i++) 
       s1+= _source_words[i] + std::string(" "); 
     return s1; 
   }
    
  // Daniel
  std::string 
  Alignment::get_target_words(int s, int e) const {
    std::string s1; 
    for(int i=s; i<=e; i++) 
      s1+= _target_words[i] + std::string(" "); 
    return s1; 
  }


}
