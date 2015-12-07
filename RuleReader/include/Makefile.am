EXTRA_HEADERS = 

hfiles=$(wildcard @top_srcdir@/include/RuleReader/*.h)
       
#match_slash = "\\/"
#insert_backslash = "\\\\\\/"
#strip_ibase = `echo @top_srcdir@/include/ | sed s/$(match_slash)/$(insert_backslash)/g`;
#lclhfiles = `echo $(hfiles) | sed s/$(strip_ibase)//`;

lclhfiles = $(subst @top_srcdir@/include/,,$(hfiles))

nobase_include_HEADERS = $(lclhfiles)

printbase = include

my-print-local: $(hfiles)
	enscript -o $(printbase).ps -2r --pretty-print=cpp --color --mark-wrapped-lines=arrow $^ || echo "some warnings"
my-print: my-print-local

#nobase_include_HEADERS=$(wildcard RuleReader/*.h) 
