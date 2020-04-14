%prefix pcb_bxl_%

%struct
{
	long line, first_col, last_col;
}

%union
{
	double d;
	int i;
	char *s;
	pcb_coord_t c;
}


%{
/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  BXL IO plugin - BXL grammar
 *  pcb-rnd Copyright (C) 2020 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 PET Fund in 2020)
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
#include "bxl_gram.h"
#include "bxl.h"

TODO("Can remove this once the coord unit is converted with getvalue")
#include <librnd/core/unit.h>

#define XCRD(c) pcb_bxl_coord_x(c)
#define YCRD(c) pcb_bxl_coord_y(c)

%}

/* Generic */
%token T_ID T_INTEGER T_REAL_ONLY T_QSTR

/* Keywords for footprints */
%token T_TRUE T_FALSE T_TEXTSTYLE T_FONTWIDTH T_FONTCHARWIDTH T_FONTHEIGHT
%token T_PADSTACK T_ENDPADSTACK T_SHAPES T_PADSHAPE T_HOLEDIAM T_SURFACE
%token T_PLATED T_WIDTH T_HEIGHT T_PADTYPE T_LAYER T_PATTERN T_ENDPATTERN
%token T_DATA T_ENDDATA T_ORIGINPOINT T_PICKPOINT T_GLUEPOINT T_PAD T_NUMBER
%token T_PINNAME T_PADSTYLE T_ORIGINALPADSTYLE T_ORIGIN T_ORIGINALPINNUMBER
%token T_ROTATE T_POLY T_LINE T_ENDPOINT T_ATTRIBUTE T_ATTR T_JUSTIFY
%token T_ARC T_RADIUS T_STARTANGLE T_SWEEPANGLE T_TEXT T_ISVISIBLE T_PROPERTY
%token T_WIZARD T_VARNAME T_VARDATA T_TEMPLATEDATA T_ISFLIPPED T_NOPASTE

/* Sections that are to be ignored (non-footprint data) */
%token T_SYMBOL T_ENDSYMBOL T_COMPONENT T_ENDCOMPONENT

%type <s> T_QSTR
%type <s> T_ID
%type <i> T_INTEGER
%type <d> T_REAL_ONLY
%type <d> real
%type <c> coord
%type <i> boolean

%%

full_file:
	maybe_nl
	file
	;

file:
	/* empty */
	|statement nl file
	;

statement:
	  text_style
	| pad_stack
	| pattern
	| boring_section
	;

/*** common and misc ***/

boolean:
	  T_TRUE       { $$ = 1; }
	| T_FALSE      { $$ = 0; }
	;

nl:
	  '\n'
	| '\n' nl
	;

maybe_nl:
	  /* empty */
	| nl
	;

real:
	  T_INTEGER            { $$ = $1; }
	| T_REAL_ONLY          { $$ = $1; }
	;

coord:
	real    { $$ = PCB_MIL_TO_COORD($1); }
	;

common_attr_text:
	  T_JUSTIFY T_ID             { pcb_bxl_set_justify(ctx, $2); free($2); }
	| T_TEXTSTYLE T_QSTR         { pcb_bxl_set_text_style(ctx, $2); free($2); }
	| T_ISVISIBLE boolean        { ctx->state.is_visible = $2; }
	;

common_origin:
	T_ORIGIN coord ',' coord     { ctx->state.origin_x = XCRD($2); ctx->state.origin_y = YCRD($4); }
	;

common_layer:
	T_LAYER T_ID                 { pcb_bxl_set_layer(ctx, $2); free($2); }
	;

common_width:
	T_WIDTH coord                { ctx->state.width = $2; }
	;

/*** TextStyle ***/

text_style:
	T_TEXTSTYLE T_QSTR             { pcb_bxl_text_style_begin(ctx, $2); /* $2 is taken over */ }
		text_style_attrs             { pcb_bxl_text_style_end(ctx); }
	;

text_style_attrs:
	  /* empty */
	| '(' text_style_attr ')' text_style_attrs
	;

text_style_attr:
	  T_FONTWIDTH real             { ctx->state.text_style->width = $2; }
	| T_FONTCHARWIDTH real         { ctx->state.text_style->char_width = $2; }
	| T_FONTHEIGHT real            { ctx->state.text_style->height = $2; }
	;


/*** PadStack ***/

pad_stack:
	T_PADSTACK T_QSTR               { pcb_bxl_reset(ctx); pcb_bxl_padstack_begin(ctx, $2); /* $2 is taken over */ }
		pstk_attrs nl
		T_SHAPES ':' T_INTEGER nl     { ctx->state.num_shapes = $8; }
		pad_shapes
		T_ENDPADSTACK                 { pcb_bxl_padstack_end(ctx); }
	;

pstk_attrs:
	  /* empty */
	| '(' pstk_attr ')' pstk_attrs
	;

pstk_attr:
	  T_HOLEDIAM coord             { ctx->state.hole = $2; }
	| T_SURFACE boolean            { ctx->state.surface = $2; }
	| T_PLATED boolean             { ctx->state.plated = $2; }
	| T_NOPASTE boolean            { ctx->state.nopaste = $2; }
	;

pad_shapes:
	  /* empty */
	| pad_shape pad_shapes
	;

pad_shape:
	T_PADSHAPE T_QSTR              { pcb_bxl_padstack_begin_shape(ctx, $2); free($2); }
		padshape_attrs nl            { pcb_bxl_padstack_end_shape(ctx); }
	;

padshape_attrs:
	  /* empty */
	| '(' padshape_attr ')' padshape_attrs
	;

padshape_attr:
	  T_HEIGHT coord               { ctx->state.height = $2; }
	| T_PADTYPE T_INTEGER          { ctx->state.pad_type = $2; }
	| common_layer
	| common_width
	;

/*** Pattern ***/
pattern:
	T_PATTERN T_QSTR nl     { pcb_bxl_reset_pattern(ctx); pcb_bxl_pattern_begin(ctx, $2); free($2); }
	pattern_chldrn
	T_ENDPATTERN            { pcb_bxl_pattern_end(ctx); pcb_bxl_reset_pattern(ctx); }
	;

pattern_chldrn:
	  /* empty */
	| pattern_chld nl pattern_chldrn
	;

pattern_chld:
	  data
	| T_ORIGINPOINT '(' coord ',' coord ')'   { ctx->pat_state.origin_x = XCRD($3); ctx->pat_state.origin_y = YCRD($5); }
	| T_PICKPOINT   '(' coord ',' coord ')'   { ctx->pat_state.pick_x = XCRD($3); ctx->pat_state.pick_y = YCRD($5); }
	| T_GLUEPOINT   '(' coord ',' coord ')'   { ctx->pat_state.glue_x = XCRD($3); ctx->pat_state.glue_y = YCRD($5); }
	;

data:
	T_DATA ':' T_INTEGER nl
	data_chldrn
	T_ENDDATA
	;

data_chldrn:
	  /* empty */
	| data_chld nl data_chldrn
	;

data_chld:
	  pad
	| poly
	| line
	| attribute
	| templatedata
	| arc
	| text
	| wizard

	;

/*** Pad ***/
pad:
	T_PAD                                { pcb_bxl_pad_begin(ctx); }
		pad_attrs                          { pcb_bxl_pad_end(ctx); }
	;

pad_attrs:
	  /* empty */
	| '(' pad_attr ')' pad_attrs
	;

pad_attr:
	  T_NUMBER T_INTEGER                 { ctx->state.pin_number = $2; }
	| T_PINNAME T_QSTR                   { ctx->state.pin_name = $2; /* takes over $2 */ }
	| T_PADSTYLE T_QSTR                  { pcb_bxl_pad_set_style(ctx, $2); free($2); }
	| T_ORIGINALPADSTYLE T_QSTR          { /* what's this?! */ free($2); }
	| T_ORIGINALPINNUMBER T_INTEGER      { /* what's this?! */ }
	| T_ROTATE real                      { ctx->state.rot = $2; }
	| common_origin
	;

/*** Poly ***/
poly:
	T_POLY                { pcb_bxl_poly_begin(ctx); }
		poly_attrs          { pcb_bxl_poly_end(ctx); }
	;

poly_attrs:
	  /* empty */
	| '(' poly_attr ')' poly_attrs
	;

poly_attr:

	  T_PROPERTY T_QSTR            { pcb_bxl_add_property(ctx, (pcb_any_obj_t *)ctx->state.poly, $2); free($2); }
	| coord ',' coord              { pcb_bxl_poly_add_vertex(ctx, XCRD($1), YCRD($3)); }
	| common_origin
	| common_layer
	| common_width
	;

/*** Line ***/
line:
	T_LINE                        { pcb_bxl_reset(ctx); }
		line_attrs                  { pcb_bxl_add_line(ctx); pcb_bxl_reset(ctx); }
	;

line_attrs:
	  /* empty */
	| '(' line_attr ')' line_attrs
	;

line_attr:
	  T_ENDPOINT coord ',' coord   { ctx->state.endp_x = XCRD($2); ctx->state.endp_y = YCRD($4); }
	| common_origin
	| common_layer
	| common_width
	;

/*** Attribute ***/
attribute:
	T_ATTRIBUTE                         { pcb_bxl_reset(ctx); ctx->state.is_text = 0; }
		attribute_attrs                   { pcb_bxl_add_text(ctx); pcb_bxl_reset(ctx); }
	;

attribute_attrs:
	  /* empty */
	| '(' attribute_attr ')' attribute_attrs
	;

attribute_attr:
	  T_ATTR T_QSTR T_QSTR      { pcb_bxl_set_attr_val(ctx, $2, $3); /* $2 and $3 are taken over */ }
	| common_attr_text
	| common_origin
	| common_layer
	;


/*** Arc ***/
arc:
	T_ARC                   { pcb_bxl_reset(ctx); }
		arc_attrs             { pcb_bxl_add_arc(ctx); pcb_bxl_reset(ctx); }
	;

arc_attrs:
	  /* empty */
	| '(' arc_attr ')' arc_attrs
	;

arc_attr:
	  T_RADIUS coord        { ctx->state.radius = $2;  }
	| T_STARTANGLE real     { ctx->state.arc_start = $2; }
	| T_SWEEPANGLE real     { ctx->state.arc_delta = $2; }
	| common_origin
	| common_layer
	| common_width
	;

/*** Text ***/
text:
	T_TEXT                        { pcb_bxl_reset(ctx); ctx->state.is_text = 1; }
		text_attrs                  { pcb_bxl_add_text(ctx); pcb_bxl_reset(ctx); }
	;

text_attrs:
	  /* empty */
	| '(' text_attr ')' text_attrs
	;

text_attr:
	  T_TEXT T_QSTR               { pcb_bxl_set_text_str(ctx, $2); /* $2 is taken over */ }
	| T_ISFLIPPED boolean         { ctx->state.flipped = $2; }
	| T_ROTATE real               { ctx->state.rot = $2; }
	| common_attr_text
	| common_origin
	| common_layer
	;

/*** Wizard & template ***/
wizard:
	T_WIZARD wizard_attrs
	;

templatedata:
	T_TEMPLATEDATA wizard_attrs
	;

wizard_attrs:
	  /* empty */
	| '(' wizard_attr ')' wizard_attrs
	;

wizard_attr:
	  T_VARNAME T_QSTR      { free($2); }
	| T_VARDATA T_QSTR      { free($2); }
	| common_origin
	;

/*** Sections not interesting for pcb-rnd ***/
boring_section:
	  { ctx->in_error = 1; } T_SYMBOL     error  T_ENDSYMBOL    { ctx->in_error = 0; }
	| { ctx->in_error = 1; } T_COMPONENT  error  T_ENDCOMPONENT { ctx->in_error = 0; }
	;
