%prefix pcb_bxl_%

%struct
{
	int line, first_col, last_col;
}

%union
{
	double d;
	int i;
	char *s;
}


%{
#include <stdio.h>
#include "bxl_gram.h"
#include "bxl.h"
%}

/* Generic */
%token T_ID T_INTEGER T_REAL_ONLY T_QSTR

/* Keyword */
%token T_TRUE T_FALSE T_TEXTSTYLE T_FONTWIDTH T_FONTCHARWIDTH T_FONTHEIGHT
%token T_PADSTACK T_ENDPADSTACK T_SHAPES T_PADSHAPE T_HOLEDIAM T_SURFACE
%token T_PLATED T_WIDTH T_HEIGHT T_PADTYPE T_LAYER T_PATTERN T_ENDPATTERN
%token T_DATA T_ENDDATA T_ORIGINPOINT T_PICKPOINT T_GLUEPOINT T_PAD T_NUMBER
%token T_PINNAME T_PADSTYLE T_ORIGINALPADSTYLE T_ORIGIN T_ORIGINALPINNUMBER
%token T_ROTATE



%%

file:
	/* empty */
	| file statement
	;

statement:
	  text_style
	| pad_stack
	| pattern
	;

/*** common and misc ***/

boolean:
	  T_TRUE
	| T_FALSE
	;

nl:
	  '\n'
	| '\n' nl
	;

real:
	  T_INTEGER
	| T_REAL_ONLY
	;

coord:
	real
	;

/*** TextStyle ***/

text_style:
	T_TEXTSTYLE T_QSTR text_attrs nl
	;

text_attrs:
	  /* empty */
	| '(' text_attr ')' text_attrs
	;

text_attr:
	  T_FONTWIDTH T_INTEGER
	| T_FONTCHARWIDTH T_INTEGER
	| T_FONTHEIGHT T_INTEGER
	;


/*** PadStack ***/

pad_stack:
	T_PADSTACK T_QSTR pstk_attrs nl
	T_SHAPES ':' T_INTEGER nl
	pad_shapes
	T_ENDPADSTACK nl
	;

pstk_attrs:
	  /* empty */
	| '(' pstk_attr ')' pstk_attrs
	;

pstk_attr:
	  T_HOLEDIAM T_INTEGER
	| T_SURFACE boolean
	| T_PLATED boolean
	;

pad_shapes:
	  /* empty */
	| pad_shape pad_shapes
	;

pad_shape:
	T_PADSHAPE T_QSTR pad_attrs nl
	;

pad_attrs:
	  /* empty */
	| '(' pad_attr ')' pad_attrs
	;

pad_attr:
	  T_WIDTH real
	| T_HEIGHT real
	| T_PADTYPE T_INTEGER
	| T_LAYER T_ID
	;

/*** Pattern ***/
pattern:
	T_PATTERN T_QSTR nl
	pattern_chldrn
	T_ENDPATTERN nl
	;

pattern_chldrn:
	  /* empty */
	| pattern_chld nl pattern_chldrn
	;

pattern_chld:
	  data
	| T_ORIGINPOINT '(' coord ',' coord ')'
	| T_PICKPOINT   '(' coord ',' coord ')'
	| T_GLUEPOINT   '(' coord ',' coord ')'
	;

data:
	T_DATA ':' T_INTEGER nl
	data_chldrn
	T_ENDDATA nl
	;

data_chldrn:
	  /* empty */
	| data_chld nl data_chldrn
	;

data_chld:
	  pad
	;

/*** pad ***/
pad:
	T_PAD pad_attrs
	;

pad_attrs:
	  /* empty */
	| '(' pad_attr ')' pad_attrs
	;

pad_attr:
	  T_NUMBER T_INTEGER
	| T_PINNAME T_QSTR
	| T_PADSTYLE T_QSTR
	| T_ORIGINALPADSTYLE T_QSTR
	| T_ORIGIN coord ',' coord
	| T_ORIGINALPINNUMBER T_INTEGER
	| T_ROTATE real
	;
