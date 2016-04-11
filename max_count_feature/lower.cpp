# include <boost/locale.hpp>

int main()
{
    //const boost::locale::generator gen;
    //const std::locale loc = gen.generate(std::locale(),"en_US.utf-8");
    //std::cin.imbue(loc);
    std::string line;
    while (getline(std::cin,line)) {
        std::cout << boost::locale::to_lower(line) << std::endl;
    }
    return 0;
}