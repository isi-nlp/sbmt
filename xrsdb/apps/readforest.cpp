# include <forest_reader.hpp>
# include <iostream>
int main()
{
    exmp::forest fst;
    sbmt::feature_dictionary fdict;
    while (getforest(std::cin, fst, fdict)) {
        std::cout << "read\n" << fst << '\n';
    }
    return 0;
}