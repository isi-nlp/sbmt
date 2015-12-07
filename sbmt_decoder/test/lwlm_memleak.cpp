#include <boost/test/auto_unit_test.hpp>
#include <boost/ref.hpp>
#include <sbmt/logmath.hpp>
#include <sbmt/ngram/LWNgramLM.hpp>
#include <map>
#include <set>

//#include "test_util.hpp"

using namespace sbmt;
using namespace sbmt::logmath;
using namespace std;

////////////////////////////////////////////////////////////////////////////////

int main(int argc,char *argv[])
{
    LWNgramLM lm;
    std::string a="lw.ngram";
    if (argc>1) {
        a=argv[1];
        lm.read(a);
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
