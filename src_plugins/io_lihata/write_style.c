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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Specify default output formatting style to be more compact than 
   the canonical lihata export style */

#include "config.h"
#include "write_style.h"

#define PB_BEGIN     {"*", 2, 2}
#define PB_BEGINSP   {" *", 3, 3}
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

static lht_perstyle_t style_inlinesp = {
	/* buff */        {PB_BEGINSP, PB_EMPTY, PB_EMPTY, PB_EMPTY, PB_EMPTY, PB_SEMICOLON},
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

static lht_perstyle_t style_newline_sp = {
	/* buff */        {PB_BEGINSP, PB_EMPTY, PB_EMPTY, PB_EMPTY, PB_EMPTY, PB_NEWLINE},
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

static lht_perstyle_t style_struct_thermal = {
	/* buff */        {PB_BEGIN, PB_SPACE, PB_LBRACE, PB_EMPTY, PB_EMPTY, PB_RBRACENL},
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

static lht_perstyle_t style_structs = {
	/* buff */        {PB_BEGIN, PB_SPACE, PB_LBRACE, PB_EMPTY, PB_EMPTY, PB_RBRACENL},
	/* has_eq */      0,
	/* val_brace */   0,
	/* etype */       0,
	/* ename */       1,
	/* name_braced */ 0
};

static lht_perstyle_t style_structsp = {
	/* buff */        {PB_BEGINSP, PB_SPACE, PB_LBRACE, PB_EMPTY, PB_EMPTY, PB_RBRACENL},
	/* has_eq */      0,
	/* val_brace */   0,
	/* etype */       0,
	/* ename */       1,
	/* name_braced */ 0
};

static lht_perstyle_t style_structnp = {
	/* buff */        {PB_BEGINSP, PB_SPACE, PB_LBRACENL, PB_EMPTY, PB_EMPTY, PB_RBRACENL},
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

static lht_perstyle_t early_nl = {
	/* buff */        {PB_NEWLINE, PB_EMPTY, PB_EMPTY, PB_EMPTY, PB_EMPTY, PB_EMPTY},
};


static const char *pat_te_flags[]      = {"te:*", "ha:flags", "*", NULL};
static const char *pat_te_attr[]       = {"te:*", "ha:attributes", "*", NULL};
static lhtpers_rule_t r_ilists[]   = {
	{pat_te_flags,  &style_inline_tight, NULL},
	{pat_te_attr,   &style_newline_sp, NULL},
	{NULL, NULL, NULL}
};

static const char *pat_thermal[] = {"ha:thermal", "ha:flags", "*", NULL};
static lhtpers_rule_t r_thermal[]   = {
	{pat_thermal,  &style_struct_thermal, NULL},
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
static const char *pat_square[]     = {"te:square", "*", NULL};
static lhtpers_rule_t r_line[] = {
	{pat_x1_line,    &style_inline, NULL},
	{pat_y1_line,    &style_inline, NULL},
	{pat_x2_line,    &style_inline, NULL},
	{pat_y2_line,    &style_inline, NULL},
	{pat_thickness,  &style_inline, NULL},
	{pat_clearance,  &style_inline, NULL},
	{lhtpers_early_end, &early_nl, NULL},
	{pat_flags,      &style_nlstruct, r_thermal},
	{pat_attributes, &style_nlstruct, NULL},
	{NULL, NULL, NULL}
};

static const char *pat_lgrp1_line[] = {"te:lgrp1", "*", NULL};
static const char *pat_lgrp2_line[] = {"te:lgrp2", "*", NULL};
static const char *pat_anch1_line[] = {"te:anchor1", "*", NULL};
static const char *pat_anch2_line[] = {"te:anchor2", "*", NULL};
static lhtpers_rule_t r_rat[] = {
	{pat_x1_line,    &style_inline, NULL},
	{pat_y1_line,    &style_inline, NULL},
	{pat_lgrp1_line, &style_inline, NULL},
	{pat_anch1_line, &style_inline, NULL},
	{pat_x2_line,    &style_inline, NULL},
	{pat_y2_line,    &style_inline, NULL},
	{pat_lgrp2_line, &style_inline, NULL},
	{pat_anch2_line, &style_inline, NULL},
	{lhtpers_early_end, &early_nl, NULL},
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
	{lhtpers_early_end, &early_nl, NULL},
	{pat_flags,      &style_nlstruct, r_thermal},
	{pat_attributes, &style_nlstruct, NULL},
	{NULL, NULL, NULL}
};

static const char *pat_gfx_ref[]       = {"te:pixmap_ref", "*", NULL};
static const char *pat_gfx_sx[]        = {"te:sx", "*", NULL};
static const char *pat_gfx_sy[]        = {"te:sy", "*", NULL};
static const char *pat_gfx_cx[]        = {"te:cx", "*", NULL};
static const char *pat_gfx_cy[]        = {"te:cy", "*", NULL};
static const char *pat_gfx_rot[]       = {"te:rot", "*", NULL};
static const char *pat_gfx_xmirror[]   = {"te:xmirror", "*", NULL};
static const char *pat_gfx_ymirror[]   = {"te:ymirror", "*", NULL};
static lhtpers_rule_t r_gfx[] = {
	{pat_gfx_ref,         &style_inline, NULL},
	{pat_gfx_cx,          &style_inline, NULL},
	{pat_gfx_cy,          &style_inline, NULL},
	{pat_gfx_sx,          &style_inline, NULL},
	{pat_gfx_sy,          &style_inline, NULL},
	{pat_gfx_rot,         &style_inline, NULL},
	{pat_gfx_xmirror,     &style_inline, NULL},
	{pat_gfx_ymirror,     &style_inline, NULL},
	{lhtpers_early_end, &early_nl, NULL},
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
	{lhtpers_early_end, &early_nl, NULL},
	{pat_flags,      &style_nlstruct, r_thermal},
	{pat_attributes, &style_nlstruct, NULL},
	{NULL, NULL, NULL}
};

static const char *pat_xmirror[] = {"te:xmirror", "*", NULL};
static const char *pat_smirror[] = {"te:smirror", "*", NULL};
static const char *pat_rot[]     = {"te:rot", "*", NULL};
static const char *pat_proto[]   = {"te:proto", "*", NULL};
static lhtpers_rule_t r_psref[] = {
	{pat_proto,      &style_inline, NULL},
	{pat_x,          &style_inline, NULL},
	{pat_y,          &style_inline, NULL},
	{pat_rot,        &style_inline, NULL},
	{pat_xmirror,    &style_inline, NULL},
	{pat_smirror,    &style_inline, NULL},
	{pat_clearance,  &style_inline, NULL},
	{lhtpers_early_end, &early_nl, NULL},
	{pat_flags,      &style_nlstruct, r_thermal},
	{pat_thermal,    &style_nlstruct, r_thermal},
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
	{lhtpers_early_end, &early_nl, NULL},
	{pat_flags,      &style_nlstruct, r_thermal},
	{pat_attributes, &style_nlstruct, NULL},
	{NULL, NULL, NULL}
};

static const char *pat_ta_contour[] = {"ta:contour", "*", NULL};
static const char *pat_ta_hole[]    = {"ta:hole", "*", NULL};
static lhtpers_rule_t r_geometry[] = {
	{pat_ta_contour,       &style_istruct, NULL},
	{pat_ta_hole,          &style_istruct, NULL},
	{NULL, NULL, NULL}
};

static const char *pat_geometry[] = {"li:geometry", "*", NULL};
static lhtpers_rule_t r_polygon[] = {
	{pat_clearance,  &style_inline, NULL},
	{pat_geometry,   &style_nlstruct, r_geometry},
	{pat_flags,      &style_nlstruct, r_thermal},
	{pat_attributes, &style_nlstruct, NULL},
	{NULL, NULL, NULL}
};

static const char *pat_string[] = {"te:string", "*", NULL};
static const char *pat_role[]   = {"te:role", "*", NULL};
static const char *pat_scale[]  = {"te:scale", "*", NULL};
static const char *pat_fid[]    = {"te:fid", "*", NULL};
static const char *pat_direct[] = {"te:direction", "*", NULL};
static lhtpers_rule_t r_text[] = {
	{pat_string,     &style_inline, NULL},
	{pat_x,          &style_inline, NULL},
	{pat_y,          &style_inline, NULL},
	{pat_scale,      &style_inline, NULL},
	{pat_fid,        &style_inline, NULL},
	{pat_direct,     &style_inline, NULL},
	{pat_role,       &style_inline, NULL},
	{lhtpers_early_end, &early_nl, NULL},
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

static const char *pat_visible[] = {"te:visible", "*", NULL};
static const char *pat_group[]   = {"te:group", "*", NULL};
static const char *pat_lid[]     = {"te:lid", "*", NULL};
static const char *pat_combin[]  = {"ha:combining", "*", NULL};
static lhtpers_rule_t r_layer[] = {
	{pat_lid,              &style_newline, NULL},
	{pat_group,            &style_newline, NULL},
	{pat_combin,           &style_structs, NULL},
	{pat_visible,          &style_newline, NULL},
	{pat_attributes,       &style_nlstruct, NULL},
	{pat_objects,          &style_nlstruct, NULL},
	{NULL, NULL, NULL}
};

static const char *pat_ps_line[] = {"ha:ps_line", "*", NULL};
static lhtpers_rule_t r_ps_line[] = {
	{pat_x1_line,    &style_inline, NULL},
	{pat_y1_line,    &style_inline, NULL},
	{pat_x2_line,    &style_inline, NULL},
	{pat_y2_line,    &style_inline, NULL},
	{pat_thickness,  &style_inline, NULL},
	{pat_square,     &style_inline, NULL},
	{NULL, NULL, NULL}
};

static const char *pat_ps_circ[] = {"ha:ps_circ", "*", NULL};
static const char *pat_dia[]     = {"te:dia", "*", NULL};
static lhtpers_rule_t r_ps_circ[] = {
	{pat_x,    &style_inline, NULL},
	{pat_y,    &style_inline, NULL},
	{pat_dia,  &style_inline, NULL},
	{NULL, NULL, NULL}
};

static const char *pat_ps_shape_v4[] = {"ha:ps_shape_v4", "*", NULL};
static const char *pat_layer_mask[]  = {"ha:layer_mask", "*", NULL};
static lhtpers_rule_t r_ps_shape_v4[] = {
	{pat_ps_line,          &style_struct_thermal,  r_ps_line},
	{pat_ps_circ,          &style_struct_thermal,  r_ps_circ},
	{pat_combin,           &style_structs, NULL},
	{pat_layer_mask,       &style_struct, NULL},
	{pat_clearance,        &style_newline,  NULL},
	{NULL, NULL, NULL}
};

static const char *pat_li_shape[] = {"li:shape", "*", NULL};
static lhtpers_rule_t r_li_shape[] = {
	{pat_ps_shape_v4, &style_structi,  r_ps_shape_v4},
	{NULL, NULL, NULL}
};


static const char *pat_ps_htop[] = {"te:htop", "*", NULL};
static const char *pat_ps_hdia[] = {"te:hdia", "*", NULL};
static const char *pat_ps_hbottom[] = {"te:hbottom", "*", NULL};
static const char *pat_ps_hplated[] = {"te:hplated", "*", NULL};

static const char *pat_ps_proto[] = {"ha:ps_proto_v*", "*", NULL};
static lhtpers_rule_t r_ps_proto[] = {
	{pat_ps_hdia,     &style_inlinesp, NULL},
	{pat_ps_hplated,  &style_inline, NULL},
	{pat_ps_htop,     &style_inline, NULL},
	{pat_ps_hbottom,  &style_inline, NULL},
	{pat_li_shape,    &style_nlstruct, r_li_shape},
	{NULL, NULL, NULL}
};

static const char *pat_ps_protos[] = {"li:padstack_prototypes", "*", NULL};
static lhtpers_rule_t r_ps_protos[] = {
	{pat_ps_proto,  &style_struct, r_ps_proto},
	{NULL, NULL, NULL}
};


static lhtpers_rule_t r_data[] = {
	{pat_ps_protos,        &style_struct,   r_ps_protos},
	{pat_objects,          &style_nlstruct, NULL},
	{NULL, NULL, NULL}
};

static const char *pat_width[]   = {"te:width", "*", NULL};
static const char *pat_height[]  = {"te:height", "*", NULL};
static const char *pat_delta[]   = {"te:delta", "*", NULL};
static lhtpers_rule_t r_symbol[] = {
	{pat_width,      &style_inline, NULL},
	{pat_height,     &style_inline, NULL},
	{pat_delta,      &style_inline, NULL},
	{pat_objects,    &style_nlstruct, NULL},
	{pat_attributes,       &style_nlstruct, NULL},
	{NULL, NULL, NULL}
};


static const char *pat_cell_width[]   = {"te:cell_width", "*", NULL};
static const char *pat_cell_height[]  = {"te:cell_height", "*", NULL};
static const char *pat_ha_symbols[]   = {"ha:symbols", "*", NULL};
static lhtpers_rule_t r_font1[] = {
	{pat_cell_width,      &style_inline, NULL},
	{pat_cell_height,     &style_inline, NULL},
	{pat_attributes,      &style_nlstruct, NULL},
	{pat_ha_symbols,      &style_nlstruct, NULL},
	{NULL, NULL, NULL}
};

static const char *pat_li_styles[]   = {"li:styles", "*", NULL};
static const char *pat_ha_meta[]     = {"ha:meta", "*", NULL};
static const char *pat_ha_data[]     = {"ha:data", "*", NULL};
static const char *pat_ha_font[]     = {"ha:font", "*", NULL};
static const char *pat_ha_netlists[] = {"ha:netlists", "*", NULL};
static lhtpers_rule_t r_root[] = {
	{pat_attributes,      &style_nlstruct, NULL},
	{pat_li_styles,       &style_nlstruct, NULL},
	{pat_ha_meta,         &style_nlstruct, NULL},
	{pat_ha_data,         &style_nlstruct, NULL},
	{pat_ha_font,         &style_nlstruct, NULL},
	{pat_ha_netlists,     &style_nlstruct, NULL},
	{NULL, NULL, NULL}
};

static const char *pat_net_input[] = {"ha:input", "*", NULL};
static const char *pat_net_patch[] = {"ha:netlist_patch", "*", NULL};
static lhtpers_rule_t r_netlists[] = {
	{pat_net_input,         &style_nlstruct, NULL},
	{pat_net_patch,         &style_nlstruct, NULL},
	{NULL, NULL, NULL}
};


static const char *pat_ls_type1[] = {"te:*", "ha:type", "ha:*", "li:groups", "ha:layer_stack", "*", NULL};
static const char *pat_ls_type2[] = {"ha:type", "ha:*", "li:groups", "ha:layer_stack", "*", NULL};
static const char *pat_ls_lyr1[] = {"te:*", "li:layers", "ha:*", "li:groups", "ha:layer_stack", "*", NULL};
static const char *pat_ls_lyr2[] = {"li:layers", "ha:*", "li:groups", "ha:layer_stack", "*", NULL};
static const char *pat_ls_name[] = {"te:name", "ha:*", "li:groups", "ha:layer_stack", "*", NULL};
static const char *pat_ls_grps[] = {"li:groups", "ha:layer_stack", "*", NULL};
static const char *pat_pxm_ulzw[] = {"ha:ulzw.*", "ha:pixmaps", "*", NULL};
static const char *pat_ulzw_sx[] = {"te:sx", "ha:ulzw.*", "*", NULL};
static const char *pat_ulzw_sy[] = {"te:sy", "ha:ulzw.*", "*", NULL};
static const char *pat_ulzw_transp[] = {"te:transparent", "ha:ulzw.*", "*", NULL};
static const char *pat_ulzw_pixmap[] = {"te:pixmap", "ha:ulzw.*", "*", NULL};

static lhtpers_rule_t r_layergrp[] = {
	{pat_ls_name,  NULL, NULL},
	{pat_ls_type2, &style_structsp, NULL},
	{pat_ls_lyr2,  &style_structsp, NULL},
	{NULL, NULL, NULL}
};

static lhtpers_rule_t r_ulzw[] = {
	{pat_ulzw_sx,  &style_inlinesp, NULL},
	{pat_ulzw_sy,  &style_inline, NULL},
	{pat_ulzw_transp,  &style_inline, NULL},
	{pat_ulzw_pixmap,  &style_inline, NULL},
	{NULL, NULL, NULL}
};


static const char *pat_line[] = {"ha:line.*", "*", NULL};
static const char *pat_spoly[]= {"li:simplepoly.*", "*", NULL};
static const char *pat_rat[]  = {"ha:rat.*", "*", NULL};
static const char *pat_arc[]  = {"ha:arc.*", "*", NULL};
static const char *pat_gfx[]  = {"ha:gfx.*", "*", NULL};
static const char *pat_via[]  = {"ha:via.*", "*", NULL};
static const char *pat_psref[]= {"ha:padstack_ref.*", "*", NULL};
static const char *pat_pin[]  = {"ha:pin.*", "*", NULL};
static const char *pat_pad[]  = {"ha:pad.*", "*", NULL};
static const char *pat_poly[] = {"ha:polygon.*", "*", NULL};
static const char *pat_elem[] = {"ha:element.*", "*", NULL};
static const char *pat_text[] = {"ha:text.*", "*", NULL};
static const char *pat_data[] = {"ha:data", "*", NULL};
static const char *pat_netlists[] = {"ha:netlists", "*", NULL};
static const char *pat_font1[] = {"ha:geda_pcb", "ha:font", "*", NULL};
static const char *pat_layer[] = {"ha:*", "li:layers", "*", NULL};
static const char *pat_symbol[] = {"ha:*", "ha:symbols", "*", NULL};
static const char *pat_thermt[] = {"te:*", "ha:thermal", "*", NULL};
static const char *pat_flag[] = {"te:*", "ha:flags", "*", NULL};
static const char *pat_cell[] = {"te:*", "ta:*", "*", NULL};
static const char *pat_netinft[] = {"te:*", "li:net_info", "li:netlist_patch", "*", NULL};
static const char *pat_nettxt[] = {"te:*", "ha:*", "li:netlist_patch", "*", NULL};
static const char *pat_del_add[] = {"ha:*", "li:netlist_patch", "*", NULL};
static const char *pat_netinfo[] = {"li:net_info", "li:netlist_patch", "*", NULL};
static const char *pat_tconn[] = {"te:*", "li:conn", "ha:*", "li:*", "ha:netlists", "*", NULL};
static const char *pat_lconn[] = {"li:conn", "ha:*", "li:*", "ha:netlists", "*", NULL};
static const char *pat_layergrp[] = {"ha:*", "li:groups", "ha:layer_stack", "*", NULL};
static const char *pat_combs[]  = {"te:*", "ha:combining", "*", NULL};
static const char *pat_root[] = {"^", NULL};

static lhtpers_rule_t r_istructs[] = {
	{pat_root,    &style_struct, r_root},

	{pat_layer,   &style_nlstruct, r_layer},
	{pat_symbol,  &style_structi,  r_symbol},

	{pat_line,    &style_structi,  r_line},
	{pat_spoly,   &style_structi,  NULL},
	{pat_rat,     &style_structi,  r_rat},
	{pat_arc,     &style_structi,  r_arc},
	{pat_gfx,     &style_structi,  r_gfx},
	{pat_via,     &style_structi,  r_pinvia},
	{pat_psref,   &style_structi,  r_psref},
	{pat_pin,     &style_structi,  r_pinvia},
	{pat_pad,     &style_structi,  r_pad},
	{pat_poly,    &style_structs,  r_polygon},
	{pat_elem,    &style_structi,  r_element},
	{pat_text,    &style_structi,  r_text},
	{pat_data,    &style_structi,  r_data},
	{pat_font1,   &style_structi,  r_font1},
	{pat_netlists,&style_struct,   r_netlists},
	{pat_ps_protos,&style_struct,  r_ps_protos},
	{pat_ps_proto,&style_nlstruct, r_ps_proto},
	{pat_ps_shape_v4,&style_nlstruct,r_ps_shape_v4},
	{pat_objects, &style_nlstruct, NULL},
	{pat_flag,    &style_newline,  NULL},

	{pat_layergrp,&style_structnp, r_layergrp},
	{pat_ls_grps, &style_structnp, NULL},

	{pat_pxm_ulzw, &style_structsp, r_ulzw},

	{pat_cell,    &style_inline,   NULL},
	{pat_thermt,  &style_inline,   NULL},
	{pat_del_add, &style_struct_thermal, NULL},
	{pat_netinfo, &style_struct_thermal, NULL},
	{pat_netinft, &style_inline,   NULL},
	{pat_nettxt,  &style_inline,   NULL},
	{pat_tconn,   &style_inline,   NULL},
	{pat_lconn,   &style_struct_thermal, NULL},

	{pat_ls_type1, &style_inline,   NULL},
	{pat_ls_lyr1,  &style_inline,   NULL},
	{pat_combs,    &style_inline,   NULL},

	{NULL, NULL, NULL}
};

lhtpers_rule_t *io_lihata_out_rules[] = {
	r_istructs, r_ilists, r_ilists, r_flags, NULL
};


/*****************************************************************************/
static const char *cpat_rat_x1[]   = {"te:x1", "ha:rat.*", "*", NULL};
static const char *cpat_rat_y1[]   = {"te:y1", "ha:rat.*", "*", NULL};
static const char *cpat_rat_x2[]   = {"te:x2", "ha:rat.*", "*", NULL};
static const char *cpat_rat_y2[]   = {"te:y2", "ha:rat.*", "*", NULL};
static const char *cpat_line_x1[]  = {"te:x1", "ha:line.*", "*", NULL};
static const char *cpat_line_y1[]  = {"te:y1", "ha:line.*", "*", NULL};
static const char *cpat_line_x2[]  = {"te:x2", "ha:line.*", "*", NULL};
static const char *cpat_line_y2[]  = {"te:y2", "ha:line.*", "*", NULL};
static const char *cpat_line_th[]  = {"te:thickness", "ha:line.*", "*", NULL};
static const char *cpat_line_cl[]  = {"te:clearance", "ha:line.*", "*", NULL};
static const char *cpat_size_x[]   = {"te:x", "ha:size", "ha:meta", "*", NULL};
static const char *cpat_size_y[]   = {"te:y", "ha:size", "ha:meta", "*", NULL};
static const char *cpat_curs_x[]   = {"te:x", "ha:cursor", "ha:meta", "*", NULL};
static const char *cpat_curs_y[]   = {"te:y", "ha:cursor", "ha:meta", "*", NULL};
static const char *cpat_curs_z[]   = {"te:zoom", "ha:cursor", "ha:meta", "*", NULL};
static const char *cpat_drc_min[]  = {"te:min_*", "ha:drc", "ha:meta", "*", NULL};
static const char *cpat_drc_blt[]  = {"te:bloat", "ha:drc", "ha:meta", "*", NULL};
static const char *cpat_drc_shr[]  = {"te:shrink", "ha:drc", "ha:meta", "*", NULL};
static const char *cpat_grido[]    = {"te:offs_*", "ha:grid", "ha:meta", "*", NULL};
static const char *cpat_grids[]    = {"te:spacing", "ha:grid", "ha:meta", "*", NULL};

static const char *cpat_arc_x[]    = {"te:x", "ha:arc.*", "*", NULL};
static const char *cpat_arc_y[]    = {"te:y", "ha:arc.*", "*", NULL};
static const char *cpat_arc_w[]    = {"te:width", "ha:arc.*", "*", NULL};
static const char *cpat_arc_h[]    = {"te:height", "ha:arc.*", "*", NULL};
static const char *cpat_arc_th[]   = {"te:thickness", "ha:arc.*", "*", NULL};
static const char *cpat_arc_cl[]   = {"te:clearance", "ha:arc.*", "*", NULL};
static const char *cpat_geo_ta[]   = {"te:*", "ta:*", "li:geometry", "*", NULL};
static const char *cpat_text_x[]   = {"te:x", "ha:text.*", "*", NULL};
static const char *cpat_text_y[]   = {"te:y", "ha:text.*", "*", NULL};
static const char *cpat_pin_x[]    = {"te:x", "ha:pin.*", "*", NULL};
static const char *cpat_pin_y[]    = {"te:y", "ha:pin.*", "*", NULL};
static const char *cpat_pin_hole[] = {"te:hole", "ha:pin.*", "*", NULL};
static const char *cpat_pin_mask[] = {"te:mask", "ha:pin.*", "*", NULL};
static const char *cpat_pad_x1[]   = {"te:x1", "ha:pad.*", "*", NULL};
static const char *cpat_pad_y1[]   = {"te:y1", "ha:pad.*", "*", NULL};
static const char *cpat_pad_x2[]   = {"te:x2", "ha:pad.*", "*", NULL};
static const char *cpat_pad_y2[]   = {"te:y2", "ha:pad.*", "*", NULL};
static const char *cpat_pad_hole[] = {"te:hole", "ha:pad.*", "*", NULL};
static const char *cpat_pad_mask[] = {"te:mask", "ha:pad.*", "*", NULL};
static const char *cpat_elem_x[]   = {"te:x", "ha:element.*", "*", NULL};
static const char *cpat_elem_y[]   = {"te:y", "ha:element.*", "*", NULL};

static const char *cpat_sym_w[]   = {"te:width", "ha:*", "ha:symbols", "*", NULL};
static const char *cpat_sym_h[]   = {"te:height", "ha:*", "ha:symbols", "*", NULL};
static const char *cpat_sym_d[]   = {"te:delta", "ha:*", "ha:symbols", "*", NULL};
static const char *cpat_font_w[]  = {"te:cell_width", "ha:*", "ha:font", "*", NULL};
static const char *cpat_font_h[]  = {"te:cell_height", "ha:*", "ha:font", "*", NULL};
static const char *cpat_styl_th[] = {"te:thickness", "ha:*", "li:styles", "*", NULL};
static const char *cpat_styl_cl[] = {"te:clearance", "ha:*", "li:styles", "*", NULL};
static const char *cpat_styl_hl[] = {"te:hole", "ha:*", "li:styles", "*", NULL};
static const char *cpat_styl_da[] = {"te:diameter", "ha:*", "li:styles", "*", NULL};


lhtpers_rule_t io_lihata_out_coords[] = {
	{cpat_rat_x1,   NULL, NULL},
	{cpat_rat_y1,   NULL, NULL},
	{cpat_rat_x2,   NULL, NULL},
	{cpat_rat_y2,   NULL, NULL},
	{cpat_line_x1,  NULL, NULL},
	{cpat_line_y1,  NULL, NULL},
	{cpat_line_x2,  NULL, NULL},
	{cpat_line_y2,  NULL, NULL},
	{cpat_line_th,  NULL, NULL},
	{cpat_line_cl,  NULL, NULL},
	{cpat_size_x,   NULL, NULL},
	{cpat_size_y,   NULL, NULL},
	{cpat_curs_x,   NULL, NULL},
	{cpat_curs_y,   NULL, NULL},
	{cpat_curs_z,   NULL, NULL},
	{cpat_drc_min,  NULL, NULL},
	{cpat_drc_blt,  NULL, NULL},
	{cpat_drc_shr,  NULL, NULL},
	{cpat_grido,    NULL, NULL},
	{cpat_grids,    NULL, NULL},
	{cpat_arc_x,    NULL, NULL},
	{cpat_arc_y,    NULL, NULL},
	{cpat_arc_w,    NULL, NULL},
	{cpat_arc_h,    NULL, NULL},
	{cpat_arc_th,   NULL, NULL},
	{cpat_arc_cl,   NULL, NULL},
	{cpat_geo_ta,   NULL, NULL},
	{cpat_text_x,   NULL, NULL},
	{cpat_text_y,   NULL, NULL},
	{cpat_pin_x,    NULL, NULL},
	{cpat_pin_y,    NULL, NULL},
	{cpat_pin_hole, NULL, NULL},
	{cpat_pin_mask, NULL, NULL},
	{cpat_pad_x1,   NULL, NULL},
	{cpat_pad_y1,   NULL, NULL},
	{cpat_pad_x2,   NULL, NULL},
	{cpat_pad_y2,   NULL, NULL},
	{cpat_pad_hole, NULL, NULL},
	{cpat_pad_mask, NULL, NULL},
	{cpat_elem_x,   NULL, NULL},
	{cpat_elem_y,   NULL, NULL},
	{cpat_sym_w,    NULL, NULL},
	{cpat_sym_h,    NULL, NULL},
	{cpat_sym_d,    NULL, NULL},
	{cpat_font_w,   NULL, NULL},
	{cpat_font_h,   NULL, NULL},
	{cpat_styl_th,  NULL, NULL},
	{cpat_styl_cl,  NULL, NULL},
	{cpat_styl_hl,  NULL, NULL},
	{cpat_styl_da,  NULL, NULL},
	{NULL, NULL, NULL}
};

