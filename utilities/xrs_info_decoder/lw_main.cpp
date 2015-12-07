# include <xrs_decoder.hpp>
# include <ngram_edge_interface.hpp>

int main(int argc, char** argv)
{
    xrs_decoder< ngram_edge_interface<5> > decoder;
    return decoder.run(argc,argv);
}
