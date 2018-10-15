# include <gusc/lattice/lattice.h>
# include <gusc/lattice/ast.hpp>
# include <gusc/filesystem/syncbuf.hpp>
# include <iostream>
using namespace gusc;
using namespace std;

typedef property_container_interface::property_iterator property_iterator;
typedef pair<property_iterator,property_iterator> property_range;
typedef line_container_interface::line_iterator line_iterator;
typedef pair<line_iterator,line_iterator> line_range;
typedef lattice_ast::vertex_info_iterator vertex_info_iterator;
typedef pair<vertex_info_iterator,vertex_info_iterator> vertex_info_range;

////////////////////////////////////////////////////////////////////////////////

static property_container_interface* prop_c(lat_handle* h) 
{
    return (property_container_interface*)(h->p);
}

static property_range* prop_r(lat_property_itr* itr) 
{
    return (property_range*)(itr->p);
}

static latnode_interface* line_c(lat_handle* h) 
{
    return (latnode_interface*)(h->p);
}

static line_range* line_r(lat_line_itr* itr) 
{
    return (line_range*)(itr->p);
}

static lattice_ast::line* line_(lat_line* ln)
{
    return (lattice_ast::line*)(ln->p);
}

static lattice_ast* lat_(lat_t* lattice)
{
    return (lattice_ast*)(lattice->p);
}

static vertex_info_range* vertex_info_r(lat_vertex_info_itr* itr)
{
    return (vertex_info_range*)(itr->p);
}

static lattice_ast::vertex_info* vertex_info_(lat_vertex_info* vi)
{
    return (lattice_ast::vertex_info*)(vi->p);
}

////////////////////////////////////////////////////////////////////////////////

extern "C" {

////////////////////////////////////////////////////////////////////////////////

lat_t lat_create() 
{
    lat_t lattice = { new lattice_ast() };
    return lattice;
}

void lat_release(lat_t* lattice) 
{
    delete (lattice_ast*)(lattice->p);
}

////////////////////////////////////////////////////////////////////////////////

lat_property_itr lat_properties_open(lat_handle* h) 
{
    property_container_interface* pc = prop_c(h);
    lat_property_itr itr = { new property_range(pc->properties()) };
    return itr;
}

void lat_properties_next(lat_property_itr* itr) 
{
    prop_r(itr)->first++;
}

int lat_properties_has_next(lat_property_itr* itr) 
{
    return prop_r(itr)->first == prop_r(itr)->second ? 0 : 1;
}

lat_property lat_get_property(lat_property_itr* itr) 
{
    lattice_ast::key_value_pair kv = *(prop_r(itr)->first);
    lat_property p = { kv.key().c_str(), kv.value().c_str() };
    return p;
}

void lat_properties_close(lat_property_itr* itr) 
{
    delete (property_range*)(itr->p);
}

void lat_insert_property(lat_handle* h, lat_property* kv) 
{
    prop_c(h)->insert_property(kv->key,kv->value);
}

void lat_erase_property(lat_handle* h, lat_property_itr* pos) 
{
    property_range* pr = prop_r(pos);
    property_iterator old = pr->first;
    ++old;
    prop_c(h)->erase_property(pr->first);
    pr->first = old;
}

////////////////////////////////////////////////////////////////////////////////

lat_line_itr lat_lines_open(lat_handle* h) 
{
    line_container_interface* lc = line_c(h);
    lat_line_itr itr = { new line_range(lc->lines()) };
    return itr;
}

void lat_lines_next(lat_line_itr* itr) 
{
    line_r(itr)->first++;
}

int lat_lines_has_next(lat_line_itr* itr) 
{
    return line_r(itr)->first == line_r(itr)->second ? 0 : 1;
}

lat_line lat_get_line(lat_line_itr* itr) 
{
    lat_line ln = { &(*(line_r(itr)->first)) };
    //cout << "debug: line: " << *line_(&ln) << endl;
    return ln;
}

void lat_lines_close(lat_line_itr* itr) 
{
    delete (line_range*)(itr->p);
}

lat_line lat_insert_block(lat_handle* h) 
{
    lat_line ln = { &(*(line_c(h)->insert_block())) };
    return ln;
}

lat_line lat_insert_edge(lat_handle* h, lat_span* s, const char* lbl) 
{
    line_iterator itr = line_c(h)->insert_edge( lattice_vertex_pair(s->from, s->to)
                                              , lbl);
    lat_line ln = { &(*itr) };
    return ln;
}

void lat_erase_line(lat_handle* h, lat_line_itr* pos) 
{
    line_range* lr = line_r(pos);
    line_iterator old = lr->first;
    ++old;
    line_c(h)->erase_line(lr->first);
    lr->first = old;
}

////////////////////////////////////////////////////////////////////////////////

int lat_line_is_block(lat_line* ln)
{
    return line_(ln)->is_block() ? 1 : 0;
}

const char* lat_line_label(lat_line* ln)
{
    return line_(ln)->label().c_str();
}

lat_span lat_line_span(lat_line* ln)
{
    lattice_vertex_pair kv = line_(ln)->span();
    lat_span spn = { kv.from(), kv.to() };
    return spn;
}

////////////////////////////////////////////////////////////////////////////////

lat_vertex_info_itr lat_vertex_infos_open(lat_t* lattice)
{
    lattice_ast* lc = lat_(lattice);
    lat_vertex_info_itr itr = { new vertex_info_range(lc->vertex_infos()) };
    return itr;
}

void lat_vertex_infos_next(lat_vertex_info_itr* itr)
{
    vertex_info_r(itr)->first++;
}

int lat_vertex_infos_has_next(lat_vertex_info_itr* itr)
{
    return vertex_info_r(itr)->first == vertex_info_r(itr)->second ? 0 : 1;
}

lat_vertex_info lat_get_vertex_info(lat_vertex_info_itr* itr)
{
    lat_vertex_info vi = { &(*(vertex_info_r(itr)->first)) };
    return vi;
}

void lat_verex_infos_close(lat_vertex_info_itr* itr)
{
    delete (vertex_info_range*)(itr->p);
}

unsigned int lat_vertex_info_id(lat_vertex_info* vi)
{
    return vertex_info_(vi)->id();
}

const char* lat_vertex_info_label(lat_vertex_info* vi)
{
    return vertex_info_(vi)->label().c_str();
}

lat_vertex_info lat_insert_vertex_info(lat_t* h, unsigned int id, const char* lbl)
{
    vertex_info_iterator itr = lat_(h)->insert_vertex_info(id,lbl);
    lat_vertex_info vi = { &(*itr) };
    return vi;
}

void lat_erase_vertex_info(lat_t* h, lat_vertex_info_itr* pos)
{
    vertex_info_range* vr = vertex_info_r(pos);
    vertex_info_iterator old = vr->first;
    ++old;
    lat_(h)->erase_vertex_info(vr->first);
    vr->first = old;
}




////////////////////////////////////////////////////////////////////////////////

namespace gusc {

syncbuf::~syncbuf() {}

syncbuf::syncbuf(FILE* f)
   : std::streambuf(), fptr(f) {}

int syncbuf::overflow(int c) {
   return c != EOF ? fputc(c, fptr) : EOF;
}

int syncbuf::underflow() {
   int c = getc(fptr);
   if (c != EOF)
      ungetc(c, fptr);
   return c;
}

int syncbuf::uflow() {
   return getc(fptr);
}

int syncbuf::pbackfail(int c) {
   return c != EOF ? ungetc(c, fptr) : EOF;
}

int syncbuf::sync() {
   return fflush(fptr);
}

} // namespace gusc

////////////////////////////////////////////////////////////////////////////////

int fread_lattice(FILE* file, lat_t* lattice)
{
    syncbuf buf(file);
    std::istream str(&buf);
    getlattice(str,*lat_(lattice));
    return ferror(file);
}

int fwrite_lattice(FILE* file, lat_t* lattice)
{
    syncbuf buf(file);
    std::ostream str(&buf);
    str << *lat_(lattice) << std::flush;
    return ferror(file);
}

////////////////////////////////////////////////////////////////////////////////

} // extern "C"
