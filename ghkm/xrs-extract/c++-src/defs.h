#ifndef __DEFS_H__
#define __DEFS_H__

/* Uncomment the following for debugging mode: */
//#define DEBUG

/* Set this to true to collect counts: */
#define COLLECT_COUNTS

/* Set this to true to collect fractional counts: */
#define COLLECT_FRAC_COUNTS

/* Print header lines in output: */
#define PRINT_HEADER true

/* Maximum number of rules extracted per node:
 * to extract just necessary rules, set this to 1. */
#define MAX_TO_EXTRACT 50000

/* Maximum number of rule expansions: */
#define MAX_RULE_EXPANSIONS 4

/* Maximum phrase size on source size (when -H option is used) */
#define MAX_PHRASE_SOURCE_SIZE 4 // Daniel

/* Can we assign unaligned chinese words to 
 * preterminals? If yes, set this to true. */
#define CAN_ASSIGN_UNALIGNED_C_TO_PRETERM false

/* If set to true, fail a derivation if there is any necessary rule
 * bigger than the MAX_RULE_EXPANSIONS value. Set this to true
 * to reproduce the experiments of the paper (GHKM). */
//#define FAIL_IF_TOO_BIG true
#define FAIL_IF_TOO_BIG false

/* Maximum number of nodes on the priority queue: */
#define MAX_QUEUE_SIZE 50000

/* Expected number of rules (just an estimate for
 * improved memory allocation with reserve(): */
#define NB_RULES (250*1024*1024)

/* Misc. debugging info: */
#define PRINT_STATS_WITH_RULES 0

/* Identifier for the AT probability in the ATS file: */
#define AT_PROB_IDENTIFIER "=model1inv="

/* Maximum number of unaligned words per rule: */
#define MAX_NB_UNALIGNED 10

/* Define what string class to use for POS, tokens, etc: */
#define STRING std::string
//#define STRING num::istring

/* Define DB access method: */
//#define DB_ACCESS DB_HASH
#define DB_ACCESS DB_BTREE

/* Set this to true if you want to use default DB params: */
#define DB_DEFAULT_PARAMS true

/* Buffer for reads in bulk: */
#define DB_BUFFER_LENGTH (512*1024*1024)

/* Define expected number of rules and LHS; estimate of their
 * average size; average size of rule identifiers 
 * (these are just estimates for the Berkeley DB; 
 * it doesn't matter if they are way off, 
 * though performance might suffer) */
// The following stats were computed on necessary rules 
// (56M words)
#define DB_EXPECTED_NB_OF_RULES (140*1000*1000)
#define DB_AVERAGE_RULE_SIZE    115.48
#define DB_AVERAGE_LHS_SIZE     81.22
#define DB_AVERAGE_RULEID_SIZE  8.22

/* Set cache size for the DB (2 GB). */
#define DB_CACHE_SIZE  (2024*1024*1024)

/* Page size in the DB. */
#define DB_PAGE_SIZE (16*1024)

/* Minimum nb of keys per page: */
#define DB_NB_MIN_KEYS 2

/* size identifier: */
#define SIZEID "size"

/* To report warnings/errors: */
#ifndef CLOG
#define PRINTLOGS 2
#define CLOG(L) \
  if(PRINTLOGS >= 2) \
  std::clog<<__BASE_FILE__<<"("<<__LINE__<<"): "<<L<<std::endl
#endif

#endif
