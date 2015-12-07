/* Copyright (c) 2002 by David Chiang. All rights reserved.*/

%{
#include "config.h"

#include "pattern.h"
#include "pattern_op.h"
#include "node.h"

#define YYSTYPE node
#include "compiler.h"
%}

%x comment 
%option noyywrap

%%

%{
extern int line_number, lexerr;
extern const char *lexerrmsg;
%}

#[^\n]*		;

\"[^\"\n]*\"	{
	yytext[yyleng-1] = '\0';
	yyleng--;
	yylval = new_pattern(new_pattern_op(OP_ATOM, yytext+1), NULL, NULL);
	return SYMBOL;
}
\"	{
	lexerrmsg = "string constant exceeds line";
	lexerr = 1;
	return ERROR;
}

[ \t]		;
\n		line_number++;

->	return RARROW;
:=	return GETS;
>\*	return STARLEFTMAX;
\<\*	return STARRIGHTMAX;
>\*\?	return STARLEFTMIN;
\<\*\?	return STARRIGHTMIN;
\*	return STARMAX;
\*\?	return STARMIN;
\?\?    return OPTMIN;
[.+-;|<>()&,!?]	return yytext[0];

[A-Za-z0-9_[\]]+ {
	yylval = new_pattern(new_pattern_op(OP_ATOM, yytext), NULL, NULL);
	return SYMBOL;
}

. {
  lexerrmsg = "invalid character";
  lexerr = 1;
  return ERROR;
}
