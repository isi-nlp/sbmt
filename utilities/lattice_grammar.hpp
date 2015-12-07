# if ! defined(UTILITIES__LATTICE_GRAMMAR_HPP)
# define       UTILITIES__LATTICE_GRAMMAR_HPP

#define BOOST_SPIRIT_THREADSAFE 1

#if ! defined(PHOENIX_LIMIT) || PHOENIX_LIMIT < 6
#define  PHOENIX_LIMIT 6
#endif 

#if (!defined(PHOENIX_CONSTRUCT_LIMIT)) || (PHOENIX_CONSTRUCT_LIMIT < 6)
#define  PHOENIX_CONSTRUCT_LIMIT 6
#endif 

#if (!defined(BOOST_SPIRIT_CLOSURE_LIMIT)) || (BOOST_SPIRIT_CLOSURE_LIMIT < 6)
#define  BOOST_SPIRIT_CLOSURE_LIMIT 6
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <stdexcept>
#include <vector>
#include <list>

#include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/phoenix1.hpp>

/*
#include <boost/spirit/core.hpp>
#include <boost/spirit/attribute.hpp>
#include <boost/spirit/error_handling/exceptions.hpp>
#include <boost/spirit/utility/distinct.hpp>
#include <boost/spirit/utility/confix.hpp>
#include <boost/spirit/utility/escape_char.hpp>
#include <boost/spirit/actor/assign_actor.hpp>
#include <boost/spirit/actor/clear_actor.hpp>
#include <boost/spirit/actor/push_back_actor.hpp>
#include <boost/spirit/actor/erase_actor.hpp>
#include <boost/spirit/actor/insert_at_actor.hpp>
#include <boost/spirit/iterator/multi_pass.hpp>
#include <boost/spirit/iterator/position_iterator.hpp>
#include <boost/spirit/phoenix/binders.hpp>
#include <boost/spirit/phoenix/new.hpp>
*/

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/utility/enable_if.hpp>

#include <sbmt/search/block_lattice_tree.hpp>
#include <gusc/lattice/grammar.hpp>
#include <gusc/lattice/ast.hpp>
#include <gusc/lattice/ast_construct.hpp>

#include "phoenix_helpers.hpp"


////////////////////////////////////////////////////////////////////////////////

struct lattree_c
: public boost::spirit::classic::closure<lattree_c, gusc::lattice_ast>
{
    member1 val;
};

struct block_lattice_grammar 
    : public boost::spirit::classic::grammar<block_lattice_grammar, lattree_c::context_t>
{
    
    template <typename ST>
    struct definition {
        boost::spirit::classic::rule<ST>    lattice;
        
        definition(block_lattice_grammar const& self)
        {
            lattice = self.gram[ self.val = gusc::get_ast_(phoenix::arg1) ];
        }
        
        boost::spirit::classic::rule<ST> const& start() const 
        { return lattice; }  
    };
    gusc::lattice_grammar<gusc::lattice_ast_construct> gram;
};

////////////////////////////////////////////////////////////////////////////////

# endif //     UTILITIES__LATTICE_GRAMMAR_HPP
