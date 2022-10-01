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
#include <stdio.h>
#include "../src_plugins/order/const_gram.h"
#include "../src_plugins/order/constraint.h"

#define binop(a, b, c) NULL
#define unop(a, b) NULL

%}

/* Generic */
%token T_ID T_INTEGER T_FLOAT T_QSTR

/* Keywords for builtin functions */
%token T_IF T_ERROR
%token T_INTVAL T_FLOATVAL

%left "==" ">=" "<=" ">" "<"
%left "||"
%left "&&"
%left '+' '-'
%left '*' '/' '%'
%left '!'
%right '(' ')'

%type <s> T_QSTR
%type <s> T_ID
%type <i> T_INTEGER
%type <d> T_FLOAT

%type <tree> expr
%type <tree> stmt_if
%type <tree> stmt_error
%type <tree> stmt_block
%type <tree> statement

%%

file:
	/* empty */
	|statement file
	;

statement:
	  stmt_if
	| stmt_error
	| stmt_block
	;

/*** expressions ***/
expr:
	  '-' expr              { $$ = unop(PCB_ORDC_NEG, $2); }
	| '(' expr ')'          { $$ = $2; }
	| expr "==" expr        { $$ = binop(PCB_ORDC_EQ, $1, $3); }
	| expr "!=" expr        { $$ = binop(PCB_ORDC_NEQ, $1, $3); }
	| expr ">=" expr        { $$ = binop(PCB_ORDC_GE, $1, $3); }
	| expr "<=" expr        { $$ = binop(PCB_ORDC_LE, $1, $3); }

	| expr '+' expr         { $$ = binop(PCB_ORDC_ADD, $1, $3); }
	| expr '-' expr         { $$ = binop(PCB_ORDC_SUB, $1, $3); }

	| T_INTEGER             { $$ = NULL; }
	| T_FLOAT               { $$ = NULL; }
	| T_QSTR                { $$ = NULL; }
	| '$' T_ID              { $$ = NULL; }
	;

/*** statements ***/
stmt_block:
	'{' statement '}'      { $$ = $2; }

stmt_if:
	T_IF '(' expr ')' statement           { $$ = NULL; }
	;

stmt_error:
	T_ERROR '(' T_ID ',' T_QSTR ')' ';'   { $$ = NULL; }
	;
