%{
#include "global.h"
#include "unit.h"
#include "query_y.h"
%}

%option prefix="qry_"

%%
let             { return T_LET; }
assert          { return T_ASSERT; }
rule            { return T_RULE; }

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
[0-9]*.[0-9]+             { qry_lval.d = strtod(yytext, NULL); return T_DBL; }
[0-9]+.[0-9]*             { qry_lval.d = strtod(yytext, NULL); return T_DBL; }
[A-Za-z_][0-9A-Za-z_]*    { qry_lval.s = pcb_strdup(yytext); return T_STR; }

[.(),<>!*+/-]   { return *yytext; }

[;\r\n]         { return T_NL; }
[ \t]           { continue; }
