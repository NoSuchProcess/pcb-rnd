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
#include <librnd/core/unit.h>
#include "query.h"
#include "query_l.h"
#include <librnd/core/compat_misc.h>
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
static vti0_t *iter_active_ctx;

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

%type <n> number fields attribs let assert var constant fname fcall fargs words
%type <n> string_literal expr rule_item program_expr program_rules rule flg
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
	| rule program_rules     { if ($1 != NULL) { $$ = $1; $1->next = $2; } else { $$ = $2; } }
	;

rule:
	T_RULE words T_NL
	{ iter_ctx = pcb_qry_iter_alloc(); }
	rule_item {
		pcb_qry_node_t *nd, *tmp;
		$$ = pcb_qry_n_alloc(PCBQ_RULE);

		if ($5 != NULL) {
			tmp = $5->next; /* assume $$ has no children, keep $5's original children */
			pcb_qry_n_insert($$, $5);
			$5->next = tmp;
			
			/* set parent of all nodes */
			for(tmp = $5; tmp != NULL; tmp = tmp->next)
				tmp->parent = $$;
		}

		pcb_qry_n_insert($$, $2);

		nd = pcb_qry_n_alloc(PCBQ_ITER_CTX);
		nd->data.iter_ctx = iter_ctx;
		pcb_qry_n_insert($$, nd);

	}
	;

rule_item:
	  /* empty */                      { $$ = NULL; }
	| assert T_NL rule_item            { $$ = $1; $1->next = $3; }
	| let T_NL rule_item               { $$ = $1; $1->next = $3; }
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
	| constant               { $$ = $1; }
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

let:
		T_LET
			{
				iter_active_ctx = calloc(sizeof(vti0_t), 1);
			}
			T_STR expr
			{
				pcb_qry_node_t *nd;
				$$ = pcb_qry_n_alloc(PCBQ_LET);
				$$->precomp.it_active = iter_active_ctx;
				iter_active_ctx = NULL;
				pcb_qry_n_insert($$, $4);
				nd = pcb_qry_n_alloc(PCBQ_VAR);
				nd->data.crd = pcb_qry_iter_var(iter_ctx, $3, 1);
				pcb_qry_n_insert($$, nd);
				free($3);
			}
		;

assert:
	T_ASSERT
		{
			iter_active_ctx = calloc(sizeof(vti0_t), 1);
		}
		expr
		{
			$$ = pcb_qry_n_alloc(PCBQ_ASSERT);
			$$->precomp.it_active = iter_active_ctx;
			iter_active_ctx = NULL;
			pcb_qry_n_insert($$, $3);
		}
	;

var:
	  T_STR                  { $$ = pcb_qry_n_alloc(PCBQ_VAR); $$->data.crd = pcb_qry_iter_var(iter_ctx, $1, 1); if (iter_active_ctx != NULL) vti0_set(iter_active_ctx, $$->data.crd, 1); free($1); }
	| T_LIST '(' '@' ')'     { $$ = pcb_qry_n_alloc(PCBQ_LISTVAR); $$->data.str = pcb_strdup("@"); /* delibertely not setting iter_active, list() protects against turning it into an iterator */ }
	| '@'                    { $$ = pcb_qry_n_alloc(PCBQ_VAR); $$->data.crd = pcb_qry_iter_var(iter_ctx, "@", 1); if (iter_active_ctx != NULL) vti0_set(iter_active_ctx, $$->data.crd, 1); }
	;

/* $foo is shorthand for getconf("design/drc/foo") */
constant:
	'$' T_STR
		{
			pcb_qry_node_t *fname, *nname;

			nname = pcb_qry_n_alloc(PCBQ_DATA_STRING);
			nname->data.str = pcb_concat("design/drc/", $2, NULL);
			free($2);

			fname = pcb_qry_n_alloc(PCBQ_FNAME);
			fname->data.fnc = pcb_qry_fnc_lookup("getconf");

			$$ = pcb_qry_n_alloc(PCBQ_FCALL);
			fname->parent = nname->parent = $$;
			$$->data.children = fname;
			$$->data.children->next = nname;
		}
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
			char *old = $2->data.str, *sep = ((*old != '\0') ? " " : "");
			$2->data.str = pcb_concat($1, sep, old, NULL);
			free(old);
			free($1);
			$$ = $2;
		}
	;
