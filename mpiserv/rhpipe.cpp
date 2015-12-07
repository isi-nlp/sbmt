# include <rhpipe.hpp>
# include <signal.h>
# include <unistd.h>
# include <boost/algorithm/string.hpp>

// impl
namespace bp = boost::process;
using namespace std;

struct ignore_sigpipe {
    struct sigaction old;
    ignore_sigpipe()
    {
        struct sigaction s;
        memset(&s,0,sizeof(s));
        s.sa_handler = SIG_IGN;
        sigaction(SIGPIPE,&s,&old);
    }
    ~ignore_sigpipe()
    {
        sigaction(SIGPIPE,&old,0);
    }
};

bp::child rhpipe::make(string exec)
{
    bp::context ctxt;
    ctxt.stdout_behavior = bp::capture_stream();
    ctxt.stdin_behavior = bp::capture_stream();
    ctxt.stderr_behavior = bp::inherit_stream();
    ctxt.environment = bp::self::get_environment(); 
    for (int x = 0;; ++x) {
        try {
	  cerr << "calling launch_shell...\n";
	  bp::child subproc = bp::launch_shell(exec,ctxt);
          ::sleep(6);
	  cerr << "called launch_shell\n";
	  if (!subproc.get_stdout().good() or !subproc.get_stdin().good()) {
	    cerr << "trying launch_shell again\n";
	    continue;
	  } else {
	    return subproc;
	  }
        } catch(...) {
            if (x <= 10) {
	        cerr << "trying thrown launch_shell again\n";
                ::sleep(6);
                continue;
            } else {
                throw;
            }
        }
    }
}

int 
rhpipe::close() 
{ 
    {
        ignore_sigpipe ignore;
        subproc.get_stdin().close(); 
    }
    bp::status s = subproc.wait();
    assert(s.exited());
    return s.exit_status();
}

void 
rhpipe::send(string const& msg) 
{ 
    ignore_sigpipe ignore;
    if (!(subproc.get_stdin() << msg << endl)) {
        throw runtime_error("broken pipe");
    } 
}

void 
rhpipe::pop() 
{   
    //std::cerr << "begin pop\n";
    valid = getline(subproc.get_stdout(),curr); 
    //std::cerr << "end pop\n";
}

void rhpipe::start()
{
    if (not init) {
        //std::cerr << "begin rhpipe::start()\n";
        valid = getline(subproc.get_stdout(),curr);
        //std::cerr << "end rhpipe::start()\n";
        init = true;
    }
}

bool 
rhpipe::more() const 
{ 
    if (not init) {
        throw logic_error("call start before any pipe interface call");
    }
    return valid; 
}

string const& 
rhpipe::peek() const { return curr; }

rhpipe::rhpipe(string exec) 
  : subproc(make(exec))
  , valid(false)
  , init(false) {}

void 
array_pipe::pop()
{
    string key;
    curr.clear();
    //std::cerr << "begin array_pop::pop()\n";
    while (bool(pipe)) {
        valid = true;
        vector<string> splitarray;
        //std::cerr << "begin array_pop::split()\n";
        boost::split(splitarray,*pipe,boost::is_any_of("\t"));
        //std::cerr << "end array_pop::split()\n";
        if (curr.empty() or splitarray[0] == key) {
            if (key == "") key = splitarray[0];
            curr.push_back(
              boost::join(
                boost::make_iterator_range(splitarray.begin() + 1, splitarray.end())
              , "\t"
              )
            );
            ++pipe;
        } else {
            break;
        }
    }
    if (not valid) std::cerr << "WARNING rhpipe was empty while array_pipe was empty\n";
    valid = not curr.empty();
    //std::cerr << "end array_pop::pop()\n";
}

bool 
array_pipe::more() const
{
    return valid;
}

void 
array_pipe::start()
{
    pipe.start();
    pop();
    if (!valid) std::cerr << "WARNING initial array_pipe::pop() came back empty (did it fail to block waiting for stdout?) in start()\n";
}

vector<string> const& 
array_pipe::peek() const
{
    return curr;
}

array_pipe::array_pipe(string exec) : pipe(exec), valid(false) {}
