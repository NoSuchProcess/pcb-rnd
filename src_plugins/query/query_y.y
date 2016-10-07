%{
/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* Query language - compiler: grammar */

#include <assert.h>
#include "global.h"
#include "unit.h"
#include "query.h"
#include "query_l.h"
#include "compat_misc.h"

#define UNIT_CONV(dst, negative, val, unit) \
do { \
	dst = val; \
	if (negative) \
		dst = -dst; \
	if (unit != NULL) { \
		if (unit->family == IMPERIAL) \
			dst = PCB_MIL_TO_COORD(dst); \
		else if (unit->family == METRIC) \
			dst = PCB_MM_TO_COORD(dst); \
		dst /= unit->scale_factor; \
	} \
} while(0)

#define BINOP(dst, op1, operator, op2) \
do { \
	assert(op2->next == NULL); \
	assert(op2->next == NULL); \
	dst = pcb_qry_n_alloc(operator); \
	pcb_qry_n_insert(dst, op2); \
	pcb_qry_n_insert(dst, op1); \
} while(0)

#define UNOP(dst, operator, op) \
do { \
	assert(op->next == NULL); \
	dst = pcb_qry_n_alloc(operator); \
	pcb_qry_n_insert(dst, op); \
} while(0)

%}

%name-prefix "qry_"
%define parse.error verbose
%parse-param {pcb_qry_node_t **prg_out}

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

%type <n> number fields var fname fcall fargs words
%type <n> expr exprs program_expr program_rules rule
%type <u> maybe_unit

%%

program:
	  program_rules    { *prg_out = $1; }
	| program_expr     { *prg_out = $1; }
	;

/* The program is a single expression - useful for search */
program_expr:
	expr               { $$ = $1; }
	;

/* The program is a collection of rules - useful for the DRC */
program_rules:
	  /* empty */            { $$ = NULL; }
	| rule program_rules     { $$ = $1; $1->next = $2; }
	;

rule:
	T_RULE words T_NL exprs  { $$ = pcb_qry_n_alloc(PCBQ_RULE); $$->data.children = $2; $$->data.children->next = $4; }
	;

exprs:
	  /* empty */            { $$ = NULL; }
	| exprs expr T_NL        { $$ = $1; $1->next = $2; }
	;

expr:
	  fcall                  { $$ = $1; }
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
	| var                    { $$ = $1; }
	| var '.' fields         { $$ = pcb_qry_n_alloc(PCBQ_FIELD_OF); $$->data.children = $1; $1->next = $3;}
	;

number:
	  T_INT maybe_unit       { $$ = pcb_qry_n_alloc(PCBQ_DATA_COORD);  UNIT_CONV($$->data.crd, 0, $1, $2); }
	| T_DBL maybe_unit       { $$ = pcb_qry_n_alloc(PCBQ_DATA_DOUBLE); UNIT_CONV($$->data.dbl, 0, $1, $2); }
	| '-' T_INT maybe_unit   { $$ = pcb_qry_n_alloc(PCBQ_DATA_COORD);  UNIT_CONV($$->data.crd, 1, $2, $3); }
	| '-' T_DBL maybe_unit   { $$ = pcb_qry_n_alloc(PCBQ_DATA_DOUBLE); UNIT_CONV($$->data.dbl, 1, $2, $3); }
	;

maybe_unit:
	  /* empty */            { $$ = NULL; }
	| T_UNIT                 { $$ = $1; }
	;

fields:
	  T_STR                  { $$ = pcb_qry_n_alloc(PCBQ_FIELD); $$->data.str = $1; }
	| T_STR '.' fields       { $$ = pcb_qry_n_alloc(PCBQ_FIELD); $$->data.str = $1; $$->next = $3; }
	;

var:
	  T_STR                  { $$ = pcb_qry_n_alloc(PCBQ_VAR); $$->data.str = $1; }
	| '@'                    { $$ = pcb_qry_n_alloc(PCBQ_OBJ); }
	;

fcall:
	  fname '(' fargs ')'    { $$ = pcb_qry_n_alloc(PCBQ_FCALL); $$->data.children = $1; $$->data.children->next = $3; }
	| fname '(' ')'          { $$ = pcb_qry_n_alloc(PCBQ_FCALL); $$->data.children = $1; }
	;

fname:
	T_STR                    { $$ = pcb_qry_n_alloc(PCBQ_FNAME); $$->data.str = $1; }
	;


fargs:
	  expr                   { $$ = $1; }
	| expr ',' fargs         { $$ = $1; $$->next = $3; }
	;

words:
	  /* empty */            { $$ = pcb_qry_n_alloc(PCBQ_RNAME); $$->data.str = (const char *)pcb_strdup(""); }
	| T_STR words            {
			int l1 = strlen($2->data.str), l2 = strlen($1);
			
			$2->data.str = (const char *)realloc((void *)$2->data.str, l1+l2+2);
			memcpy((char *)$2->data.str+l1, $1, l2+1);
			free($1);
		}
	;
