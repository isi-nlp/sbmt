# include <boost/python.hpp>
# include <rhpipe.hpp>
# include <gusc/iterator/iterator_from_generator.hpp>
# include <boost/iterator/transform_iterator.hpp>

using namespace boost::python;
struct gil_release {
    gil_release() : save(PyEval_SaveThread()) {}
    ~gil_release() { PyEval_RestoreThread(save); }
    PyThreadState* save;
};

struct norefs {
    typedef std::string result_type;
    std::string operator()(std::string const& s) const { return s; }
};

struct pypipe {
    typedef std::string result_type;
    pypipe(std::string exec) 
    {
        gil_release g;
        rh.reset(new rhpipe(exec));
    }
    void send(std::string s)
    {
        //std::cerr << "send()\n";
        try {
            gil_release g;
            rh->send(s);
        } catch(std::runtime_error const& e) {
            //std::cerr << "caught: " << e.what() << '\n';
            throw;
        }
    }
    int close()
    {
        //std::cerr << "close()\n";
        try {
            gil_release g;
            return rh->close();
        } catch(...) {
            throw;
        }
    }
    void start() 
    {
        //std::cerr << "start()\n";
        try {
            gil_release g;
            rh->start();
        } catch(...) {
            throw;
        }
    }
    operator bool() const
    {
        //std::cerr << "bool() -> ";
        bool x = bool(*rh);
        //std::cerr << std::boolalpha << x;
        return x;
    }
    std::string operator()() 
    {
        //std::cerr << "operator()() -> ";
        try {
            gil_release g;
            std::string st = (*rh)();
            //std::cerr << st << '\n';
            return st;
        } catch(...) {
            throw;
        }
    }
    boost::shared_ptr<rhpipe> rh;
};

typedef boost::transform_iterator< norefs, gusc::iterator_from_generator<pypipe> >
        pypipe_iterator;

pypipe_iterator begin(pypipe const& p)
{
    //std::cerr << "iterator-from-generator::begin\n";
    return pypipe_iterator(gusc::iterator_from_generator<pypipe>(p));
}

pypipe_iterator end(pypipe const& p)
{
    //std::cerr << "iterator-from-generator::end\n";
    return pypipe_iterator(gusc::iterator_from_generator<pypipe>());
}

BOOST_PYTHON_MODULE(rhproc)
{
    class_<pypipe>("Pipe",init<std::string>())
      .def("start",&pypipe::start)
      .def("send",&pypipe::send)
      .def("close",&pypipe::close)
      .def("__iter__",range(&begin,&end))
      ;
}