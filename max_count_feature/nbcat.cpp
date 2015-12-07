# include <boost/thread/thread.hpp>
# include <boost/thread/condition.hpp>
# include <boost/thread/mutex.hpp>
# include <boost/lexical_cast.hpp>
# include <fstream>
# include <iostream>
# include <string>
# include <deque>

std::deque<std::string> dq;
boost::mutex mtx;
boost::condition not_empty_or_finished;
boost::condition not_overfull;
unsigned int overfull = 0;
bool finished = false;

void putinp(std::string const& line)
{
    boost::mutex::scoped_lock lk(mtx);
    if (overfull > 0) while (dq.size() == overfull) not_overfull.wait(lk);
    bool needs_notify = dq.empty();
    dq.push_back(line);
    if (needs_notify) not_empty_or_finished.notify_all();
}

void finishinp()
{
    boost::mutex::scoped_lock lk(mtx);
    finished = true;
    not_empty_or_finished.notify_all();
}

bool getinp(std::string& line)
{
    boost::mutex::scoped_lock lk(mtx);
    while ((not finished) and dq.empty()) not_empty_or_finished.wait(lk);
    if (not dq.empty()) {
        bool needs_notify = (dq.size() == overfull);
        line = dq.front();
        dq.pop_front();
        if (needs_notify) not_overfull.notify_all();
        return true;
    } else {
        return false;
    }
}

void read_stdin()
{
    std::string line;
    while(std::getline(std::cin,line)) {
        putinp(line);
    }
    finishinp();
}

void write_stdout()
{
    std::string line;
    while (getinp(line)) {
        std::cout << line << '\n';
	std::cout << std::flush;
    }
}

int main(int argc, char** argv)
{
    if (argc == 2) try {
        overfull = boost::lexical_cast<unsigned int>(argv[1]);
    } catch(...) {
        throw std::runtime_error("usage: nbcat [queue-size]");
    }
    if (argc > 2) throw std::runtime_error("usage: nbcat [queue-size]");
    std::cin.tie(0);
    boost::thread tread(read_stdin);
    boost::thread twrite(write_stdout);
    tread.join();
    twrite.join();
    return 0;
}
