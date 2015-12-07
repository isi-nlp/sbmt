/* Copyright (c) 2002 by David Chiang. All rights reserved. */
%{

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

%}

%start clauses

%token SYMBOL PARENT ERROR

%nonassoc LOW
%nonassoc GETS RARROW
%left '|'
%left '<' '>'
%left CONC
%nonassoc STARLEFTMAX STARRIGHTMAX STARLEFTMIN STARRIGHTMIN STARMIN STARMAX '?' OPTMIN

%left '+' '-'

%left ','
%left '&'
%nonassoc '!'

%nonassoc ')'


%%

clauses	: production ';' clauses
	| macro ';' clauses
        | error ';' clauses
	| /* empty */
	;

macro : SYMBOL GETS str_pat
	{
		macro_table_add(macros, ((pattern_op)$1->contents)->s, $3);
		pattern_op_delete($1->contents);
		node_delete($1);
	}

production : node_pat RARROW str_pat
	{
	  grammar_add(productions, $1, $3);
	}

	;

symbol_or_macro : SYMBOL
	{
		node expansion = macro_table_lookup(macros, ((pattern_op)$1->contents)->s);
		if (expansion) {
			pattern_op_delete($1->contents);
			node_delete($1);
			$$ = tree_copy(expansion, (ENDO)pattern_op_copy);
		} else {
			$$ = $1;
		}
	}


node_pat : symbol_or_macro

	| '(' node_pat ')'
	{
		$$ = $2;
	}

	| node_pat ',' node_pat
	{
		$$ = new_pattern(new_pattern_op(OP_UNION, NULL), $1, $3);
	}

	| node_pat '&' node_pat
	{
		$$ = new_pattern(new_pattern_op(OP_INTERSECTION, NULL), $1, $3);
	}
	| '!' node_pat
	{
		$$ = new_pattern(new_pattern_op(OP_COMPLEMENT, NULL), $2, NULL);
	}
        | '.'
	{
		$$ = new_pattern(new_pattern_op(OP_DOT, NULL), NULL, NULL);
	}
	;

str_pat : node_pat		%prec LOW

	| '(' str_pat ')'
	{
		$$ = $2;
	}

	| str_pat '+' symbol_or_macro
	{
	  if (((pattern_op)$3->contents)->type != OP_ATOM) {
	     yyerror("error: macro after + doesn't expand to atom");
	     YYERROR;
	  }

	  $$ = new_pattern(new_pattern_op(OP_SET, ((pattern_op)$3->contents)->s), $1, NULL);
	  pattern_delete($3);
	}

	| str_pat '-' symbol_or_macro
	{
	  if (((pattern_op)$3->contents)->type != OP_ATOM) {
	     yyerror("error: macro after - doesn't expand to atom");
	     YYERROR;
	  }
	  $$ = new_pattern(new_pattern_op(OP_RESET, ((pattern_op)$3->contents)->s), $1, NULL);
	  pattern_delete($3);
	}

	| str_pat '<' str_pat
	{
		if (yychar == '>') {
			yyerror("parse error: < and > do not associate");
			YYERROR;
		}
		$$ = new_pattern(new_pattern_op(OP_CONC_RIGHT, NULL), $1, $3);
	}

	| str_pat '>' str_pat
	{
		if (yychar == '<') {
			yyerror("parse error: > and < do not associate");
			YYERROR;
		}
		$$ = new_pattern(new_pattern_op(OP_CONC_LEFT, NULL), $1, $3);
	}

	| str_pat str_pat	%prec CONC
	{
		if (!pattern_simple($1) && !pattern_simple($2)) {
			yyerror("warning: two complex patterns cannot be concatenated without an operator, using > instead");
		}
		$$ = new_pattern(new_pattern_op(OP_CONC_LEFT, NULL), $1, $2);
	}

	| str_pat '|' str_pat
	{
		$$ = new_pattern(new_pattern_op(OP_ALT, NULL), $1, $3);
	}

	| str_pat '?'
	{
		$$ = new_pattern(new_pattern_op(OP_ALT, NULL), $1, new_pattern(new_pattern_op(OP_EMPTY, NULL), NULL, NULL));
	}

	| str_pat OPTMIN
	{
		$$ = new_pattern(new_pattern_op(OP_ALT, NULL), new_pattern(new_pattern_op(OP_EMPTY, NULL), NULL, NULL), $1);
	}

	| str_pat STARLEFTMAX
	{
		$$ = new_pattern(new_pattern_op(OP_STAR_LEFT_MAX, NULL), $1, NULL);
	}

	| str_pat STARRIGHTMAX
	{
		$$ = new_pattern(new_pattern_op(OP_STAR_RIGHT_MAX, NULL), $1, NULL);
	}

	| str_pat STARLEFTMIN
	{
		$$ = new_pattern(new_pattern_op(OP_STAR_LEFT_MIN, NULL), $1, NULL);
	}

	| str_pat STARRIGHTMIN
	{
		$$ = new_pattern(new_pattern_op(OP_STAR_RIGHT_MIN, NULL), $1, NULL);
	}

	| str_pat STARMAX
	{
		if (pedantic && !pattern_simple($1)) {
			yyerror("warning: a complex pattern may not be used with *, using <* instead");
		}
		$$ = new_pattern(new_pattern_op(OP_STAR_LEFT_MAX, NULL), $1, NULL);
	}

	| str_pat STARMIN
	{
		if (pedantic && !pattern_simple($1)) {
			yyerror("warning: a complex pattern may not be used with *?, using <*? instead");
		}
		$$ = new_pattern(new_pattern_op(OP_STAR_LEFT_MIN, NULL), $1, NULL);
	}

	;

%%

int line_number = 1, lexerr = 0;
const char *lexerrmsg;
extern char *filename;

int yyerror(char *s) {
  fprintf(stderr, "%s:%d: %s\n", filename, line_number, lexerr?lexerrmsg:s);
  return 0;
}

