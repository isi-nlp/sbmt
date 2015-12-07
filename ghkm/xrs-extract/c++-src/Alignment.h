#ifndef __ALIGNMENT_H__
#define __ALIGNMENT_H__

/***************************************************************************
 * Alignment.C/h
 *
 * Class that encapsulates word-to-word alignments, that is: 
 *  - two alignment tables (target- to source-language and source- to target-
 *    language
 *  - a source-language string
 *  - a target-language string
 * 
 * Author: Michel Galley, galley@cs.columbia.edu
 * $Id: Alignment.h,v 1.4 2006/11/17 00:34:39 wwang Exp $
 ***************************************************************************/

#include <vector>
#include <set>

#include "defs.h"
#include "nstring.h"
#include "hashutils.h"

namespace mtrule {

  class Alignment;

  // align: set of words (listed by their index) a given word aligns to:
  typedef std::set<int> align;
  typedef std::set<int>::iterator align_it;
  typedef std::set<int>::const_iterator align_cit;
  // align_set: set of align objects:
  typedef std::vector< align* > align_set;
  typedef align_set::iterator align_set_it;
  typedef align_set::const_iterator align_set_cit;

  // fully specified alignment:
  typedef std::pair<int,int> alignp;
  typedef std::set<alignp*> alignp_set;
  typedef alignp_set::iterator alignp_it;
  typedef alignp_set::const_iterator alignp_cit;

  class Alignment {

	public:

    // Use a stoplist for bad alignments:
	 static void load_stoplist(std::istream& is); 
	
	 // Constructor and destructor:
	 Alignment(std::string& align_str, std::string& e_str, std::string& c_str);
	 ~Alignment();

	 // Accessors:

	 // Access alignments:
	 align& target_to_source( int ti ) { return *_target_to_source[ti]; }
	 align& source_to_target( int si ) { return *_source_to_target[si]; }
	 
	 // Access source- and target-language strings:
	 std::string str_source() const;
	 std::string str_target() const;
	 const STRING& get_source_word(int i) const { return _source_words[i]; }
	 const STRING& get_target_word(int i) const { return _target_words[i]; }

	 std::string get_source_words(int a, int b) const; // Daniel
	 std::string get_target_words(int a, int b) const; // Daniel

	 // Length of alignment tables:
	 int get_source_alignment_len() const { return _source_to_target.size(); }
	 int get_target_alignment_len() const { return _target_to_source.size(); }
	 
	 // Length of source- and target-language strings:
	 int get_source_len() const { return _source_words.size(); }
	 int get_target_len() const { return _target_words.size(); }
	 
    // is the alignment bad?
	 bool is_bad() const  { return _bad; }
	 
	 // Make sure that the chinese and english sentences
	 // are long enough to contain all alignments:
	 bool consistency_check(bool verbose) const;

    // Function used to print alignments:
	 void print_alignments(bool loose) const;

	 // Function to throw out bad alignments:
	 void delete_badalignments();

	protected:

	 static StrHashM _badalign; 
	
	 align_set _source_to_target; //!< source to target mapping (e.g. english to chinese)
	 align_set _target_to_source; //!< target to source mapping (e.g. chinese to english)
	 std::vector<STRING> 
	   _source_words, //!< sentence in the source language
		_target_words; //!< sentence in the target language
	 bool _bad; //!< set to true means that the alignment is bad
	 int _nb_alignments; //!< count of number of alignments between etree and cstring

	 void checkResize( int, int );
  };

}

#endif 
