/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2013..2015 Tibor 'Igor2' Palinkas
 * 
 *  This module, rats.c, was written and is Copyright (C) 1997 by harry eaton
 *  this module is also subject to the GNU GPL as described below
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

/* Change History:
 * Started 6/10/97
 * Added support for minimum length rat lines 6/13/97
 * rat lines to nearest line/via 8/29/98
 * support for netlist window 10/24/98
 */

#include "global.h"

/* Single-word netlist class names */
const char *pcb_netlist_names[NUM_NETLISTS];

/* Allocate and append a patch line to the patch list */
void rats_patch_append(PCBTypePtr pcb, rats_patch_op_t op, const char *id, const char *a1, const char *a2);

/* Free the patch list and all memory claimed by patch list items */
void rats_patch_destroy(PCBTypePtr pcb);

/* Same as rats_patch_append() but also optimize previous entries so that
   redundant entries are removed */
void rats_patch_append_optimize(PCBTypePtr pcb, rats_patch_op_t op, const char *id, const char *a1, const char *a2);

/* Create [NETLIST_EDITED] from [NETLIST_INPUT] applying the patch */
void rats_patch_make_edited(PCBTypePtr pcb);

/* apply a single patch record on [NETLIST_EDITED]; returns non-zero on failure */
int rats_patch_apply(PCBTypePtr pcb, rats_patch_line_t * patch);

/**** exporter ****/

/* Special text exporter:
   save all patch lines as an ordered list of text lines
   if fmt is non-zero, generate pcb savefile compatible lines, else generate a back annotation patch */
int rats_patch_fexport(PCBTypePtr pcb, FILE * f, int fmt_pcb);

/* Generic, callback based exporter: */

                          /* EVENT ARGUMENTS */
typedef enum {            /* netn is always the net name, unless specified otherwise */
	PCB_RPE_INFO_BEGIN,     /* netn */
	PCB_RPE_INFO_TERMINAL,  /* netn; val is the terminal pin/pad name */
	PCB_RPE_INFO_END,       /* netn */

	PCB_RPE_CONN_ADD,       /* netn; val is the terminal pin/pad name */
	PCB_RPE_CONN_DEL,       /* netn; val is the terminal pin/pad name */
	PCB_RPE_ATTR_CHG        /* netn; key is the attribute name, val is the new attribute value */
} pcb_rats_patch_export_ev_t;

/* Call cb() for each item to output; PCB_PTRE_INFO* is calculated/called only
   if need_info_lines is true */
int rats_patch_export(PCBTypePtr pcb, pcb_bool_t need_info_lines, void (*cb)(void *ctx, pcb_rats_patch_export_ev_t ev, const char *netn, const char *key, const char *val), void *ctx);
