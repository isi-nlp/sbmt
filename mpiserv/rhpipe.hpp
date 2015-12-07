# ifndef RHPIPE__RHPIPE_HPP
# define RHPIPE__RHPIPE_HPP

# include <boost/process.hpp>
# include <gusc/generator.hpp>
# include <vector>

struct rhpipe 
 : gusc::peekable_generator_facade<rhpipe,std::string> {
    rhpipe(std::string exec);
    void send(std::string const& msg); // may block, throw
    int close();
    void start();
private:
    friend class gusc::generator_access;
    bool more() const;
    std::string const& peek() const;
    void pop(); // may block
    boost::process::child subproc;
    std::string curr;
    bool valid;
    bool init;
    static boost::process::child make(std::string e);
};

struct array_pipe
 : gusc::peekable_generator_facade< array_pipe, std::vector<std::string> > {
    array_pipe(std::string exec);
    void send(std::string const& msg) { pipe.send(msg); }
    int close() { return pipe.close(); }
    void start();
private:
    friend class gusc::generator_access;
    rhpipe pipe;
    std::vector<std::string> curr;
    bool valid;
    bool more() const;
    std::vector<std::string> const& peek() const;
    void pop(); // may block
};


typedef boost::tuple<
          int
        , boost::optional<
            std::vector<std::string>
          >
        > recovery_pipe_message;



# endif
