---------------------------------------------------------

DIRECTORIES

scripts:        main scripts to run rule extraction
configs:        rule extraction configuration files
                (used by scripts/the_button.pl)
bin:            rule extraction and scoring binary files
                (refer to the README file in scripts for 
                high-level rule extraction programs)
{c++,perl}-src: code of the rule extractor 
samples:        sample etree/cstring/alignment files
contrib:        misc. scripts that are not used in 
                the rule extraction pipeline.

---------------------------------------------------------

INSTALLATION

1) First, you need to setup some environment variables 
   to be able to compile and run the rule extraction tool, 
   e.g. in bash:

   export LD_LIBRARY_PATH=/auto/hpc-22/dmarcu/nlg/contrib/xrs-extract/pcre/lib:/auto/hpc-22/dmarcu/nlg/contrib/local/db-4.2/lib:$LD_LIBRARY_PATH

2) Make sure you create your own copy of the CVS repository, 
   instead of just copying my local files.

   $ cvs -d :ext:$USER@nlg0.isi.edu:/home/graehl/isd/hpc-cvs/cvs-repository checkout xrs-extract

3) Enter the source directory and run make:

   $ cd src
   $ make 

4) Check that everything is working fine:

   $ make check

---------------------------------------------------------

RULE EXTRACTION

To extract rules, collect counts, and extract derivations, simply
adapt scripts/the_button.pl to your needs and run it. When run 
without any argument, it is configured to extract rules only from 
a small sample of the data.

Rule extraction is managed through configuration files passed as
argument to scripts/the_button.pl. Sample configuration files
can be found in the 'configs' directory. It is advised to make copies
of these files (e.g. create big3.cfg out of big3.cfg.template), and
adapt these copies to your local enviroment. Make sure you never 
add these copies to the CVS repository, since this might interfere
with the configuration files of others.

Variables of special interest:

* set $END_DERIV to 2562204 to extract rules from the full data
  (lines 2562205 to 2626879 are corrupted and shouldn't be used);

* modify $N_PAR to set the number of cluster nodes you want to use;

* use $OUTDIR to specify an output directory;

* use $ARG_LIM to impose constraints on the size of composed rules
  (note: size constraints never affect minimal rules).
  $ARG_LIM must contain two numbers separated by ':'.
  
  1) the first sets the maximum number of rule LHS extracted at 
     each node (because of unaligned Chinese words, the # of rule LHS <= 
     the # of rules). If you set it to 0 or 1, only minimal rules are 
     extracted.
  
  2) the second sets the maximum number of internal nodes in the
     LHS (pre-terminals are not considered internal here). 
     DT("the")             : 0 internal node
     NPB(x0:DT x1:NN)      : 1 internal node
     NPB(DT("the") x1:NN)  : 1 internal node
     NP-C(NPB(x0:DT x1:NN) : 2 internal nodes
     
  The following values were used to generate the so-called 'big3'
  and 'big4' rule sets in summer04/resources/rule-extraction/extracts:
     big3: 1000:3
     big4: 1000:4

Once you are done setting up a configuration file, simply run:
$ ./the_button.pl CONFIGFILE
e.g. to recreate the big3 rule set: 
$ ./the_button.pl ../configs/big3.cfg
(Note: more information about that script is available in PIPELINE.txt)

---------------------------------------------------------

QUESTIONS, FEEDBACK, COMPLAINTS

Email me at galley@cs.columbia.edu.

--michel
