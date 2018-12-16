/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *  Copyright (C) 2017 Erich S. Heinzle
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
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
 *
 */

/* Eagle binary tree parser */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#include "eagle_bin.h"
#include "egb_tree.h"
#include "error.h"
#include "math_helper.h"
#include "misc_util.h"

/* Describe a bitfield: width of the field that hosts the bitfield, first
and last bit offsets, inclusive. Bit offsets are starting from 0 at LSB. */
#define BITFIELD(field_width_bytes, first_bit, last_bit) \
	(((unsigned long)(field_width_bytes)) << 16 | ((unsigned long)(first_bit)) << 8 | ((unsigned long)(last_bit)))

typedef enum {
	T_BMB, /* bit-mask-bool: apply the bitmask in 'len' to the the byte at offs and use the result as a boolean */
	T_UBF, /* unsigned bitfield; offset is the first byte in block[], len is a BITFIELD() */
	T_INT,
	T_DBL,
	T_STR
} pcb_eagle_type_t;

typedef enum {
	SS_DIRECT,	    	/* it specifies the number of direct children */
	SS_RECURSIVE,	 	/* it specifies the number of all children, recursively */
	SS_RECURSIVE_MINUS_1	/* same as SS_RECURSIVE, but decrease the children count first */
} pcb_eagle_ss_type_t;


typedef struct {
	int offs; /* 0 is the terminator (no more offsets) */
	unsigned long len;
	int val;
} fmatch_t;

typedef struct {
	int offs; /* 0 is the terminator (no more offsets) */
	int len;
	pcb_eagle_ss_type_t ss_type;
	char *tree_name; /* if not NULL, create a subtree using this name and no attributes */
} subsect_t;

typedef struct {
	char *name; /* NULL is the terminator (no more attributes) */
	pcb_eagle_type_t type;
	int offs;
	unsigned len;
} attrs_t;

typedef struct {
	unsigned int cmd, cmd_mask;/* rule matches only if block[0] == cmd */
	char *name;
	fmatch_t fmatch[4];  /* rule matches only if all fmatch integer fields match their val */
	subsect_t subs[8];/* how to extract number of subsections (direct children) */
	attrs_t attrs[32];/* how to extract node attributes */
} pcb_eagle_script_t;

typedef struct
{
	egb_node_t *root, *layers, *drawing, *libraries, *elements, *firstel, *signals, *board, *drc;
	long mdWireWire, wire2pad, wire2via, pad2pad, pad2via, via2via, pad2smd, via2smd,
	smd2smd, copper2dimension, drill2hole, msWidth, minDrill, psTop, psBottom, psFirst;
	/* DRC values for spacing and width */ 
	double rvPadTop, rvPadInner, rvPadBottom; /* DRC values for via sizes */
	double rvViaOuter, rvViaInner, rvMicroViaOuter, rvMicroViaInner;
	/* the following design rule values don't map obviously to XML parameters */
	long restring_limit1_mil, restring_limit2_mil, restring_limit3_mil,
	restring_limit4_mil, restring_limit5_mil, restring_limit6_mil,
	restring_limit7_mil, restring_limit8_mil, restring_limit9_mil,
	restring_limit10_mil, restring_limit11_mil, restring_limit12_mil,
	restring_limit13_mil, restring_limit14_mil, mask_limit1_mil,
	mask_limit2_mil, mask_limit3_mil, mask_limit4_mil;
	double mask_percentages1_ratio, mask_percentages2_ratio;
	char *free_text;
	char *free_text_cursor;
	size_t free_text_len;
} egb_ctx_t;

#define TERM {0}

static const pcb_eagle_script_t pcb_eagle_script[] = {
	{ PCB_EGKW_SECT_START, 0xFF7F, "drawing",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
/*			{2, 2, SS_DIRECT, NULL},*/
			{4, 4, SS_RECURSIVE_MINUS_1, NULL},
			TERM
		},
		{ /* attributes */
			{"subsecs", T_INT, 2, 2},
			{"numsecs", T_INT, 4, 4},
			{"subsecsMSB", T_INT, 3, 1},
			{"subsecsLSB", T_INT, 2, 1},
			{"numsecsMSB2", T_INT, 7, 1},
			{"numsecsMSB1", T_INT, 6, 1},
			{"numsecsMSB0", T_INT, 5, 1},
			{"numsecsLSB", T_INT, 4, 1},
			{"v1", T_INT, 8, 1},
			{"v2", T_INT, 9, 1},
			TERM
		},
	},
	{ PCB_EGKW_SECT_UNKNOWN11, 0xFFFF, "unknown11" },
	{ PCB_EGKW_SECT_GRID, 0xFFFF, "grid",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			{"display", T_BMB, 2, 0x01},
			{"visible", T_BMB, 2, 0x02},
			{"unit", T_UBF, 3, BITFIELD(1, 0, 3)},
			{"altunit", T_UBF, 3, BITFIELD(1, 4, 7)},
			{"multiple",T_INT, 4, 3},
			{"size", T_DBL, 8, 8},
			{"altsize", T_DBL, 16, 8},
			TERM
		},
	},
	{ PCB_EGKW_SECT_LAYER, 0xFF7F, "layer",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			{"side", T_BMB, 2, 0x10},
			{"visible", T_UBF, 2, BITFIELD(1, 2, 3)},
			{"active", T_BMB, 2, 0x02},
			{"number",T_UBF, 3, BITFIELD(1, 0, 7)},
			{"other",T_INT, 4, 1},
			{"fill", T_UBF, 5, BITFIELD(1, 0, 3)},
			{"color",T_UBF, 6, BITFIELD(1, 0, 5)},
			{"name", T_STR, 15, 9},
			TERM
		},
	},
	{ PCB_EGKW_SECT_SCHEMA, 0xFFFF, "schema",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			{4, 4, SS_DIRECT, NULL},
			TERM
		},
		{ /* attributes */
			{"shtsubsecs", T_INT, 8, 4},
			{"atrsubsecs", T_INT, 12, 4}, /* 0 if v4 */
			{"xref_format",  T_STR, 19, 5},
			TERM
		},
	},
	{ PCB_EGKW_SECT_LIBRARY, 0xFF7F, "library",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			{4, 4, SS_RECURSIVE, NULL},
			{8, 4, SS_RECURSIVE, NULL},
			{12, 4, SS_RECURSIVE, NULL},
			/*{4, 4, SS_DIRECT, NULL},*/
			TERM
		},
		{ /* attributes */
			{"devsubsecs", T_INT, 4, 4},
			{"symsubsecs", T_INT, 8, 4},
			{"pacsubsecs", T_INT, 12, 4},
			{"children", T_INT, 8, 4},
			{"name",  T_STR, 16, 8},
			TERM
		},
	},
	{ PCB_EGKW_SECT_DEVICES, 0xFF7F, "devices",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			{4, 4, SS_DIRECT, NULL},
			TERM
		},
		{ /* attributes */
			{"children", T_INT, 8, 4},
			{"library",  T_STR, 16, 8},
			TERM
		},
	},
	{ PCB_EGKW_SECT_SYMBOLS, 0xFF7F, "symbols",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			{4, 4, SS_RECURSIVE, NULL},
			TERM
		},
		{ /* attributes */
			{"children", T_INT, 8, 4},
			{"library",  T_STR, 16, 8},
			TERM
		},
	},
	{ PCB_EGKW_SECT_PACKAGES, 0xFF5F, "packages",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			{4, 4, SS_RECURSIVE, NULL},
			TERM
		},
		{ /* attributes */
			{"subsects", T_INT, 4, 4},
			{"children", T_INT, 8, 2},
			{"desc",  T_STR, 10, 6},
			{"library",  T_STR, 16, 8},
			TERM
		},
	},
	{ PCB_EGKW_SECT_SCHEMASHEET, 0xFFFF, "schemasheet",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			{2, 2, SS_DIRECT, NULL},
			TERM
		},
		{ /* attributes */
			{"minx",  T_INT, 4, 2},
			{"miny",  T_INT, 6, 2},
			{"maxx",  T_INT, 8, 2},
			{"maxy",  T_INT, 10, 2},
			{"partsubsecs",  T_INT, 12, 4},
			{"bussubsecs",  T_INT, 16, 4},
			{"netsubsecs",  T_INT, 20, 4},
			TERM
		},
	},
	{ PCB_EGKW_SECT_BOARD, 0xFF37, "board",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			{12, 4, SS_RECURSIVE, "libraries"}, /* lib */
			{2,  2, SS_DIRECT, "plain"}, /* globals */
			{16, 4, SS_RECURSIVE, "elements"}, /* package refs */
			{20, 4, SS_RECURSIVE, "signals"}, /* nets */
			TERM
		},
		{ /* attributes */
			{"minx",  T_INT, 4, 2},
			{"miny",  T_INT, 6, 2},
			{"maxx",  T_INT, 8, 2},
			{"maxy",  T_INT, 10, 2},
			{"defsubsecs",  T_INT, 12, 4},
			{"pacsubsecs",  T_INT, 16, 4},
			{"netsubsecs",  T_INT, 20, 4},
			TERM
		},
	},
	{ PCB_EGKW_SECT_SIGNAL, 0xFFB3, "signal",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			{2, 2, SS_DIRECT, NULL},
			TERM
		},
		{ /* attributes */
			{"minx",  T_INT, 4, 2},
			{"miny",  T_INT, 6, 2},
			{"maxx",  T_INT, 8, 2},
			{"maxy",  T_INT, 10, 2},
			{"airwires",  T_BMB, 12, 0x02},
			{"netclass",  T_UBF, 13, BITFIELD(1, 0, 3)},
			{"name",  T_STR, 16, 8},
			TERM
		},
	},
	{ PCB_EGKW_SECT_SYMBOL, 0xFFFF, "symbol",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			{2, 2, SS_DIRECT, NULL},
			TERM
		},
		{ /* attributes */
			{"minx",  T_INT, 4, 2},
			{"miny",  T_INT, 6, 2},
			{"maxx",  T_INT, 8, 2},
			{"maxy",  T_INT, 10, 2},
			{"name",  T_STR, 16, 8},
			TERM
		},
	},
	{ PCB_EGKW_SECT_PACKAGE, 0xFFDF, "package",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			{2, 2, SS_RECURSIVE, NULL},
			TERM
		},
		{ /* attributes */
			{"minx",  T_INT, 4, 2},
			{"miny",  T_INT, 6, 2},
			{"maxx",  T_INT, 8, 2},
			{"maxy",  T_INT, 10, 2},
			{"desc",  T_STR, 13, 5},
			{"name",  T_STR, 18, 6},
			TERM
		},
	},
	{ PCB_EGKW_SECT_SCHEMANET, 0xFFFF, "schemanet",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			{2, 2, SS_DIRECT, NULL},
			TERM
		},
		{ /* attributes */
			{"minx",  T_INT, 4, 2},
			{"miny",  T_INT, 6, 2},
			{"maxx",  T_INT, 8, 2},
			{"maxy",  T_INT, 10, 2},
			{"netclass",  T_UBF, 13, BITFIELD(1, 0, 3)},
			{"name",  T_STR, 18, 6},
			TERM
		},
	},
	{ PCB_EGKW_SECT_PATH, 0xFFFF, "path",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			{2, 2, SS_DIRECT, NULL},
			TERM
		},
		{ /* attributes */
			{"minx",  T_INT, 4, 2},
			{"miny",  T_INT, 6, 2},
			{"maxx",  T_INT, 8, 2},
			{"maxy",  T_INT, 10, 2},
			TERM
		},
	},
	{ PCB_EGKW_SECT_POLYGON, 0xFFF7, "polygon",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			{2, 2, SS_DIRECT, NULL},
			TERM
		},
		{ /* attributes */
			{"minx",  T_INT, 4, 2},
			{"miny",  T_INT, 6, 2},
			{"maxx",  T_INT, 8, 2},
			{"maxy",  T_INT, 10, 2},
			{"width",  T_INT, 12, 2},
			{"spacing",  T_INT, 14, 2},
			{"isolate",  T_INT, 16, 2},
			{"layer",  T_UBF, 18, BITFIELD(1,0,7)},
			{"pour",  T_BMB, 19, 0x01},
			{"rank",  T_BMB, 19, BITFIELD(1, 1, 3)},
			{"thermals",  T_BMB, 19, 0x80},
			{"orphans",  T_BMB, 19, 0x40},
			TERM
		},
	},
	{ PCB_EGKW_SECT_LINE, 0xFF43, "wire",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			{"layer",  T_UBF, 3, BITFIELD(1,0,7)},
			{"half_width",  T_INT, 20, 2},
			{"stflags",  T_BMB, 22, 0x33},
			/*{"bin_style",  T_BMB, 22, 0x03},*/
			/*{"bin_cap",  T_BMB, 22, 0x10},*/
			{"clockwise",  T_BMB, 22, 0x20},
			{"linetype", T_UBF, 23, BITFIELD(1, 0, 7)},
			{"linetype_0_x1",  T_INT, 4, 4},
			{"x",  T_INT, 4, 4}, /* only needed if wire used for polygon vertex coord */
			{"linetype_0_y1",  T_INT, 8, 4},
			{"y",  T_INT, 8, 4}, /* only needed if wire used for polygon vertex coord */
			{"linetype_0_x2",  T_INT, 12, 4},
			{"linetype_0_y2",  T_INT, 16, 4},
			{"arc_negflags", T_BMB, 19, 0x1f},
			/*{"c_negflag", T_BMB, 19, 0x01},*/
			{"arc_c1",  T_UBF, 7, BITFIELD(1, 0, 7)},
			{"arc_c2",  T_UBF, 11, BITFIELD(1, 0, 7)},
			{"arc_c3",  T_UBF, 15, BITFIELD(1, 0, 7)},
			{"arc_x1",  T_UBF, 4, BITFIELD(3, 0, 23)},
			{"arc_y1",  T_UBF, 8, BITFIELD(3, 0, 23)},
			{"arc_x2",  T_UBF, 12, BITFIELD(3, 0, 23)},
			{"arc_y2",  T_UBF, 16, BITFIELD(3, 0, 23)},
			TERM
		},
	},
	{ PCB_EGKW_SECT_ARC, 0xFFFF, "arc",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			{"layer",  T_UBF, 3, BITFIELD(1,0,7)},
			{"half_width",  T_INT, 20, 2},
			{"clockwise",  T_BMB, 22, 0x20},
			{"arctype",  T_UBF, 23, BITFIELD(1, 0, 7)},
			{"arc_negflags", T_BMB, 19, 0x1f},
			/*{"c_negflag", T_BMB, 19, 0x01},*/
			{"arc_c1",  T_UBF, 7, BITFIELD(1, 0, 7)},
			{"arc_c2",  T_UBF, 11, BITFIELD(1, 0, 7)},
			{"arc_c3",  T_UBF, 15, BITFIELD(1, 0, 7)},
			{"arc_x1",  T_UBF, 4, BITFIELD(3, 0, 23)},
			{"arc_y1",  T_UBF, 8, BITFIELD(3, 0, 23)},
			{"arc_x2",  T_UBF, 12, BITFIELD(3, 0, 23)},
			{"arc_y2",  T_UBF, 16, BITFIELD(3, 0, 23)},
			{"arctype_other_x1",  T_INT, 4, 4},
			{"arctype_other_y1",  T_INT, 8, 4},
			{"arctype_other_x2",  T_INT, 12, 4},
			{"arctype_other_y2",  T_INT, 16, 4},
			TERM
		},
	},
	{ PCB_EGKW_SECT_CIRCLE, 0xFF53, "circle",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			{"layer", T_UBF, 3, BITFIELD(1,0,7)},
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"radius",  T_INT, 12, 4},
			{"half_width", T_INT, 20, 4},
			TERM
		},
	},
	{ PCB_EGKW_SECT_RECTANGLE, 0xFF5F, "rectangle",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			{"layer", T_UBF, 3, BITFIELD(1,0,7)},
			{"x1",  T_INT, 4, 4},
			{"y1",  T_INT, 8, 4},
			{"x2",  T_INT, 12, 4},
			{"y2",  T_INT, 16, 4},
			{"bin_rot",  T_INT, 20, 2}, /* ? should it be T_UBF, 16, BITFIELD(2, 0, 11)},*/
			TERM
		},
	},
	{ PCB_EGKW_SECT_JUNCTION, 0xFFFF, "junction",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			{"layer", T_UBF, 3, BITFIELD(1,0,7)},
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"width_2",  T_INT, 12, 2},
			TERM
		},
	},
	{ PCB_EGKW_SECT_HOLE, 0xFF53, "hole",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			{"x", T_INT, 4, 4},
			{"y", T_INT, 8, 4},
			{"half_diameter", T_UBF, 12, BITFIELD(2, 0, 15)},
			{"half_drill", T_UBF, 12, BITFIELD(2, 0, 15)}, /* try duplicating field */
			TERM
		},
	},
	{ PCB_EGKW_SECT_VIA, 0xFF7F, "via",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			{"shape", T_INT, 2, 1},
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"half_drill", T_UBF, 12, BITFIELD(2, 0, 15)},
			{"half_diameter", T_UBF, 14, BITFIELD(2, 0, 15)},
			{"layers",  T_UBF, 16, BITFIELD(1,0,7)}, /*not 1:1 mapping */
			{"stop",  T_BMB, 17, 0x01},
			TERM
		},
	},
	{ PCB_EGKW_SECT_PAD, 0xFF5F, "pad",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			{"shape", T_INT, 2, 1},
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"half_drill",  T_UBF, 12, BITFIELD(2, 0, 15)},
			{"half_diameter",  T_UBF, 14, BITFIELD(2, 0, 15)},
			{"bin_rot" , T_INT, 16, 2}, /* ? maybe T_UBF, 16, BITFIELD(2, 0, 11)}, */
			{"stop",  T_BMB, 18, 0x01},
			{"thermals",  T_BMB, 18, 0x04},
			{"first",  T_BMB, 18, 0x08},
			{"name",  T_STR, 19, 5},
			TERM
		},
	},
	{ PCB_EGKW_SECT_SMD, 0xFF7F, "smd",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			{"roundness", T_INT, 2, 1},
			{"layer", T_UBF, 3, BITFIELD(1,0,7)},
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"half_dx",  T_UBF, 12, BITFIELD(2, 0, 15)},
			{"half_dy",  T_UBF, 14, BITFIELD(2, 0, 15)},
			{"bin_rot",  T_UBF, 16, BITFIELD(2, 0, 11)},
			{"stop",  T_BMB, 18, 0x01},
			{"cream",  T_BMB, 18, 0x02},
			{"thermals",  T_BMB, 18, 0x04},
			{"first",  T_BMB, 18, 0x08},
			{"name",  T_STR, 19, 5},
			TERM
		},
	},
	{ PCB_EGKW_SECT_PIN, 0xFF7F, "pin",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			{"function", T_UBF, 2, BITFIELD(1, 0, 1)},
			{"visible", T_UBF, 2, BITFIELD(1, 6, 7)},
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"direction",  T_UBF, 12, BITFIELD(1, 0, 3)},
			{"length",  T_UBF, 12, BITFIELD(1, 4, 5)},
			{"bin_rot",  T_UBF, 12, BITFIELD(1, 6, 7)},
			{"direction",  T_UBF, 12, BITFIELD(1, 0, 3)},
			{"swaplevel",  T_INT, 13, 1},
			{"name",  T_STR, 14, 10},
			TERM
		},
	},
	{ PCB_EGKW_SECT_GATE, 0xFF7F, "gate",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"addlevel",  T_INT, 12, 1},
			{"swap",  T_INT, 13, 1},
			{"symno",  T_INT, 14, 2},
			{"name",  T_STR, 16, 8},
			TERM
		},
	},
	{ PCB_EGKW_SECT_ELEMENT, 0xFF53, "element",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			{2, 2, SS_DIRECT, NULL},
			TERM
		},
		{ /* attributes */
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"library",  T_INT, 12, 2},
			{"package",  T_INT, 14, 2},
			{"bin_rot",  T_UBF, 16, BITFIELD(2, 0, 11)}, /* result is n*1024 */
			{"mirrored", T_BMB, 17, 0x10},
			{"spin", T_BMB, 17, 0x40},
			TERM
		},
	},
	{ PCB_EGKW_SECT_ELEMENT2, 0xFF5F, "element2",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			{"name",  T_STR, 2, 8},
			{"value",  T_STR, 10, 14},
			TERM
		},
	},
	{ PCB_EGKW_SECT_INSTANCE, 0xFFFF, "instance",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			{2, 2, SS_DIRECT, NULL},
			TERM
		},
		{ /* attributes */
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"placed", T_INT, 12, 2}, /* == True for v4 */
			{"gateno", T_INT, 14, 2},
			{"bin_rot",  T_UBF, 16, BITFIELD(2, 10, 11)}, /* was 0, 11}, */
			/* _get_uint16_mask(16, 0x0c00) */
			{"mirrored",  T_UBF, 16, BITFIELD(2, 12, 12)},
			/* _get_uint16_mask(16, 0x1000) */
			{"smashed",  T_BMB, 18, 0x01},
			TERM
		},
	},
	{ PCB_EGKW_SECT_TEXT, 0xFF53, "text",  /* basic text block */
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			{"layer", T_UBF, 3, BITFIELD(1,0,7)},
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"half_size",  T_INT, 12, 2},
			{"ratio", T_UBF, 14, BITFIELD(2, 2, 6)},
			/*self._get_uint8_mask(14, 0x7c) >> 2 },*/
			{"bin_rot" , T_UBF, 16, BITFIELD(2, 0, 11)},
			/*self._get_uint16_mask(16, 0x0fff)*/
			{"mirrored" , T_UBF, 16, BITFIELD(2, 12, 12)},
			/*bool(self._get_uint16_mask(16, 0x1000))*/
			{"spin" , T_UBF, 16, BITFIELD(2, 14, 14)},
			/*bool(self._get_uint16_mask(16, 0x4000))*/
			{"textfield",  T_STR, 18, 5},
			TERM
		},
	},
	{ PCB_EGKW_SECT_NETBUSLABEL, 0xFFFF, "netbuslabel",  /* text base section equiv. */
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			{"layer", T_UBF, 3, BITFIELD(1,0,7)},
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"size",  T_INT, 12, 2},
			{"ratio", T_UBF, 14, BITFIELD(2, 2, 6)},
			/*self._get_uint8_mask(14, 0x7c) >> 2 },*/
			{"bin_rot" , T_UBF, 16, BITFIELD(2, 0, 11)},
			/*self._get_uint16_mask(16, 0x0fff)*/
			{"mirrored" , T_UBF, 16, BITFIELD(2, 12, 12)},
			/*bool(self._get_uint16_mask(16, 0x1000))*/
			{"spin" , T_UBF, 16, BITFIELD(2, 14, 14)},
			/*bool(self._get_uint16_mask(16, 0x4000))*/
			{"textfield",  T_STR, 18, 5},
			TERM
		},
	},
	{ PCB_EGKW_SECT_SMASHEDNAME, 0xFF73, "name", /* text base section equiv. */
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			{"layer", T_UBF, 3, BITFIELD(1,0,7)},
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"size",  T_INT, 12, 2},
			{"ratio", T_UBF, 14, BITFIELD(2, 2, 6)},
			/*self._get_uint8_mask(14, 0x7c) >> 2 },*/
			{"bin_rot" , T_UBF, 16, BITFIELD(2, 0, 11)},
			/*self._get_uint16_mask(16, 0x0fff)*/
			{"mirrored" , T_UBF, 16, BITFIELD(2, 12, 12)},
			/*bool(self._get_uint16_mask(16, 0x1000))*/
			{"spin" , T_UBF, 16, BITFIELD(2, 14, 14)},
			/*bool(self._get_uint16_mask(16, 0x4000))*/
			{"textfield",  T_STR, 18, 5},
			TERM
		},
	},
	{ PCB_EGKW_SECT_SMASHEDVALUE, 0xFF73, "value",  /* text base section equiv. */
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			{"layer", T_UBF, 3, BITFIELD(1,0,7)},
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"size",  T_INT, 12, 2},
			{"ratio", T_UBF, 14, BITFIELD(2, 2, 6)},
			/*self._get_uint8_mask(14, 0x7c) >> 2 },*/
			{"bin_rot" , T_UBF, 16, BITFIELD(2, 0, 11)},
			/*self._get_uint16_mask(16, 0x0fff)*/
			{"mirrored" , T_UBF, 16, BITFIELD(2, 12, 12)},
			/*bool(self._get_uint16_mask(16, 0x1000))*/
			{"spin" , T_UBF, 16, BITFIELD(2, 14, 14)},
			/*bool(self._get_uint16_mask(16, 0x4000))*/
			{"textfield",  T_STR, 18, 5},
			TERM
		},
	},
	{ PCB_EGKW_SECT_PACKAGEVARIANT, 0xFF7F, "packagevariant",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			{2, 2, SS_DIRECT, NULL},
			TERM
		},
		{ /* attributes */
			{"package", T_INT, 4, 2},
			{"table",  T_STR, 6, 13},
			{"name",  T_STR, 19, 5},
			TERM
		},
	},
	{ PCB_EGKW_SECT_DEVICE, 0xFF7F, "device",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			{2, 2, SS_RECURSIVE, "gates"}, /* gates */
			{4, 2, SS_RECURSIVE, "variants"}, /* variants */
			/* ? {2, 2, SS_DIRECT, NULL},*/
			TERM			
		},
		{ /* attributes */
			{"gates", T_INT, 2, 2},
			{"variants", T_INT, 4, 2},
			{"prefix",  T_STR, 8, 5},
			{"desc",  T_STR, 13, 5},
			{"name",  T_STR, 18, 5},
			TERM
		},
	},
	{ PCB_EGKW_SECT_PART, 0xFFFF, "part",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			{2, 2, SS_DIRECT, NULL},
			TERM
		},
		{ /* attributes */
			{"lib",  T_INT, 4, 2},
			{"device",  T_INT, 6, 2},
			{"variant",  T_INT, 8, 1},
			{"technology",  T_INT, 9, 2},
			{"name",  T_STR, 11, 5},
			{"value",  T_STR, 16, 8},
			TERM
		},
	},
	{ PCB_EGKW_SECT_SCHEMABUS, 0xFFFF },
	{ PCB_EGKW_SECT_VARIANTCONNECTIONS, 0xFF7F, "variantconnections",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			/* a sequence of 22 bytes or 11 H?, in format (x1,y1), (x2,y2), ... */
			TERM
		},
	},
	{ PCB_EGKW_SECT_SCHEMACONNECTION, 0xFFFF },
	{ PCB_EGKW_SECT_CONTACTREF, 0xFFF57, "contactref",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			{"partnumber",  T_INT, 4, 2},
			{"pin",  T_INT, 6, 2},
			TERM
		},
	},
	{ PCB_EGKW_SECT_SMASHEDPART, 0xFFFF, "smashedpart", /* same as text basesection */
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			{"layer", T_UBF, 3, BITFIELD(1,0,7)},
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"size",  T_INT, 12, 2},
			{"ratio", T_UBF, 14, BITFIELD(2, 2, 6)},
			/*self._get_uint8_mask(14, 0x7c) >> 2 },*/
			{"bin_rot" , T_UBF, 16, BITFIELD(2, 0, 11)},
			/*self._get_uint16_mask(16, 0x0fff)*/
			{"mirrored" , T_UBF, 16, BITFIELD(2, 12, 12)},
			/*bool(self._get_uint16_mask(16, 0x1000))*/
			{"spin" , T_UBF, 16, BITFIELD(2, 14, 14)},
			/*bool(self._get_uint16_mask(16, 0x4000))*/
			{"textfield",  T_STR, 18, 5},
			TERM
		},
	},
	{ PCB_EGKW_SECT_SMASHEDGATE, 0xFFFF, "smashedgate", /* same as text basesection */
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			{"layer", T_UBF, 3, BITFIELD(1,0,7)},
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"size",  T_INT, 12, 2},
			{"ratio", T_UBF, 14, BITFIELD(2, 2, 6)},
			/*self._get_uint8_mask(14, 0x7c) >> 2 },*/
			{"bin_rot" , T_UBF, 16, BITFIELD(2, 0, 11)},
			/*self._get_uint16_mask(16, 0x0fff)*/
			{"mirrored" , T_UBF, 16, BITFIELD(2, 12, 12)},
			/*bool(self._get_uint16_mask(16, 0x1000))*/
			{"spin" , T_UBF, 16, BITFIELD(2, 14, 14)},
			/*bool(self._get_uint16_mask(16, 0x4000))*/
			{"textfield",  T_STR, 18, 5},
			TERM
		},
	},
	{ PCB_EGKW_SECT_ATTRIBUTE, 0xFF7F, "attribute", /* same as text basesection */
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			{"layer", T_UBF, 3, BITFIELD(1,0,7)},
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"size",  T_INT, 12, 2},
			{"ratio", T_UBF, 14, BITFIELD(2, 2, 6)},
			/*self._get_uint8_mask(14, 0x7c) >> 2 },*/
			{"bin_rot" , T_UBF, 16, BITFIELD(2, 0, 11)},
			/*self._get_uint16_mask(16, 0x0fff)*/
			{"mirrored" , T_UBF, 16, BITFIELD(2, 12, 12)},
			/*bool(self._get_uint16_mask(16, 0x1000))*/
			{"spin" , T_UBF, 16, BITFIELD(2, 14, 14)},
			/*bool(self._get_uint16_mask(16, 0x4000))*/
			{"textfield",  T_STR, 18, 5},
			TERM
		},
	},
	{ PCB_EGKW_SECT_ATTRIBUTEVALUE, 0xFFFF, "attribute-value",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			{"symbol",  T_STR, 2, 5},
			{"attribute",  T_STR, 7, 17},
			TERM
		},
	},
	{ PCB_EGKW_SECT_FRAME, 0xFFFF, "frame",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			/*{"drawables-subsec", T_INT, 2, 2},*/
			TERM
		},
		{ /* attributes */
			{"layer",  T_UBF, 3, BITFIELD(1,0,7)},
			{"x1",  T_INT, 4, 4},
			{"y1",  T_INT, 8, 4},
			{"x2",  T_INT, 12, 4},
			{"y2",  T_INT, 16, 4},
			{"cols",  T_INT, 20, 1},
			{"rows",  T_INT, 21, 1},
			{"borders",  T_INT, 22, 1},
			TERM
		},
	},
	{ PCB_EGKW_SECT_SMASHEDXREF, 0xFFFF, "smashedxref",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			{"layer", T_UBF, 3, BITFIELD(1,0,7)},
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"size",  T_INT, 12, 2},
			{"ratio", T_UBF, 14, BITFIELD(2, 2, 6)},
			/*self._get_uint8_mask(14, 0x7c) >> 2 },*/
			{"bin_rot" , T_UBF, 16, BITFIELD(2, 0, 11)},
			/*self._get_uint16_mask(16, 0x0fff)*/
			{"mirrored" , T_UBF, 16, BITFIELD(2, 12, 12)},
			/*bool(self._get_uint16_mask(16, 0x1000))*/
			{"spin" , T_UBF, 16, BITFIELD(2, 14, 14)},
			/*bool(self._get_uint16_mask(16, 0x4000))*/
			{"textfield",  T_STR, 18, 5},
			TERM
		},
	},

	/* unknown leaves */
	{ 0x5300, 0xFFFF },

	/* end of table */
	{ 0 }
};

static unsigned long load_ulong_se(unsigned char *src, int offs, unsigned long len, int sign_extend)
{
	int n;
	unsigned long l = 0;

	if ((sign_extend) && (src[offs+len-1] & 0x80))
		l = -1;

	for(n = 0; n < len; n++) {
		l <<= 8;
		l |= src[offs+len-n-1];
	}
	return l;
}

static unsigned long load_ulong(unsigned char *src, int offs, unsigned long len)
{
	return load_ulong_se(src, offs, len, 0);
}

static long load_long(unsigned char *src, int offs, unsigned long len)
{
	return (long)load_ulong_se(src, offs, len, 1);
}

static int load_bmb(unsigned char *src, int offs, unsigned long len)
{
	return !!(src[offs] & len);
}

static unsigned long load_ubf(unsigned char *src, int offs, unsigned long field)
{
	unsigned long val;
	int len, first, last, mask;

	len = (field >> 16) & 0xff;
	first = (field >> 8) & 0xff;
	last = field & 0xff;
	mask = (1 << (first - last + 1)) - 1;

	val = load_ulong(src, offs, len);
	val >>= first;
	return val & mask;
}

static char *load_str(unsigned char *src, char *dst, int offs, unsigned long len)
{
	memcpy(dst, src+offs, len);
	dst[len] = '\0';
	return dst;
}

static double load_double(unsigned char *src, int offs, unsigned long len)
{
	double d;
	assert(len == sizeof(double));
	memcpy(&d, src+offs, sizeof(double));
	return d;
}

/* a function to convert binary format rotations into XML compatible RXXX format */
int bin_rot2degrees(const char *rot, char *tmp, int mirrored)
{
	long deg;
	char *end;
	if (rot == NULL) {
		return -1;
	} else {
		tmp[0] = 'M';
		tmp[mirrored] = 'R';
		tmp[mirrored + 1] = '0';
		tmp[mirrored + 2] = '\0'; /* default "R0" for v3,4,5 where bin_rot == 0*/
		deg = strtol(rot, &end, 10);
		/*printf("Calculated deg == %ld pre bin_rot2degree conversion\n", deg);*/
		if (*end != '\0') {
TODO(": convert this to proper error reporting")
			printf("unexpected binary field 'rot' value suffix\n");
			return -1;
		}
		if (deg >= 1024) {
			deg = (360*deg)/4096; /* v4, v5 do n*1024 */
			sprintf(&tmp[mirrored + 1], "%ld", deg);
			/*printf("Did deg == %ld bin_rot2degree conversion\n", deg);*/
			return 0;
		} else if (deg > 0) { /* v3 */
			deg = deg && 0x00f0; /* only need bottom 4 bits for v3, it seems*/
			deg = deg*90;
			/*printf("About to do deg < 1024 bin_rot2degree conversion\n");*/
			sprintf(&tmp[mirrored + 1], "%ld", deg);
			return 0;
		} else {
			/*printf("Default deg == 0 bin_rot2degree conversion\n");*/
			return 0;
		}
	}
	return -1; /* should not get here */
}

int read_notes(void *ctx, FILE *f, const char *fn, egb_ctx_t *egb_ctx)
{
	unsigned char block[8];
	unsigned char free_text[400];
	int text_remaining = 0;
	egb_ctx->free_text_len = 0;
	egb_ctx->free_text = NULL;
	egb_ctx->free_text_cursor = NULL;

	if (fread(block, 1, 8, f) != 8) {
		pcb_message(PCB_MSG_ERROR, "Short attempted free text section read. Text section not found.\n");
		return -1;
	}

	if (load_long(block, 0, 1) == 0x13 && load_long(block, 1, 1) == 0x12) {
	}
	else {
		pcb_message(PCB_MSG_ERROR, "Failed to find 0x1312 start of pre-DRC free text section.\n");
		return -1;
	}

	text_remaining = egb_ctx->free_text_len = (int)load_long(block, 4, 2);
	text_remaining += 4; /* there seems to be a 4 byte checksum or something at the end */

TODO("TODO instead of skipping the text, we need to load it completely with drc_ctx->free_text pointing to it")
	while (text_remaining > 400) {
		if (fread(free_text, 1, 400, f) != 400) {
			pcb_message(PCB_MSG_ERROR, "Short attempted free text block read. Truncated file?\n");
			return -1;
		}
		text_remaining -= 400;
	}
	if (fread(free_text, 1, text_remaining, f) != text_remaining) {
		pcb_message(PCB_MSG_ERROR, "Short attempted free text block read. Truncated file?\n");
		return -1;
	}
	return 0;
}

int read_drc(void *ctx, FILE *f, const char *fn, egb_ctx_t *drc_ctx)
{
	unsigned char block[4];
	int DRC_length_used = 244;
	unsigned char DRC_block[244];
	unsigned char c;
	int DRC_preamble_end_found = 0;

	/* these are sane, default values for DRC in case not present, i.e. in v3 no DRC section */
	drc_ctx->mdWireWire = 12;
	drc_ctx->msWidth = 10;
	drc_ctx->rvPadTop = 0.25;
	drc_ctx->rvPadInner = 0.25;
	drc_ctx->rvPadBottom = 0.25;

	if (fread(block, 1, 4, f) != 4) {
TODO(": convert this to proper error reporting")
		pcb_trace("E: short attempted DRC preamble read; preamble not found. Truncated file?\n");
		return -1;
	}

	/* look for DRC start sentinel */
	if (!(load_long(block, 0, 1) == 0x10
		&& load_long(block, 1, 1) == 0x04
		&& load_long(block, 2, 1) == 0x00
		&& load_long(block, 3, 1) == 0x20)) {
TODO(": convert this to proper error reporting")
		pcb_trace("E: start of DRC preamble not found where it was expected.\n");
		pcb_trace("E: drc byte 0 : %d\n", (int)load_long(block, 0, 1) );
		pcb_trace("E: drc byte 1 : %d\n", (int)load_long(block, 1, 1) );
		pcb_trace("E: drc byte 2 : %d\n", (int)load_long(block, 2, 1) );
		pcb_trace("E: drc byte 3 : %d\n", (int)load_long(block, 3, 1) );
		return -1;
	}

	while (!DRC_preamble_end_found) {
		if (fread(&c, 1, 1, f) != 1) { /* the text preamble is not necessarily n * 4 bytes */
TODO(": convert this to proper error reporting")
			pcb_trace("E: short attempted DRC preamble read. Truncated file?\n");
			return -1;
		} else {
			if (c == '\0') { /* so we step through, looking for each 0x00 */
				if (fread(block, 1, 4, f) != 4) { /* the text preamble seems to n * 24 bytes */
TODO(": convert this to proper error reporting")
					pcb_trace("E: short attempted DRC preamble read. Truncated file?\n");
					return -1;
				}
				if (load_long(block, 0, 1) == 0x78
					&& load_long(block, 1, 1) == 0x56
					&& load_long(block, 2, 1) == 0x34
					&& load_long(block, 3, 1) == 0x12) {
						DRC_preamble_end_found = 1;
				}
			}
		}
	}

	if (fread(DRC_block, 1, DRC_length_used, f) != DRC_length_used) {
TODO(": convert this to proper error reporting")
		pcb_trace("E: short DRC value block read. DRC section incomplete. Truncated file?\n");
		return -1;
	}

	/* first ~134 bytes contain the most useful DRC stuff, such as
	wire2wire wire2pad wire2via pad2pad pad2via via2via pad2smd via2smd smd2smd
	also, restring order: padtop padinner padbottom viaouter viainner
	(microviaouter microviainner) */

	drc_ctx->mdWireWire = (long)(load_long(DRC_block, 0, 4)/2.54/100);
	drc_ctx->wire2pad = (long)(load_long(DRC_block, 4, 4)/2.54/100);
	drc_ctx->wire2via = (long)(load_long(DRC_block, 8, 4)/2.54/100);
	drc_ctx->pad2pad = (long)(load_long(DRC_block, 12, 4)/2.54/100);
	drc_ctx->pad2via = (long)(load_long(DRC_block, 16, 4)/2.54/100);
	drc_ctx->via2via = (long)(load_long(DRC_block, 20, 4)/2.54/100);
	drc_ctx->pad2smd = (long)(load_long(DRC_block, 24, 4)/2.54/100);
	drc_ctx->via2smd = (long)(load_long(DRC_block, 28, 4)/2.54/100);
	drc_ctx->smd2smd = (long)(load_long(DRC_block, 32, 4)/2.54/100);
	drc_ctx->copper2dimension = (long)(load_long(DRC_block, 44, 4)/2.54/100);
	drc_ctx->drill2hole = (long)(load_long(DRC_block, 52, 4)/2.54/100);

	drc_ctx->msWidth = (long)(load_long(DRC_block, 64, 4)/2.54/100);
	drc_ctx->minDrill = (long)(load_long(DRC_block, 68, 4)/2.54/100);

	/*in version 5, this is wedged inbetween drill and pad ratios:
	  min_micro_via, blind_via_ratio, int, float, 12 bytes*/

	drc_ctx->rvPadTop = load_double(DRC_block, 84, 8);
	drc_ctx->rvPadInner = load_double(DRC_block, 92, 8);
	drc_ctx->rvPadBottom = load_double(DRC_block, 100, 8);

	drc_ctx->rvViaOuter = load_double(DRC_block, 108, 8);
	drc_ctx->rvViaInner = load_double(DRC_block, 116, 8);
	drc_ctx->rvMicroViaOuter = load_double(DRC_block, 124, 8);
	drc_ctx->rvMicroViaInner = load_double(DRC_block, 132, 8);

	/* not entirely sure what these do as they don't map obviously to XML */
	drc_ctx->restring_limit1_mil = (long)load_long(DRC_block, 140, 4)/2.54/100;
	drc_ctx->restring_limit2_mil = (long)load_long(DRC_block, 144, 4)/2.54/100;
	drc_ctx->restring_limit3_mil = (long)load_long(DRC_block, 148, 4)/2.54/100;
	drc_ctx->restring_limit4_mil = (long)load_long(DRC_block, 152, 4)/2.54/100;
	drc_ctx->restring_limit5_mil = (long)load_long(DRC_block, 156, 4)/2.54/100;
	drc_ctx->restring_limit6_mil = (long)load_long(DRC_block, 160, 4)/2.54/100;
	drc_ctx->restring_limit7_mil = (long)load_long(DRC_block, 164, 4)/2.54/100;
	drc_ctx->restring_limit8_mil = (long)load_long(DRC_block, 168, 4)/2.54/100;
	drc_ctx->restring_limit9_mil = (long)load_long(DRC_block, 172, 4)/2.54/100;
	drc_ctx->restring_limit10_mil = (long)load_long(DRC_block, 176, 4)/2.54/100;
	drc_ctx->restring_limit11_mil = (long)load_long(DRC_block, 180, 4)/2.54/100;
	drc_ctx->restring_limit12_mil = (long)load_long(DRC_block, 184, 4)/2.54/100;
	drc_ctx->restring_limit13_mil = (long)load_long(DRC_block, 188, 4)/2.54/100;
	drc_ctx->restring_limit14_mil = (long)load_long(DRC_block, 192, 4)/2.54/100;

	/* pad shapes */
	drc_ctx->psTop = load_long(DRC_block, 196, 4);
	drc_ctx->psBottom = load_long(DRC_block, 200, 4);
	drc_ctx->psFirst = load_long(DRC_block, 204, 4);

	/* not sure how these map to XML design rules parameters*/
	drc_ctx->mask_percentages1_ratio = load_double(DRC_block, 208, 8);
	drc_ctx->mask_percentages2_ratio = load_double(DRC_block, 216, 8);
	drc_ctx->mask_limit1_mil = (long)load_long(DRC_block, 224, 4)/2.54/100;
	drc_ctx->mask_limit2_mil = (long)load_long(DRC_block, 228, 4)/2.54/100;
	drc_ctx->mask_limit3_mil = (long)load_long(DRC_block, 232, 4)/2.54/100;
	drc_ctx->mask_limit4_mil = (long)load_long(DRC_block, 236, 4)/2.54/100;

	/* populate the drc_ctx struct to return the result of the DRC block read attempt */

	return 0;
}


int read_block(long *numblocks, int level, void *ctx, FILE *f, const char *fn, egb_node_t *parent)
{
	unsigned char block[24];
	const pcb_eagle_script_t *sc;
	const subsect_t *ss;
	const attrs_t *at;
	const fmatch_t *fm;
	char ind[256];
	int processed = 0;
	egb_node_t *lpar;

	memset(ind, ' ', level);
	ind[level] = '\0';

	/* load the current block */
	if (fread(block, 1, 24, f) != 24) {
TODO(": convert this to proper error reporting")
		pcb_trace("E: short read\n");
		return -1;
	}
	processed++;

	if (*numblocks < 0 && load_long(block, 0, 1) == 0x10) {
		*numblocks = load_long(block, 4, 4);
	}

	for(sc = pcb_eagle_script; sc->cmd != 0; sc++) {
		int match = 1;
		unsigned int cmdh = (sc->cmd >> 8) & 0xFF, cmdl = sc->cmd & 0xFF;
		unsigned int mskh = (sc->cmd_mask >> 8) & 0xFF, mskl = sc->cmd_mask & 0xFF;

		assert((cmdh & mskh) == cmdh);
		assert((cmdl & mskl) == cmdl);

		if ((cmdh != (block[0] & mskh)) || (cmdl != (block[1] & mskl)))
			continue;

		for(fm = sc->fmatch; fm->offs != 0; fm++) {
			if (load_long(block, fm->offs, fm->len) != fm->val) {
				match = 0;
				break;
			}
		}
		if (match)
			goto found;
	}

TODO(": convert this to proper error reporting")
	pcb_trace("E: unknown block ID 0x%02x%02x at offset %ld\n", block[0], block[1], ftell(f));
	return -1;

	found:;

	if (sc->name != NULL)
		parent = egb_node_append(parent, egb_node_alloc(sc->cmd, sc->name));
	else
		parent = egb_node_append(parent, egb_node_alloc(sc->cmd, "UNKNOWN"));

	for(at = sc->attrs; at->name != NULL; at++) {
		char buff[128];
		*buff = '\0';
		switch(at->type) {
			case T_BMB:
				sprintf(buff, "%d", load_bmb(block, at->offs, at->len));
				break;
			case T_UBF:
				sprintf(buff, "%ld", load_ubf(block, at->offs, at->len));
				break;
			case T_INT:
				sprintf(buff, "%ld", load_long(block, at->offs, at->len));
				break;
			case T_DBL:
				sprintf(buff, "%f", load_double(block, at->offs, at->len));
				break;
			case T_STR:
				load_str(block, buff, at->offs, at->len);
				break;
		}
		egb_node_prop_set(parent, at->name, buff);
	}

	*numblocks = *numblocks - 1;

	for(ss = sc->subs; ss->offs != 0; ss++) {
		unsigned long int n, numch = load_ulong(block, ss->offs, ss->len);

		if (ss->ss_type == SS_DIRECT) {
			long lp = 0;
			if (ss->tree_name != NULL)
				lpar = egb_node_append(parent, egb_node_alloc(0, ss->tree_name));
			else
				lpar = parent;
			for(n = 0; n < numch && *numblocks > 0; n++) {
				int res = read_block(numblocks, level+1, ctx, f, fn, lpar);
				if (res < 0)
					return res;
				processed += res;
				lp += res;
			}
		}
		else {
			long rem, lp = 0;
			if (ss->tree_name != NULL)
				lpar = egb_node_append(parent, egb_node_alloc(0, ss->tree_name));
			else
				lpar = parent;
			if (ss->ss_type == SS_RECURSIVE_MINUS_1)
				numch--;
			rem = numch;
			for(n = 0; n < numch && rem > 0; n++) {
				int res = read_block(&rem, level+1, ctx, f, fn, lpar);
				if (res < 0)
					return res;
				*numblocks -= res;
				processed += res;
				lp += res;
			}
		}
	}

	return processed;
}

static egb_node_t *find_node(egb_node_t *first, int id)
{
	egb_node_t *n;

	for(n = first; n != NULL; n = n->next)
		if (n->id == id)
			return n;

	return NULL;
}

static egb_node_t *find_node_name(egb_node_t *first, const char *id_name)
{
	egb_node_t *n;

	for(n = first; n != NULL; n = n->next)
		if (strcmp(n->id_name, id_name) == 0)
			return n;

	return NULL;
}

static int arc_decode(void *ctx, egb_node_t *elem, int arctype, int linetype)
{
	htss_entry_t *e;

	char itoa_buffer[64];
	long c, cx, cy, x1, x2, y1, y2, x3, y3, radius;
	double theta_1, theta_2, delta_theta;
	double delta_x, delta_y;
	int arc_flags = 0;
	int clockwise = 1;
	c = 0;

	if (linetype == 129 || arctype == 0) {
		arc_flags = atoi(egb_node_prop_get(elem, "arc_negflags"));
		for (e = htss_first(&elem->props); e; e = htss_next(&elem->props, e)) {
			if (strcmp(e->key, "arc_x1") == 0) {
				if (arc_flags && 0x02) {
					x1 = -atoi(e->value);
					sprintf(itoa_buffer, "%ld", -x1);
					egb_node_prop_set(elem, "x1", itoa_buffer);
				} else {
					x1 = atoi(e->value);
					egb_node_prop_set(elem, "x1", e->value);
				}
			} else if (strcmp(e->key, "arc_y1") == 0) {
				if (arc_flags && 0x04) {
					y1 = -atoi(e->value);
					sprintf(itoa_buffer, "%ld", -y1);
					egb_node_prop_set(elem, "y1", itoa_buffer);
				} else {
					y1 = atoi(e->value);
					egb_node_prop_set(elem, "y1", e->value);
				}
			} else if (strcmp(e->key, "arc_x2") == 0) {
				if (arc_flags && 0x08) {
					x2 = -atoi(e->value);
					sprintf(itoa_buffer, "%ld", -x2);
					egb_node_prop_set(elem, "x2", itoa_buffer);
				} else { 
					x2 = atoi(e->value);
					egb_node_prop_set(elem, "x2", e->value);
				}
			} else if (strcmp(e->key, "arc_y2") == 0) {
				if (arc_flags && 0x10) {
					y2 = -atoi(e->value);
					sprintf(itoa_buffer, "%ld", -y2);
					egb_node_prop_set(elem, "y2", itoa_buffer);
				} else {
					y2 = atoi(e->value);
					egb_node_prop_set(elem, "y2", e->value);
				}
			} else if (strcmp(e->key, "arc_c1") == 0) {
				c += atoi(e->value);
			} else if (strcmp(e->key, "arc_c2") == 0) {
				c += 256*atoi(e->value);
			} else if (strcmp(e->key, "arc_c3") == 0) {
				c += 256*256*atoi(e->value);
			} else if (strcmp(e->key, "arc_negflags") == 0) {
				arc_flags = atoi(e->value);
			} else if (strcmp(e->key, "clockwise") == 0) {
				clockwise = atoi(e->value);
			}
			/* add width doubling routine here */
		}
		if (arc_flags && 0x01) {
			c = -c;
		}
		x3 = (x1+x2)/2;
		y3 = (y1+y2)/2;

		if (PCB_ABS(x2-x1) < PCB_ABS(y2-y1)) {
			cx = c;
			cy = (long)((x3-cx)*(x2-x1)/(double)((y2-y1)) + y3);
		} else {
			cy = c;
			cx = (long)((y3-cy)*(y2-y1)/(double)((x2-x1)) + x3);
		}
		radius = (long)pcb_distance((double)cx, (double)cy, (double)x2, (double)y2);
		sprintf(itoa_buffer, "%ld", radius);
		egb_node_prop_set(elem, "radius", itoa_buffer);

		sprintf(itoa_buffer, "%ld", cx);
		egb_node_prop_set(elem, "x", itoa_buffer);
		sprintf(itoa_buffer, "%ld", cy);
		egb_node_prop_set(elem, "y", itoa_buffer);

		if (cx == x2 && cy == y1 && x2 < x1 && y2 > y1) { /* RUQ */
			egb_node_prop_set(elem, "StartAngle", "90");
			egb_node_prop_set(elem, "Delta", "90");
		} else if (cx == x1 && cy == y2 && x2 < x1 && y1 > y2) { /* LUQ */
			egb_node_prop_set(elem, "StartAngle", "0");
			egb_node_prop_set(elem, "Delta", "90");
		} else if (cx == x2 && cy == y1 && x2 > x1 && y1 > y2) { /* LLQ */
			egb_node_prop_set(elem, "StartAngle", "270");
			egb_node_prop_set(elem, "Delta", "90");
		} else if (cx == x1 && cy == y2 && x2 > x1 && y2 > y1) { /* RLQ */
			egb_node_prop_set(elem, "StartAngle", "180");
			egb_node_prop_set(elem, "Delta", "90");
		} else {
TODO("TODO need negative flags checked for c, x1, x2, y1, y2 > ~=838mm")
			delta_x = (double)(x1 - cx);
			delta_y = (double)(y1 - cy);
			theta_1 = PCB_RAD_TO_DEG*atan2(-delta_y, delta_x);
			theta_2 = PCB_RAD_TO_DEG*atan2(-(y2 - cy), (x2 - cx));

			theta_1 = 180 - theta_1; /* eagle coord system */
			theta_2 = 180 - theta_2; /* eagle coord system */

			delta_theta = (theta_2 - theta_1);

			if (!clockwise) {
				delta_theta = -delta_theta; 
			}

			while (theta_1 > 360) {
				theta_1 -= 360;
			}
			while (delta_theta < -180) { /* this seems to fix pathological cases */
				delta_theta += 360;
			}
			while (delta_theta > 180) {
				delta_theta -= 360;
			}
			sprintf(itoa_buffer, "%ld", (long)(theta_1));
			egb_node_prop_set(elem, "StartAngle", itoa_buffer);

			sprintf(itoa_buffer, "%ld", (long)(delta_theta));
			egb_node_prop_set(elem, "Delta", itoa_buffer);
		}
TODO("TODO still need to fine tune non-trivial non 90 degree arcs start and delta for 0x81, 0x00")
	} else if ((linetype > 0 && linetype < 129) || arctype > 0) {
		int x1_ok, x2_ok, y1_ok, y2_ok, cxy_ok;
		x1_ok = x2_ok = y1_ok = y2_ok = cxy_ok = 0;
		for (e = htss_first(&elem->props); e; e = htss_next(&elem->props, e)) {
			if (strcmp(e->key, "arctype_other_x1") == 0) {
				x1 = atoi(e->value);
				x1_ok = 1;
			} else if (strcmp(e->key, "arctype_other_y1") == 0) {
				y1 = atoi(e->value);
				y1_ok = 1;
			} else if (strcmp(e->key, "arctype_other_x2") == 0) {
				x2 = atoi(e->value);
				x2_ok = 1;
			} else if (strcmp(e->key, "arctype_other_y2") == 0) {
				y2 = atoi(e->value);
				y2_ok = 1;
			} else if (strcmp(e->key, "linetype_0_x1") == 0) {
				x1 = atoi(e->value);
				x1_ok = 1;
			} else if (strcmp(e->key, "linetype_0_y1") == 0) {
				y1 = atoi(e->value);
				y1_ok = 1;
			} else if (strcmp(e->key, "linetype_0_x2") == 0) {
				x2 = atoi(e->value);
				x2_ok = 1;
			} else if (strcmp(e->key, "linetype_0_y2") == 0) {
				y2 = atoi(e->value);
				y2_ok = 1;
			}
			
		}
		if (linetype == 0x78 || arctype == 0x01) {
			cx = MIN(x1, x2);
			cy = MIN(y1, y2);
			cxy_ok = 1;
			egb_node_prop_set(elem, "StartAngle", "180");
			egb_node_prop_set(elem, "Delta", "90");
		} else if (linetype == 0x79 || arctype == 0x02) {
			cx = MAX(x1, x2);
			cy = MIN(y1, y2);
			cxy_ok = 1;
			egb_node_prop_set(elem, "StartAngle", "270");
			egb_node_prop_set(elem, "Delta", "90");
		} else if (linetype == 0x7a || arctype == 0x03) {
			cx = MAX(x1, x2);
			cy = MAX(y1, y2);
			cxy_ok = 1;
			egb_node_prop_set(elem, "StartAngle", "0");
			egb_node_prop_set(elem, "Delta", "90");
		} else if (linetype == 0x7b || arctype == 0x04) {
			cx = MIN(x1, x2);
			cy = MAX(y1, y2);
			cxy_ok = 1;
			egb_node_prop_set(elem, "StartAngle", "90");
			egb_node_prop_set(elem, "Delta", "90");
		} else if (linetype == 0x7c || arctype == 0x05) { /* 124 */
			cx = (x1 + x2)/2;
			cy = (y1 + y2)/2;
			cxy_ok = 1;
			egb_node_prop_set(elem, "StartAngle", "90");
			egb_node_prop_set(elem, "Delta", "180");
		} else if (linetype == 0x7d || arctype == 0x06) { /* 125 */
			cx = (x1 + x2)/2;
			cy = (y1 + y2)/2;
			cxy_ok = 1;
			egb_node_prop_set(elem, "StartAngle", "270");
			egb_node_prop_set(elem, "Delta", "180");
		} else if (linetype == 0x7e || arctype == 0x07) {
			cx = (x1 + x2)/2;
			cy = (y1 + y2)/2;
			cxy_ok = 1;
			egb_node_prop_set(elem, "StartAngle", "180");
			egb_node_prop_set(elem, "Delta", "180");
		} else if (linetype == 0x7f || arctype == 0x08) {
			cx = (x1 + x2)/2;
			cy = (y1 + y2)/2;
			cxy_ok = 1;
			egb_node_prop_set(elem, "StartAngle", "0");
			egb_node_prop_set(elem, "Delta", "180");
		}

		if (!cxy_ok) {
			pcb_message(PCB_MSG_ERROR, "cx and cy not set in arc/linetype: %d/%d\n", linetype, arctype);
			cx = cy = 0;
		} else if (!(x1_ok && x2_ok && y1_ok && y2_ok)) {
			pcb_message(PCB_MSG_ERROR, "x1/2 or y1/2 not set in binary arc\n");
		}
		radius = (long)(pcb_distance((double)cx, (double)cy, (double)x2, (double)y2));
		sprintf(itoa_buffer, "%ld", radius);
		egb_node_prop_set(elem, "radius", itoa_buffer);

		sprintf(itoa_buffer, "%ld", cx);
		egb_node_prop_set(elem, "x", itoa_buffer);
		sprintf(itoa_buffer, "%ld", cy);
		egb_node_prop_set(elem, "y", itoa_buffer);

	}
	return 0;
}

/* Return the idxth library from the libraries subtree, or NULL if not found */
static egb_node_t *library_ref_by_idx(egb_node_t *libraries, long idx)
{
	egb_node_t *n;

	/* count children of libraries */
	for(n = libraries->first_child; (n != NULL) && (idx > 1); n = n->next, idx--) ;
	if (n == NULL)
		pcb_message(PCB_MSG_ERROR, "io_eagle bin: eagle_library_ref_by_idx() can't find library index %ld\n", idx);
	return n;
}

/* Return the idxth package from the library subtree, or NULL if not found */
static egb_node_t *package_ref_by_idx(egb_node_t *library, long idx)
{
	egb_node_t *n, *pkgs;

	/* find library/0x1500->packages/0x1900 node */
	for(pkgs = library->first_child; (pkgs != NULL) && ((pkgs->id & 0xFF00) != 0x1900); pkgs = pkgs->next);
	if (pkgs == NULL)
		pcb_message(PCB_MSG_ERROR, "io_eagle bin: eagle_pkg_ref_by_idx() can't find packages node in library tree\n", idx);
	/* count children of library */
	for(n = pkgs->first_child; (n != NULL) && (idx > 1); n = n->next, idx--);
	if (n == NULL)
		pcb_message(PCB_MSG_ERROR, "io_eagle bin: eagle_pkg_ref_by_idx() can't find package index %ld\n", idx);
	return n;
}

/* Return the idxth element instance from the elements subtree, or NULL if not found */
static egb_node_t *elem_ref_by_idx(egb_node_t *elements, long idx)
{
	egb_node_t *n;

	/* count children of elelements */
	for(n = elements->first_child; (n != NULL) && (idx > 1); n = n->next, idx--) ;
	if (n == NULL)
		pcb_message(PCB_MSG_ERROR, "io_eagle bin: eagle_elem_ref_by_idx() can't find element placement index %ld\n", idx);
	return n;
}

/* allocate the pin name of the j-th pin of a given element in the elements subtree to contactref node cr; allocate default of pin_number if name text not found for pin */
static int cr_pin_name_by_elem_idx(egb_node_t *cr, egb_ctx_t *egb_ctx, long elem_idx)
{
	int pin_idx = (int)(atoi(egb_node_prop_get(cr, "pin")));
	int pin_num;
	egb_node_t *p, *lib, *pkg;
	egb_node_t *elements = egb_ctx->elements;
	egb_node_t *libraries = egb_ctx->libraries;
	egb_node_t *e = elem_ref_by_idx(elements, elem_idx);

	pin_num = pin_idx;
	if (e == NULL)
		return 1;

	lib = library_ref_by_idx(libraries, atoi(egb_node_prop_get(e, "library")));
	if (lib == NULL)
		return 1;

	pkg = package_ref_by_idx(lib, atoi(egb_node_prop_get(e, "package")));
	if (pkg == NULL)
		return 1;

	for (p = pkg->first_child; (p != NULL) && (pin_num > 1) ; p = p->next) {
		if ((p->id & 0xFF00) == 0x2a00 || (p->id & 0xFF00) == 0x2b00 || (p->id & 0xFF00) == 0x2c00 ) { /* we found a pad _or_ pin _or_ SMD */
			pin_num--;
		}
	}

	if (p == NULL || pin_num > 1) {
		egb_node_prop_set(cr, "pad", "PIN_NOT_FOUND");
		return 0;
	}
	
	if (egb_node_prop_get(p, "name")) {
		egb_node_prop_set(cr, "pad", egb_node_prop_get(p, "name"));
		return 0;
	} else {
		egb_node_prop_set(cr, "pad", egb_node_prop_get(cr, "pin"));
		return 0;
	}
}


/* Return the refdes of the idxth element instance from the elements subtree, or NULL if not found */
static const char *elem_refdes_by_idx(egb_node_t *elements, long idx)
{
	egb_node_t *e = elem_ref_by_idx(elements, idx);
	if (e == NULL)
		return NULL;
	return egb_node_prop_get(e, "name");
}

/* bin path walk; the ... is a 0 terminated list of pcb_eagle_binkw_t IDs */
static egb_node_t *tree_id_path(egb_node_t *subtree, ...)
{
	egb_node_t *nd = subtree;
	pcb_eagle_binkw_t target;
	va_list ap;

	va_start(ap, subtree);

	/* get next path element */
	while((target = va_arg(ap, pcb_eagle_binkw_t)) != 0) {
		/* look for target on this level */
		for(nd = nd->first_child;;nd = nd->next) {
			if (nd == NULL) {/* target not found on this level */
				va_end(ap);
				return NULL;
			}
			if (nd->id == target) /* found, skip to next level */
				break;
		}
	}

	va_end(ap);
	return nd;
}


static int postprocess_wires(void *ctx, egb_node_t *root)
{
	htss_entry_t *e;
	egb_node_t *n;
	int line_type = -1;
	long half_width = 0; 
	char tmp[32];

	if (root->id == PCB_EGKW_SECT_LINE) {
		for (e = htss_first(&root->props); e; e = htss_next(&root->props, e))
			if (strcmp(e->key, "linetype") == 0 && strcmp(e->value, "0") == 0) {
				line_type = 0;
			} else if (strcmp(e->key, "linetype") == 0) {
				line_type = atoi(e->value);
			}
	}

	if(line_type == 0) {
		for (e = htss_first(&root->props); e; e = htss_next(&root->props, e)) {
			if (strcmp(e->key, "linetype_0_x1") == 0) {
				egb_node_prop_set(root, "x1", e->value);
			} else if (strcmp(e->key, "linetype_0_y1") == 0) {
				egb_node_prop_set(root, "y1", e->value);
			} else if (strcmp(e->key, "linetype_0_x2") == 0) {
				egb_node_prop_set(root, "x2", e->value);
			} else if (strcmp(e->key, "linetype_0_y2") == 0) {
				egb_node_prop_set(root, "y2", e->value);
			} else if (strcmp(e->key, "half_width") == 0) {
				half_width = atoi(e->value);
				sprintf(tmp, "%ld", half_width*2);
				egb_node_prop_set(root, "width", tmp);
			} /* <- added width doubling routine here */
		}
	} else {
		arc_decode(ctx, root, -1, line_type);
	}

	for(n = root->first_child; n != NULL; n = n->next)
		postprocess_wires(ctx, n);
	return 0;
}

static int postprocess_arcs(void *ctx, egb_node_t *root)
{
	htss_entry_t *e;
	egb_node_t *n;
	int arc_type = -1;
	long half_width= 0;
	char tmp[32];

	if (root->id == PCB_EGKW_SECT_ARC) {
		for (e = htss_first(&root->props); e; e = htss_next(&root->props, e))
			if (strcmp(e->key, "arctype") == 0 && strcmp(e->value, "0") == 0) {
				arc_type = 0;
			} else if (strcmp(e->key, "arctype") == 0) { /* found types 5, 3, 2 so far */
				arc_type = atoi(e->value);
			}
	}

	if (arc_type == 0) {
		for (e = htss_first(&root->props); e; e = htss_next(&root->props, e)) {
			if (strcmp(e->key, "arctype_0_x1") == 0) {
				egb_node_prop_set(root, "x1", e->value);
			} else if (strcmp(e->key, "arctype_0_y1") == 0) {
				egb_node_prop_set(root, "y1", e->value);
			} else if (strcmp(e->key, "arctype_0_x2") == 0) {
				egb_node_prop_set(root, "x2", e->value);
			} else if (strcmp(e->key, "arctype_0_y2") == 0) {
				egb_node_prop_set(root, "y2", e->value);
			} else if (strcmp(e->key, "half_width") == 0) {
				half_width = atoi(e->value);
				sprintf(tmp, "%ld", half_width*2);
				egb_node_prop_set(root, "width", tmp);
			} /* <- added width doubling routine here */
		}
	} else {
		for (e = htss_first(&root->props); e; e = htss_next(&root->props, e)) {
			if (strcmp(e->key, "arctype_other_x1") == 0) {
				egb_node_prop_set(root, "x1", e->value);
			} else if (strcmp(e->key, "arctype_other_y1") == 0) {
				egb_node_prop_set(root, "y1", e->value);
			} else if (strcmp(e->key, "arctype_other_x2") == 0) {
				egb_node_prop_set(root, "x2", e->value);
			} else if (strcmp(e->key, "arctype_other_y2") == 0) {
				egb_node_prop_set(root, "y2", e->value);
			} else if (strcmp(e->key, "half_width") == 0) {
				half_width = atoi(e->value);
				sprintf(tmp, "%ld", half_width*2);
				egb_node_prop_set(root, "width", tmp);
			} /* <- added width doubling routine here */
		}
		arc_decode(ctx, root, arc_type, -1);
	}

	for(n = root->first_child; n != NULL; n = n->next)
		postprocess_arcs(ctx, n);
	return 0;
}

/* post process circles to double their width */ 
static int postprocess_circles(void *ctx, egb_node_t *root)
{
	htss_entry_t *e;
	egb_node_t *n;
	long half_width= 0;
	char tmp[32];

	if (root->id == PCB_EGKW_SECT_CIRCLE) {
		for (e = htss_first(&root->props); e; e = htss_next(&root->props, e)) {
			if (strcmp(e->key, "half_width") == 0) {
				half_width = atoi(e->value);
				sprintf(tmp, "%ld", half_width*2);
				egb_node_prop_set(root, "width", tmp);
				break;
			} /* <- added width doubling routine here */
		}
	}

	for(n = root->first_child; n != NULL; n = n->next)
		postprocess_circles(ctx, n);
	return 0;
}

/* post process rotation to make the rotation values xml format 'RXXX' compliant */
static int postprocess_rotation(void *ctx, egb_node_t *root, int node_type)
{
	htss_entry_t *e;
	egb_node_t *n;
	char tmp[32];
	int mirrored = 0; /* default not mirrored */

	if (root != NULL && root->id == node_type) {
		for (e = htss_first(&root->props); e; e = htss_next(&root->props, e)) {
			if (e->key != NULL &&  strcmp(e->key, "mirrored") == 0) {
				if (e->value != NULL)
					mirrored = (*e->value != '0');
				break; 
			}
		}
		for (e = htss_first(&root->props); e; e = htss_next(&root->props, e)) {
			if (e->key != NULL &&  strcmp(e->key, "bin_rot") == 0) {
				bin_rot2degrees(e->value, tmp, mirrored);
				egb_node_prop_set(root, "rot", tmp);
				break; /* NB if no break here, htss_next(&root->props, e) fails */
			}
		}
	}
	for(n = root->first_child; n != NULL; n = n->next)
		postprocess_rotation(ctx, n, node_type);
	return 0;
}

/* we post process the PCB_EGKW_SECT_SMD nodes to double the dx and dy dimensions pre XML parsing */
static int postprocess_smd(void *ctx, egb_node_t *root)
{
	htss_entry_t *e;
	egb_node_t *n;
	long half_dx = 0;
	long half_dy = 0;
	char tmp[32];

	if (root != NULL && root->id == PCB_EGKW_SECT_SMD) {
		for (e = htss_first(&root->props); e; e = htss_next(&root->props, e)) {
			if (strcmp(e->key, "half_dx") == 0) {
				half_dx = atoi(e->value);
				sprintf(tmp, "%ld", half_dx*2);
				egb_node_prop_set(root, "dx", tmp);
			} else if (strcmp(e->key, "half_dy") == 0) {
				half_dy = atoi(e->value);
				sprintf(tmp, "%ld", half_dy*2);
				egb_node_prop_set(root, "dy", tmp);
			}
		}
	}

	for(n = root->first_child; n != NULL; n = n->next)
		postprocess_smd(ctx, n);
	return 0;
}

/* we post process the
PCB_EGKW_SECT_PAD and PCB_EGKW_SECT_HOLE and PCB_EGKW_SECT_VIA and PCB_EGKW_SECT_TEXT
nodes to double the drill, diameter, text height dimensions pre XML parsing */
static int postprocess_dimensions(void *ctx, egb_node_t *root)
{
	htss_entry_t *e;
	egb_node_t *n;
	long half_drill = 0;
	long half_diameter = 0;
	long half_size = 0;
	char tmp[32];
TODO("TODO padstacks - need to convert obround pins to appropriate padstack types")
	if (root != NULL && (root->id == PCB_EGKW_SECT_PAD
		|| root->id == PCB_EGKW_SECT_HOLE || root->id == PCB_EGKW_SECT_VIA
		|| root->id == PCB_EGKW_SECT_TEXT)) {
		for (e = htss_first(&root->props); e; e = htss_next(&root->props, e)) {
			if (strcmp(e->key, "half_drill") == 0) {
				half_drill = atoi(e->value);
				sprintf(tmp, "%ld", half_drill*2);
				egb_node_prop_set(root, "drill", tmp);
			} else if (strcmp(e->key, "half_diameter") == 0) {
				half_diameter = atoi(e->value);
				sprintf(tmp, "%ld", half_diameter*2);
				egb_node_prop_set(root, "diameter", tmp);
			} else if (strcmp(e->key, "half_size") == 0) {
				half_size = atoi(e->value);
				sprintf(tmp, "%ld", half_size*2);
				egb_node_prop_set(root, "size", tmp);
			}
		}
	}

	for(n = root->first_child; n != NULL; n = n->next)
		postprocess_dimensions(ctx, n);
	return 0;
}

/* look for contactrefs, and  1) append "name"="refdes" fields to contactref nodes as "element" "refdes" and 2) append "pad" = (pin_name(if present) or default pin_num) for netlist loading */
static int postproc_contactrefs(void *ctx, egb_ctx_t *egb_ctx)
{
	htss_entry_t *e;
	egb_node_t *cr, *n, *next, *next2;

	if (egb_ctx->signals == NULL)
		return 0; /* probably a library file */

	for(n = egb_ctx->signals->first_child; n != NULL; n = next) {
		next = n->next;
		if (n->first_child != NULL && n->first_child->id == PCB_EGKW_SECT_CONTACTREF) {
			for (cr = n->first_child; cr != NULL; cr = next2) {
				next2 = cr->next;
				for (e = htss_first(&cr->props); e; e = htss_next(&cr->props, e)) {
					if (strcmp(e->key, "partnumber") == 0) {
						int element_num = atoi(e->value);
						egb_node_prop_set(cr, "element", elem_refdes_by_idx(egb_ctx->elements, (long) element_num));
						cr_pin_name_by_elem_idx(cr, egb_ctx, (long) element_num);
					}
				}
			}
		}
	}
	return 0;
}


/* look for elements, and subsequent element2 blocks, and copy name/value fields to element */
static int postproc_elements(void *ctx, egb_ctx_t *egb_ctx)
{
	htss_entry_t *e;
	egb_node_t *el2, *n, *q, *next, *next2;

	if (egb_ctx->elements == NULL) 
		return 0; /* probably a library file */

	for(n = egb_ctx->firstel; n != NULL; n = next) {
		next = n->next;
		if (n->first_child && n->first_child->id == PCB_EGKW_SECT_ELEMENT2) {
			el2 = n;
			for(q = el2->first_child; q != NULL; q = next2) {
				next2 = q->next;
				for (e = htss_first(&q->props); e; e = htss_next(&q->props, e)) {
					if (strcmp(e->key, "name") == 0) {
						if (e->value != NULL && e->value[0] == '-' && e->value[1] == '\0') {
							egb_node_prop_set(n, "name", "HYPHEN");
							pcb_message(PCB_MSG_WARNING, "Substituted invalid name %s in PCB_EKGW_SECT_ELEMENT with 'HYPHEN'\n", e->value);
						} else {
							egb_node_prop_set(n, "name", e->value);
						}
					}
					else if (strcmp(e->key, "value") == 0) {
						egb_node_prop_set(n, "value", e->value);
					}
				}
			}
		}
		/* we now add element x,y fields to refdes/value element2 node */
		for (e = htss_first(&n->props); e; e = htss_next(&n->props, e)) {
			if (strcmp(e->key, "x") == 0) {
				egb_node_prop_set(el2, "x", e->value);
			}
			else if (strcmp(e->key, "y") == 0) {
				egb_node_prop_set(el2, "y", e->value);
			}
		}
		/* could potentially add default size, rot to text somewhere around here
		   or look harder for other optional nodes defining these parameters here */
	}
	return 0;
}

TODO("TODO netlist - this code flattens the signals so the XML parser finds everything, but connectivity info for nested nets is not preserved in the process #")
TODO("TODO netlist labels - eagle bin often has invalid net labels, i.e.'-', '+' so may need to filter#")
/* take any sub level signal /signals/signal1/signal2 and move it up a level to /signals/signal2 */
static int postproc_signal(void *ctx, egb_ctx_t *egb_ctx)
{
	egb_node_t *n, *p, *prev2, *next, *next2;

	egb_node_t *signal;

	if (egb_ctx->signals == NULL)
		return  0; /* probably a library */

	signal = egb_ctx->signals->first_child;

	for(n = signal; n != NULL; n = next) {
		next = n->next;
		if (n->id == PCB_EGKW_SECT_SIGNAL) {
			for(p = n->first_child, prev2 = NULL; p != NULL; p = next2) {
				next2 = p->next;
				if (p->id == PCB_EGKW_SECT_SIGNAL) {
					egb_node_unlink(n, prev2, p);
					egb_node_append(egb_ctx->signals, p);
				}
				else
					prev2 = p;
			}
		}
	}
	return 0;
}

/* populate the newly created drc node in the binary tree before XML parsing, if board present */
static int postproc_drc(void *ctx, egb_ctx_t *egb_ctx)
{
	char tmp[32];
	egb_node_t *current;

	if (egb_ctx->drc == NULL)
		return 0; /* probably a library with no board node */

	current = egb_node_append(egb_ctx->drc, egb_node_alloc(PCB_EGKW_SECT_DRC, "param"));
	sprintf(tmp, "%ldmil", egb_ctx->mdWireWire);
	egb_node_prop_set(current, "name", "mdWireWire");
	egb_node_prop_set(current, "value", tmp);

	current = egb_node_append(egb_ctx->drc, egb_node_alloc(PCB_EGKW_SECT_DRC, "param"));
	sprintf(tmp, "%ldmil", egb_ctx->msWidth);
	egb_node_prop_set(current, "name", "msWidth");
	egb_node_prop_set(current, "value", tmp);

	current = egb_node_append(egb_ctx->drc, egb_node_alloc(PCB_EGKW_SECT_DRC, "param"));
	sprintf(tmp, "%f", egb_ctx->rvPadTop);
	egb_node_prop_set(current, "name", "rvPadTop");
	egb_node_prop_set(current, "value", tmp);

	current = egb_node_append(egb_ctx->drc, egb_node_alloc(PCB_EGKW_SECT_DRC, "param"));
	sprintf(tmp, "%f", egb_ctx->rvPadInner);
	egb_node_prop_set(current, "name", "rvPadInner");
	egb_node_prop_set(current, "value", tmp);

	current = egb_node_append(egb_ctx->drc, egb_node_alloc(PCB_EGKW_SECT_DRC, "param"));
	sprintf(tmp, "%f", egb_ctx->rvPadBottom);
	egb_node_prop_set(current, "name", "rvPadBottom");
	egb_node_prop_set(current, "value", tmp);

	return 0;
}

/* take each /drawing/layer and move them into a newly created /drawing/layers/ */
static int postproc_layers(void *ctx, egb_ctx_t *egb_ctx)
{
	egb_node_t *n, *prev, *next;

	for(n = egb_ctx->drawing->first_child, prev = NULL; n != NULL; n = next) {
		next = n->next; /* need to save this because unlink() will ruin it */
		if (n->id == PCB_EGKW_SECT_LAYER) {
			egb_node_unlink(egb_ctx->drawing, prev, n);
			egb_node_append(egb_ctx->layers, n);
		}
		else
			prev = n;
	}

	return 0;
}

/* insert a library node above each packages node to match the xml */
static int postproc_libs(void *ctx, egb_ctx_t *egb_ctx)
{
	egb_node_t *n, *lib;

	if (egb_ctx->libraries == NULL) /* a library .lbr has a sole library in it, unlike a board */
		return 0;

	for(n = egb_ctx->libraries->first_child; n != NULL; n = egb_ctx->libraries->first_child) {
		if (n->id == PCB_EGKW_SECT_LIBRARY)
			break;

		if (n->id != PCB_EGKW_SECT_PACKAGES) {
			pcb_message(PCB_MSG_ERROR, "postproc_libs(): unexpected node under libraries (must be packages)\n");
			return -1;
		}

		egb_node_unlink(egb_ctx->libraries, NULL, n);
		lib = egb_node_append(egb_ctx->libraries, egb_node_alloc(PCB_EGKW_SECT_LIBRARY, "library"));
		egb_node_append(lib, n);
	}

	return 0;
}

static int postproc(void *ctx, egb_node_t *root, egb_ctx_t *drc_ctx)
{

	egb_node_t *n, *signal;
	egb_ctx_t eagle_bin_ctx;
	egb_ctx_t *egb_ctx_p;
	egb_ctx_p = &eagle_bin_ctx;

	/* this preliminary code does not assume a board node is present, i.e. could be library .lbr */
	eagle_bin_ctx.root = root;
	eagle_bin_ctx.drawing = root->first_child;
	eagle_bin_ctx.layers = egb_node_append(root, egb_node_alloc(PCB_EGKW_SECT_LAYERS, "layers"));
	eagle_bin_ctx.drc = NULL;      /* this will be looked for if board node present */
	eagle_bin_ctx.signals = NULL;  /* this will be looked for if board node present */
	eagle_bin_ctx.elements = NULL; /* this will be looked for if board node present */
	eagle_bin_ctx.libraries = NULL;/* this will be looked for if board node present */

	/* populate context with default DRC settings, since DRC is not present in binary v3 */
	eagle_bin_ctx.mdWireWire = drc_ctx->mdWireWire;
	eagle_bin_ctx.msWidth = drc_ctx->msWidth;
	eagle_bin_ctx.rvPadTop = drc_ctx->rvPadTop;
	eagle_bin_ctx.rvPadInner = drc_ctx->rvPadInner;
	eagle_bin_ctx.rvPadBottom = drc_ctx->rvPadBottom;

	eagle_bin_ctx.board = find_node(eagle_bin_ctx.drawing->first_child, PCB_EGKW_SECT_BOARD);
	if (eagle_bin_ctx.board == NULL) {
TODO(": convert this to proper error reporting")
		pcb_trace("No board node found, this may be a library file.\n");
	} else {
		/* the following code relies on the board node being present, i.e. a layout */
		/* create a drc node, since DRC block if present in binary file comes after the tree */
		eagle_bin_ctx.drc = egb_node_append(eagle_bin_ctx.board, egb_node_alloc(PCB_EGKW_SECT_DRC, "designrules"));
		eagle_bin_ctx.libraries = find_node_name(eagle_bin_ctx.board->first_child, "libraries");
		if (eagle_bin_ctx.libraries == NULL) { /* layouts have a libraries node it seems */
TODO(": convert this to proper error reporting")
			pcb_trace("Eagle binary layout is missing a board/libraries node.\n");
			return -1;
		}

		for(n = eagle_bin_ctx.board->first_child, signal = NULL; signal == NULL && n != NULL; n = n->next) {
			if (n->first_child && n->first_child->id == PCB_EGKW_SECT_SIGNAL) {
				signal = n->first_child;
				eagle_bin_ctx.signals = signal->parent;
			}
		}

		for(n = eagle_bin_ctx.board->first_child, eagle_bin_ctx.firstel = NULL;
				eagle_bin_ctx.firstel == NULL && n != NULL; n = n->next) {
			if (n->first_child && n->first_child->id == PCB_EGKW_SECT_ELEMENT) {
				eagle_bin_ctx.firstel = n->first_child;
				eagle_bin_ctx.elements = eagle_bin_ctx.firstel->parent;
			}
		}
	}

	/* after post processing layers, we populate the DRC node first, if present... */

	return postproc_layers(ctx, egb_ctx_p) || postproc_drc(ctx, egb_ctx_p)
		|| postproc_libs(ctx, egb_ctx_p) || postproc_elements(ctx, egb_ctx_p)
		|| postproc_signal(ctx, egb_ctx_p) || postproc_contactrefs(ctx, egb_ctx_p)
		|| postprocess_wires(ctx, root) || postprocess_arcs(ctx, root)
		|| postprocess_circles(ctx, root) || postprocess_smd(ctx, root)
		|| postprocess_dimensions(ctx, root)
		|| postprocess_rotation(ctx, root, PCB_EGKW_SECT_SMD)
		|| postprocess_rotation(ctx, root, PCB_EGKW_SECT_PIN)
		|| postprocess_rotation(ctx, root, PCB_EGKW_SECT_RECTANGLE)
		|| postprocess_rotation(ctx, root, PCB_EGKW_SECT_PAD)
		|| postprocess_rotation(ctx, root, PCB_EGKW_SECT_TEXT)
		|| postprocess_rotation(ctx, root, PCB_EGKW_SECT_SMASHEDVALUE)
		|| postprocess_rotation(ctx, root, PCB_EGKW_SECT_SMASHEDNAME)
		|| postprocess_rotation(ctx, root, PCB_EGKW_SECT_NETBUSLABEL)
		|| postprocess_rotation(ctx, root, PCB_EGKW_SECT_SMASHEDXREF)
		|| postprocess_rotation(ctx, root, PCB_EGKW_SECT_ATTRIBUTE)
		|| postprocess_rotation(ctx, root, PCB_EGKW_SECT_SMASHEDGATE)
		|| postprocess_rotation(ctx, root, PCB_EGKW_SECT_SMASHEDPART)
		|| postprocess_rotation(ctx, root, PCB_EGKW_SECT_INSTANCE)
		|| postprocess_rotation(ctx, root, PCB_EGKW_SECT_ELEMENT);
}

int pcb_egle_bin_load(void *ctx, FILE *f, const char *fn, egb_node_t **root)
{
	long test = -1;
	long *numblocks = &test;
	int res = 0;

	egb_ctx_t eagle_bin_ctx;

/*	pcb_trace("blocks remaining prior to function call = %ld\n", *numblocks);*/

	*root = egb_node_alloc(0, "eagle");

	res = read_block(numblocks, 1, ctx, f, fn, *root);
	if (res < 0) {
TODO(": convert this to proper error reporting")
		pcb_trace("Problem with remaining blocks... is this a library file?\n");
		return res;
	}
/*	pcb_trace("blocks remaining after outer function call = %ld (after reading %d blocks)\n\n", *numblocks, res);*/

	/* could test if < v4 as v3.xx seems to have no DRC or Netclass or Free Text end blocks */
	read_notes(ctx, f, fn, &eagle_bin_ctx);
	/* read_drc will determine sane defaults if no DRC block found */
	if (read_drc(ctx, f, fn, &eagle_bin_ctx) != 0) {
TODO(": convert this to proper error reporting")
		pcb_trace("No DRC section found, either a v3 binary file or a binary library file.\n");
	} /* we now use the eagle_bin_ctx results for post_proc */

	return postproc(ctx, *root, &eagle_bin_ctx);
}

