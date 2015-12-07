# include <boost/random/uniform_real.hpp>
# include <boost/random/mersenne_twister.hpp>
# include <boost/lexical_cast.hpp>
# include <iostream>
# include <gusc/generator/product_heap_generator.hpp>
# include <gusc/generator/lazy_sequence.hpp>
# include <gusc/generator/any_generator.hpp>

boost::mt19937 gen(time(0));

class noisygen {
public:
    typedef double result_type;
    noisygen(double n, double m) : n(n), x(-m,m), k(0) {}
    double operator()() { return n*k++ + x(gen); }
    operator bool() { return true; }
private:
    double n;
    boost::uniform_real<> x;
    int k;
};

class plus {
public:
    typedef double result_type;
    double operator()(double a, double b, double c) const
    {
        return a + b + c;
    }
};

int main(int argc, char** argv)
{
    double a = boost::lexical_cast<double>(argv[1]);
    double b = boost::lexical_cast<double>(argv[2]);
    double c = boost::lexical_cast<double>(argv[3]);
    

    double avg = 0.0;
    int N = 100000;
    for (int x = 0; x != N; ++x) {
        gusc::any_generator<double> g = gusc::generate_product_heap(
                                          plus()
                                        , std::greater<double>()
                                        , gusc::make_lazy_sequence(noisygen(a,5))
                                        , gusc::make_lazy_sequence(noisygen(b,5))
                                        , gusc::make_lazy_sequence(noisygen(c,5))
                                        );
        int n = 0;
        double s = 0.0;
        while (g and n != 100) {
            ++n;
            double d = g();
            s += d;
            //std::cout << d << '\n';
        }
        avg += s/N;
    }
    std::cout << avg << '\n';
    return 0;
}