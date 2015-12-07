lib_LTLIBRARIES = libRuleReader.la

libRuleReader_la_SOURCES = RuleNode.cc               \
                           Rule.cc                   

INCLUDES = -I@top_srcdir@/include    

printbase = src

my-print-local: $(libRuleReader_la_SOURCES)
	enscript -o $(printbase).ps -2r --pretty-print=cpp --color --mark-wrapped-lines=arrow $^ || echo "some warnings"
my-print: my-print-local
