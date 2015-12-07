check_PROGRAMS = RuleReader_tests
RuleReader_tests_SOURCES = test_RuleReader.cpp     

RuleReader_tests_LDADD = -l@boost_unit_test_framework_lib@ @top_srcdir@/src/libRuleReader.la

INCLUDES = -I@top_srcdir@/include

TESTS = RuleReader_tests

printbase = test

my-print-local: $(RuleReader_tests_SOURCES)
	enscript -o $(printbase).ps -2r --pretty-print=cpp --color --mark-wrapped-lines=arrow $^ || echo "some warnings"
my-print: my-print-local
