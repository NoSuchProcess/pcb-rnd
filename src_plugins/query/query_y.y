%{
#include <assert.h>
#include "global.h"
#include "unit.h"
#include "query.h"

#define UNIT_CONV(dst, negative, val, unit) \
do { \
	dst = val; \
	if (negative) \
		dst = -dst; \
	if (unit != NULL) { \
		if (unit->family == IMPERIAL) \
			dst = PCB_MIL_TO_COORD(dst); \
		dst *= unit->scale_factor; \
	} \
} while(0)

#define BINOP(dst, op1, operator, op2) \
do { \
	assert(op1->next = NULL); \
	assert(op2->next = NULL); \
	dst = pcb_qry_n_alloc(operator); \
	pcb_qry_n_insert(dst, op2); \
	pcb_qry_n_insert(dst, op1); \
} while(0)

#define UNOP(dst, operator, op) \
do { \
	assert(op->next = NULL); \
	dst = pcb_qry_n_alloc(operator); \
	pcb_qry_n_insert(dst, op); \
} while(0)

%}

%name-prefix "qry_"

%verbose

%union {
	char *s;
	Coord c;
	double d;
	const Unit *u;
	pcb_qry_node_t *n;
}

%token     T_LET T_ASSERT T_RULE
%token     T_OR T_AND T_EQ T_NEQ T_GTEQ T_LTEQ
%token     T_NL
%token <u> T_UNIT
%token <s> T_STR
%token <c> T_INT
%token <d> T_DBL

/* the usual binary operators */
%left T_OR
%left T_AND
%left T_EQ T_NEQ
%left '<' '>' T_GTEQ T_LTEQ
%left '+' '-'
%left '*' '/'
%left '.'

/* Unary operators */
%right '!'

%left '('

%type <n> expr number
%type <u> maybe_unit

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
	| number                 { $$ = $1; }
	| '!' expr               { UNOP($$, PCBQ_OP_NOT, $2); }
	| '(' expr ')'           { $$ = $2; }
	| expr T_AND expr        { BINOP($$, $1, PCBQ_OP_AND, $3); }
	| expr T_OR expr         { BINOP($$, $1, PCBQ_OP_OR, $3); }
	| expr T_EQ expr         { BINOP($$, $1, PCBQ_OP_EQ, $3); }
	| expr T_NEQ expr        { BINOP($$, $1, PCBQ_OP_NEQ, $3); }
	| expr T_GTEQ expr       { BINOP($$, $1, PCBQ_OP_GTEQ, $3); }
	| expr T_LTEQ expr       { BINOP($$, $1, PCBQ_OP_LTEQ, $3); }
	| expr '>' expr          { BINOP($$, $1, PCBQ_OP_GT, $3); }
	| expr '<' expr          { BINOP($$, $1, PCBQ_OP_LT, $3); }
	| expr '+' expr          { BINOP($$, $1, PCBQ_OP_ADD, $3); }
	| expr '-' expr          { BINOP($$, $1, PCBQ_OP_SUB, $3); }
	| expr '*' expr          { BINOP($$, $1, PCBQ_OP_MUL, $3); }
	| expr '/' expr          { BINOP($$, $1, PCBQ_OP_DIV, $3); }
	| expr '.' T_STR
	;

number:
	  T_INT maybe_unit       { $$ = pcb_qry_n_alloc(PCBQ_DATA_COORD);  UNIT_CONV($$->crd, 0, $1, $2); }
	| T_DBL maybe_unit       { $$ = pcb_qry_n_alloc(PCBQ_DATA_DOUBLE); UNIT_CONV($$->dbl, 0, $1, $2); }
	| '-' T_INT maybe_unit   { $$ = pcb_qry_n_alloc(PCBQ_DATA_COORD);  UNIT_CONV($$->crd, 1, $2, $3); }
	| '-' T_DBL maybe_unit   { $$ = pcb_qry_n_alloc(PCBQ_DATA_DOUBLE); UNIT_CONV($$->dbl, 1, $2, $3); }
	;

maybe_unit:
	  /* empty */            { $$ = NULL; }
	| T_UNIT                 { $$ = $1; }
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
