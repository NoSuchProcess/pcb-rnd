%prefix pcb_bxl_%

%{
#include <stdio.h>
#include "bxl.h"
%}

%token T_ID T_INTEGER T_REAL T_QSTR

%%

file:
	/* empty */
	| file statement
	;

statement:
	  text_style
	| pad_stack
	;

boolean:
	  "True"
	| "False"
	;

/*** TextStyle ***/

text_style:
	"TextStyle" T_QSTR text_attrs '\n'
	;

text_attrs:
	  /* empty */
	| text_attr text_attrs
	;

text_attr:
	  '(' "FontWidth" T_INTEGER ')'
	| '(' "FontCharWidth" T_INTEGER ')'
	| '(' "FontHeight" T_INTEGER ')'
	;


/*** PadStack ***/

pad_stack:
	"PadStack" T_QSTR pstk_attrs '\n'
	"Shapes" ':' T_INT
	pad_shapes
	"EndPadStack"
	;

pstk_attrs:
	  /* empty */
	| pstk_attr pstk_attrs
	;

pstk_attr:
	  '(' "HoleDiam" T_INTEGER ')'
	| '(' "Surface" boolean ')'
	| '(' "Plated" boolean ')'
	;

pad_shapes:
	  /* empty */
	pad_shape pad_shapes
	;

pad_shape:
	"PadShape" T_QSTR pad_attrs '\n'


pad_attrs:
	  /* empty */
	| pad_attr pad_attrs
	;

pad_attr:
	  '(' "Width" T_REAL ')'
	| '(' "Height" T_REAL ')'
	| '(' "PadType" T_INT ')'
	| '(' "Layer" T_ID ')'
	;
