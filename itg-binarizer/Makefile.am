bin_PROGRAMS = itg_binarizer

itg_binarizer_SOURCES = binal.cc

itg_binarizer_LDADD = -lRuleReader \
                      -lz \
                      -l@boost_program_options_lib@ \
                      -l@boost_filesystem_lib@ 

printbase = itg_binarizer
psfile = $(printbase).ps
pdffile = $(printbase).pdf

print print-ps: $(itg_binarizer_SOURCES)
	@echo printfiles $(printfiles)
	enscript -o $(psfile) -2r --pretty-print=cpp --color --mark-wrapped-lines=arrow $^ || echo "some warnings"

print-pdf: print-ps
	@echo "### this may take a while. . ."
	gs -q -sDEVICE=pdfwrite -sOutputFile=$(pdffile) -dNOPAUSE -dSAFER -dBATCH $(psfile)
