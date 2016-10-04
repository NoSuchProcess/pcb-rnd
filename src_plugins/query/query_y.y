%{
#include "global.h"
#include "unit.h"
%}

%name-prefix "qry_"

%verbose

%union {
	char *s;
	Coord c;
	double d;
	const Unit *u;
}

%token     T_LET T_ASSERT T_RULE
%token     T_OR T_AND T_EQ T_NEQ T_GTEQ T_LTEQ
%token     T_NL
%token <u> T_UNIT
%token <s> T_STR
%token <c> T_INT
%token <d> T_DBL

%left T_AND T_OR
%left '!'
%left '<' '>' T_EQ T_NEQ T_GTEQ T_LTEQ
%left '+' '-'
%left '*' '/'
%left '.'

%%

program:
	  program_rules
	| program_expr
	;

/* The program is a single expression - useful for search */
program_expr:
	expr
	;

/* The program is a collection of rules - useful for the DRC */
program_rules:
	  /* empty */
	| program rule
	;

rule:
	T_RULE words T_NL exprs
	;

exprs:
	  /* empty */
	| exprs expr T_NL
	;

expr:
	  fcall
	| T_STR
	| '!' expr
	| '(' expr ')'
	| expr T_AND expr
	| expr T_OR expr
	| expr T_EQ expr
	| expr T_NEQ expr
	| expr T_GTEQ expr
	| expr T_LTEQ expr
	| expr '>' expr
	| expr '<' expr
	| expr '+' expr
	| expr '-' expr
	| expr '*' expr
	| expr '/' expr
	| expr '.' T_STR
	;

fcall:
	  T_STR '(' fargs ')'
	| T_STR '(' ')'
	;

fargs:
	  expr
	| fargs ',' expr
	;

words:
	  /* empty */
	| words T_STR
	;
