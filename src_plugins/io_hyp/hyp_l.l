
/*
 * read hyperlynx files
 * Copyright 2012, 2016 Koen De Vleeschauwer.
 *
 * This file is part of pcb-rnd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

%option noyywrap nodefault yylineno debug

/* avoid collision with other scanners */
%option prefix="hyy"

%{
#include <stdlib.h>

char *pcb_strdup(const char *s); /* from #include "compat_misc.h" */

#include "hyp_y.h"

/* copy a string between quotes */
char *strunquote(const char *);

/* remember hyperlynx file section we're in */
int section = -1;

%}

 /*
  * The scanner knows five states: INITIAL, STATE_STRING, STATE_POSINT, STATE_FLOAT and STATE_COMMENT
  *
  * In state INITIAL the scanner recognizes:
  * - punctuation (such as {}()=, )
  * - keywords (such as VERSION, BOARD, NET), 
  * - strings (both between double quotes " and unquoted),
  * - and floating point numbers (with optional suffix, eg. 10 uF), 
  *
  * In STATE_STRING the scanner recognizes punctuation and strings only.
  * This avoids unquoted strings being interpreted as numbers, or keywords.
  *
  * In STATE_POSINT the scanner recognizes a positive integer.
  *
  * In STATE_FLOAT the scanner recognizes a floating point number (without suffix).
  *
  * In STATE_COMMENT the scanner discards all text up to the next 
  * right brace } , and then continues in state INITIAL.
  *
  */

%x STATE_STRING STATE_POSINT STATE_FLOAT STATE_COMMENT STATE_COMMENT_EOL

 /* whitespace: space, tab, vertical tab, form feed */
WS                  [ \t\v\f]

 /* accept windows and unix newlines */
NEWLINE             [\r\n]+

 /* 
  * lines with an asterisk in the first column are comments 
  */
COMMENT             ^\*[^\n\r]*[\n\r]+

 /* Left-hand side of an assignment: check whether next token is equals sign */
LHS                 [ \t\v\f]*"="

 /*
  * Positive integer
  */

POSINT              [0-9]+

 /*
  * Floating point numbers
  */

 /* ordinary floating point numbers */
SIMPLE_FLOAT        [-+]?([0-9]+"."[0-9]*|"."?[0-9]+)([Ee][-+]?[0-9]+)?

 /* floating point numbers with suffix, e,g. pF, nH */
FLOAT_SUFFIX        [A-Za-z]*{WS}+
FLOAT_YOTTA         {SIMPLE_FLOAT}{WS}*"Y"{FLOAT_SUFFIX}
FLOAT_ZETA          {SIMPLE_FLOAT}{WS}*"Z"{FLOAT_SUFFIX}
FLOAT_EXA           {SIMPLE_FLOAT}{WS}*"E"{FLOAT_SUFFIX}
FLOAT_PETA          {SIMPLE_FLOAT}{WS}*"P"{FLOAT_SUFFIX}
FLOAT_TERA          {SIMPLE_FLOAT}{WS}*"T"{FLOAT_SUFFIX}
FLOAT_GIGA          {SIMPLE_FLOAT}{WS}*"G"{FLOAT_SUFFIX}
FLOAT_MEGA          {SIMPLE_FLOAT}{WS}*"M"{FLOAT_SUFFIX}
FLOAT_KILO          {SIMPLE_FLOAT}{WS}*[Kk]{FLOAT_SUFFIX}
FLOAT_MILLI         {SIMPLE_FLOAT}{WS}*"m"{FLOAT_SUFFIX}
FLOAT_MICRO         {SIMPLE_FLOAT}{WS}*[uU]{FLOAT_SUFFIX}
FLOAT_NANO          {SIMPLE_FLOAT}{WS}*[nN]{FLOAT_SUFFIX}
FLOAT_PICO          {SIMPLE_FLOAT}{WS}*[pP]{FLOAT_SUFFIX}
FLOAT_FEMTO         {SIMPLE_FLOAT}{WS}*[fF]{FLOAT_SUFFIX}
FLOAT_ATTO          {SIMPLE_FLOAT}{WS}*"a"{FLOAT_SUFFIX}
FLOAT_ZEPTO         {SIMPLE_FLOAT}{WS}*"z"{FLOAT_SUFFIX}
FLOAT_YOCTO         {SIMPLE_FLOAT}{WS}*"y"{FLOAT_SUFFIX}
  
 /* 
  * Strings
  */

 /* an unquoted string */
STRING              [^ \t\v\f\r\n\{\}\(\)=\"]+

 /* a string enclosed in double quotes " */
QUOTED_STRING       \"([^\"\n]|\"\")*\"

%%

 /* When in STATE_COMMENT skip all comment until next right brace */
<STATE_COMMENT>{
[^\}]*              { BEGIN INITIAL; /* skip all comment until next right brace */ }
}

 /* When in STATE_COMMENT_EOL skip all comment until end-of-line */
<STATE_COMMENT_EOL>{
[^\r\n]*{NEWLINE}+ { BEGIN INITIAL; /* skip all comment until next end-of-line */ }
}


 /* skip comments and whitespace */
<*>{

  {COMMENT}           { /* skip comments */ }

  {WS}+               { /* skip whitespace */ }

  {NEWLINE}+          { /* skip newlines */ }

}

 /*
  * Hyperlynx keywords 
  */

 /* Sections */

"BOARD_FILE"        {section = H_BOARD_FILE; return H_BOARD_FILE;}
"VERSION"           {section = H_VERSION; return H_VERSION;}
"DATA_MODE"         {section = H_DATA_MODE; return H_DATA_MODE;}
"UNITS"             {section = H_UNITS; return H_UNITS;}
"PLANE_SEP"         {section = H_PLANE_SEP; return H_PLANE_SEP;}
"BOARD"             {section = H_BOARD; BEGIN STATE_COMMENT_EOL; return H_BOARD;}
"STACKUP"           {section = H_STACKUP; BEGIN STATE_COMMENT_EOL; return H_STACKUP;}
"DEVICES"           {section = H_DEVICES; BEGIN STATE_COMMENT_EOL; return H_DEVICES;}
"SUPPLIES"          {section = H_SUPPLIES; BEGIN STATE_COMMENT_EOL; return H_SUPPLIES;}
"PADSTACK"          {section = H_PADSTACK; BEGIN STATE_STRING; return H_PADSTACK;}
"NET"               {section = H_NET; BEGIN STATE_STRING; return H_NET;}
"NET_CLASS"         {section = H_NET_CLASS; return H_NET_CLASS;}
"END"               {section = H_END; return H_END;}
"KEY"               {section = H_KEY; return H_KEY;}

 /* Keywords */

"A"                 {return H_A;}
"ARC"               {return H_ARC;}
"COPPER"            {return H_COPPER;}
"CURVE"             {return H_CURVE;}
"DETAILED"          {if (section == H_DATA_MODE) BEGIN STATE_COMMENT; return H_DETAILED;}
"DIELECTRIC"        {return H_DIELECTRIC;}
"ENGLISH"           {return H_ENGLISH;}
"LENGTH"            {if (section == H_UNITS) BEGIN STATE_COMMENT; return H_LENGTH;}
"LINE"              {return H_LINE;}
"METRIC"            {return H_METRIC;}
"M"                 {return H_M;}
"N"                 {return H_N;}
"OPTIONS"           {return H_OPTIONS;}
"PAD"               {return H_PAD;}
"PERIMETER_ARC"     {return H_PERIMETER_ARC;}
"PERIMETER_SEGMENT" {return H_PERIMETER_SEGMENT;}
"PIN"               {return H_PIN;}
"PLANE"             {return H_PLANE;}
"POLYGON"           {return H_POLYGON;}
"POLYLINE"          {return H_POLYLINE;}
"POLYVOID"          {return H_POLYVOID;}
"POUR"              {return H_POUR;}
"S"                 {return H_S;}
"T"                 {return H_T;}
"SEG"               {return H_SEG;}
"SIGNAL"            {return H_SIGNAL;}
"SIMPLIFIED"        {if (section == H_DATA_MODE) BEGIN STATE_COMMENT; return H_SIMPLIFIED; }
"SIM_BOTH"          {return H_SIM_BOTH;}
"SIM_IN"            {return H_SIM_IN;}
"SIM_OUT"           {return H_SIM_OUT;}
"USEG"              {return H_USEG;}
"VIA"               {return H_VIA;}
"WEIGHT"            {if (section == H_UNITS) BEGIN STATE_COMMENT; return H_WEIGHT;}

 /* Assignments */

"A"/{LHS}           {return H_A;}
"A1"/{LHS}          {return H_A1;}
"A2"/{LHS}          {return H_A2;}
"BR"/{LHS}          {return H_BR;}
"C"/{LHS}           {return H_C;}
"C?"/{LHS}          {return H_C_QM;}
"CO?"/{LHS}         {return H_CO_QM;}
"D"/{LHS}           {return H_D;}
"ER"/{LHS}          {return H_ER;}
"F"/{LHS}           {return H_F;}
"ID"/{LHS}          {BEGIN STATE_POSINT; return H_ID;}
"L"/{LHS}           {BEGIN STATE_STRING; return H_L;}
"L1"/{LHS}          {BEGIN STATE_STRING; return H_L1;}
"L2"/{LHS}          {BEGIN STATE_STRING; return H_L2;}
"LPS"/{LHS}         {return H_LPS;}
"LT"/{LHS}          {return H_LT;}
"M"/{LHS}           {BEGIN STATE_STRING; return H_M;}
"N"/{LHS}           {BEGIN STATE_STRING; return H_N;}
"NAME"/{LHS}        {BEGIN STATE_STRING; return H_NAME;}
 /* P is used as "plating thickness" in "stackup/signal" and as "padstack" in "net/via" */
"P"/{LHS}           {if (section == H_NET) BEGIN STATE_STRING; return H_P;}
"PKG"/{LHS}         {BEGIN STATE_STRING; return H_PKG;}
"PR?"/{LHS}         {return H_PR_QM;}
"PS"/{LHS}          {return H_PS;}
"R"/{LHS}           {return H_R;}
"REF"/{LHS}         {BEGIN STATE_STRING; return H_REF;}
"S"/{LHS}           {BEGIN STATE_STRING; return H_S;}
"SX"/{LHS}          {return H_SX;}
"SY"/{LHS}          {return H_SY;}
"S1"/{LHS}          {BEGIN STATE_STRING; return H_S1;}
"S1X"/{LHS}         {return H_S1X;}
"S1Y"/{LHS}         {return H_S1Y;}
"S2"/{LHS}          {BEGIN STATE_STRING; return H_S2;}
"S2X"/{LHS}         {return H_S2X;}
"S2Y"/{LHS}         {return H_S2Y;}
"T"/{LHS}           {return H_T;}
"TC"/{LHS}          {return H_TC;}
"USE_DIE_FOR_METAL"/{LHS}           {return H_USE_DIE_FOR_METAL;}
"V"/{LHS}           {BEGIN STATE_STRING; return H_V;}
"V?"/{LHS}          {return H_V_QM;}
"VAL"/{LHS}         {return H_VAL;}
"W"/{LHS}           {return H_W;}
"X"/{LHS}           {return H_X;}
"X1"/{LHS}          {return H_X1;}
"X2"/{LHS}          {return H_X2;}
"XC"/{LHS}          {return H_XC;}
"Y"/{LHS}           {return H_Y;}
"Y1"/{LHS}          {return H_Y1;}
"Y2"/{LHS}          {return H_Y2;}
"YC"/{LHS}          {return H_YC;}
"Z"/{LHS}           {return H_Z;}
"ZL"/{LHS}          {return H_ZL;}
"ZLEN"/{LHS}        {return H_ZLEN;}
"ZW"/{LHS}          {return H_ZW;}
 
 /* Booleans */

"YES"|"yes"         {hyylval.boolval = 1; return H_BOOL; }
"NO"|"no"           {hyylval.boolval = 0; return H_BOOL; }

 /* Floats */

                    /* ordinary floating point numbers */
{SIMPLE_FLOAT}      {hyylval.floatval = strtod(hyytext, NULL); return H_FLOAT;}

                    /* floating point numbers with suffix, e,g. pF, nH */
{FLOAT_YOTTA}       {hyylval.floatval = strtod(hyytext, NULL) * 1e24; return H_FLOAT;}
{FLOAT_ZETA}        {hyylval.floatval = strtod(hyytext, NULL) * 1e21; return H_FLOAT;}
{FLOAT_EXA}         {hyylval.floatval = strtod(hyytext, NULL) * 1e18; return H_FLOAT;}
{FLOAT_PETA}        {hyylval.floatval = strtod(hyytext, NULL) * 1e15; return H_FLOAT;}
{FLOAT_TERA}        {hyylval.floatval = strtod(hyytext, NULL) * 1e12; return H_FLOAT;}
{FLOAT_GIGA}        {hyylval.floatval = strtod(hyytext, NULL) * 1e9; return H_FLOAT;}
{FLOAT_MEGA}        {hyylval.floatval = strtod(hyytext, NULL) * 1e6; return H_FLOAT;}
{FLOAT_KILO}        {hyylval.floatval = strtod(hyytext, NULL) * 1e3; return H_FLOAT;}
{FLOAT_MILLI}       {hyylval.floatval = strtod(hyytext, NULL) * 1e-3; return H_FLOAT;}
{FLOAT_MICRO}       {hyylval.floatval = strtod(hyytext, NULL) * 1e-6; return H_FLOAT;}
{FLOAT_NANO}        {hyylval.floatval = strtod(hyytext, NULL) * 1e-9; return H_FLOAT;}
{FLOAT_PICO}        {hyylval.floatval = strtod(hyytext, NULL) * 1e-12; return H_FLOAT;}
{FLOAT_FEMTO}       {hyylval.floatval = strtod(hyytext, NULL) * 1e-15; return H_FLOAT;}
{FLOAT_ATTO}        {hyylval.floatval = strtod(hyytext, NULL) * 1e-18; return H_FLOAT;}
{FLOAT_ZEPTO}       {hyylval.floatval = strtod(hyytext, NULL) * 1e-21; return H_FLOAT;}
{FLOAT_YOCTO}       {hyylval.floatval = strtod(hyytext, NULL) * 1e-24; return H_FLOAT;}

                    /* floating point numbers in VERSION and PLANE_SEP have no suffix and are followed by optional comments */
<STATE_FLOAT>{
{SIMPLE_FLOAT}      {hyylval.floatval = strtod(hyytext, NULL); BEGIN STATE_COMMENT; return H_FLOAT;}
}

                    /* A positive integer is used only in polygon/polyline/polyvoid "ID = posint" */
<STATE_POSINT>{
{POSINT}            { BEGIN INITIAL; hyylval.intval = atoi(hyytext); return H_POSINT; }
}

 
                    /* 
                     * This is a workaround for syntactically incorrect .hyp files.
                     * We accept the following constructs as representing an empty string:
                     * NAME= L1=somelayer
                     * NAME= )
                     * NAME= }
                     */
<STATE_STRING>{
([A-Z][A-Z1-2_]*{WS}*"="|")"|"}")  { yyless(0); BEGIN INITIAL; hyylval.strval = pcb_strdup(""); return H_STRING; } /* emit empty string and reprocess */
}

<*>{

  "{"                 {return '{';}

  "}"                 {BEGIN STATE_COMMENT_EOL; return '}';}

  "("                 {if (section == H_PADSTACK) BEGIN STATE_STRING; return '(';}

                      /* allow for comment after the closing bracket ) */
  ")"                 {BEGIN STATE_COMMENT_EOL; return ')';}

  ","                 {return ',';}

  "="                 {if ((section == H_VERSION) || (section == H_PLANE_SEP)) BEGIN STATE_FLOAT; return '=';}

                      /* string */
  {STRING}            { 
                        /*
                         * Commas are not allowed in strings in the padstack section
                         * unless the string is enclosed in double quotes (").
                         */

                        if ((section == H_PADSTACK) && strchr(hyytext, ','))
                          REJECT 
                        else 
                        { 
                          BEGIN INITIAL; 
                          hyylval.strval = pcb_strdup(hyytext); 
                          return H_STRING;
                        }
                      }

                      /* string in double quotes */
  {QUOTED_STRING}     {BEGIN INITIAL; hyylval.strval = strunquote(hyytext); return H_STRING;}

  <<EOF>>             {yyterminate();}

                      /* have bison catch all unrecognized characters with parse errors */
  .                   {return hyytext[0];}

}

%%

 /* 
  * copy a quoted string.
  * e.g. "data 0" -> data 0
  * a double quote inside the string can be escaped by writing two consecutive double quotes
  * e.g. "net ""hi""" -> net "hi"
  */

 char *strunquote(const char *src) 
 {
   char* dst;
   size_t len = strlen(src) + 1;
   dst = (char *)malloc(len);
   if (dst != NULL) 
   {
     char* p = (char *)src + 1; /* first char after initial quote */
     char* q = dst;
     do 
       if (*p == '"') p++;
     while ((*q++ = *p++) != '\0');
   }
   return dst;
 }

 /* not truncated */
