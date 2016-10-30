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

/* Specify default output formatting style to be more compact than 
   the canonical lihata export style */

#include "write_style.h"

#define PB_BEGIN     {"*", 2, 2}
#define PB_BEGINNL   {"\n *", 4, 4}
#define PB_EMPTY     {"", 1, 1}
#define PB_SEMICOLON {";", 2, 2}
#define PB_SPACE     {" ", 2, 2}
#define PB_LBRACE    {"{", 2, 2}
#define PB_LBRACENL  {"{\n", 3, 3}
#define PB_LBRACENLI {"{\n*", 4, 4}
#define PB_RBRACE    {"}", 2, 2}
#define PB_RBRACENL  {"}\n", 3, 3}
#define PB_RBRACENLI {"}\n*", 4, 4}
#define PB_RBRACESC  {"};", 3, 3}
#define PB_NEWLINE   {"\n", 2, 2}
#define PB_DEFAULT   {NULL, 0, 0}

/* Space spearated key=val; pairs */
static lht_perstyle_t style_inline = {
	/* buff */        {PB_SPACE, PB_EMPTY, PB_EMPTY, PB_EMPTY, PB_EMPTY, PB_SEMICOLON},
	/* has_eq */      1,
	/* val_brace */   0,
	/* etype */       0,
	/* ename */       1,
	/* name_braced */ 0
};

/* tightly packed key=val; pairs */
static lht_perstyle_t style_inline_tight = {
	/* buff */        {PB_EMPTY, PB_EMPTY, PB_EMPTY, PB_EMPTY, PB_EMPTY, PB_SEMICOLON},
	/* has_eq */      1,
	/* val_brace */   0,
	/* etype */       0,
	/* ename */       1,
	/* name_braced */ 0
};

static lht_perstyle_t style_newline = {
	/* buff */        {PB_BEGIN, PB_EMPTY, PB_EMPTY, PB_EMPTY, PB_EMPTY, PB_NEWLINE},
	/* has_eq */      1,
	/* val_brace */   0,
	/* etype */       0,
	/* ename */       1,
	/* name_braced */ 0
};

static lht_perstyle_t style_istruct = {
	/* buff */        {PB_SPACE, PB_SPACE, PB_LBRACE, PB_EMPTY, PB_EMPTY, PB_RBRACESC},
	/* has_eq */      1,
	/* val_brace */   1,
	/* etype */       0,
	/* ename */       1,
	/* name_braced */ 0
};

static lht_perstyle_t style_struct = {
	/* buff */        {PB_BEGIN, PB_SPACE, PB_LBRACENL, PB_EMPTY, PB_EMPTY, PB_RBRACENL},
	/* has_eq */      0,
	/* val_brace */   0,
	/* etype */       0,
	/* ename */       1,
	/* name_braced */ 0
};

static lht_perstyle_t style_structi = {
	/* buff */        {PB_BEGIN, PB_SPACE, PB_LBRACENLI, PB_EMPTY, PB_EMPTY, PB_RBRACENL},
	/* has_eq */      0,
	/* val_brace */   0,
	/* etype */       0,
	/* ename */       1,
	/* name_braced */ 0
};

static lht_perstyle_t style_struct_therm = {
	/* buff */        {PB_BEGIN, PB_SPACE, PB_LBRACE, PB_EMPTY, PB_EMPTY, PB_RBRACENLI},
	/* has_eq */      0,
	/* val_brace */   0,
	/* etype */       0,
	/* ename */       1,
	/* name_braced */ 0
};

static lht_perstyle_t style_nlstruct = {
	/* buff */        {PB_BEGINNL, PB_SPACE, PB_LBRACENL, PB_EMPTY, PB_EMPTY, PB_RBRACENL},
	/* has_eq */      0,
	/* val_brace */   0,
	/* etype */       0,
	/* ename */       1,
	/* name_braced */ 0
};

static const char *pat_te_flags[]  = {"te:*", "ha:flags", "*", NULL};
static const char *pat_te_attr[]   = {"te:*", "ha:attributes", "*", NULL};
static lhtpers_rule_t r_ilists[]   = {
	{pat_te_flags,  &style_inline_tight, NULL},
	{pat_te_attr,   &style_newline, NULL},
	{NULL, NULL, NULL}
};

static const char *pat_thermal[] = {"ha:thermal", "ha:flags", "*", NULL};
static lhtpers_rule_t r_thermal[]   = {
	{pat_thermal,  &style_struct_therm, NULL},
	{NULL, NULL, NULL}
};

static const char *pat_ha_flags[] = {"ha:flags", "*", NULL};
static lhtpers_rule_t r_flags[]   = {
	{pat_ha_flags,  &style_istruct, r_thermal},
	{NULL, NULL, NULL}
};

static const char *pat_x1_line[]    = {"te:x1", "*", NULL};
static const char *pat_y1_line[]    = {"te:y1", "*", NULL};
static const char *pat_x2_line[]    = {"te:x2", "*", NULL};
static const char *pat_y2_line[]    = {"te:y2", "*", NULL};
static const char *pat_thickness[]  = {"te:thickness", "*", NULL};
static const char *pat_clearance[]  = {"te:clearance", "*", NULL};
static const char *pat_flags[]      = {"ha:flags", "*", NULL};
static const char *pat_attributes[] = {"ha:attributes", "*", NULL};
static lhtpers_rule_t r_line[] = {
	{pat_x1_line,    &style_inline, NULL},
	{pat_y1_line,    &style_inline, NULL},
	{pat_x2_line,    &style_inline, NULL},
	{pat_y2_line,    &style_inline, NULL},
	{pat_thickness,  &style_inline, NULL},
	{pat_clearance,  &style_inline, NULL},
	{pat_flags,      &style_nlstruct, r_thermal},
	{pat_attributes, &style_nlstruct, NULL},
	{NULL, NULL, NULL}
};

static const char *pat_x[]             = {"te:x", "*", NULL};
static const char *pat_y[]             = {"te:y", "*", NULL};
static const char *pat_width_arc[]     = {"te:width", "*", NULL};
static const char *pat_height_arc[]    = {"te:height", "*", NULL};
static const char *pat_astart_arc[]    = {"te:astart", "*", NULL};
static const char *pat_adelta_arc[]    = {"te:adelta", "*", NULL};
static lhtpers_rule_t r_arc[] = {
	{pat_x,          &style_inline, NULL},
	{pat_y,          &style_inline, NULL},
	{pat_width_arc,  &style_inline, NULL},
	{pat_height_arc, &style_inline, NULL},
	{pat_astart_arc, &style_inline, NULL},
	{pat_adelta_arc, &style_inline, NULL},
	{pat_thickness,  &style_inline, NULL},
	{pat_clearance,  &style_inline, NULL},
	{pat_flags,      &style_nlstruct, r_thermal},
	{pat_attributes, &style_nlstruct, NULL},
	{NULL, NULL, NULL}
};

static const char *pat_name[] = {"te:name", "*", NULL};
static const char *pat_numb[] = {"te:number", "*", NULL};
static const char *pat_hole[] = {"te:hole", "*", NULL};
static const char *pat_mask[] = {"te:mask", "*", NULL};
static lhtpers_rule_t r_pinvia[] = {
	{pat_name,       &style_inline, NULL},
	{pat_numb,       &style_inline, NULL},
	{pat_x,          &style_inline, NULL},
	{pat_y,          &style_inline, NULL},
	{pat_hole,       &style_inline, NULL},
	{pat_mask,       &style_inline, NULL},
	{pat_thickness,  &style_inline, NULL},
	{pat_clearance,  &style_inline, NULL},
	{pat_flags,      &style_nlstruct, r_thermal},
	{pat_attributes, &style_nlstruct, NULL},
	{NULL, NULL, NULL}
};

static lhtpers_rule_t r_pad[] = {
	{pat_name,       &style_inline, NULL},
	{pat_numb,       &style_inline, NULL},
	{pat_x1_line,    &style_inline, NULL},
	{pat_y1_line,    &style_inline, NULL},
	{pat_x2_line,    &style_inline, NULL},
	{pat_y2_line,    &style_inline, NULL},
	{pat_mask,       &style_inline, NULL},
	{pat_thickness,  &style_inline, NULL},
	{pat_clearance,  &style_inline, NULL},
	{pat_flags,      &style_nlstruct, r_thermal},
	{pat_attributes, &style_nlstruct, NULL},
	{NULL, NULL, NULL}
};

static const char *pat_geometry[] = {"te:geometry", "*", NULL};
static lhtpers_rule_t r_polygon[] = {
	{pat_geometry,   &style_nlstruct, NULL},
	{pat_flags,      &style_nlstruct, r_thermal},
	{pat_attributes, &style_nlstruct, NULL},
	{NULL, NULL, NULL}
};

static const char *pat_objects[] = {"li:objects", "*", NULL};
static lhtpers_rule_t r_element[] = {
	{pat_x,          &style_inline, NULL},
	{pat_y,          &style_inline, NULL},
	{pat_flags,      &style_nlstruct, r_thermal},
	{pat_attributes, &style_nlstruct, NULL},
	{pat_objects,    &style_nlstruct, NULL},
	{NULL, NULL, NULL}
};


static const char *pat_line[] = {"ha:line.*", "*", NULL};
static const char *pat_arc[]  = {"ha:arc.*", "*", NULL};
static const char *pat_via[]  = {"ha:via.*", "*", NULL};
static const char *pat_pin[]  = {"ha:pin.*", "*", NULL};
static const char *pat_pad[]  = {"ha:pad.*", "*", NULL};
static const char *pat_poly[] = {"ha:polygon.*", "*", NULL};
static const char *pat_elem[] = {"ha:element.*", "*", NULL};
static const char *pat_objs[] = {"li:objects", "*", NULL};
static lhtpers_rule_t r_istructs[] = {
	{pat_line,    &style_structi, r_line, NULL},
	{pat_arc,     &style_structi, r_arc, NULL},
	{pat_via,     &style_structi, r_pinvia, NULL},
	{pat_pin,     &style_structi, r_pinvia, NULL},
	{pat_pad,     &style_structi, r_pad, NULL},
	{pat_poly,    &style_structi, r_polygon, NULL},
	{pat_elem,    &style_structi, r_element, NULL},
	{pat_objs,    &style_structi, NULL, NULL},
	{NULL, NULL, NULL}
};

lhtpers_rule_t *io_lihata_out_rules[] = {
	r_istructs, r_ilists, r_ilists, r_flags, NULL
};
