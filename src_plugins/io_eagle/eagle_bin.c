/*
 *														COPYRIGHT
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "eagle_bin.h"
#include "egb_tree.h"

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
			{"stflags",  T_BMB, 22, 0x20},
			{"linetype",  T_INT, 23, 1},
			{"linetype_0_x1",  T_INT, 4, 4},
			{"linetype_0_y1",  T_INT, 8, 4},
			{"linetype_0_x2",  T_INT, 12, 4},
			{"linetype_0_y2",  T_INT, 16, 4},
			{"linetype_129_negflags", T_INT, 19, 1},
			{"linetype_129_c1",  T_INT, 7, 1},
			{"linetype_129_c2",  T_INT, 11, 1},
			{"linetype_129_c3",  T_INT, 15, 1},
			{"linetype_129_x1",  T_INT, 4, 3},
			{"linetype_129_y1",  T_INT, 8, 3},
			{"linetype_129_x2",  T_INT, 12, 3},
			{"linetype_129_y2",  T_INT, 16, 3},
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
			{"clockwise",  T_BMB, 22, 0x20},
			{"arctype",  T_INT, 23, 1},
			{"arctype_0_negflags", T_INT, 19, 1},
			{"arctype_0_c1",  T_INT, 7, 1},
			{"arctype_0_c2",  T_INT, 11, 1},
			{"arctype_0_c3",  T_INT, 15, 1},
			{"arctype_0_x1",  T_INT, 4, 3},
			{"arctype_0_y1",  T_INT, 8, 3},
			{"arctype_0_x2",  T_INT, 12, 3},
			{"arctype_0_y2",  T_INT, 16, 3},
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
			{"angle",  T_INT, 16, 2},
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
			{"angle",  T_UBF, 16, BITFIELD(2, 0, 11)},
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
			{"angle",  T_UBF, 12, BITFIELD(1, 6, 7)},
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
			{"angle",  T_UBF, 16, BITFIELD(2, 10, 11)},
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

static unsigned long load_ulong(unsigned char *src, int offs, unsigned long len)
{
	int n;
	unsigned long l = 0;
	for(n = 0; n < len; n++) {
		l <<= 8;
		l |= src[offs+len-n-1];
	}
	return l;
}

/* a bifield -> signed long conversion function is needed */

static long load_long(unsigned char *src, int offs, unsigned long len)
{
	return (long)load_ulong(src, offs, len);
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

static int postproc_layers(void *ctx, egb_node_t *root)
{
	egb_node_t *n, *p;
	egb_node_t *layers = egb_node_append(root, egb_node_alloc(PCB_EGKW_SECT_LAYERS, "layers"));
	for(n = root->first_child->first_child, p = NULL; n != NULL; p = n, n = n->next) {
		if (n->id == PCB_EGKW_SECT_LAYER) {
#warning TODO: reparent these
		}
	}
	return 0;
}

static int postproc(void *ctx, egb_node_t *root)
{
	return postproc_layers(ctx, root);
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

