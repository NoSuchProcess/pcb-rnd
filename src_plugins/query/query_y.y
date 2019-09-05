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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* Query language - compiler: grammar */

#include <assert.h>
#include "unit.h"
#include "query.h"
#include "query_l.h"
#include "compat_misc.h"
#include "flag_str.h"
#include "fields_sphash.h"

#define UNIT_CONV(dst, negative, val, unit) \
do { \
	dst = val; \
	if (negative) \
		dst = -dst; \
	if (unit != NULL) { \
		if (unit->family == PCB_UNIT_IMPERIAL) \
			dst = PCB_MIL_TO_COORD(dst); \
		else if (unit->family == PCB_UNIT_METRIC) \
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

static pcb_query_iter_t *iter_ctx;

static char *attrib_prepend_free(char *orig, char *prep, char sep)
{
	int l1 = strlen(orig), l2 = strlen(prep);
	char *res = malloc(l1+l2+2);
	memcpy(res, prep, l2);
	res[l2] = sep;
	memcpy(res+l2+1, orig, l1+1);
	free(orig);
	free(prep);
	return res;
}

static pcb_qry_node_t *make_regex_free(char *str)
{
	pcb_qry_node_t *res = pcb_qry_n_alloc(PCBQ_DATA_REGEX);
	res->data.str = str;
	res->precomp.regex = re_se_comp(str);
	if (res->precomp.regex == NULL)
		yyerror(NULL, "Invalid regex\n");
	return res;
}


static pcb_qry_node_t *make_flag_free(char *str)
{
	const pcb_flag_bits_t *i = pcb_strflg_name(str, 0x7FFFFFFF);
	pcb_qry_node_t *nd;

	if (i == NULL) {
		yyerror(NULL, "Unknown flag");
		free(str);
		return NULL;
	}

	nd = pcb_qry_n_alloc(PCBQ_FLAG);
	nd->precomp.flg = i;
	free(str);
	return nd;
}


%}

%name-prefix "qry_"
%error-verbose
%parse-param {pcb_qry_node_t **prg_out}

%union {
	char *s;
	pcb_coord_t c;
	double d;
	const pcb_unit_t *u;
	pcb_qry_node_t *n;
}

%token     T_LET T_ASSERT T_RULE T_LIST T_INVALID T_FLD_P T_FLD_A T_FLD_FLAG
%token     T_OR T_AND T_EQ T_NEQ T_GTEQ T_LTEQ
%token     T_NL
%token <u> T_UNIT
%token <s> T_STR T_QSTR
%token <c> T_INT
%token <d> T_DBL
%token <n> T_CONST

/* the usual binary operators */
%left T_THUS
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

%type <n> number fields attribs var fname fcall fargs words string_literal
%type <n> expr exprs program_expr program_rules rule flg
%type <u> maybe_unit

%%

program:
	  program_rules    { *prg_out = $1; }
	| program_expr     { *prg_out = $1; }
	;

/* The program is a single expression - useful for search */
program_expr:
	{ iter_ctx = pcb_qry_iter_alloc(); }
	expr               {
		$$ = pcb_qry_n_alloc(PCBQ_EXPR_PROG);
		$$->data.children = pcb_qry_n_alloc(PCBQ_ITER_CTX);
		$$->data.children->parent = $$;
		$$->data.children->data.iter_ctx = iter_ctx;
		$$->data.children->next = $2;
		$2->parent = $$;
	}
	;

/* The program is a collection of rules - useful for the DRC */
program_rules:
	  /* empty */            { $$ = NULL; }
	| rule program_rules     { $$ = $1; $1->next = $2; }
	;

rule:
	T_RULE words T_NL exprs  {
		$$ = pcb_qry_n_alloc(PCBQ_RULE);
		$$->data.children = $2;
		$2->parent = $$;
		$$->data.children->next = pcb_qry_n_alloc(PCBQ_ITER_CTX);
		$$->data.children->next->data.iter_ctx = iter_ctx;
		$$->data.children->next->next = $4;
		$4->parent = $$;
		}
	;

exprs:
	  /* empty */            { $$ = NULL; }
	| exprs expr T_NL        { $$ = $1; $1->next = $2; }
	;

expr:
	  fcall                  { $$ = $1; }
	| number                 { $$ = $1; }
	| string_literal         { $$ = $1; }
	| T_INVALID              { $$ = pcb_qry_n_alloc(PCBQ_DATA_INVALID); }
	| '!' expr               { UNOP($$, PCBQ_OP_NOT, $2); }
	| '(' expr ')'           { $$ = $2; }
	| expr T_THUS expr       { BINOP($$, $1, PCBQ_OP_THUS, $3); }
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
	| expr '~' T_STR         { BINOP($$, $1, PCBQ_OP_MATCH, make_regex_free($3)); }
	| expr '~' T_QSTR        { BINOP($$, $1, PCBQ_OP_MATCH, make_regex_free($3)); }
	| T_CONST                { $$ = $1; }
	| var                    { $$ = $1; }
	| var '.' fields         {
		pcb_qry_node_t *n;
		$$ = pcb_qry_n_alloc(PCBQ_FIELD_OF);
		$$->data.children = $1;
		$1->next = $3;
		$1->parent = $$;
		for(n = $3; n != NULL; n = n->next)
			n->parent = $$;
		}
	;

number:
	  T_INT maybe_unit       { $$ = pcb_qry_n_alloc(PCBQ_DATA_COORD);  UNIT_CONV($$->data.crd, 0, $1, $2); }
	| T_DBL maybe_unit       { $$ = pcb_qry_n_alloc(PCBQ_DATA_DOUBLE); UNIT_CONV($$->data.dbl, 0, $1, $2); }
	| '-' T_INT maybe_unit   { $$ = pcb_qry_n_alloc(PCBQ_DATA_COORD);  UNIT_CONV($$->data.crd, 1, $2, $3); }
	| '-' T_DBL maybe_unit   { $$ = pcb_qry_n_alloc(PCBQ_DATA_DOUBLE); UNIT_CONV($$->data.dbl, 1, $2, $3); }
	;

string_literal:
	T_QSTR                   { $$ = pcb_qry_n_alloc(PCBQ_DATA_STRING);  $$->data.str = $1; }
	;

maybe_unit:
	  /* empty */            { $$ = NULL; }
	| T_UNIT                 { $$ = $1; }
	;

fields:
	  T_STR                  { $$ = pcb_qry_n_alloc(PCBQ_FIELD); $$->data.str = $1; $$->precomp.fld = query_fields_sphash($1); }
	| T_STR '.' fields       { $$ = pcb_qry_n_alloc(PCBQ_FIELD); $$->data.str = $1; $$->precomp.fld = query_fields_sphash($1); $$->next = $3; }
	| T_FLD_P fields         { $$ = $2; /* just ignore .p. */ }
	| T_FLD_P T_FLD_FLAG T_STR { $$ = make_flag_free($3); }
	| T_FLD_A attribs        { $$ = pcb_qry_n_alloc(PCBQ_FIELD); $$->data.str = pcb_strdup("a"); $$->precomp.fld = query_fields_sphash("a"); $$->next = $2; }
	;

attribs:
	  T_STR                  { $$ = pcb_qry_n_alloc(PCBQ_FIELD); $$->data.str = $1; }
	| T_STR '.' attribs      { $$ = pcb_qry_n_alloc(PCBQ_FIELD); $$->data.str = attrib_prepend_free((char *)$3->data.str, $1, '.'); }
	| T_QSTR                 { $$ = pcb_qry_n_alloc(PCBQ_FIELD); $$->data.str = $1; }
	;

var:
	  T_STR                  { $$ = pcb_qry_n_alloc(PCBQ_VAR); $$->data.crd = pcb_qry_iter_var(iter_ctx, $1, 1); free($1); }
	| T_LIST '(' '@' ')'     { $$ = pcb_qry_n_alloc(PCBQ_LISTVAR); $$->data.str = pcb_strdup("@"); }
	| '@'                    { $$ = pcb_qry_n_alloc(PCBQ_VAR); $$->data.crd = pcb_qry_iter_var(iter_ctx, "@", 1); }
	;

fcall:
	  fname '(' fargs ')'    { $$ = pcb_qry_n_alloc(PCBQ_FCALL); $$->data.children = $1; $$->data.children->next = $3; $1->parent = $3->parent = $$; }
	| fname '(' ')'          { $$ = pcb_qry_n_alloc(PCBQ_FCALL); $$->data.children = $1; $1->parent = $$; }
	;

fname:
	T_STR   {
		$$ = pcb_qry_n_alloc(PCBQ_FNAME);
		$$->data.fnc = pcb_qry_fnc_lookup($1);
		if ($$->data.fnc == NULL) {
			yyerror(NULL, "Unknown function");
			free($1);
			return -1;
		}
		free($1);
	}
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
