# include <boost/locale.hpp>

int main()
{
    const boost::locale::generator gen;
    const std::locale loc = gen("en_US.utf-8");
    //std::wcout.imbue(loc);
    std::string line;
    std::ios_base::sync_with_stdio(false);
    while (getline(std::cin,line)) {
      std::cout << boost::locale::to_lower(line,loc) << std::endl;
    }
    return 0;
}
