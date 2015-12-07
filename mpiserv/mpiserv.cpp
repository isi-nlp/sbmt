# include <rhpipe.hpp>
# include <boost/mpi.hpp>
# include <boost/serialization/vector.hpp>
# include <boost/serialization/map.hpp>
# include <boost/serialization/string.hpp>
# include <boost/function.hpp>
# include <boost/format.hpp>
# include <boost/thread.hpp>
# include <boost/thread/condition.hpp>
# include <cassert>

namespace mpi = boost::mpi;
using namespace std;
using boost::format;

//destination
int from_server = 1;
int to_client = 1;
int from_client = 2; 
int to_server = 2;

//tag
int load_msg = 3;

// mpi jobs
int submitter = 0;
int receiver = 1;

int loadmax = 2;
int max_attempts = 3;

struct message {
    enum type_t { uninit, clientload, flush, finish, input, output };
    message(type_t type) : type(type) { assert(type <= finish); }
    message(int task, type_t type) : type(type), task(task) { assert(type == finish); }
    message(int task, int attempt, string const& in) 
      : type(input)
      , task(task)
      , attempt(attempt)
      , in(in) {}
      
   message() : type(uninit), task(-1) {}
    
   message(int task, vector<string> const& out) 
     : type(output)
     , task(task)
     , out(out) {}
    
    type_t type;
    int task;
    int attempt;
    string in;
    vector<string> out;
    
    template <class A>
    void serialize(A& a, const unsigned int v)
    {
        a & type;
        a & task;
        a & attempt;
        a & in;
        a & out;
    }
};

typedef map<int,int> loadmap;
typedef vector<string> result;
loadmap clientload(mpi::communicator const& world)
{
    loadmap cload;
    for (int x = 2; x != world.size(); ++x) cload[x] = 0;
    return cload;
}

////////////////////////////////////////////////////////////////////////////////
//
// return minimal load client. 
// communicates with receiver to keep track of what has been submitted to a 
// client, and what has been completed.
//
////////////////////////////////////////////////////////////////////////////////
int find_min(mpi::communicator& world, loadmap& cload)
{
    int lm,client;
    lm = loadmax;
    client = 2;
    while (lm >= loadmax) {
        boost::tie(client,lm) = *(cload.begin());
        int c, x;
        BOOST_FOREACH(boost::tie(c,x), cload) {
            if (x < lm) {
                lm = x;
                client = c;
            }
        }
        if (lm >= loadmax) {
            cerr << "sender: all clients busy. send load request to receiver\n";
            world.send(receiver,to_server,message(message::clientload));
            loadmap diff;
            cerr << "sender: all clients busy. waiting for client load from receiver\n";
            world.recv(receiver,load_msg,diff);
            BOOST_FOREACH(boost::tie(c,x),diff) {
                cload[c] -= x;
            }
        }
    }
    return client;
}

void send_to_client(mpi::communicator& world)
{
    int count = 1;
    loadmap cload = clientload(world);
    string line;
    while (getline(cin,line)) {
        int id = find_min(world,cload);
        cerr << format("sender: send task %s to client %s\n") % count % id;
        world.send(id,to_client,message(count,1,line));
        cload[id] += 1;
        ++count;
    }
    cerr << "sender: send finish request to receiver\n";
    world.send(receiver,to_server,message(count,message::finish));
    cerr << "sender: finished\n";
}

struct threadstate {
    boost::mutex mtx;
    boost::condition cnd;
    deque<message::type_t> messages;
    bool jobs_in_finished;
    deque<message> jobs_in;
    threadstate() : jobs_in_finished(false) {}
};

void receive_from_server(mpi::communicator& world, array_pipe& proc, threadstate& state)
{
    while (true) {
        message msg;
        {
            boost::mutex::scoped_lock lk(state.mtx);
            while (state.jobs_in.size() >= unsigned(loadmax) and not state.jobs_in_finished) {
                cerr << format("client %s: recv waiting for lower load\n") % world.rank();
                state.cnd.wait(lk);
            }
            if (not state.jobs_in_finished) {
                cerr << format( "client %s: recv resuming\n") % world.rank();
                world.recv(mpi::any_source,from_server,msg);
                if (msg.type == message::finish or msg.type == message::flush) {
                    state.messages.push_back(msg.type);
                    state.jobs_in_finished = true;
                    state.cnd.notify_all();
                    break;
                }
            } else {
                break;
            }
            cerr << format("client %s: recv task %s %s\n") % world.rank() % msg.task % msg.in;
            state.jobs_in.push_back(msg);
        }
        
        try {
            proc.send(msg.in);
            cerr << format("client %s: sent task %s %s to proc\n") % world.rank() % msg.task % msg.in;
        } catch(runtime_error const& e) {
            boost::mutex::scoped_lock lk(state.mtx);
            state.messages.push_back(message::flush);
            state.jobs_in_finished = true;
            state.cnd.notify_all();
        }
    }
    proc.close();
}

void send_to_server(mpi::communicator& world, array_pipe& proc, threadstate& state)
{
    cerr << format("client %s: send_to_server started\n") % world.rank();
    proc.start();
    cerr << format("client %s: proc started\n") % world.rank();
    while (proc) { 
        vector<string> result = *proc;
        cerr << format("client %s: got result\n") % world.rank();
        {
            boost::mutex::scoped_lock lk(state.mtx);
            assert(not state.jobs_in.empty());
            message msg = state.jobs_in.front();
            state.jobs_in.pop_front();
            cerr << format("client %s: send task %s %s to receiver\n") % world.rank() % msg.task % msg.in; 
            world.send(receiver,to_server,message(msg.task,result));
            state.cnd.notify_all();
        }
        ++proc;
    }
    {
        boost::mutex::scoped_lock lk(state.mtx);
        state.jobs_in_finished = true;
        state.messages.push_back(message::flush);
        cerr << format("client %s: placed flush request\n") % world.rank();
        while (not state.jobs_in.empty()) {
	    message msg = state.jobs_in.front();
 	    cerr <<format("client %s: send task %s %s to receiver\n") % world.rank() % msg.task % msg.in;
            world.send(receiver,to_server,msg);
            state.jobs_in.pop_front();
        }
	cerr << format("client %s: no more tasks to send to receiver\n") % world.rank();
        state.cnd.notify_all();
    }
    cerr << format("client %s: send_to_server closing\n") % world.rank();
}

void run_client(mpi::communicator& world, string cmd)
{
    try {
        while (true) {
            using boost::ref;
            using boost::bind;
            using boost::thread;
            threadstate state;
            array_pipe proc(cmd);
            //thread recv(bind(receive_from_server,ref(world),ref(proc),ref(state)));
            thread send(bind(send_to_server,ref(world),ref(proc),ref(state)));
            receive_from_server(world,proc,state);
            message::type_t msg;
            {
                boost::mutex::scoped_lock lk(state.mtx);
                while (state.messages.empty()) state.cnd.wait(lk);
                msg = state.messages.front();
            }
            send.join();
            if (msg == message::flush) {
                cerr << format("client %s: flushed\n") % world.rank(); 
                continue;
            } else {
                cerr << format("client %s: finished\n") % world.rank();
                return;
            }
        }
    } catch (boost::system::system_error const& e) {
        cerr << format("client %s: failed to start %s:\n%s\nrejecting all future tasks\n") % world.rank() % cmd % e.what();
    }
    while (true) {
        message msg;
        world.recv(mpi::any_source,from_server,msg);
        if (msg.type == message::flush) continue;
        else if (msg.type == message::finish) return;
        else world.send(receiver,to_server,msg);
    }
}

void submit_load(mpi::communicator& world, loadmap& cload)
{
    world.send(submitter,load_msg,cload);
    cload = clientload(world);
}

void receive_from_client(mpi::communicator& world,boost::function<void(message const&)> send)
{
    int ns = 2;
    bool finished = false; 
    int minrecv = 1; 
    bool recvreq = false; 
    loadmap cload = clientload(world);
    map<int,message> jobs_out;
    while (true) {
        message msg; mpi::status status = world.recv(mpi::any_source,from_client,msg);
        if (msg.type == message::clientload) {
            recvreq = true; int c,v; 
            BOOST_FOREACH(boost::tie(c,v),cload) if (v > 0) {
                recvreq = false; submit_load(world,cload); break;
            }
        } else {
            if (not (msg.type == message::finish)) {
                cload[status.source()] += 1;
                cerr << format("receiver: received task %s from client %s\n") % msg.task % status.source();
            }
            if ((msg.type == message::input) and (msg.attempt < max_attempts)) {
                ns = ((ns - 2 + 1) % (world.size() - 2)) + 2;
                ++msg.attempt;
                cerr << format("receiver: received failure. resending task %s from client %s to client %s\n") 
                      % msg.task % status.source() % ns;
                world.send(ns,to_client,message(message::flush));
                world.send(ns,to_client,msg);
                if (finished) world.send(ns,to_client,message(message::flush));
                cload[ns] -= 1;
            } else {
                // send the output, and discard it. then store to have a record we finished it.
                send(msg);
                vector<string>().swap(msg.out);
                jobs_out[msg.task] = msg;
                
                if (msg.type == message::finish) {
                    cerr << "receiver: received finish request from sender\n";
                    for (int client = 2; client != world.size(); ++client) {
                        world.send(client,to_client,message(message::flush));
                    }
                    finished = true;
                }
                map<int,message>::iterator pos = jobs_out.find(minrecv);
                if (pos == jobs_out.end()) {
                    cerr << boost::format("receiver: waiting on task %s\n") % minrecv;
                }
                while (pos != jobs_out.end()) {
                    if (pos->second.type == message::finish) {
                        cerr << "receiver: sending finish requests to clients\n";
                        for (int x = 2; x != world.size(); ++x) {
                            world.send(x,to_client,message(message::finish));
                        }
                        cerr << "receiver: finished\n";
                        return;
                    } else {
                        //send(pos->second);
                    }
                    jobs_out.erase(pos);
                    minrecv += 1;
                    pos = jobs_out.find(minrecv);
                }
            }
            if (recvreq) {
                recvreq = false;
                submit_load(world,cload);
            }
        }
    }
}

void printout(message const& msg)
{ 
    BOOST_FOREACH(string const& line, msg.out) {
        cout << line << endl;
    }
}

int main(int argc, char** argv)
{
    mpi::environment env(argc, argv);
    mpi::communicator world;
    if (world.rank() == receiver) {
        receive_from_client(world,printout);
    } else if (world.rank() == submitter) {
        send_to_client(world);
    } else {
        run_client(world,argv[1]);
    }
    return 0;
}
