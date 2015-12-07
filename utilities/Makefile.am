# note how most file inputs are one-to-a-line.  I find this goes a long way 
# toward avoiding svn conflicts :-)
#
# also, if you are noticing all of the -lpthread libadds, this is because I
# couldn't get successful static builds without them.  --Michael

bin_PROGRAMS = grammar_view \
               mini_decoder \
               unknown_word_rules \
               archive_grammar \
               new_decoder_weight_format \
               archive_integrity_check

bin_SCRIPTS =  srilm-restrict-vocab split-byline.pl \
               etree-prior.pl \
               lw-lm-score.pl \
               sbtm-score.pl \
               paste-byline.pl \
               libgraehl.pl \
               lib-corpus-map.pl \
               ApplicableRuleFilter.pm \
               gen-archive-pkg.pl \
               make-instruction-file.pl \
	       xrs-subset.pl \
	       make-sub-lm 
#	       lm-strip-ss.pl

archive_integrity_check_SOURCES = archive_integrity_check.cpp
archive_integrity_check_LDADD = -lsbmt \
                                -l@boost_program_options_lib@ \
                                -lz

grammar_view_SOURCES = grammar_view.cpp
grammar_view_LDADD   = -lsbmt -l@boost_program_options_lib@ -lz

unknown_word_rules_SOURCES = unknown_word_rules.cpp
unknown_word_rules_LDADD   = -lsbmt \
                             -l@boost_program_options_lib@ \
                             -l@boost_filesystem_lib@ \
                             -lz 

mini_decoder_SOURCES = mini_decoder.cpp \
                       decode_sequence_reader.cpp \
                       numproc.cpp

mini_decoder_LDADD   = -lsbmt \
                       -l@boost_program_options_lib@ \
                       -l@boost_filesystem_lib@ \
                       -lz \
                       -lpthread

archive_grammar_SOURCES = archive_grammar.cpp

archive_grammar_LDADD   = -lsbmt \
                          -lz \
                          -lpthread \
                          -l@boost_program_options_lib@


new_decoder_weight_format_SOURCES = new_decoder_weight_format.cpp

new_decoder_weight_format_LDADD = -lsbmt -l@boost_regex_lib@ \
                                  -lz \
                                  -lpthread \
                                  -l@boost_program_options_lib@

PRINTFILES = utilities.ps
PRINTINFILES = $(wildcard *.cpp) \
               $(wildcard *.hpp) \
               $(wildcard *.ipp) \
               $(wildcard *.pl)  \
               $(wildcard *.pm)

print print-ps:
	enscript -o $(PRINTFILES) \
	         -2r --pretty-print=cpp \
	         --color \
	         --mark-wrapped-lines=arrow \
	         $(PRINTINFILES) \
	|| echo "some warnings "
    

print-pdf: print-ps
	@echo "###converting to pdf (this may take a while. . .)";
	gs -q \
	   -sDEVICE=pdfwrite \
	   -sOutputFile=utilities.pdf \
	   -dNOPAUSE \
	   -dSAFER \
	   -dBATCH \
	   $(PRINTFILES)
