#include <iterator>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <ext/hash_set>

using namespace std;

class A {
    int i;
    __gnu_cxx::hash_set<int > H;
public:
    A(int a=0) : i(a) {}
    void print() { std::cout<<i<<endl;
        copy(H.begin(), H.end(), ostream_iterator<int>(cout, " "));
    }

    void dd() { H.insert(1);H.insert(2);}

   

};

int main()
{
    A u(1);
    u.dd();
    A b=u;

    b.print();

    return 0;

    ifstream datafile("test.txt");
    vector<string> bob;
    __gnu_cxx::hash_set<string> H;

    copy(istream_iterator<string>(datafile),istream_iterator<string>(),inserter(bob, bob.begin()));
    copy(bob.begin(), bob.end(), ostream_iterator<string>(cout, " "));

    return 0;
}
