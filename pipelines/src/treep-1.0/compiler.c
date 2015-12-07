
/*  A Bison parser, made from compiler.y
    by GNU Bison version 1.28  */

#define YYBISON 1  /* Identify Bison output.  */

#define	SYMBOL	257
#define	PARENT	258
#define	ERROR	259
#define	LOW	260
#define	GETS	261
#define	RARROW	262
#define	CONC	263
#define	STARLEFTMAX	264
#define	STARRIGHTMAX	265
#define	STARLEFTMIN	266
#define	STARRIGHTMIN	267
#define	STARMIN	268
#define	STARMAX	269
#define	OPTMIN	270

#line 2 "compiler.y"


#include "config.h"

#include <stdio.h>
#ifdef HAVE_STRING_H
# include <string.h>
#endif

#include "marker.h"

#include "node.h"
#include "pattern.h"
#include "grammar.h"

#define YYSTYPE node

  extern int pedantic;

#ifndef YYSTYPE
#define YYSTYPE int
#endif
#ifndef YYDEBUG
#define YYDEBUG 1
#endif

#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		54
#define	YYFLAG		-32768
#define	YYNTBASE	30

#define YYTRANSLATE(x) ((unsigned)(x) <= 270 ? yytranslate[x] : 36)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    25,     2,     2,     2,     2,    24,     2,    28,
    26,     2,    21,    23,    22,    29,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,    27,    10,
     2,    11,    19,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     9,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     3,     4,     5,     6,
     7,     8,    12,    13,    14,    15,    16,    17,    18,    20
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     4,     8,    12,    13,    17,    21,    23,    25,    29,
    33,    37,    40,    42,    44,    48,    52,    56,    60,    64,
    67,    71,    74,    77,    80,    83,    86,    89,    92
};

static const short yyrhs[] = {    32,
    27,    30,     0,    31,    27,    30,     0,     1,    27,    30,
     0,     0,     3,     7,    35,     0,    34,     8,    35,     0,
     3,     0,    33,     0,    28,    34,    26,     0,    34,    23,
    34,     0,    34,    24,    34,     0,    25,    34,     0,    29,
     0,    34,     0,    28,    35,    26,     0,    35,    21,    33,
     0,    35,    22,    33,     0,    35,    10,    35,     0,    35,
    11,    35,     0,    35,    35,     0,    35,     9,    35,     0,
    35,    19,     0,    35,    20,     0,    35,    13,     0,    35,
    14,     0,    35,    15,     0,    35,    16,     0,    35,    18,
     0,    35,    17,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
    45,    46,    47,    48,    51,    58,    65,    78,    80,    85,
    90,    94,    98,   104,   106,   111,   122,   132,   141,   150,
   158,   163,   168,   173,   178,   183,   188,   193,   201
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","SYMBOL",
"PARENT","ERROR","LOW","GETS","RARROW","'|'","'<'","'>'","CONC","STARLEFTMAX",
"STARRIGHTMAX","STARLEFTMIN","STARRIGHTMIN","STARMIN","STARMAX","'?'","OPTMIN",
"'+'","'-'","','","'&'","'!'","')'","';'","'('","'.'","clauses","macro","production",
"symbol_or_macro","node_pat","str_pat", NULL
};
#endif

static const short yyr1[] = {     0,
    30,    30,    30,    30,    31,    32,    33,    34,    34,    34,
    34,    34,    34,    35,    35,    35,    35,    35,    35,    35,
    35,    35,    35,    35,    35,    35,    35,    35,    35
};

static const short yyr2[] = {     0,
     3,     3,     3,     0,     3,     3,     1,     1,     3,     3,
     3,     2,     1,     1,     3,     3,     3,     3,     3,     2,
     3,     2,     2,     2,     2,     2,     2,     2,     2
};

static const short yydefact[] = {     0,
     0,     7,     0,     0,    13,     0,     0,     8,     0,     0,
     0,     7,    12,     0,     0,     0,     0,     0,     0,     3,
     0,    14,     5,     9,     2,     1,     6,    10,    11,    14,
     0,     0,     0,     0,    24,    25,    26,    27,    29,    28,
    22,    23,     0,     0,    20,    15,    21,    18,    19,    16,
    17,     0,     0,     0
};

static const short yydefgoto[] = {    20,
     6,     7,     8,    22,    45
};

static const short yypact[] = {     0,
   -23,    12,    89,    89,-32768,   -18,    -5,-32768,     3,     0,
    91,-32768,-32768,   -16,     0,     0,    91,    89,    89,-32768,
    91,   -10,    42,-32768,-32768,-32768,    42,     9,-32768,   -16,
    21,    91,    91,    91,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,    41,    41,    82,-32768,    62,    82,    82,-32768,
-32768,    48,    54,-32768
};

static const short yypgoto[] = {    93,
-32768,-32768,   -28,     2,   104
};


#define	YYLAST		138


static const short yytable[] = {    -4,
     1,     9,     2,    10,    13,    14,    18,    19,    15,    24,
    17,     9,    18,    19,    50,    51,     9,     9,    11,    28,
    29,    16,    30,    12,     3,    18,    19,     4,     5,    32,
    33,    34,    19,    35,    36,    37,    38,    39,    40,    41,
    42,    43,    44,    12,    12,     3,    46,    53,    21,     5,
    32,    33,    34,    54,    35,    36,    37,    38,    39,    40,
    41,    42,    43,    44,    12,     0,     3,     0,     0,    21,
     5,    33,    34,     0,    35,    36,    37,    38,    39,    40,
    41,    42,    43,    44,    12,     0,     3,     0,     0,    21,
     5,    12,    52,    12,    35,    36,    37,    38,    39,    40,
    41,    42,    43,    44,     0,     0,     3,    25,    26,    21,
     5,     0,     0,     3,    23,     3,     4,     5,    21,     5,
    27,     0,     0,     0,    31,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,    47,    48,    49
};

static const short yycheck[] = {     0,
     1,     0,     3,    27,     3,     4,    23,    24,    27,    26,
     8,    10,    23,    24,    43,    44,    15,    16,     7,    18,
    19,    27,    21,     3,    25,    23,    24,    28,    29,     9,
    10,    11,    24,    13,    14,    15,    16,    17,    18,    19,
    20,    21,    22,     3,     3,    25,    26,     0,    28,    29,
     9,    10,    11,     0,    13,    14,    15,    16,    17,    18,
    19,    20,    21,    22,     3,    -1,    25,    -1,    -1,    28,
    29,    10,    11,    -1,    13,    14,    15,    16,    17,    18,
    19,    20,    21,    22,     3,    -1,    25,    -1,    -1,    28,
    29,     3,     0,     3,    13,    14,    15,    16,    17,    18,
    19,    20,    21,    22,    -1,    -1,    25,    15,    16,    28,
    29,    -1,    -1,    25,    11,    25,    28,    29,    28,    29,
    17,    -1,    -1,    -1,    21,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    32,    33,    34
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/lib/bison.simple"
/* This file comes from bison-1.28.  */

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

#ifndef YYSTACK_USE_ALLOCA
#ifdef alloca
#define YYSTACK_USE_ALLOCA
#else /* alloca not defined */
#ifdef __GNUC__
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi) || (defined (__sun) && defined (__i386))
#define YYSTACK_USE_ALLOCA
#include <alloca.h>
#else /* not sparc */
/* We think this test detects Watcom and Microsoft C.  */
/* This used to test MSDOS, but that is a bad idea
   since that symbol is in the user namespace.  */
#if (defined (_MSDOS) || defined (_MSDOS_)) && !defined (__TURBOC__)
#if 0 /* No need for malloc.h, which pollutes the namespace;
	 instead, just don't use alloca.  */
#include <malloc.h>
#endif
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
/* I don't know what this was needed for, but it pollutes the namespace.
   So I turned it off.   rms, 2 May 1997.  */
/* #include <malloc.h>  */
 #pragma alloca
#define YYSTACK_USE_ALLOCA
#else /* not MSDOS, or __TURBOC__, or _AIX */
#if 0
#ifdef __hpux /* haible@ilog.fr says this works for HPUX 9.05 and up,
		 and on HPUX 10.  Eventually we can turn this on.  */
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#endif /* __hpux */
#endif
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc */
#endif /* not GNU C */
#endif /* alloca not defined */
#endif /* YYSTACK_USE_ALLOCA not defined */

#ifdef YYSTACK_USE_ALLOCA
#define YYSTACK_ALLOC alloca
#else
#define YYSTACK_ALLOC malloc
#endif

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Define __yy_memcpy.  Note that the size argument
   should be passed with type unsigned int, because that is what the non-GCC
   definitions require.  With GCC, __builtin_memcpy takes an arg
   of type size_t, but it can handle unsigned int.  */

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (to, from, count)
     char *to;
     char *from;
     unsigned int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *to, char *from, unsigned int count)
{
  register char *t = to;
  register char *f = from;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 217 "/usr/lib/bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#ifdef __cplusplus
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else /* not __cplusplus */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif /* not __cplusplus */
#else /* not YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* not YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
#ifdef YYPARSE_PARAM
int yyparse (void *);
#else
int yyparse (void);
#endif
#endif

int
yyparse(YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;
  int yyfree_stacks = 0;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  if (yyfree_stacks)
	    {
	      free (yyss);
	      free (yyvs);
#ifdef YYLSP_NEEDED
	      free (yyls);
#endif
	    }
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
#ifndef YYSTACK_USE_ALLOCA
      yyfree_stacks = 1;
#endif
      yyss = (short *) YYSTACK_ALLOC (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss, (char *)yyss1,
		   size * (unsigned int) sizeof (*yyssp));
      yyvs = (YYSTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs, (char *)yyvs1,
		   size * (unsigned int) sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls, (char *)yyls1,
		   size * (unsigned int) sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 5:
#line 52 "compiler.y"
{
		macro_table_add(macros, ((pattern_op)yyvsp[-2]->contents)->s, yyvsp[0]);
		pattern_op_delete(yyvsp[-2]->contents);
		node_delete(yyvsp[-2]);
	;
    break;}
case 6:
#line 59 "compiler.y"
{
	  grammar_add(productions, yyvsp[-2], yyvsp[0]);
	;
    break;}
case 7:
#line 66 "compiler.y"
{
		node expansion = macro_table_lookup(macros, ((pattern_op)yyvsp[0]->contents)->s);
		if (expansion) {
			pattern_op_delete(yyvsp[0]->contents);
			node_delete(yyvsp[0]);
			yyval = tree_copy(expansion, (ENDO)pattern_op_copy);
		} else {
			yyval = yyvsp[0];
		}
	;
    break;}
case 9:
#line 81 "compiler.y"
{
		yyval = yyvsp[-1];
	;
    break;}
case 10:
#line 86 "compiler.y"
{
		yyval = new_pattern(new_pattern_op(OP_UNION, NULL), yyvsp[-2], yyvsp[0]);
	;
    break;}
case 11:
#line 91 "compiler.y"
{
		yyval = new_pattern(new_pattern_op(OP_INTERSECTION, NULL), yyvsp[-2], yyvsp[0]);
	;
    break;}
case 12:
#line 95 "compiler.y"
{
		yyval = new_pattern(new_pattern_op(OP_COMPLEMENT, NULL), yyvsp[0], NULL);
	;
    break;}
case 13:
#line 99 "compiler.y"
{
		yyval = new_pattern(new_pattern_op(OP_DOT, NULL), NULL, NULL);
	;
    break;}
case 15:
#line 107 "compiler.y"
{
		yyval = yyvsp[-1];
	;
    break;}
case 16:
#line 112 "compiler.y"
{
	  if (((pattern_op)yyvsp[0]->contents)->type != OP_ATOM) {
	     yyerror("error: macro after + doesn't expand to atom");
	     YYERROR;
	  }

	  yyval = new_pattern(new_pattern_op(OP_SET, ((pattern_op)yyvsp[0]->contents)->s), yyvsp[-2], NULL);
	  pattern_delete(yyvsp[0]);
	;
    break;}
case 17:
#line 123 "compiler.y"
{
	  if (((pattern_op)yyvsp[0]->contents)->type != OP_ATOM) {
	     yyerror("error: macro after - doesn't expand to atom");
	     YYERROR;
	  }
	  yyval = new_pattern(new_pattern_op(OP_RESET, ((pattern_op)yyvsp[0]->contents)->s), yyvsp[-2], NULL);
	  pattern_delete(yyvsp[0]);
	;
    break;}
case 18:
#line 133 "compiler.y"
{
		if (yychar == '>') {
			yyerror("parse error: < and > do not associate");
			YYERROR;
		}
		yyval = new_pattern(new_pattern_op(OP_CONC_RIGHT, NULL), yyvsp[-2], yyvsp[0]);
	;
    break;}
case 19:
#line 142 "compiler.y"
{
		if (yychar == '<') {
			yyerror("parse error: > and < do not associate");
			YYERROR;
		}
		yyval = new_pattern(new_pattern_op(OP_CONC_LEFT, NULL), yyvsp[-2], yyvsp[0]);
	;
    break;}
case 20:
#line 151 "compiler.y"
{
		if (!pattern_simple(yyvsp[-1]) && !pattern_simple(yyvsp[0])) {
			yyerror("warning: two complex patterns cannot be concatenated without an operator, using > instead");
		}
		yyval = new_pattern(new_pattern_op(OP_CONC_LEFT, NULL), yyvsp[-1], yyvsp[0]);
	;
    break;}
case 21:
#line 159 "compiler.y"
{
		yyval = new_pattern(new_pattern_op(OP_ALT, NULL), yyvsp[-2], yyvsp[0]);
	;
    break;}
case 22:
#line 164 "compiler.y"
{
		yyval = new_pattern(new_pattern_op(OP_ALT, NULL), yyvsp[-1], new_pattern(new_pattern_op(OP_EMPTY, NULL), NULL, NULL));
	;
    break;}
case 23:
#line 169 "compiler.y"
{
		yyval = new_pattern(new_pattern_op(OP_ALT, NULL), new_pattern(new_pattern_op(OP_EMPTY, NULL), NULL, NULL), yyvsp[-1]);
	;
    break;}
case 24:
#line 174 "compiler.y"
{
		yyval = new_pattern(new_pattern_op(OP_STAR_LEFT_MAX, NULL), yyvsp[-1], NULL);
	;
    break;}
case 25:
#line 179 "compiler.y"
{
		yyval = new_pattern(new_pattern_op(OP_STAR_RIGHT_MAX, NULL), yyvsp[-1], NULL);
	;
    break;}
case 26:
#line 184 "compiler.y"
{
		yyval = new_pattern(new_pattern_op(OP_STAR_LEFT_MIN, NULL), yyvsp[-1], NULL);
	;
    break;}
case 27:
#line 189 "compiler.y"
{
		yyval = new_pattern(new_pattern_op(OP_STAR_RIGHT_MIN, NULL), yyvsp[-1], NULL);
	;
    break;}
case 28:
#line 194 "compiler.y"
{
		if (pedantic && !pattern_simple(yyvsp[-1])) {
			yyerror("warning: a complex pattern may not be used with *, using <* instead");
		}
		yyval = new_pattern(new_pattern_op(OP_STAR_LEFT_MAX, NULL), yyvsp[-1], NULL);
	;
    break;}
case 29:
#line 202 "compiler.y"
{
		if (pedantic && !pattern_simple(yyvsp[-1])) {
			yyerror("warning: a complex pattern may not be used with *?, using <*? instead");
		}
		yyval = new_pattern(new_pattern_op(OP_STAR_LEFT_MIN, NULL), yyvsp[-1], NULL);
	;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 543 "/usr/lib/bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;

 yyacceptlab:
  /* YYACCEPT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 0;

 yyabortlab:
  /* YYABORT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 1;
}
#line 211 "compiler.y"


int line_number = 1, lexerr = 0;
const char *lexerrmsg;
extern char *filename;

int yyerror(char *s) {
  fprintf(stderr, "%s:%d: %s\n", filename, line_number, lexerr?lexerrmsg:s);
  return 0;
}

