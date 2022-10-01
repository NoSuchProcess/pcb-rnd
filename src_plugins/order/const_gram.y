%prefix pcb_ordc_%

%struct
{
	long line, first_col, last_col;
}

%union
{
	double d;
	int i;
	char *s;
	void *tree;
}


%{
/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  order plugin - constraint language grammar
 *  pcb-rnd Copyright (C) 2020 Tibor 'Igor2' Palinkas
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
 */
#include "../src_plugins/order/const_gram.h"
#include "../src_plugins/order/constraint.h"


%}

/* Constants */
%token T_CINT T_CFLOAT T_STRING T_QSTR T_ID

/* Operators */
%token T_EQ T_NEQ T_GE T_LE T_GT T_LT T_AND T_OR

/* Keywords for builtin functions */
%token T_IF T_ERROR T_INT T_FLOAT T_STR


%left T_EQ T_NEQ T_GE T_LE T_GT T_LT
%left T_OR
%left T_AND
%left '+' '-'
%left '*' '/' '%'
%left '!'
%right '(' ')'

%type <s> T_QSTR
%type <s> T_ID
%type <i> T_CINT
%type <d> T_CFLOAT

%type <tree> expr
%type <tree> stmt_if
%type <tree> stmt_error
%type <tree> stmt_block
%type <tree> statement

%%

file:
	/* empty */
	| statement file    { prepend(ctx->root, $1); }
	;

statement:
	  stmt_if           { $$ = $1; }
	| stmt_error        { $$ = $1; }
	| stmt_block        { $$ = $1; }
	;

/*** expressions ***/
expr:
	  '-' expr              { $$ = unop(PCB_ORDC_NEG, $2); }
	| '(' expr ')'          { $$ = $2; }
	| expr T_EQ expr        { $$ = binop(PCB_ORDC_EQ, $1, $3); }
	| expr T_NEQ expr       { $$ = binop(PCB_ORDC_NEQ, $1, $3); }
	| expr T_GE expr        { $$ = binop(PCB_ORDC_GE, $1, $3); }
	| expr T_LE expr        { $$ = binop(PCB_ORDC_LE, $1, $3); }
	| expr T_GT expr        { $$ = binop(PCB_ORDC_GT, $1, $3); }
	| expr T_LT expr        { $$ = binop(PCB_ORDC_LT, $1, $3); }
	| expr T_AND expr       { $$ = binop(PCB_ORDC_AND, $1, $3); }
	| expr T_OR expr        { $$ = binop(PCB_ORDC_OR, $1, $3); }

	| expr '+' expr         { $$ = binop(PCB_ORDC_ADD, $1, $3); }
	| expr '-' expr         { $$ = binop(PCB_ORDC_SUB, $1, $3); }

	| T_CINT                { $$ = int2node($1); }
	| T_CFLOAT              { $$ = float2node($1); }
	| T_QSTR                { $$ = qstr2node($1); }
	| '$' T_ID              { $$ = var2node($2); }

	| T_INT '(' expr ')'    { $$ = unop(PCB_ORDC_INT, $3); }
	| T_STRING '(' expr ')' { $$ = unop(PCB_ORDC_STRING, $3); }
	| T_FLOAT '(' expr ')'  { $$ = unop(PCB_ORDC_FLOAT, $3); }
	;

/*** statements ***/
stmt_block:
	'{' statement '}'      { $$ = $2; }

stmt_if:
	T_IF '(' expr ')' statement           { $$ = binop(PCB_ORDC_IF, $3, $5); }
	;

stmt_error:
	T_ERROR '(' T_ID ',' T_QSTR ')' ';'   { $$ = binop(PCB_ORDC_ERROR, id2node($3), qstr2node($5)); }
	;

%%

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

static pcb_ordc_node_t *new_node(pcb_ordc_node_type_t ty)
{
	pcb_ordc_node_t *n = calloc(sizeof(pcb_ordc_node_t), 1);
	n->type = ty;
	return n;
}

static pcb_ordc_node_t *binop(pcb_ordc_node_type_t ty, pcb_ordc_node_t *a, pcb_ordc_node_t *b)
{
	pcb_ordc_node_t *n = new_node(ty);
	assert((a == NULL) || (a->next == NULL));
	assert((b == NULL) || (b->next == NULL));
	n->ch_first = a;
	if (a != NULL)
		a->next = b;
	return n;
}

#define unop(ty, a) binop(ty, a, NULL)

static pcb_ordc_node_t *id2node(char *s)
{
	pcb_ordc_node_t *n = new_node(PCB_ORDC_ID);
	n->val.s = s;
	return n;
}

static pcb_ordc_node_t *qstr2node(char *s)
{
	pcb_ordc_node_t *n = new_node(PCB_ORDC_QSTR);
	n->val.s = s;
	return n;
}

static pcb_ordc_node_t *var2node(char *s)
{
	pcb_ordc_node_t *n = new_node(PCB_ORDC_VAR);
	n->val.s = s;
	return n;
}

static pcb_ordc_node_t *int2node(long l)
{
	pcb_ordc_node_t *n = new_node(PCB_ORDC_CINT);
	n->val.l = l;
	return n;
}

static pcb_ordc_node_t *float2node(double d)
{
	pcb_ordc_node_t *n = new_node(PCB_ORDC_CFLOAT);
	n->val.d = d;
	return n;
}


static void prepend(pcb_ordc_node_t *parent, pcb_ordc_node_t *newch)
{
	assert(newch->next == NULL);

	newch->next = parent->ch_first;
	parent->ch_first = newch;
}

