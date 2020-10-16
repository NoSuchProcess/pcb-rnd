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
#include <genht/htsp.h>
#include <genht/hash.h>
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
		if (unit->family == RND_UNIT_IMPERIAL) \
			dst = RND_MIL_TO_COORD(dst); \
		else if (unit->family == RND_UNIT_METRIC) \
			dst = RND_MM_TO_COORD(dst); \
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
static htsp_t *user_funcs;

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

static void link_user_funcs_(pcb_qry_node_t *root, int allow)
{
	pcb_qry_node_t *n, *f, *fname;

	for(n = root; n != NULL; n = n->next) {
		if (pcb_qry_nodetype_has_children(n->type))
			link_user_funcs_(n->data.children, allow);
		if (n->type == PCBQ_FCALL) {
			fname = n->data.children;
			if (fname->precomp.fnc.bui != NULL) /* builtin */
				continue;

			if (user_funcs != NULL)
				f = htsp_get(user_funcs, fname->data.str);
			else
				f = NULL;

			fname->precomp.fnc.uf = f;
			if (f == NULL) {
				yyerror(NULL, "user function not defined:");
				yyerror(NULL, fname->data.str);
			}
		}
	}

}

static void link_user_funcs(pcb_qry_node_t *root, int allow)
{
	link_user_funcs_(root, allow);

	if (user_funcs != NULL)
		htsp_free(user_funcs);
	user_funcs = NULL;
}


%}

%name-prefix "qry_"
%error-verbose
%parse-param {pcb_qry_node_t **prg_out}

%union {
	char *s;
	rnd_coord_t c;
	double d;
	const rnd_unit_t *u;
	pcb_qry_node_t *n;
}

%token     T_LET T_ASSERT T_RULE T_LIST T_INVALID T_FLD_P T_FLD_A T_FLD_FLAG
%token     T_FUNCTION T_RETURN
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

%type <n> number fields attribs let assert var constant fcallname fcall
%type <n> fargs words freturn fdefarg fdefargs fdef_ fdef fdefname rule_or_fdef
%type <n> string_literal expr rule_item program_expr program_rules rule
%type <u> maybe_unit

%%

program:
	  program_rules    { *prg_out = $1; link_user_funcs($1, 1); }
	| program_expr     { *prg_out = $1; assert(user_funcs == NULL); link_user_funcs($1, 0); }
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
	rule_or_fdef T_NL rule_item {
		$$ = $1;
		if ($3 != NULL)
			pcb_qry_n_append($$, $3);
	}
	;

rule_or_fdef:
	  fdef { $$ = $1; }
	| T_RULE words  {
			pcb_qry_node_t *nd;
			iter_ctx = pcb_qry_iter_alloc();
			$$ = pcb_qry_n_alloc(PCBQ_RULE);
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
	| freturn T_NL rule_item           { $$ = $1; $1->next = $3; }
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
	| expr '.' fields        {
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
	| T_FLD_A attribs        { $$ = pcb_qry_n_alloc(PCBQ_FIELD); $$->data.str = rnd_strdup("a"); $$->precomp.fld = query_fields_sphash("a"); $$->next = $2; }
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

freturn:
	T_RETURN
		{
			iter_active_ctx = calloc(sizeof(vti0_t), 1);
		}
		expr
		{
			$$ = pcb_qry_n_alloc(PCBQ_RETURN);
			$$->precomp.it_active = iter_active_ctx;
			iter_active_ctx = NULL;
			pcb_qry_n_insert($$, $3);
		}
	;

var:
	  T_STR                  { $$ = pcb_qry_n_alloc(PCBQ_VAR); $$->data.crd = pcb_qry_iter_var(iter_ctx, $1, 1); if (iter_active_ctx != NULL) vti0_set(iter_active_ctx, $$->data.crd, 1); free($1); }
	| T_LIST '(' '@' ')'     { $$ = pcb_qry_n_alloc(PCBQ_LISTVAR); $$->data.str = rnd_strdup("@"); /* delibertely not setting iter_active, list() protects against turning it into an iterator */ }
	| T_LIST '(' T_STR ')'   { $$ = pcb_qry_n_alloc(PCBQ_LISTVAR); $$->data.str = $3; /* delibertely not setting iter_active, list() protects against turning it into an iterator */ }
	| '@'                    { $$ = pcb_qry_n_alloc(PCBQ_VAR); $$->data.crd = pcb_qry_iter_var(iter_ctx, "@", 1); if (iter_active_ctx != NULL) vti0_set(iter_active_ctx, $$->data.crd, 1); }
	;

/* $foo is shorthand for getconf("design/drc/foo") */
constant:
	'$' T_STR
		{
			pcb_qry_node_t *fname, *nname;

			nname = pcb_qry_n_alloc(PCBQ_DATA_STRING);
			nname->data.str = rnd_concat("design/drc/", $2, NULL);
			free($2);

			fname = pcb_qry_n_alloc(PCBQ_FNAME);
			fname->data.str = NULL;
			fname->precomp.fnc.bui = pcb_qry_fnc_lookup("getconf");

			$$ = pcb_qry_n_alloc(PCBQ_FCALL);
			fname->parent = nname->parent = $$;
			$$->data.children = fname;
			$$->data.children->next = nname;
		}
	;

fcall:
	  fcallname '(' fargs ')'    { $$ = pcb_qry_n_alloc(PCBQ_FCALL); $$->data.children = $1; $$->data.children->next = $3; $1->parent = $3->parent = $$; }
	| fcallname '(' ')'          { $$ = pcb_qry_n_alloc(PCBQ_FCALL); $$->data.children = $1; $1->parent = $$; }
	;

fcallname:
	T_STR   {
		$$ = pcb_qry_n_alloc(PCBQ_FNAME);
		$$->precomp.fnc.bui = pcb_qry_fnc_lookup($1);
		if ($$->precomp.fnc.bui != NULL) {
			/* builtin function */
			free($1);
			$$->data.str = NULL;
		}
		else
			$$->data.str = $1; /* user function: save the name */
	}
	;


fargs:
	  expr                   { $$ = $1; }
	| expr ',' fargs         { $$ = $1; $$->next = $3; }
	;


fdefarg:
	T_STR {
		$$ = pcb_qry_n_alloc(PCBQ_ARG);
		$$->data.crd = pcb_qry_iter_var(iter_ctx, $1, 1);
	}
	;

fdefargs:
	  fdefarg                  { $$ = $1; }
	| fdefarg ',' fdefarg      { $$ = $1; $$->next = $3; }
	;

fdefname:
	T_STR   {
		$$ = pcb_qry_n_alloc(PCBQ_FNAME);
		$$->data.str = $1;
	}
	;

fdef_:
	  '(' fdefargs ')'    { $$ = $2; }
	| '(' ')'             { $$ = NULL; }
	;

fdef:
	T_FUNCTION fdefname
		{ iter_ctx = pcb_qry_iter_alloc(); }
	 fdef_ {
		pcb_qry_node_t *nd;

		$$ = pcb_qry_n_alloc(PCBQ_FUNCTION);

		if ($4 != NULL)
			pcb_qry_n_append($$, $4);

		pcb_qry_n_insert($$, $2);

		nd = pcb_qry_n_alloc(PCBQ_ITER_CTX);
		nd->data.iter_ctx = iter_ctx;
		pcb_qry_n_insert($$, nd);

		if (user_funcs == NULL)
			user_funcs = htsp_alloc(strhash, strkeyeq);
		htsp_set(user_funcs, (char *)$2->data.str, $$);
	}
	;
words:
	  /* empty */            { $$ = pcb_qry_n_alloc(PCBQ_RNAME); $$->data.str = (const char *)rnd_strdup(""); }
	| T_STR words            {
			char *old = (char *)$2->data.str, *sep = ((*old != '\0') ? " " : "");
			$2->data.str = rnd_concat($1, sep, old, NULL);
			free(old);
			free($1);
			$$ = $2;
		}
	;
