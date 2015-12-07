# include <rhpipe.hpp>
# include <boost/thread.hpp>
# include <algorithm>
# include <iostream>

using namespace std;

void write(array_pipe& pipe)
{
    cerr << "writer opened" << endl;
    pipe.start();
    while (pipe) {
        copy(pipe->begin(),pipe->end(),ostream_iterator<string>(cout," "));
        cout << endl;
        ++pipe;
    }
    cerr << "writer closed" << endl;
}

void read(array_pipe& pipe)
{
    cerr << "reader opened" << endl;
    string line;
    while (getline(cin,line)) {
        try {
            pipe.send(line);
        } catch(runtime_error const& e) {
            cerr << "pipe threw "<< e.what() << '\n';
            break;
        }
    }
    int ecode = pipe.close();
    cerr << "reader closed. exit=" << ecode <<"\n";
}


int main(int argc, char** argv)
{
    cin.tie(0);
    
    string exec = argv[1];
    for (char** argi = argv + 2; argi != argv + argc; ++argi) {
        exec += ' ' + *argi;
    }
    while (true) {
        array_pipe pipe(exec);

        boost::thread async(boost::bind(&write,boost::ref(pipe)));
        read(pipe);
        cerr << "calling join\n";
        async.join();
        cerr << "joined\n";
    }
    return 0;
}