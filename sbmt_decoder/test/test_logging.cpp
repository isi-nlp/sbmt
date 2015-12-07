# include <boost/test/auto_unit_test.hpp>
# include <boost/bind.hpp>
# include <boost/thread/xtime.hpp>
# include <boost/regex.hpp>
# include <boost/lexical_cast.hpp>
# include <sstream>
# include <sbmt/logging.hpp>
# include <graehl/shared/program_options.hpp>
# include <map>
# include <functional>
# include <sbmt/hash/thread_pool.hpp>

using namespace sbmt::io;
using namespace std;

////////////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE(test_logging)
{
    stringbuf strbuf;
    logging_stream logstr(strbuf,"mylog",lvl_warning);

    string foo("foo");
    string bar("bar");
    string bif("bif");
    string baz("baz");
    string bul("bul");

    logstr << warning_msg << boost::format("%s %s") % foo % bar;
    continue_log(logstr) << endmsg;
    BOOST_CHECK_EQUAL(strbuf.str(),"[mylog][warning]: foo bar\n");
    strbuf.str("");

    logstr << error_msg << foo << " " << bar << " " << bif << endmsg;
    BOOST_CHECK_EQUAL(strbuf.str(),"[mylog][error]: foo bar bif\n");
    strbuf.str("");

    logstr << warning_msg << boost::format("%s %s") % foo % baz << endmsg;
    BOOST_CHECK_EQUAL(strbuf.str(),"[mylog][warning]: foo baz\n");
    strbuf.str("");

    logstr << warning_msg << bar << " " << foo << endmsg;
    BOOST_CHECK_EQUAL(strbuf.str(),"[mylog][warning]: bar foo\n");
    strbuf.str("");

    logstr << error_msg << bar << " " << bar << endmsg;
    BOOST_CHECK_EQUAL(strbuf.str(),"[mylog][error]: bar bar\n");
    strbuf.str("");

    SBMT_WARNING_MSG_TO(logstr, "%s %s %s", foo % bar % bul);
    BOOST_CHECK_EQUAL(strbuf.str(),"[mylog][warning]: foo bar bul\n");
    strbuf.str("");

    SBMT_ERROR_STREAM_TO(logstr, bul << " " << bar);
    BOOST_CHECK_EQUAL(strbuf.str(),"[mylog][error]: bul bar\n");
    strbuf.str("");

    SBMT_INFO_MSG_TO(logstr, "%s %s", foo % bar);
    BOOST_CHECK_EQUAL(strbuf.str(),"");

    SBMT_PEDANTIC_MSG_TO(logstr, "%s %s", foo % bar);
    BOOST_CHECK_EQUAL(strbuf.str(),"");

}

BOOST_AUTO_TEST_CASE(test_logging_levels)
{
    using boost::lexical_cast;
    BOOST_CHECK_EQUAL(lvl_none, logging_level(0));
    BOOST_CHECK_EQUAL(lvl_error, logging_level(1));
    BOOST_CHECK_EQUAL(lvl_warning, logging_level(2));
    BOOST_CHECK_EQUAL(lvl_terse, logging_level(3));
    BOOST_CHECK_EQUAL(lvl_info, logging_level(4));
    BOOST_CHECK_EQUAL(lvl_verbose, logging_level(5));
    BOOST_CHECK_EQUAL(lvl_debug, logging_level(6));
    BOOST_CHECK_EQUAL(lvl_pedantic, logging_level(7));


    BOOST_CHECK_EQUAL(lexical_cast<logging_level>("none"),    lvl_none);
    BOOST_CHECK_EQUAL(lexical_cast<logging_level>("error"),   lvl_error);
    BOOST_CHECK_EQUAL(lexical_cast<logging_level>("warning"), lvl_warning);
    BOOST_CHECK_EQUAL(lexical_cast<logging_level>("terse"),   lvl_terse);
    BOOST_CHECK_EQUAL(lexical_cast<logging_level>("info"),    lvl_info);
    BOOST_CHECK_EQUAL(lexical_cast<logging_level>("verbose"), lvl_verbose);

    stringstream sstr;

    sstr << lvl_none;
    BOOST_CHECK_EQUAL(sstr.str(),"none");
    sstr.str("");

    sstr << lvl_error;
    BOOST_CHECK_EQUAL(sstr.str(),"error");
    sstr.str("");

    sstr << lvl_warning;
    BOOST_CHECK_EQUAL(sstr.str(),"warning");
    sstr.str("");

    sstr << lvl_terse;
    BOOST_CHECK_EQUAL(sstr.str(),"terse");
    sstr.str("");

    sstr << lvl_info;
    BOOST_CHECK_EQUAL(sstr.str(),"info");
    sstr.str("");

    sstr << lvl_verbose;
    BOOST_CHECK_EQUAL(sstr.str(),"verbose");
    sstr.str("");
}

////////////////////////////////////////////////////////////////////////////////

SBMT_REGISTER_LOGGING_DOMAIN(parent);
SBMT_REGISTER_CHILD_LOGGING_DOMAIN(child,parent);


BOOST_AUTO_TEST_CASE(test_logfile_registry)
{
    BOOST_CHECKPOINT("begin test_logfile_registry");
    using namespace std;
    using namespace sbmt::io;

    char* argv[] = { "dummyprogname"
                   , "--parent.child.level=warning"
                   , "--parent.file=-2"
                   , "--parent.level=error"
                   };
    int argc=4;

    namespace po = boost::program_options;
    graehl::printable_options_description<ostream>
        od = logfile_registry::instance().options();

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, od),vm);
    po::notify(vm);

    stringbuf strbuf;
    streambuf* errbuf = logfile_registry::instance().rdbuf("-2");
    logfile_registry::instance().rdbuf("-2",&strbuf);

    string foo("foo"), bar("bar");

    SBMT_INFO_MSG(child, "%s %s", foo % bar);

    SBMT_WARNING_MSG(child, "%s %s", foo % bar);

    SBMT_ERROR_MSG(child, "%s %s", foo % bar);

    SBMT_INFO_MSG(parent, "%s %s", foo % bar);

    SBMT_WARNING_MSG(parent, "%s %s", foo % bar);

    SBMT_ERROR_MSG(parent, "%s %s", foo % bar);

    SBMT_INFO_STREAM(child, foo <<" "<< bar);

    SBMT_WARNING_STREAM(child, foo <<" "<< bar);

    SBMT_ERROR_STREAM(child, foo <<" "<< bar);

    SBMT_INFO_STREAM(parent, foo <<" "<< bar);

    SBMT_WARNING_STREAM(parent, foo <<" "<< bar);

    SBMT_ERROR_STREAM(parent, foo <<" "<< bar);

    logfile_registry::instance().rdbuf("-2",errbuf);

    BOOST_CHECK_EQUAL( "[parent.child][warning]: foo bar\n"
                       "[parent.child][error]: foo bar\n"
                       "[parent][error]: foo bar\n"
                       "[parent.child][warning]: foo bar\n"
                       "[parent.child][error]: foo bar\n"
                       "[parent][error]: foo bar\n"
                     , strbuf.str()
                     );
}


////////////////////////////////////////////////////////////////////////////////

SBMT_REGISTER_LOGGING_DOMAIN(parent2);
SBMT_SET_DOMAIN_LOGGING_LEVEL(parent2, error);
SBMT_SET_DOMAIN_LOGFILE(parent2, "-2");

SBMT_REGISTER_CHILD_LOGGING_DOMAIN(child2, parent2);
SBMT_SET_DOMAIN_LOGGING_LEVEL(child2, warning);

SBMT_REGISTER_LOGGING_DOMAIN(parent3);
SBMT_SET_DOMAIN_LOGGING_LEVEL(parent3, pedantic);
SBMT_SET_DOMAIN_LOGFILE(parent3,"-0");


BOOST_AUTO_TEST_CASE(test_logfile_registry2)
{
    BOOST_CHECKPOINT("begin test_logfile_registry2");
    using namespace std;
    using namespace sbmt::io;

    streambuf* errbuf = logfile_registry::instance().rdbuf("-2");
    stringbuf strbuf;
    logfile_registry::instance().rdbuf("-2", &strbuf);

    string foo("foo"), bar("bar");

    SBMT_INFO_MSG(child2, "%s %s msg", foo % bar);

    SBMT_WARNING_MSG(child2, "%s %s msg", foo % bar);

    SBMT_ERROR_MSG(child2, "%s %s msg", foo % bar);

    SBMT_INFO_MSG(parent2, "%s %s msg", foo % bar);

    SBMT_WARNING_MSG(parent2, "%s %s msg", foo % bar);

    SBMT_ERROR_MSG(parent2, "%s %s msg", foo % bar);

    SBMT_INFO_STREAM(child2, foo <<" "<< bar << " str");

    SBMT_WARNING_STREAM(child2, foo <<" "<< bar << " str");

    SBMT_ERROR_STREAM(child2, foo <<" "<< bar << " str");

    SBMT_INFO_STREAM(parent2, foo <<" "<< bar << " str");

    SBMT_WARNING_STREAM(parent2, foo <<" "<< bar << " str");

    SBMT_ERROR_STREAM(parent2, foo <<" "<< bar << " str");

    logfile_registry::instance().rdbuf("-2", errbuf);

    BOOST_CHECK_EQUAL( "[parent2.child2][warning]: foo bar msg\n"
                       "[parent2.child2][error]: foo bar msg\n"
                       "[parent2][error]: foo bar msg\n"
                       "[parent2.child2][warning]: foo bar str\n"
                       "[parent2.child2][error]: foo bar str\n"
                       "[parent2][error]: foo bar str\n"
                     , strbuf.str()
                     );

    // parent3 has logfile set to -0, but no null errors should occur
    SBMT_INFO_STREAM(parent3, foo <<" "<< bar);
    SBMT_WARNING_STREAM(parent3, foo <<" "<< bar);
    SBMT_ERROR_STREAM(parent3, foo <<" "<< bar);

}

////////////////////////////////////////////////////////////////////////////////

SBMT_REGISTER_LOGGING_DOMAIN(pdomain);

SBMT_REGISTER_CHILD_LOGGING_DOMAIN(domain0,pdomain);
SBMT_SET_DOMAIN_LOGGING_LEVEL(domain0, debug);
SBMT_SET_DOMAIN_LOGFILE(domain0,"foobar");

SBMT_REGISTER_CHILD_LOGGING_DOMAIN(domain1,pdomain);
SBMT_SET_DOMAIN_LOGGING_LEVEL(domain1, info);
SBMT_SET_DOMAIN_LOGFILE(domain1,"foobar");

SBMT_REGISTER_CHILD_LOGGING_DOMAIN(domain2,pdomain);
SBMT_SET_DOMAIN_LOGGING_LEVEL(domain2, warning);
SBMT_SET_DOMAIN_LOGFILE(domain2,"foobar");

SBMT_REGISTER_CHILD_LOGGING_DOMAIN(domain3,pdomain);
SBMT_SET_DOMAIN_LOGGING_LEVEL(domain3, error);
SBMT_SET_DOMAIN_LOGFILE(domain3,"foobar");


BOOST_AUTO_TEST_CASE(test_logfile_registry_duplicate_files)
{
    BOOST_CHECKPOINT("begin test_logfile_registry_duplicate_files");
    using namespace std;
    using namespace sbmt::io;

    logfile_registry& registry = logfile_registry::instance();
    stringbuf strbuf;
    streambuf* foobarbuf = registry.rdbuf("foobar");
    registry.rdbuf("foobar", &strbuf);

    string foo("foo"), bar("bar");

    SBMT_INFO_MSG(domain1, "%s %s", foo % bar);

    SBMT_WARNING_MSG(domain1, "%s %s", foo % bar);

    SBMT_ERROR_MSG(domain1, "%s %s", foo % bar);

    SBMT_INFO_MSG(domain2, "%s %s", foo % bar);

    SBMT_WARNING_MSG(domain2, "%s %s", foo % bar);

    SBMT_ERROR_MSG(domain2, "%s %s", foo % bar);

    SBMT_INFO_MSG(domain1, "%s %s", foo % bar);

    SBMT_WARNING_MSG(domain1, "%s %s", foo % bar);

    SBMT_ERROR_MSG(domain1, "%s %s", foo % bar);

    SBMT_INFO_MSG(domain2, "%s %s", foo % bar);

    SBMT_WARNING_MSG(domain2, "%s %s", foo % bar);

    SBMT_ERROR_MSG(domain2, "%s %s", foo % bar);

    registry.rdbuf("foobar", foobarbuf);

    BOOST_CHECK_EQUAL(
        "[pdomain.domain1][info]: foo bar\n"
        "[pdomain.domain1][warning]: foo bar\n"
        "[pdomain.domain1][error]: foo bar\n"
        "[pdomain.domain2][warning]: foo bar\n"
        "[pdomain.domain2][error]: foo bar\n"
        "[pdomain.domain1][info]: foo bar\n"
        "[pdomain.domain1][warning]: foo bar\n"
        "[pdomain.domain1][error]: foo bar\n"
        "[pdomain.domain2][warning]: foo bar\n"
        "[pdomain.domain2][error]: foo bar\n"
      , strbuf.str()
    );
}


////////////////////////////////////////////////////////////////////////////////

void logstuff( logging_stream& log
             , logging_level lvl
             , unsigned int x
             , int id
             , stringstream& sstr )
{
    if (lvl <= log.minimal_level()) {
        sstr << "[" << log.name() << "][" << lvl << "]: ";
        sstr << "id=" << id << " " << x << " is the message" << endl;
    }
    log << logging_level_manip(lvl) << "id=" << id << " " <<  x  << " " ;
    continue_log(log) << "is" << " " << "the" << " " << "message" << endmsg;
}

void logstuff( logging_domain const& d
             , unsigned int n
             , int id
             , stringstream& sstr )
{
    logging_stream& log = logfile_registry::instance().log(d);
    for (unsigned int x = 0; x != n; ++x) {
        logstuff(log,lvl_error,x,id, sstr);
        logstuff(log,lvl_warning,x,id, sstr);
        logstuff(log,lvl_info,x,id, sstr);
        logstuff(log,lvl_debug,x,id, sstr);
        logstuff(log,lvl_pedantic,x,id, sstr);
    }
}

////////////////////////////////////////////////////////////////////////////////

void logbunch( vector<logging_domain> const& v
             , unsigned int n
             , unsigned int m
             , int id
             , stringstream& sstr )
{
    for (unsigned int x = 0; x != m; ++x) {
        vector<logging_domain>::const_iterator itr = v.begin(),
                                               end = v.end();
        for (; itr != end; ++itr) logstuff(*itr, n, id, sstr);
    }
}

////////////////////////////////////////////////////////////////////////////////

SBMT_REGISTER_LOGGING_DOMAIN(thrd_domain);
SBMT_SET_DOMAIN_LOGFILE(thrd_domain,"thrd_foobar");

SBMT_REGISTER_CHILD_LOGGING_DOMAIN(thrd_domain0,thrd_domain);
SBMT_SET_DOMAIN_LOGGING_LEVEL(thrd_domain0, debug);
SBMT_SET_DOMAIN_LOGFILE(thrd_domain0,"thrd_foobar");

SBMT_REGISTER_CHILD_LOGGING_DOMAIN(thrd_domain1,thrd_domain);
SBMT_SET_DOMAIN_LOGGING_LEVEL(thrd_domain1, info);

SBMT_REGISTER_CHILD_LOGGING_DOMAIN(thrd_domain2,thrd_domain);
SBMT_SET_DOMAIN_LOGGING_LEVEL(thrd_domain2, warning);
SBMT_SET_DOMAIN_LOGFILE(thrd_domain2,"thrd_foobar");

SBMT_REGISTER_CHILD_LOGGING_DOMAIN(thrd_domain3,thrd_domain);
SBMT_SET_DOMAIN_LOGGING_LEVEL(thrd_domain3, error);

SBMT_REGISTER_CHILD_LOGGING_DOMAIN(thrd_domain4,thrd_domain);
SBMT_SET_DOMAIN_LOGGING_LEVEL(thrd_domain4, debug);
SBMT_SET_DOMAIN_LOGFILE(thrd_domain4,"thrd_foobar");

SBMT_REGISTER_CHILD_LOGGING_DOMAIN(thrd_domain5,thrd_domain);
SBMT_SET_DOMAIN_LOGGING_LEVEL(thrd_domain5, info);

SBMT_REGISTER_CHILD_LOGGING_DOMAIN(thrd_domain6,thrd_domain);
SBMT_SET_DOMAIN_LOGGING_LEVEL(thrd_domain6, warning);
SBMT_SET_DOMAIN_LOGFILE(thrd_domain6,"thrd_foobar");

SBMT_REGISTER_CHILD_LOGGING_DOMAIN(thrd_domain7,thrd_domain);
SBMT_SET_DOMAIN_LOGGING_LEVEL(thrd_domain7, error);

BOOST_AUTO_TEST_CASE(test_multi_thread_logging)
{
    int const num_threads = 100;
    int const num_jobs = 20;
    int const num_spawns = 10;
    int const loop1 = 10;
    int const loop2 = 10;
    BOOST_CHECKPOINT("begin test_multi_thread_logging");
    using namespace boost;

    /// redirect logging to a common stringbuffer for testing
    logfile_registry& registry = logfile_registry::instance();
    ofstream fstr("thrd_foobar_impl");
    streambuf* foobarbuf = registry.rdbuf("thrd_foobar");
    registry.rdbuf("thrd_foobar", fstr.rdbuf());

    /// we will simulate logging to multiple domains on multiple threads,
    /// all writing to the same buffer.
    const unsigned int D = 8;
    logging_domain dv[D] = { thrd_domain0
                           , thrd_domain1
                           , thrd_domain2
                           , thrd_domain3
                           , thrd_domain4
                           , thrd_domain5
                           , thrd_domain6
                           , thrd_domain7
                           };
    vector<logging_domain> dvec(dv, dv + D);

    /// each stringstream here once the threads have run should have the
    /// contents of the log messages separated out by job ids.  they represent
    /// what the logbuffer would contain if it had been restricted to a single
    /// thread job.
    vector<boost::shared_ptr<stringstream> > expected(num_jobs);
    for (int i = 0; i != num_jobs; ++i) expected[i].reset(new stringstream());

    /// the threads are run multiple times.  each time, the registry will
    /// do cleanup when the threads close; no cleanup errors must occur.
    for (int m = 0; m != num_spawns; ++m) {
        BOOST_CHECKPOINT("starting threads");
        sbmt::thread_pool pool(num_threads);
        for (int x = 0; x != num_jobs; ++x) {
            pool.add(boost::bind( logbunch
                                , dvec
                                , loop1
                                , loop2
                                , x
                                , boost::ref(*(expected[x]))
                                ) );
        }
        BOOST_CHECKPOINT("joining");
        pool.join();
        BOOST_CHECKPOINT("joined");
    }

    /// separate the log into component jobs by the id=x portion.
    boost::regex id_regex("id=(\\d+)");
    vector<boost::shared_ptr<stringstream> > actual(num_jobs);
    for (int i = 0; i != num_jobs; ++i) actual[i].reset(new stringstream());

    fstr.close();
    ifstream sstr("thrd_foobar_impl");
    string line;
    while (getline(sstr,line)) {
        if (line == "") continue;
        smatch m;
        // make sure we can find the id=x in each line,
        // then insert the line into the appropriate component stream
        BOOST_REQUIRE(regex_search(line,m,id_regex));
        int x = lexical_cast<int>(m.str(1));
        *(actual[x]) << line << endl;
    }

    /// assert that once multi-thread interleaving is counteracted, we get the
    /// expected order of results.
    for (int i = 0; i != num_jobs; ++i) {
        BOOST_CHECK_EQUAL(actual[i]->str(), expected[i]->str());
    }

    // necessary, so that the owner of the original buffer cleans it up when
    // when it leaves scope, and does not attempt to flush to a non-existent
    // string buffer.
    registry.rdbuf("thrd_foobar",foobarbuf);
}
