# include <xrs_decoder.hpp>
# include <composite_edge_interface.hpp>

int main(int argc, char** argv)
{
    xrs_decoder<composite_edge_interface> decoder;
    return decoder.run(argc,argv);
}
