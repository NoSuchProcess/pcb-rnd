%{
/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2019 Tibor 'Igor2' Palinkas
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
 */

/* Query language - compiler: lexical analyzer */

#include "unit.h"
#include "query.h"
#include "query_y.h"
#include "compat_misc.h"
#include "layer.h"

static const char *pcb_qry_program, *pcb_qry_program_ptr;
static int qry_yy_input(char *buf, int buflen);
static pcb_qry_node_t *make_constant(char *str, long val);
#define YY_INPUT(buf, res, buflen) (res = qry_yy_input(buf, buflen))
%}

%option prefix="qry_"

%%
["][^"]*["]     { qry_lval.s = pcb_strdup(yytext+1); qry_lval.s[strlen(qry_lval.s)-1] = '\0'; return T_QSTR; /*"*/ }
['][^']*[']     { qry_lval.s = pcb_strdup(yytext+1); qry_lval.s[strlen(qry_lval.s)-1] = '\0'; return T_QSTR; }

let             { return T_LET; }
assert          { return T_ASSERT; }
rule            { return T_RULE; }
list            { return T_LIST; }
invalid         { return T_INVALID; }
p[.]            { return T_FLD_P; }
a[.]            { return T_FLD_A; }
flag[.]         { return T_FLD_FLAG; }

"LINE_POINT"    { qry_lval.n = make_constant(yytext, PCB_OBJ_LINE_POINT); return T_CONST; }
"POLY_POINT"    { qry_lval.n = make_constant(yytext, PCB_OBJ_POLY_POINT); return T_CONST; }
"LINE"          { qry_lval.n = make_constant(yytext, PCB_OBJ_LINE); return T_CONST; }
"TEXT"          { qry_lval.n = make_constant(yytext, PCB_OBJ_TEXT); return T_CONST; }
"POLYGON"       { qry_lval.n = make_constant(yytext, PCB_OBJ_POLY); return T_CONST; }
"ARC"           { qry_lval.n = make_constant(yytext, PCB_OBJ_ARC); return T_CONST; }
"RAT"           { qry_lval.n = make_constant(yytext, PCB_OBJ_RAT); return T_CONST; }
"PADSTACK"      { qry_lval.n = make_constant(yytext, PCB_OBJ_PSTK); return T_CONST; }
"PSTK"          { qry_lval.n = make_constant(yytext, PCB_OBJ_PSTK); return T_CONST; }
"SUBC"          { qry_lval.n = make_constant(yytext, PCB_OBJ_SUBC); return T_CONST; }
"SUBCIRCUIT"    { qry_lval.n = make_constant(yytext, PCB_OBJ_SUBC); return T_CONST; }
"NET"           { qry_lval.n = make_constant(yytext, PCB_OBJ_NET); return T_CONST; }
"LAYER"         { qry_lval.n = make_constant(yytext, PCB_OBJ_LAYER); return T_CONST; }
"LAYERGRP"      { qry_lval.n = make_constant(yytext, PCB_OBJ_LAYERGRP); return T_CONST; }

"TRUE"          { qry_lval.n = make_constant(yytext, 1); return T_CONST; }
"VISIBLE"       { qry_lval.n = make_constant(yytext, 1); return T_CONST; }
"ON"            { qry_lval.n = make_constant(yytext, 1); return T_CONST; }
"YES"           { qry_lval.n = make_constant(yytext, 1); return T_CONST; }

"FALSE"         { qry_lval.n = make_constant(yytext, 0); return T_CONST; }
"INVISIBLE"     { qry_lval.n = make_constant(yytext, 0); return T_CONST; }
"OFF"           { qry_lval.n = make_constant(yytext, 0); return T_CONST; }
"NO"            { qry_lval.n = make_constant(yytext, 0); return T_CONST; }

"TOP"           { qry_lval.n = make_constant(yytext, PCB_LYT_TOP); return T_CONST; }
"BOTTOM"        { qry_lval.n = make_constant(yytext, PCB_LYT_BOTTOM); return T_CONST; }
"INTERN"        { qry_lval.n = make_constant(yytext, PCB_LYT_INTERN); return T_CONST; }
"INTERNAL"      { qry_lval.n = make_constant(yytext, PCB_LYT_INTERN); return T_CONST; }
"COPPER"        { qry_lval.n = make_constant(yytext, PCB_LYT_COPPER); return T_CONST; }
"SILK"          { qry_lval.n = make_constant(yytext, PCB_LYT_SILK); return T_CONST; }
"MASK"          { qry_lval.n = make_constant(yytext, PCB_LYT_MASK); return T_CONST; }
"PASTE"         { qry_lval.n = make_constant(yytext, PCB_LYT_PASTE); return T_CONST; }
"MECH"          { qry_lval.n = make_constant(yytext, PCB_LYT_MECH); return T_CONST; }
"DOC"           { qry_lval.n = make_constant(yytext, PCB_LYT_DOC); return T_CONST; }
"BOUNDARY"      { qry_lval.n = make_constant(yytext, PCB_LYT_BOUNDARY); return T_CONST; }



mm              { qry_lval.u = get_unit_struct_by_allow(PCB_UNIT_ALLOW_MM); return T_UNIT; }
m               { qry_lval.u = get_unit_struct_by_allow(PCB_UNIT_ALLOW_M); return T_UNIT; }
um              { qry_lval.u = get_unit_struct_by_allow(PCB_UNIT_ALLOW_UM); return T_UNIT; }
cm              { qry_lval.u = get_unit_struct_by_allow(PCB_UNIT_ALLOW_CM); return T_UNIT; }
nm              { qry_lval.u = get_unit_struct_by_allow(PCB_UNIT_ALLOW_NM); return T_UNIT; }
mil             { qry_lval.u = get_unit_struct_by_allow(PCB_UNIT_ALLOW_MIL); return T_UNIT; }
inch            { qry_lval.u = get_unit_struct_by_allow(PCB_UNIT_ALLOW_IN); return T_UNIT; }

thus            { return T_THUS; }
[|][|]          { return T_OR; }
[&][&]          { return T_AND; }
[=][=]          { return T_EQ; }
[!][=]          { return T_NEQ; }
[>][=]          { return T_GTEQ; }
[<][=]          { return T_LTEQ; }

[0-9]+                    { qry_lval.c = strtol(yytext, NULL, 10); return T_INT; }
[.][0-9]+                 { qry_lval.d = strtod(yytext, NULL); return T_DBL; }
[0-9]+[.][0-9]*           { qry_lval.d = strtod(yytext, NULL); return T_DBL; }
[A-Za-z_][0-9A-Za-z_]*    { qry_lval.s = pcb_strdup(yytext); return T_STR; }

[@().,<>!*+/~-] { return *yytext; }

[;\r\n]         { return T_NL; }
[ \t]           { continue; }

%%

static int qry_yy_input(char *buf, int buflen)
{
	int len;
	for(len = 0; (*pcb_qry_program_ptr != '\0') && (buflen > 0); len++,buflen--) {
/*		printf("IN: '%c'\n",  *pcb_qry_program_ptr);*/
		*buf = *pcb_qry_program_ptr;
		buf++;
		pcb_qry_program_ptr++;
	}
	return len;
}

void pcb_qry_set_input(const char *script)
{
	pcb_qry_program = pcb_qry_program_ptr = script;
}

static pcb_qry_node_t *make_constant(char *str, long val)
{
	pcb_qry_node_t *res = pcb_qry_n_alloc(PCBQ_DATA_CONST);
	res->data.str = pcb_strdup(str);
	res->precomp.cnst = val;
	return res;
}
