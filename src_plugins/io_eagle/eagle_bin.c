/*
 *				COPYRIGHT
 *
 *	pcb-rnd, interactive printed circuit board design
 *	Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *	Copyright (C) 2017 Erich S. Heinzle
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *	Contact addresses for paper mail and Email:
 *	Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *	Thomas.Nau@rz.uni-ulm.de
 *
 */

/* Eagle binary tree parser */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

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
	SS_DIRECT,            /* it specifies the number of direct children */
	SS_RECURSIVE,         /* it specifies the number of all children, recursively */
	SS_RECURSIVE_MINUS_1  /* same as SS_RECURSIVE, but decrease the children count first */
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
			{"number",T_INT, 3, 1},
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
	{ PCB_EGKW_SECT_LIBRARY, 0xFFFF, "library",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			{4, 4, SS_DIRECT, NULL},
			TERM
		},
		{ /* attributes */
			{"symsubsecs", T_INT, 8, 4},
			{"pacsubsecs", T_INT, 12, 4},
			{"children", T_INT, 8, 4},
			{"name",  T_STR, 16, 8},
			TERM
		},
	},
	{ PCB_EGKW_SECT_DEVICES, 0xFFFF, "devices",
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
	{ PCB_EGKW_SECT_SYMBOL, 0xFFFF, "symbols",
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
			{"name",  T_INT, 16, 8},
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
			{"name",  T_INT, 16, 8},
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
			{"desc",  T_INT, 13, 5},
			{"name",  T_INT, 18, 6},
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
			{"name",  T_INT, 18, 6},
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
			{"layer",  T_INT, 18, 1},
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
			{"layer",  T_INT, 3, 1},
			{"width",  T_INT, 20, 2},
			{"width_doubling_bin",  T_INT, 20, 2}, /* for additional thickness */
			{"stflags",  T_BMB, 22, 0x20},
			{"linetype",  T_INT, 23, 1},
			{"linetype_0_x1",  T_INT, 4, 4},
			{"linetype_0_y1",  T_INT, 8, 4},
			{"linetype_0_x2",  T_INT, 12, 4},
			{"linetype_0_y2",  T_INT, 16, 4},
			{"arc_negflags", T_INT, 19, 1},
			{"arc_c1",  T_INT, 7, 1},
			{"arc_c2",  T_INT, 11, 1},
			{"arc_c3",  T_INT, 15, 1},
			{"arc_x1",  T_INT, 4, 3},
			{"arc_y1",  T_INT, 8, 3},
			{"arc_x2",  T_INT, 12, 3},
			{"arc_y2",  T_INT, 16, 3},
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
			{"layer",  T_INT, 3, 1},
			{"width",  T_INT, 20, 2},
			{"width_doubling_bin",  T_INT, 20, 2}, /* for additional thickness */
			{"clockwise",  T_BMB, 22, 0x20},
			{"arctype",  T_INT, 23, 1},
			{"arc_negflags", T_INT, 19, 1},
			{"arc_c1",  T_INT, 7, 1},
			{"arc_c2",  T_INT, 11, 1},
			{"arc_c3",  T_INT, 15, 1},
			{"arc_x1",  T_INT, 4, 3},
			{"arc_y1",  T_INT, 8, 3},
			{"arc_x2",  T_INT, 12, 3},
			{"arc_y2",  T_INT, 16, 3},
			{"arctype_other_x1",  T_INT, 4, 4},
			{"arctype_other_y1",  T_INT, 8, 4},
			{"arctype_other_x2",  T_INT, 12, 4},
			{"arctype_other_y2",  T_INT, 16, 4},
			TERM
		},
	},
	{ PCB_EGKW_SECT_CIRCLE, 0xFF5F, "circle",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			{"layer", T_INT, 3, 1},
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"radius",  T_INT, 12, 4},
			{"width", T_INT, 20, 4},
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
			{"layer", T_INT, 3, 1},
			{"x1",  T_INT, 4, 4},
			{"y1",  T_INT, 8, 4},
			{"x2",  T_INT, 12, 4},
			{"y2",  T_INT, 16, 4},
			{"rot",  T_INT, 20, 2},
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
			{"layer", T_INT, 3, 1},
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"width_2",  T_INT, 12, 2},
			TERM
		},
	},
	{ PCB_EGKW_SECT_HOLE, 0xFF5F, "hole",
		{ /* field match */
			TERM
		},
		{ /* subsection sizes */
			TERM
		},
		{ /* attributes */
			{"x", T_INT, 4, 4},
			{"y", T_INT, 8, 4},
			{"diameter", T_INT, 12, 4},
			{"drill", T_INT, 12, 4}, /* try duplicating field */
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
			{"drill",  T_INT, 12, 2},
			{"diameter",  T_INT, 14, 2},
			{"layers",  T_INT, 16, 1}, /*not 1:1 mapping */
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
			{"drill",  T_INT, 12, 2},
			{"diameter",  T_INT, 14, 2},
			{"rot",  T_INT, 16, 2},
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
			{"layer", T_INT, 3, 1},
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"width",  T_INT, 12, 2},
			{"height",  T_INT, 14, 2},
			{"rot",  T_UBF, 16, BITFIELD(2, 0, 11)},
			{"stop",  T_BMB, 18, 0x01},
			{"cream",  T_BMB, 18, 0x02},
			{"thermals",  T_BMB, 18, 0x04},
			{"first",  T_BMB, 18, 0x08},
			{"name",  T_STR, 19, 5},
			TERM
		},
	},
	{ PCB_EGKW_SECT_PIN, 0xFFFF, "pin",
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
			{"rot",  T_UBF, 12, BITFIELD(1, 6, 7)},
			{"direction",  T_UBF, 12, BITFIELD(1, 0, 3)},
			{"swaplevel",  T_INT, 13, 1},
			{"name",  T_STR, 14, 10},
			TERM
		},
	},
	{ PCB_EGKW_SECT_GATE, 0xFFFF, "gate",
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
			{"rot",  T_UBF, 16, BITFIELD(2, 0, 11)},
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
			{"rot",  T_UBF, 16, BITFIELD(2, 10, 11)},
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
			{"layer", T_INT, 3, 1},
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"size",  T_INT, 12, 2},
			{"ratio", T_UBF, 14, BITFIELD(2, 2, 6)},
			/*self._get_uint8_mask(14, 0x7c) >> 2 },*/
			{"rot" , T_UBF, 16, BITFIELD(2, 0, 11)},
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
			{"layer", T_INT, 3, 1},
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"size",  T_INT, 12, 2},
			{"ratio", T_UBF, 14, BITFIELD(2, 2, 6)},
			/*self._get_uint8_mask(14, 0x7c) >> 2 },*/
			{"rot" , T_UBF, 16, BITFIELD(2, 0, 11)},
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
			{"layer", T_INT, 3, 1},
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"size",  T_INT, 12, 2},
			{"ratio", T_UBF, 14, BITFIELD(2, 2, 6)},
			/*self._get_uint8_mask(14, 0x7c) >> 2 },*/
			{"rot" , T_UBF, 16, BITFIELD(2, 0, 11)},
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
			{"layer", T_INT, 3, 1},
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"size",  T_INT, 12, 2},
			{"ratio", T_UBF, 14, BITFIELD(2, 2, 6)},
			/*self._get_uint8_mask(14, 0x7c) >> 2 },*/
			{"rot" , T_UBF, 16, BITFIELD(2, 0, 11)},
			/*self._get_uint16_mask(16, 0x0fff)*/
			{"mirrored" , T_UBF, 16, BITFIELD(2, 12, 12)},
			/*bool(self._get_uint16_mask(16, 0x1000))*/
			{"spin" , T_UBF, 16, BITFIELD(2, 14, 14)},
			/*bool(self._get_uint16_mask(16, 0x4000))*/
			{"textfield",  T_STR, 18, 5},
			TERM
		},
	},
	{ PCB_EGKW_SECT_PACKAGEVARIANT, 0xFFFF },
	{ PCB_EGKW_SECT_DEVICE, 0xFFFF },
	{ PCB_EGKW_SECT_PART, 0xFFFF },
	{ PCB_EGKW_SECT_SCHEMABUS, 0xFFFF },
	{ PCB_EGKW_SECT_VARIANTCONNECTIONS, 0xFFFF },
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
			{"layer", T_INT, 3, 1},
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"size",  T_INT, 12, 2},
			{"ratio", T_UBF, 14, BITFIELD(2, 2, 6)},
			/*self._get_uint8_mask(14, 0x7c) >> 2 },*/
			{"rot" , T_UBF, 16, BITFIELD(2, 0, 11)},
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
			{"layer", T_INT, 3, 1},
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"size",  T_INT, 12, 2},
			{"ratio", T_UBF, 14, BITFIELD(2, 2, 6)},
			/*self._get_uint8_mask(14, 0x7c) >> 2 },*/
			{"rot" , T_UBF, 16, BITFIELD(2, 0, 11)},
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
			{"layer", T_INT, 3, 1},
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"size",  T_INT, 12, 2},
			{"ratio", T_UBF, 14, BITFIELD(2, 2, 6)},
			/*self._get_uint8_mask(14, 0x7c) >> 2 },*/
			{"rot" , T_UBF, 16, BITFIELD(2, 0, 11)},
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
			{"layer",  T_INT, 3, 1},
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
			{"layer", T_INT, 3, 1},
			{"x",  T_INT, 4, 4},
			{"y",  T_INT, 8, 4},
			{"size",  T_INT, 12, 2},
			{"ratio", T_UBF, 14, BITFIELD(2, 2, 6)},
			/*self._get_uint8_mask(14, 0x7c) >> 2 },*/
			{"rot" , T_UBF, 16, BITFIELD(2, 0, 11)},
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

int read_notes(void *ctx, FILE *f, const char *fn)
{
	unsigned char block[8];
	unsigned char free_text[400];
	int notes_length = 0;
	int text_remaining = 0;

	if (fread(block, 1, 8, f) != 8) {
		printf("E: short attempted free text section read. Text section, DRC, not found.\n");
		return -1;
	}

	if (load_long(block, 0, 1) == 0x13
		&& load_long(block, 1, 1) == 0x12) {
		printf("Start of pre-DRC free text section found.\n");
	} else {
		printf("Failed to find 0x1312 start of pre-DRC free text section.\n");
		return -1;
	}

	text_remaining = notes_length = (int)load_long(block, 4, 2);
	text_remaining += 4; /* there seems to be a 4 byte checksum or something at the end */
	printf("Pre-DRC free text section length remaining: %d\n", notes_length);

	while (text_remaining > 400) {
		if (fread(free_text, 1, 400, f) != 400) {
			printf("E: short attempted free text block read. Truncated file?\n");
			return -1;
		}
		text_remaining -= 400;
	}
	if (fread(free_text, 1, text_remaining, f) != text_remaining) {
		printf("E: short attempted free text block read. Truncated file?\n");
		return -1;
	} else {
		printf("Pre-DRC free text block has been read.\n");
	}
	return 0;
}

int read_drc(void *ctx, FILE *f, const char *fn)
{
	unsigned char block[4];
	int DRC_length_used = 244;
	unsigned char DRC_block[244];
	unsigned char c;
	int DRC_preamble_end_found = 0;

	if (fread(block, 1, 4, f) != 4) {
		printf("E: short attempted DRC preamble read; preamble not found. Truncated file?\n");
		return -1;
	}

	/* look for DRC start sentinel */
	if (!(load_long(block, 0, 1) == 0x10
		&& load_long(block, 1, 1) == 0x04
		&& load_long(block, 2, 1) == 0x00
		&& load_long(block, 3, 1) == 0x20)) {
		printf("E: start of DRC preamble not found where it was expected.\n");
		printf("E: drc byte 0 : %d\n", (int)load_long(block, 0, 1) );
		printf("E: drc byte 1 : %d\n", (int)load_long(block, 1, 1) );
		printf("E: drc byte 2 : %d\n", (int)load_long(block, 2, 1) );
		printf("E: drc byte 3 : %d\n", (int)load_long(block, 3, 1) );
		return -1;
	} else {
		printf("start of DRC preamble section 0x10 0x04 0x00 0x20 found.\n");
	}

	while (!DRC_preamble_end_found) {
		if (fread(&c, 1, 1, f) != 1) { /* the text preamble is not necessarily n * 4 bytes */
			printf("E: short attempted DRC preamble read. Truncated file?\n");
			return -1;
		} else {
			if (c == '\0') { /* so we step through, looking for each 0x00 */
				if (fread(block, 1, 4, f) != 4) { /* the text preamble seems to n * 24 bytes */
					printf("E: short attempted DRC preamble read. Truncated file?\n");
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

	printf("found end of DRC preamble text x78x56x34x12. About to load DRC rules.\n");

	if (fread(DRC_block, 1, DRC_length_used, f) != DRC_length_used) {
		printf("E: short DRC value block read. DRC section incomplete. Truncated file?\n");
		return -1;
	}

#warning TODO: put this in the tree instead of printing

	/* first ~134 bytes contain the most useful DRC stuff, such as
	# wire2wire wire2pad wire2via pad2pad pad2via via2via pad2smd via2smd smd2smd
	self.clearances, data = _cut('<9I', data, 36)i.e. 9 integers, 4 bytes each
	# restring order: padtop padinner padbottom viaouter viainner
	(microviaouter microviainner)
	restring_percentages = 7 doubles, 56 bytes total */
	printf("wire2wire: %f mil\n", load_long(DRC_block, 0, 4)/2.54/100);
	printf("wire2pad: %f mil\n", load_long(DRC_block, 4, 4)/2.54/100);
	printf("wire2via: %f mil\n", load_long(DRC_block, 8, 4)/2.54/100);
	printf("pad2pad: %f mil\n", load_long(DRC_block, 12, 4)/2.54/100);
	printf("pad2via: %f mil\n", load_long(DRC_block, 16, 4)/2.54/100);
	printf("via2via: %f mil\n", load_long(DRC_block, 20, 4)/2.54/100);
	printf("pad2smd: %f mil\n", load_long(DRC_block, 24, 4)/2.54/100);
	printf("via2smd: %f mil\n", load_long(DRC_block, 28, 4)/2.54/100);
	printf("smd2smd: %f mil\n", load_long(DRC_block, 32, 4)/2.54/100);
	printf("copper2dimension: %f mil\n", load_long(DRC_block, 44, 4)/2.54/100);
	printf("drill2hole: %f mil\n", load_long(DRC_block, 52, 4)/2.54/100);

	printf("min_width: %f mil\n", load_long(DRC_block, 64, 4)/2.54/100);
	printf("min_drill: %f mil\n", load_long(DRC_block, 68, 4)/2.54/100);
	/*in version 5, this is wedged inbetween drill and pad ratios:
	  min_micro_via, blind_via_ratio, int, float, 12 bytes*/
	printf("padtop ratio: %f\n", load_double(DRC_block, 84, 8));
	printf("padinner ratio: %f\n", load_double(DRC_block, 92, 8));
	printf("padbottom ratio: %f\n", load_double(DRC_block, 100, 8));
	printf("viaouter ratio: %f\n", load_double(DRC_block, 108, 8));
	printf("viainner ratio: %f\n", load_double(DRC_block, 116, 8));
	printf("microviaouter ratio: %f\n", load_double(DRC_block, 124, 8));
	printf("microviainner ratio: %f\n", load_double(DRC_block, 132, 8));

	printf("restring limit1 (mil): %f\n", load_long(DRC_block, 140, 4)/2.54/100);
	printf("restring limit2 (mil): %f\n", load_long(DRC_block, 144, 4)/2.54/100);
	printf("restring limit3 (mil): %f\n", load_long(DRC_block, 148, 4)/2.54/100);
	printf("restring limit4 (mil): %f\n", load_long(DRC_block, 152, 4)/2.54/100);
	printf("restring limit5 (mil): %f\n", load_long(DRC_block, 156, 4)/2.54/100);
	printf("restring limit6 (mil): %f\n", load_long(DRC_block, 160, 4)/2.54/100);
	printf("restring limit7 (mil): %f\n", load_long(DRC_block, 164, 4)/2.54/100);
	printf("restring limit8 (mil): %f\n", load_long(DRC_block, 168, 4)/2.54/100);
	printf("restring limit9 (mil): %f\n", load_long(DRC_block, 172, 4)/2.54/100);
	printf("restring limit10 (mil): %f\n", load_long(DRC_block, 176, 4)/2.54/100);
	printf("restring limit11 (mil): %f\n", load_long(DRC_block, 180, 4)/2.54/100);
	printf("restring limit12 (mil): %f\n", load_long(DRC_block, 184, 4)/2.54/100);
	printf("restring limit13 (mil): %f\n", load_long(DRC_block, 188, 4)/2.54/100);
	printf("restring limit14 (mil): %f\n", load_long(DRC_block, 192, 4)/2.54/100);

	printf("pad_shapes1 (equiv -1): %ld\n", load_long(DRC_block, 196, 4));
	printf("pad_shapes2 (equiv -1): %ld\n", load_long(DRC_block, 200, 4));
	printf("pad_shapes3 (equiv -1): %ld\n", load_long(DRC_block, 204, 4));
	printf("mask_percentages1 ratio: %f\n", load_double(DRC_block, 208, 8));
	printf("mask_percentages2 ratio: %f\n", load_double(DRC_block, 216, 8));

	printf("mask limit1 (mil): %f\n", load_long(DRC_block, 224, 4)/2.54/100);
	printf("mask limit2 (mil): %f\n", load_long(DRC_block, 228, 4)/2.54/100);
	printf("mask limit3 (mil): %f\n", load_long(DRC_block, 232, 4)/2.54/100);
	printf("mask limit4 (mil): %f\n", load_long(DRC_block, 236, 4)/2.54/100);

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
		printf("E: short read\n");
		return -1;
	}
	processed++;

	if (*numblocks < 0 && load_long(block, 0, 1) == 0x10) {
		*numblocks = load_long(block, 4, 4);
		printf("numblocks found in start block: %ld\n", *numblocks);
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

	printf("E: unknown block ID 0x%02x%02x at offset %ld\n", block[0], block[1], ftell(f));
	return -1;

	found:;

	if (sc->name != NULL) {
		printf("%s[%s (%x)]\n", ind, sc->name, sc->cmd);
		parent = egb_node_append(parent, egb_node_alloc(sc->cmd, sc->name));
	}
	else {
		printf("%s[UNKNOWN (%x)]\n", ind, sc->cmd);
		parent = egb_node_append(parent, egb_node_alloc(sc->cmd, "UNKNOWN"));
	}

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
		printf("%s %s=%s\n", ind, at->name, buff);
		egb_node_prop_set(parent, at->name, buff);
	}

	*numblocks = *numblocks - 1;

	for(ss = sc->subs; ss->offs != 0; ss++) {
		unsigned long int n, numch = load_ulong(block, ss->offs, ss->len);

		if (ss->ss_type == SS_DIRECT) {
			long lp = 0;
			printf("%s #About to parse %ld direct sub-blocks for %s {\n", ind, numch, ss->tree_name);
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
			printf("%s # fin, processed=%d lp=%ld }\n", ind, processed, lp);
		}
		else {
			long rem, lp = 0;
			printf("%s #About to parse %ld recursive sub-blocks for %s {\n", ind, numch, ss->tree_name);
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
			printf("%s # fin, processed=%d lp=%ld }\n", ind, processed, lp);
		}
	}

	printf("%s (blocks remaining at end of read_block routine = %ld (processed=%d))\n", ind, *numblocks, processed);

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
	egb_node_t *n;

	char itoa_buffer[64];
	long c, cx, cy, x1, x2, y1, y2, x3, y3, radius;
	double theta;
	double delta_x, delta_y;
	c = 0;

	if (linetype == 0x81 || linetype == -127 || arctype == 0) {

/*		if self.linetype == 0x81:
			# 4 4-byte fields each contain 3 bytes of x1, y1, x2, y2 respectively.
			# The 4th bytes of these fields combine to a 4-byte field whichs contains 3 byt
es of c,
			# which is the x or y coordinate of the arc center point, depending on the slop
e of the line.
			# The 4th byte of c contains flags which tell which of the fields are negative 
(two's complement)
			self._get_bytes(4, 15)

			# Extend 3-byte coordinate fields to 4 bytes, taking the negative-flags into ac
count
			negflags = self._get_uint8_mask(19, 0x1f)
			self._get_zero_mask(19, 0xe0)
			ext = ['\xff' if negflags & (1 << i) else '\x00' for i in range(5)]
			xydata = self.data[7:16:4] + ext[0] + self.data[4:7] + ext[1] + self.data[8:11]
 + ext[2] + self.data[12:15] + ext[3] + self.data[16:19] + ext[4]
			c, x1, y1, x2, y2 = struct.unpack('<iiiii', xydata)
*/
			for (e = htss_first(&elem->props); e; e = htss_next(&elem->props, e)) {
			if (strcmp(e->key, "arc_x1") == 0) {
						x1 = atoi(e->value);
						pcb_trace("arc x1: %s, %ld\n", e->value, x1);
						egb_node_prop_set(elem, "x1", e->value);
					} else if (strcmp(e->key, "arc_y1") == 0) {
						y1 = atoi(e->value);
						pcb_trace("arc y1: %s, %ld\n", e->value, y1);
						egb_node_prop_set(elem, "y1", e->value);
					} else if (strcmp(e->key, "arc_x2") == 0) {
						x2 = atoi(e->value);
						pcb_trace("arc x2: %s, %ld\n", e->value, x2);
						egb_node_prop_set(elem, "x2", e->value);
					} else if (strcmp(e->key, "arc_y2") == 0) {
						y2 = atoi(e->value);
						pcb_trace("arc y2: %s, %ld\n", e->value, y2);
						egb_node_prop_set(elem, "y2", e->value);
					} else if (strcmp(e->key, "arc_c1") == 0) {
						c += atoi(e->value);
						pcb_trace("c1: %s, %ld\n", e->value, c);
					} else if (strcmp(e->key, "arc_c2") == 0) {
						c += 256*atoi(e->value);
						pcb_trace("c2: %s, %ld\n", e->value, c);
					} else if (strcmp(e->key, "arc_c3") == 0) {
						c += 256*256*atoi(e->value);
						pcb_trace("c3: %s, %ld\n", e->value, c);
						pcb_trace("c: %ld\n", c);
					}
					/* add width doubling routine here */
			}

		x3 = (x1+x2)/2;
		y3 = (y1+y2)/2;

		if (PCB_ABS(x2-x1) < PCB_ABS(y2-y1)) {
			cx = c;
			cy = (x3-cx)*(x2-x1)/(y2-y1) + y3;
		} else {
			cy = c;
			cx = (y3-cy)*(y2-y1)/(x2-x1) + x3;
		}
		radius = (long)pcb_distance((double)cx, (double)cy, (double)x2, (double)y2);
		sprintf(itoa_buffer, "%ld", radius);
		egb_node_prop_set(elem, "radius", itoa_buffer);

		sprintf(itoa_buffer, "%ld", cx);
		egb_node_prop_set(elem, "x", itoa_buffer);
		sprintf(itoa_buffer, "%ld", cy);
		egb_node_prop_set(elem, "y", itoa_buffer);
/*
			self.x1 = x1
			self.y1 = y1
			self.x2 = x2
			self.y2 = y2
			x3, y3 = (x1+x2)/2., (y1+y2)/2.
			if abs(x2-x1) < abs(y2-y1):
				self.cx = cx = c
				self.cy = (x3-cx)*(x2-x1)/float(y2-y1)+y3
				xst, yst = '', '?'
			else:
				self.cy = cy = c
				self.cx = (y3-cy)*(y2-y1)/float(x2-x1)+x3
				xst, yst = '?', ''
*/

			if (cx == x2 && cy == y1 && x2 < x1 && y2 > y1) {
				egb_node_prop_set(elem, "StartAngle", "90");
				egb_node_prop_set(elem, "Delta", "90");
			} else if (cx == x1 && cy == y2 && x2 < x1 && y1 > y2) {
				egb_node_prop_set(elem, "StartAngle", "0");
				egb_node_prop_set(elem, "Delta", "90");
			} else if (cx == x2 && cy == y1 && x2 > x1 && y1 > y2) {
				egb_node_prop_set(elem, "StartAngle", "270");
				egb_node_prop_set(elem, "Delta", "90");
			} else if (cx == x1 && cy == y2 && x2 > x1 && y2 > y1) {
				egb_node_prop_set(elem, "StartAngle", "180");
				egb_node_prop_set(elem, "Delta", "90");
			} else {
#warning TODO probably need negative flags applied to c, x1, x2, y1, y2
				delta_x = (double)(x1 - cx);
				delta_y = (double)(y1 - cy);
				theta = atan2(delta_y, delta_x);
				theta = PCB_RAD_TO_DEG*theta;
				theta += 180; /* make it conform to pcb-rnd coord origin -ve X */
				sprintf(itoa_buffer, "%ld", (long)theta);
				egb_node_prop_set(elem, "StartAngle", itoa_buffer);

				theta = acos(((x1-cx)*(x2-cx)+(y1-cy)*(y2-cy))/(radius*radius));
				theta = PCB_RAD_TO_DEG*theta;
				if ((cy > y1 && cy > y2 && x2 < x1)
						|| (cy < y1 && cy < y2 && x2 > x1)
						|| (cx > x1 && cx > x2 && y2 > y1)
						|| (cx < x1 && cx < x2 && y1 > y2) ) {
					theta += 90;
				}
				sprintf(itoa_buffer, "%ld", (long)theta);
				egb_node_prop_set(elem, "Delta", itoa_buffer);
			}
#warning TODO still need to figure out non-trivial non 90 degree arcs start and delta for 0x81, 0x00

	} else if ((linetype > 0 && linetype != 0x81) || arctype > 0) {
			for (e = htss_first(&elem->props); e; e = htss_next(&elem->props, e)) {
					if (strcmp(e->key, "arctype_other_x1") == 0) {
						x1 = atoi(e->value);
						pcb_trace("arc x1: %s, %ld\n", e->value, x1);
					} else if (strcmp(e->key, "arctype_other_y1") == 0) {
						y1 = atoi(e->value);
						pcb_trace("arc y1: %s, %ld\n", e->value, y1);
					} else if (strcmp(e->key, "arctype_other_x2") == 0) {
						x2 = atoi(e->value);
						pcb_trace("arc x2: %s, %ld\n", e->value, x2);
					} else if (strcmp(e->key, "arctype_other_y2") == 0) {
						y2 = atoi(e->value);
						pcb_trace("arc y2: %s, %ld\n", e->value, y2);
					} else if (strcmp(e->key, "linetype_0_x1") == 0) {
						x1 = atoi(e->value);
						pcb_trace("arc x1: %s, %ld\n", e->value, x1);
					} else if (strcmp(e->key, "linetype_0_y1") == 0) {
						y1 = atoi(e->value);
						pcb_trace("arc y1: %s, %ld\n", e->value, y1);
					} else if (strcmp(e->key, "linetype_0_x2") == 0) {
						x2 = atoi(e->value);
						pcb_trace("arc x2: %s, %ld\n", e->value, x2);
					} else if (strcmp(e->key, "linetype_0_y2") == 0) {
						y2 = atoi(e->value);
						pcb_trace("arc y2: %s, %ld\n", e->value, y2);
					}
			}
/* arctype = {0x00: '', 0x01: '90 downleft', 0x02: '90 downright', 0x03: '90 upright', 0x0
4: '90 upleft', 0x05: '180 left', 0x06: '180 right', 0x07: '180 down', 0x08: '180 up'} */
			if (linetype == 0x78 || arctype == 0x01) {
				cx = MIN(x1, x2);
				cy = MIN(y1, y2);
				egb_node_prop_set(elem, "StartAngle", "90");
				egb_node_prop_set(elem, "Delta", "90");
			} else if (linetype == 0x79 || arctype == 0x02) {
				cx = MAX(x1, x2);
				cy = MIN(y1, y2);
				egb_node_prop_set(elem, "StartAngle", "0");
				egb_node_prop_set(elem, "Delta", "90");
			} else if (linetype == 0x7a || arctype == 0x03) {
				cx = MAX(x1, x2);
				cy = MAX(y1, y2);
				egb_node_prop_set(elem, "StartAngle", "270");
				egb_node_prop_set(elem, "Delta", "90");
			} else if (linetype == 0x7b || arctype == 0x04) {
				cx = MIN(x1, x2);
				cy = MAX(y1, y2);
				egb_node_prop_set(elem, "StartAngle", "180");
				egb_node_prop_set(elem, "Delta", "90");
			} else if (linetype == 0x7c || arctype == 0x05) { /* 124 */
				cx = (x1 + x2)/2;
				cy = (y1 + y2)/2;
				egb_node_prop_set(elem, "StartAngle", "90");
				egb_node_prop_set(elem, "Delta", "180");
			} else if (linetype == 0x7d || arctype == 0x06) { /* 125 */
				cx = (x1 + x2)/2;
				cy = (y1 + y2)/2;
				egb_node_prop_set(elem, "StartAngle", "270");
				egb_node_prop_set(elem, "Delta", "180");
			} else if (linetype == 0x7e || arctype == 0x07) {
				cx = (x1 + x2)/2;
				cy = (y1 + y2)/2;
				egb_node_prop_set(elem, "StartAngle", "180");
				egb_node_prop_set(elem, "Delta", "180");
			} else if (linetype == 0x7e || arctype == 0x07) {
				cx = (x1 + x2)/2;
				cy = (y1 + y2)/2;
				egb_node_prop_set(elem, "StartAngle", "0");
				egb_node_prop_set(elem, "Delta", "180");
			}

			radius = (long)(pcb_distance((double)cx, (double)cy, (double)x2, (double)y2));
			pcb_trace("Using radius for post-processed arc: %ld\n", radius);	
			sprintf(itoa_buffer, "%ld", radius);
			egb_node_prop_set(elem, "radius", itoa_buffer);

			sprintf(itoa_buffer, "%ld", cx);
			egb_node_prop_set(elem, "x", itoa_buffer);
			sprintf(itoa_buffer, "%ld", cy);
			egb_node_prop_set(elem, "y", itoa_buffer);
/*		else:

			self.x1 = self._get_int32(4)
			self.y1 = self._get_int32(8)
			self.x2 = self._get_int32(12)
			self.y2 = self._get_int32(16)

			if self.linetype == 0x78:
				self.cx = min(self.x1, self.x2)
				self.cy = min(self.y1, self.y2)
			elif self.linetype == 0x79:
				self.cx = max(self.x1, self.x2)
				self.cy = min(self.y1, self.y2)
			elif self.linetype == 0x7a:
				self.cx = max(self.x1, self.x2)
				self.cy = max(self.y1, self.y2)
			elif self.linetype == 0x7b:
				self.cx = min(self.x1, self.x2)
				self.cy = max(self.y1, self.y2)
			elif self.linetype in (0x7c, 0x7d, 0x7e, 0x7f):
				self.cx = (self.x1 + self.x2) / 2.
				self.cy = (self.y1 + self.y2) / 2.
*/
		
	}
	return 0;
}

static int postprocess_wires(void *ctx, egb_node_t *root)
{
	htss_entry_t *e;
	egb_node_t *n, *first_c, *wire_t;
	int line_type = -1;

	if (root->id == PCB_EGKW_SECT_LINE) {
		pcb_trace("######## found a line ######\n");
		for (e = htss_first(&root->props); e; e = htss_next(&root->props, e))
			if (strcmp(e->key, "linetype") == 0 && strcmp(e->value, "0") == 0) {
				line_type = 0;
				pcb_trace("Found linetype 0\n");
			} else if (strcmp(e->key, "linetype") == 0) {
				pcb_trace("Found linetype: %s\n", e->value);
				line_type = atoi(e->value);
			}
	}

	switch(line_type) {
		case 0:		/*pcb_trace("Process linetype 0\n");*/
				for (e = htss_first(&root->props); e; e = htss_next(&root->props, e)) {
					if (strcmp(e->key, "linetype_0_x1") == 0) {
						egb_node_prop_set(root, "x1", e->value);
						/*pcb_trace("Created x1: %s\n", e->value); */
					} else if (strcmp(e->key, "linetype_0_y1") == 0) {
						egb_node_prop_set(root, "y1", e->value);
						/*pcb_trace("Created y1: %s\n", e->value);*/
					} else if (strcmp(e->key, "linetype_0_x2") == 0) {
						egb_node_prop_set(root, "x2", e->value);
						/*pcb_trace("Created x2: %s\n", e->value);*/
					} else if (strcmp(e->key, "linetype_0_y2") == 0) {
						egb_node_prop_set(root, "y2", e->value);
						/*pcb_trace("Created y2: %s\n", e->value);*/
					} /* add width doubling routine here */
				}
				break;
		case 129:	pcb_trace("Process linetype 129\n");
				break;
	}

	if (line_type > 0 || line_type == -127) {
		pcb_trace("Treating linetype %d as arc########################\n", line_type);
		arc_decode(ctx, root, -1, line_type);
	}

	for(n = root->first_child; n != NULL; n = n->next)
		postprocess_wires(ctx, n);
	return 0;
}

static int postprocess_arcs(void *ctx, egb_node_t *root)
{
	htss_entry_t *e;
	egb_node_t *n, *first_c, *arc_t;
	int arc_type = -1;

	if (root->id == PCB_EGKW_SECT_ARC) {
		pcb_trace("######## found an arc ######\n");
		for (e = htss_first(&root->props); e; e = htss_next(&root->props, e))
			if (strcmp(e->key, "arctype") == 0 && strcmp(e->value, "0") == 0) {
				arc_type = 0;
				pcb_trace("Found arctype 0\n");
			} else if (strcmp(e->key, "arctype") == 0) { /* found types 5, 3, 2 so far */
				arc_type = atoi(e->value);
				pcb_trace("Found arctype: %s, %d\n", e->value, arc_type);
			}
	}

	switch(arc_type) {
		case 0:		pcb_trace("Post-processing arctype 0\n");
				for (e = htss_first(&root->props); e; e = htss_next(&root->props, e)) {
					if (strcmp(e->key, "arctype_0_x1") == 0) {
						egb_node_prop_set(root, "x1", e->value);
						pcb_trace("Created arc x1: %s\n", e->value);
					} else if (strcmp(e->key, "arctype_0_y1") == 0) {
						egb_node_prop_set(root, "y1", e->value);
						pcb_trace("Created arc y1: %s\n", e->value);
					} else if (strcmp(e->key, "arctype_0_x2") == 0) {
						egb_node_prop_set(root, "x2", e->value);
						pcb_trace("Created arc x2: %s\n", e->value);
					} else if (strcmp(e->key, "arctype_0_y2") == 0) {
						egb_node_prop_set(root, "y2", e->value);
						pcb_trace("Created arc y2: %s\n", e->value);
					} /* could add width doubling routine here */
				}
				break;
/*		case -1:	break;*/
		default:	pcb_trace("Post-processing arctype %d\n", arc_type);
				for (e = htss_first(&root->props); e; e = htss_next(&root->props, e)) {
					if (strcmp(e->key, "arctype_other_x1") == 0) {
						egb_node_prop_set(root, "x1", e->value);
						pcb_trace("Created arc x1: %s\n", e->value);
					} else if (strcmp(e->key, "arctype_other_y1") == 0) {
						egb_node_prop_set(root, "y1", e->value);
						pcb_trace("Created arc y1: %s\n", e->value);
					} else if (strcmp(e->key, "arctype_other_x2") == 0) {
						egb_node_prop_set(root, "x2", e->value);
						pcb_trace("Created arc x2: %s\n", e->value);
					} else if (strcmp(e->key, "arctype_other_y2") == 0) {
						egb_node_prop_set(root, "y2", e->value);
						pcb_trace("Created arc y2: %s\n", e->value);
					} /* could add width doubling routine here */
				}
	}
	if (arc_type >= 0) {
		pcb_trace("Proceeding with arc decode call\n");
		arc_decode(ctx, root, arc_type, -1);
	}

	for(n = root->first_child; n != NULL; n = n->next)
		postprocess_arcs(ctx, n);
	return 0;
}


/* look for elements, and subsequent element2 blocks, and copy name/value fields to element */
static int postproc_elements(void *ctx, egb_node_t *root)
{
	htss_entry_t *e;
	egb_node_t *board, *el1, *el2, *n, *q, *next, *next2;
	egb_node_t *drawing = root->first_child;

	for(n = drawing->first_child, board = NULL; board == NULL && n != NULL; n = next) {
		next = n->next;
		if (board == NULL && n->id == PCB_EGKW_SECT_BOARD) {
			pcb_trace("Found PCB_EKGW_SECT_BOARD\n");
			board = n;
		}
	}

	for(n = board->first_child, el1 = NULL; el1 == NULL && n != NULL; n = n->next) {
		pcb_trace("found board subnode ID: %d\n", n->id);
		if (n->first_child && n->first_child->id == PCB_EGKW_SECT_ELEMENT) {
			pcb_trace("Found PCB_EKGW_SECT_ELEMENT\n");
			el1 = n->first_child;
		} 
	}

	for(n = el1; n != NULL; n = next) {
		next = n->next;
		pcb_trace("inspecting el1 subnode: %d\n", n->id);
		if (n->first_child && n->first_child->id == PCB_EGKW_SECT_ELEMENT2) {
			pcb_trace("Found PCB_EKGW_SECT_ELEMENT2\n");
			el2 = n;
			for(q = el2->first_child; q != NULL; q = next2) {
				next2 = q->next;
				for (e = htss_first(&q->props); e; e = htss_next(&q->props, e)) {
					if (strcmp(e->key, "name") == 0) {
						egb_node_prop_set(n, "name", e->value);
						pcb_trace("Copied name %s to PCB_EKGW_SECT_ELEMENT\n", e->value);
					}
					else if (strcmp(e->key, "value") == 0) {
						egb_node_prop_set(n, "value", e->value);
						pcb_trace("Copied value %s to PCB_EKGW_SECT_ELEMENT\n", e->value);
					}
				}
			}
		}
		/* we now add element x,y fields to refdes/value element2 node */
		for (e = htss_first(&n->props); e; e = htss_next(&n->props, e)) {
			if (strcmp(e->key, "x") == 0) {
				egb_node_prop_set(el2, "x", e->value);
				pcb_trace("Added element x %s to PCB_EKGW_SECT_ELEMENT2\n", e->value);
			}
			else if (strcmp(e->key, "y") == 0) {
				egb_node_prop_set(el2, "y", e->value);
				pcb_trace("Added element y %s to PCB_EKGW_SECT_ELEMENT2\n", e->value);
			}
		}
		/* could potentially add default size, rot to text somewhere around here
		   or look harder for other optional nodes defining these parameters here */
	}
	return 0;
}

/* take each /drawing/layer and move them into a newly created /drawing/layers/ */
static int postproc_layers(void *ctx, egb_node_t *root)
{
	egb_node_t *n, *prev, *next;
	egb_node_t *layers = egb_node_append(root, egb_node_alloc(PCB_EGKW_SECT_LAYERS, "layers"));
	egb_node_t *drawing = root->first_child;

	for(n = drawing->first_child, prev = NULL; n != NULL; n = next) {
		next = n->next; /* need to save this because unlink() will ruin it */
		if (n->id == PCB_EGKW_SECT_LAYER) {
			egb_node_unlink(drawing, prev, n);
			egb_node_append(layers, n);
		}
		else
			prev = n;
	}

	return 0;
}

/* insert a library node above each packages node to match the xml */
static int postproc_libs(void *ctx, egb_node_t *root)
{
	egb_node_t *n, *lib;
	egb_node_t *drawing = root->first_child;
	egb_node_t *board, *libraries;

	board = find_node(drawing->first_child, PCB_EGKW_SECT_BOARD);
	if (board == NULL)
		return -1;

	libraries = find_node_name(board->first_child, "libraries");
	if (libraries == NULL)
		return -1;

	for(n = libraries->first_child; n != NULL; n = libraries->first_child) {
		if (n->id == PCB_EGKW_SECT_LIBRARY)
			break;

		if (n->id != PCB_EGKW_SECT_PACKAGES) {
			pcb_message(PCB_MSG_ERROR, "postproc_libs(): unexpected node under libraries (must be packages)\n");
			return -1;
		}

		egb_node_unlink(libraries, NULL, n);
		lib = egb_node_append(libraries, egb_node_alloc(PCB_EGKW_SECT_LIBRARY, "library"));
		egb_node_append(lib, n);
	}

	return 0;
}

static int postproc(void *ctx, egb_node_t *root)
{
	return postproc_layers(ctx, root) || postproc_libs(ctx, root)
		|| postproc_elements(ctx, root) || postprocess_wires(ctx, root) || postprocess_arcs(ctx, root);
}

int pcb_egle_bin_load(void *ctx, FILE *f, const char *fn, egb_node_t **root)
{
	long test = -1;
	long *numblocks = &test;
	int res = 0;

	printf("blocks remaining prior to function call = %ld\n", *numblocks);

	*root = egb_node_alloc(0, "eagle");

	res = read_block(numblocks, 1, ctx, f, fn, *root);
	if (res < 0) {
		return res;
	}
	printf("blocks remaining after outer function call = %ld (after reading %d blocks)\n\n", *numblocks, res);

	printf("Section blocks have been parsed. Next job is finding DRC.\n\n");

	/* need to test if < v4 as v3.xx seems to have no DRC or Netclass or Free Text sections at the end */
	read_notes(ctx, f, fn);
	read_drc(ctx, f, fn);

	return postproc(ctx, *root);
}

