# if ! defined(GUSC__LATTICE__AST_HPP)
# define       GUSC__LATTICE__AST_HPP

# include <boost/tuple/tuple.hpp>
# include <utility>
# include <string>
# include <list>
# include <vector>
# include <boost/shared_ptr.hpp>
# include <stdexcept>
# include <iosfwd>

namespace gusc {

class lattice_ast;

class lattice_vertex_pair {
public:
    size_t from() const;
    size_t to() const;
    lattice_vertex_pair(size_t f, size_t t);
    lattice_vertex_pair(boost::tuple<size_t,size_t> const& p);
private:
    boost::tuple<size_t,size_t> p;
};

////////////////////////////////////////////////////////////////////////////////

class property_container_interface {
public:
    struct key_value_pair {
        std::string const& key() const;

        std::string const& value() const;

        key_value_pair(boost::tuple<std::string,std::string> const& p);

        key_value_pair(std::string const& k, std::string const& v);
    private:
        boost::tuple<std::string,std::string> p;
    };
protected:
    typedef std::list<key_value_pair> property_container_t;
public:
    typedef property_container_t::iterator property_iterator;
    typedef property_container_t::const_iterator const_property_iterator;
    
    virtual 
    std::pair<const_property_iterator,const_property_iterator> 
    properties() const = 0;
    
    virtual 
    std::pair<property_iterator,property_iterator> 
    properties() = 0;

    virtual
    property_iterator insert_property( std::string const& key
                                     , std::string const& value ) = 0;

    virtual
    void erase_property(property_iterator pos) = 0;
protected:
    virtual ~property_container_interface() {}
};

////////////////////////////////////////////////////////////////////////////////

class property_container : public property_container_interface {
private:
    property_container_t props;
public:
    virtual
    std::pair<const_property_iterator,const_property_iterator> properties() const;
    virtual
    std::pair<property_iterator,property_iterator> properties();
    virtual
    property_iterator insert_property( std::string const& key
                                     , std::string const& value );                             
    virtual
    void erase_property(property_iterator pos);

    property_container() {}
    template <class Itr>
    property_container(Itr begin, Itr end) : props(begin,end) {}
    virtual ~property_container() {}
};

////////////////////////////////////////////////////////////////////////////////

struct lattice_line;

struct lattice_line_impl;

////////////////////////////////////////////////////////////////////////////////

struct line_container_interface {
    typedef lattice_line line;
    typedef lattice_vertex_pair vertex_pair;
    typedef std::list<lattice_line> container_t;
    typedef container_t::iterator line_iterator;
    typedef container_t::const_iterator const_line_iterator;
    
    virtual 
    std::pair<const_line_iterator,const_line_iterator> lines() const = 0;

    virtual 
    std::pair<line_iterator,line_iterator> lines() = 0;

    virtual
    line_iterator insert_edge( vertex_pair const& spn
                             , std::string const& lbl ) = 0;

    virtual
    line_iterator insert_block() = 0;
    
    virtual 
    void erase_line(line_iterator pos) = 0;
protected:
    virtual ~line_container_interface() {}
};

////////////////////////////////////////////////////////////////////////////////

struct line_container : line_container_interface {
    virtual 
    std::pair<const_line_iterator,const_line_iterator> lines() const;
    virtual 
    std::pair<line_iterator,line_iterator> lines();
    virtual
    line_iterator insert_edge( vertex_pair const& spn
                             , std::string const& lbl );
    virtual
    line_iterator insert_block();
    
    virtual 
    void erase_line(line_iterator pos);

    virtual ~line_container() {}
private:
    container_t lns;
};

struct latnode_interface : property_container_interface
                         , line_container_interface {};

struct latnode : latnode_interface {
public:
    virtual 
    std::pair<const_line_iterator,const_line_iterator> lines() const
    { return lns.lines(); }
    virtual 
    std::pair<line_iterator,line_iterator> lines()
    { return lns.lines(); }
    virtual
    line_iterator insert_edge( vertex_pair const& spn
                             , std::string const& lbl )
    { return lns.insert_edge(spn,lbl); }
    virtual
    line_iterator insert_block()
    { return lns.insert_block(); }
    
    virtual 
    void erase_line(line_iterator pos)
    { lns.erase_line(pos); }
    
    virtual
    std::pair<const_property_iterator,const_property_iterator> properties() const
    { return props.properties(); }
    virtual
    std::pair<property_iterator,property_iterator> properties()
    { return props.properties(); }
    virtual
    property_iterator insert_property( std::string const& key
                                     , std::string const& value )
    { return props.insert_property(key,value); }                      
    virtual
    void erase_property(property_iterator pos)
    { return props.erase_property(pos); }
private:
    line_container lns;
    property_container props;
};

////////////////////////////////////////////////////////////////////////////////

struct lattice_line : latnode_interface {
    typedef lattice_vertex_pair vertex_pair;
    
    bool is_block() const;
    
    lattice_vertex_pair span() const;
    
    std::string const& label() const;
    
    virtual
    std::pair<line_iterator,line_iterator> lines();

    virtual
    std::pair<const_line_iterator,const_line_iterator> lines() const;
    
    virtual 
    std::pair<const_property_iterator,const_property_iterator> 
    properties() const;
    
    virtual 
    std::pair<property_iterator,property_iterator> 
    properties();

    virtual
    property_iterator insert_property( std::string const& key
                                     , std::string const& value );

    virtual
    void erase_property(property_iterator pos);

    virtual
    line_iterator insert_edge(vertex_pair const& spn, std::string const& lbl);

    virtual
    line_iterator insert_block();

    void erase_line(line_iterator pos);

    lattice_line( vertex_pair const& spn, std::string const& lbl );
    
    lattice_line();
    
private:
    boost::shared_ptr<lattice_line_impl> impl;
    friend class lattice_ast;
};

////////////////////////////////////////////////////////////////////////////////

struct lattice_line_impl : latnode {
    typedef lattice_vertex_pair vertex_pair;
    bool isblk;
    vertex_pair spn;
    std::string lbl;
    lattice_line_impl();
    lattice_line_impl(vertex_pair const& spn, std::string const& lbl);
};

////////////////////////////////////////////////////////////////////////////////

class lattice_ast : public latnode {
public:    
    typedef lattice_vertex_pair vertex_pair;
    
    struct vertex_info : property_container {
    private:
        size_t id_;
        std::string label_;
        vertex_info(size_t id, std::string const& lbl);
    public:
        friend class lattice_ast;
        std::string const& label() const;
        size_t id() const;
    };
    
    ////////////////////////////////////////////////////////////////////////////
    
    typedef std::list<vertex_info>::iterator vertex_info_iterator;
    typedef std::list<vertex_info>::const_iterator const_vertex_info_iterator;
    
    vertex_info_iterator insert_vertex_info( size_t id
                                           , std::string const& label );
                                           
    void erase_vertex_info(vertex_info_iterator pos);

    std::pair<const_vertex_info_iterator,const_vertex_info_iterator> 
        vertex_infos() const;
    
    std::pair<vertex_info_iterator,vertex_info_iterator> vertex_infos();

private:
    std::list<vertex_info> vertices;
};

////////////////////////////////////////////////////////////////////////////////

std::ostream& operator << (std::ostream&, lattice_ast::key_value_pair const&);

std::ostream& operator << (std::ostream&, lattice_ast::vertex_pair const&);

std::ostream& operator << (std::ostream&, lattice_ast::vertex_info const&);

std::ostream& operator << (std::ostream&, lattice_ast::line const&);

std::ostream& operator << (std::ostream&, lattice_ast const&);

////////////////////////////////////////////////////////////////////////////////

std::istream& getlattice(std::istream&, lattice_ast&);

std::istream& operator >> (std::istream&, lattice_ast&);

////////////////////////////////////////////////////////////////////////////////

} // namespace gusc

# endif //     GUSC__LATTICE__AST_HPP
