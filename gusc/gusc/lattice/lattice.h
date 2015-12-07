# if ! defined(GUSC__LATTICE__LATTICE_H)
# define       GUSC__LATTICE__LATTICE_H

# include <stdio.h>

# if defined(__cplusplus)
extern "C" {
# endif

/* lat is shorthand for lattice abstract syntax tree */

/* lat_handle can be an entire lattice, or a line or block in the lattice. 
   typedefs and function signatures should tell you which it is 
 */
struct lat_handle {
    void* p;
};

/* the toplevel lattice object. lattice objects must always be initialized
   with lat_create(), and released with lat_release()
 */
typedef lat_handle lat_t;

/* a lat_line is either a block or an edge in the lattice.
   data in a lat_line is managed by the owning lattice, and is valid for the
   life of the lattice (as long as the line hasnt been released by a call to
   lat_erase_line()).  there is no need to release anything
 */
typedef lat_handle lat_line;

/* an iterator over a collection of lat_lines.
  lat_line_itr must be initialized with lat_lines_open(), and released with
  lat_lines_close()
  \code
  lat_line_itr itr = lat_lines_open(&lat_or_line);
  ...
  lat_lines_close(&itr);
  \endcode
*/
struct lat_line_itr {
    void* p;
};

/* vertex_info is optional in a lattice. that is, you can create edges without
   also having vertex_info. its only if you want to attach properties to 
   vertices.  the decoder ignores vertex information
   a lat_vertex_info is valid for the life of the lattice (as long as it 
   hasnt been released by a call to lat_erase_vertex_info()). there is no need
   to release
 */
typedef lat_handle lat_vertex_info;

/* an iterator over a collection of lat_vertex_info.
  lat_vertex_info_itr is initialized with lat_vertex_infos_open(), and released
  with lat_vertex_infos_close()
  only the top-level lattice stores vertex_info (lat_lines do not)
  \code
  lat_vertex_info_itr itr = lat_vertex_infos_open(&lat);
    ...
  lat_vertex_infos_close(&itr);
  \endcode
 */
struct lat_vertex_info_itr {
    void* p;
};

/* lattices, lines, and vertex_info may all have properties attached to them.
   properties are simply a pair of strings called keys/values.  despite the 
   name, they do not constitute an associative array, in that you cannot query
   for a value given a key.
   
   there is no need to create/delete a lat_property.  the data inside is valid
   for the life of the owning lattice, (as long as no call has been made to
   lat_erase_property()), and will be cleaned up on lat_release()
 */
struct lat_property {
    const char* key;
    const char* value;
};

/* an iterator over a collection of properties
   lat_property_itr must be initialized with lat_properties_open() and  
   need to be released with lat_property_close()
 */
struct lat_property_itr {
    void* p;
};

/* the span of a given edge or block */
struct lat_span {
    const unsigned int from;
    const unsigned int to;
};

/* create/destroy a lat. do not attempt to manipulate any lattice object that
   was not first created with lat_create
   
   lattices contain properties, vertex_infos, and lines (edges or blocks)
   
   an example of printing a lattice:
   \code
   
   void print_lat(file, lat_t* lat)
   {
       printf("lattice ");
       print_lat_properties(lat); 
       printf(" {\n");
       print_vertex_infos(lat);
       print_lat_lines(lat);
       printf("}");
   }
   
   lat_t lat = lat_create();
   FILE* file = fopen("lattice.txt","r");
   while (!feof(file)) {
       fread_lattice(file,&lat);
       print_lat(&lat);
   }
   lat_release(&lat);
   \endcode
 */ 
lat_t lat_create();
void lat_release(lat_t*);

/* retrieve a range of properties from a lat_t, lat_line, or lat_vertex_info
   use in for loops as in:
   \code
   void print_lat_property(lat_property* p)
   {
       // note this function does not properly escape the value string.
       // the real fwrite_lattice function does.
       printf("%s=\"%s\"",p->key, p->value);
   }
   void print_lat_properties(lat_handle* line)
   {
       lat_property_itr itr = lat_properties_open(line);
       if (lat_properties_has_next(&itr)) {
           lat_property prop = lat_get_property(&itr);
           print_lat_property(&prop);
           lat_properties_next(&itr);
       }
       for (; lat_properties_has_next(&itr); lat_properties_next(&itr)) {
           lat_property prop = lat_get_property(&itr);
           printf(" ");
           print_lat_property(&prop);
       }
   }
   \endcode
 */
lat_property_itr lat_properties_open(lat_handle*);
void             lat_properties_next(lat_property_itr*);
int              lat_properties_has_next(lat_property_itr*);
lat_property     lat_get_property(lat_property_itr*);
void             lat_properties_close(lat_property_itr*);

/* insert a key/value property into a lattice, line, or vertex_info */
void lat_insert_property(lat_handle*, lat_property*);
void lat_erase_property(lat_handle*, lat_property_itr*);

/* retrieve the lines (edges or blocks) from a lattice or block 
   \code
   void print_lat_lines(lat_handle* line)
   {
       lat_line_itr itr = lat_lines_open(line);
       for (; lat_lines_has_next(&itr); lat_lines_next(&itr)) {
           lat_line ln = lat_get_line(&itr);
           print_lat_line(&ln);
           printf(";\n");
       }
       lat_lines_close(&itr);
   }

   void print_lat_line(lat_line* line)
   {
       if (!lat_line_is_block(line)) {
           lat_span spn = lat_line_span(line);
           const char* lbl = lat_line_label(line);
           printf("[%i,%i] \"%s\" ", spn.from, spn.to, lbl);
           print_lat_properties(line);
       } else {
           printf("block ");
           print_lat_properties(line);
           printf(" {\n");
           print_lat_lines(line);
           printf("}");
       }
   }
 */
lat_line_itr lat_lines_open(lat_handle*);
void         lat_lines_next(lat_line_itr*);
int          lat_lines_has_next(lat_line_itr*);
lat_line     lat_get_line(lat_line_itr*);
void         lat_lines_close(lat_line_itr*);

/* insert a block into lattice or a parent block 
   invalidates sibling lat_line_ptr references.
 */
lat_line lat_insert_block(lat_handle*);

/* insert an edge into a lattice or a parent block. 
   specify the span markers and a label (or NULL).
   invalidates sibling lat_line_ptr references.
 */
lat_line lat_insert_edge(lat_handle*, lat_span*, const char*);

/* erase a line.  */
void lat_erase_line(lat_handle*, lat_line_itr*);

/* query a lattice line */
int         lat_line_is_block(lat_line*);
const char* lat_line_label(lat_line*);
lat_span    lat_line_span(lat_line*);

lat_vertex_info_itr lat_vertex_infos_open(lat_t*);
void                lat_vertex_infos_next(lat_vertex_info_itr*);
int                 lat_vertex_infos_has_next(lat_vertex_info_itr*);
lat_vertex_info     lat_get_vertex_info(lat_vertex_info_itr*);
void                lat_verex_infos_close(lat_vertex_info_itr*);

unsigned int lat_vertex_info_id(lat_vertex_info*);
const char*  lat_vertex_info_label(lat_vertex_info*);

lat_vertex_info lat_insert_vertex_info(lat_t*, unsigned int, const char*);
void            lat_erase_vertex_info(lat_t*, lat_vertex_info_itr*);

int fread_lattice(FILE*,lat_t*);
int fwrite_lattice(FILE*,lat_t*);

# if defined(__cplusplus)
} /* extern "C"  */
# endif

# endif /*     GUSC__LATTICE__LATTICE_H */
