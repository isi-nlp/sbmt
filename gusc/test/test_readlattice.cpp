# include <boost/test/auto_unit_test.hpp>
# include <gusc/lattice/ast.hpp>
# include <sstream>
# include <iostream>
# include <boost/lexical_cast.hpp>
# include <boost/tuple/tuple.hpp>
# include <fstream>

using namespace std;

string lat1 =
"lattice id=\"10\" {\n"
"[0,1] \"foo\" a=\"A\" b=\"B\";\n"
"[1,2] \"foo2\" a=\"A2\" b=\"B2\";\n"
"}";

string lat2 =
"lattice id=\"11\" {\n"
"[0,1] \"bar\" a=\"A\" b=\"B\";\n"
"[1,2] \"bar2\" a=\"A2\" b=\"B2\";\n"
"}";

BOOST_AUTO_TEST_CASE(read_single_lattice)
{
    gusc::lattice_ast ast;
    
    stringstream latin(lat1);
    stringstream latout;
    
    gusc::getlattice(latin,ast);
    
    latout << ast;
    
    BOOST_CHECK_EQUAL(latout.str(),lat1);
}

BOOST_AUTO_TEST_CASE(read_multiple_lattices)
{
    vector<gusc::lattice_ast> latvec;
    stringstream latin(lat1 + "\n\n" + lat2);
    stringstream latout;
    gusc::lattice_ast ast;
    int x = 0;
    while(gusc::getlattice(latin,ast)) {
        latout << ast << "\n";
        ++x;
    }
    BOOST_CHECK_EQUAL(x,2);
    BOOST_CHECK_EQUAL(latout.str(), lat1 + "\n" + lat2 + "\n");
}

BOOST_AUTO_TEST_CASE(add_properties)
{
    using boost::lexical_cast;
    using boost::tie;
    gusc::lattice_ast ast;
    
    stringstream latin(lat1);
    stringstream latout;
    
    gusc::getlattice(latin,ast);
    
    gusc::lattice_ast::line_iterator i,e;
    tie(i,e) = ast.lines();
    int x = 1;
    for (; i != e; ++i) {
        i->insert_property("c","C" + lexical_cast<string>(x));
        ++x;
    }
    ast.insert_property("g","G");
    
    string newlat =
    "lattice id=\"10\" g=\"G\" {\n"
    "[0,1] \"foo\" a=\"A\" b=\"B\" c=\"C1\";\n"
    "[1,2] \"foo2\" a=\"A2\" b=\"B2\" c=\"C2\";\n"
    "}";
    
    latout << ast;
    
    BOOST_CHECK_EQUAL(latout.str(), newlat);
}

# include <boost/iostreams/device/file_descriptor.hpp>
# include <boost/iostreams/stream.hpp>
/*
BOOST_AUTO_TEST_CASE(test_fd_read)
{
    const char* filename = GUSC_TEST_DIR "/latfile.txt";
    
    namespace io = boost::iostreams;
    typedef io::file_descriptor_source device_t;
    FILE* fs = fopen(filename, "r");
    device_t dev(fileno(fs));
    io::filtering_istream ifs(dev);
    gusc::lattice_ast ast;
    while (getlattice(ifs,ast)) {
        std::cout << ast << std::endl;
    }
    device_t dev2(fileno(fs));
    io::stream<device_t> ifs2(dev2);
    gusc::lattice_ast ast2;
    while (getlattice(ifs2,ast2)) {
        std::cout << ast2 << std::endl;
    }
    fclose(fs);
} */

# include <gusc/lattice/lattice.h>

void write_lat_line(FILE* file, lat_line* line);
void write_lat_properties(FILE* file, lat_handle* line);

void write_lat_lines(FILE* file, lat_handle* line)
{
    lat_line_itr itr = lat_lines_open(line);
    for (; lat_lines_has_next(&itr); lat_lines_next(&itr)) {
        lat_line ln = lat_get_line(&itr);
        write_lat_line(file,&ln);
        fprintf(file,";\n");
    }
    lat_lines_close(&itr);
}

void write_lat_line(FILE* file, lat_line* line)
{
    if (!lat_line_is_block(line)) {
        lat_span spn = lat_line_span(line);
        const char* lbl = lat_line_label(line);
        fprintf(file,"[%i,%i] \"%s\" ", spn.from, spn.to, lbl);
        write_lat_properties(file,line);
    } else {
        fprintf(file,"block ");
        write_lat_properties(file,line);
        fprintf(file," {\n");
        write_lat_lines(file,line);
        fprintf(file,"}");
    }
}

void write_lat_vertex_info(FILE* file, lat_vertex_info* vi)
{
    unsigned int id = lat_vertex_info_id(vi);
    const char* lbl = lat_vertex_info_label(vi);
    fprintf(file,"[%i] \"%s\" ",id,lbl);
    write_lat_properties(file,vi);
}

void write_lat_vertex_infos(FILE* file, lat_t* lat)
{
    lat_vertex_info_itr itr = lat_vertex_infos_open(lat);
    for (; lat_vertex_infos_has_next(&itr); lat_vertex_infos_next(&itr)) {
        lat_vertex_info vi = lat_get_vertex_info(&itr);
        write_lat_vertex_info(file,&vi);
        fprintf(file,";\n");
    }
}

void write_lat_property(FILE* file, lat_property* p)
{
    fprintf(file,"%s=\"%s\"",p->key, p->value);
}

void write_lat_properties(FILE* file, lat_handle* line)
{
    lat_property_itr itr = lat_properties_open(line);
    if (lat_properties_has_next(&itr)) {
        lat_property prop = lat_get_property(&itr);
        write_lat_property(file,&prop);
        lat_properties_next(&itr);
    }
    for (; lat_properties_has_next(&itr); lat_properties_next(&itr)) {
        lat_property prop = lat_get_property(&itr);
        fprintf(file," ");
        write_lat_property(file,&prop);
    }
}

void write_lat(FILE* file, lat_t* lat)
{
    fprintf(file,"lattice ");
    write_lat_properties(file,lat);
    fprintf(file," {\n");
    write_lat_vertex_infos(file,lat);
    write_lat_lines(file,lat);
    fprintf(file,"}");
}
/*
BOOST_AUTO_TEST_CASE(c_interface)
{
    const char* filename = GUSC_TEST_DIR "/latfile.txt";
    printf("opening %s\n",filename);
    FILE* file = fopen(filename, "r");
    lat_t lattice = lat_create();
    while (!feof(file)) {
        
        fread_lattice(file,&lattice);
        fwrite_lattice(stdout,&lattice);
        fprintf(stdout,"\n");
        write_lat(stdout,&lattice);
        fprintf(stdout,"\n");
    }
    fclose(file);
    lat_release(&lattice);
    
    
} */


