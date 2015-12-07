# include <iostream>
# include <sbmt/edge/any_info.hpp>

int main(int argc, char** argv)
{
    std::cerr << sbmt::info_registry_options() << std::endl;
    return 0;
}
