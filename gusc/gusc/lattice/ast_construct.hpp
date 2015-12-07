# if ! defined(GUSC__LATTICE__AST_CONSTRUCT_HPP)
# define       GUSC__LATTICE__AST_CONSTRUCT_HPP

# include <gusc/lattice/ast.hpp>
# include <stack>
# include <gusc/phoenix_helpers.hpp>

namespace gusc {

////////////////////////////////////////////////////////////////////////////////
///
///  lattice_ast_construct implements the Callback concept for lattice_grammar,
///  with the callback methods designed to create a lattice_ast object from a
///  lattice source file
///
///  \see lattice_grammar<Callback>
///
////////////////////////////////////////////////////////////////////////////////
struct lattice_ast_construct {
    std::stack<lattice_ast::line_iterator> blockstack;
    lattice_ast ast;
    
    template <class Itr>
    void begin_block(Itr beg, Itr end)
    {
        lattice_ast::line_iterator bpos;
        if (blockstack.empty()) {
            bpos = ast.insert_block();
        } else {
            bpos = blockstack.top()->insert_block();
        }
        for (Itr itr = beg; itr != end; ++itr) {
            bpos->insert_property(boost::get<0>(*itr), boost::get<1>(*itr));
        }
        blockstack.push(bpos);
    }
    
    void end_block()
    {
        blockstack.pop();
    }
    
    template <class Itr>
    void new_edge( boost::tuple<size_t,size_t> const& span
                 , std::string const& lbl
                 , Itr beg, Itr end )
    {
        lattice_ast::line_iterator pos;
        if (blockstack.empty()) {
            pos = ast.insert_edge(span,lbl);
        } else {
            pos = blockstack.top()->insert_edge(span,lbl);
        }
        for (Itr itr = beg; itr != end; ++itr) {
            pos->insert_property(boost::get<0>(*itr), boost::get<1>(*itr));
        }
    }
    
    template <class Itr>
    void new_vertex(size_t id, std::string const& lbl, Itr beg, Itr end)
    {
        lattice_ast::vertex_info_iterator pos = ast.insert_vertex_info(id,lbl);
        for (Itr itr = beg; itr != end; ++itr) {
            pos->insert_property(boost::get<0>(*itr), boost::get<1>(*itr));
        }
    }
    
    template <class Itr>
    void properties(Itr beg, Itr end)
    {
        for (Itr itr = beg; itr != end; ++itr) 
            ast.insert_property(boost::get<0>(*itr), boost::get<1>(*itr));
    }
};

////////////////////////////////////////////////////////////////////////////////
///
///  this boost::phoenix style object makes it possible to extract a lattice_ast
///  from the lattice_ast_construct when it is stored in a lattice_grammar
///
////////////////////////////////////////////////////////////////////////////////
struct get_ast_ph {
    template <class G> struct result { typedef lattice_ast type; };
    template <class G> lattice_ast const& operator()(G const& g) const
    {
        return g.ast;
    } 
};

namespace {
phoenix::function<get_ast_ph> get_ast_;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc

# endif //     GUSC__LATTICE__AST_CONSTRUCT_HPP
