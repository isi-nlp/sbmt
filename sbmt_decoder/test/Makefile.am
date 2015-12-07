check_PROGRAMS = sbmt_tests lwlm_leak_test
sourcefiles        = test_main.cpp                \
                     test_ngram_lm.cpp \
                     test_tag_prior.cpp          \
                     test_hash_table.cpp          \
                     test_priority_table.cpp      \
                     test_edge_filter.cpp         \
                     test_token.cpp               \
                     test_logmath.cpp             \
                     test_logging.cpp             \
                     test_sentence.cpp            \
                     test_tokenizer.cpp           \
                     test_rule_input.cpp          \
                     test_syntax_rule.cpp         \
                     test_grammar_file_reader.cpp \
                     test_grammar_in_memory.cpp   \
                     test_edge_factory.cpp        \
                     test_score_combine.cpp       \
                     test_span.cpp                \
                     test_item.cpp                \
                     grammar_examples.cpp         \
                     test_chart_span.cpp          \
                     test_lm_string.cpp           \
                     test_cube.cpp                \
                     test_cell_filter.cpp         \
                     test_span_filter.cpp         \
                     test_filter_bank.cpp         \
                     test_thread_pool.cpp \
		     test_alignment.cpp \
		     test_in_memory_token_storage.cpp

sbmt_tests_CPPFLAGS = -I@top_srcdir@/include \
                      -DSBMT_TEST_DIR="\"@top_srcdir@/test/\"" \
                      $(AM_CPPFLAGS)

sbmt_tests_SOURCES = $(sourcefiles)

lwlm_leak_test_SOURCES = lwlm_memleak.cpp

lwlm_leak_test_LDADD = @top_builddir@/src/libsbmt.la

sbmt_tests_LDADD = -l@boost_unit_test_framework_lib@ \
                     @top_builddir@/src/libsbmt.la 


TESTS = sbmt_tests

printbase = test
my-print-local: $(sourcefiles)
	enscript -o $(printbase).ps -2r --pretty-print=cpp --color --mark-wrapped-lines=arrow $^ || echo "some warnings"

my-print: my-print-local


