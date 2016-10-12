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

/* Query language - compiler: lexical analyzer */

#include "global.h"
#include "unit.h"
#include "query.h"
#include "query_y.h"
#include "compat_misc.h"

static const char *pcb_qry_program, *pcb_qry_program_ptr;
static int qry_yy_input(char *buf, int buflen);
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

mm              { qry_lval.u = get_unit_struct_by_allow(ALLOW_MM); return T_UNIT; }
m               { qry_lval.u = get_unit_struct_by_allow(ALLOW_M); return T_UNIT; }
um              { qry_lval.u = get_unit_struct_by_allow(ALLOW_UM); return T_UNIT; }
cm              { qry_lval.u = get_unit_struct_by_allow(ALLOW_CM); return T_UNIT; }
nm              { qry_lval.u = get_unit_struct_by_allow(ALLOW_NM); return T_UNIT; }
mil             { qry_lval.u = get_unit_struct_by_allow(ALLOW_MIL); return T_UNIT; }
inch            { qry_lval.u = get_unit_struct_by_allow(ALLOW_IN); return T_UNIT; }

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

[@().,<>!*+/-]   { return *yytext; }

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
