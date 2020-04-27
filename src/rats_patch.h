/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2013..2015 Tibor 'Igor2' Palinkas
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

#ifndef PCB_RATS_PATCH_H
#define PCB_RATS_PATCH_H

#include <stdio.h>
#include "board.h"

typedef enum {
	RATP_ADD_CONN,
	RATP_DEL_CONN,
	RATP_CHANGE_ATTRIB
} pcb_rats_patch_op_t;

struct pcb_ratspatch_line_s {
	pcb_rats_patch_op_t op;
	char *id;
	union {
		char *net_name;
		char *attrib_name;
	} arg1;
	union {
		char *attrib_val;
	} arg2;

	pcb_ratspatch_line_t *prev, *next;
};



/* Single-word netlist class names */
extern const char *pcb_netlist_names[PCB_NUM_NETLISTS];

/* Allocate and append a patch line to the patch list */
void pcb_ratspatch_append(pcb_board_t *pcb, pcb_rats_patch_op_t op, const char *id, const char *a1, const char *a2, int undoable);

/* Free the patch list and all memory claimed by patch list items */
void pcb_ratspatch_destroy(pcb_board_t *pcb);

/* Same as pcb_ratspatch_append() but also optimize previous entries so that
   redundant entries are removed */
void pcb_ratspatch_append_optimize(pcb_board_t *pcb, pcb_rats_patch_op_t op, const char *id, const char *a1, const char *a2);

/* Create [PCB_NETLIST_EDITED] from [PCB_NETLIST_INPUT] applying the patch */
void pcb_ratspatch_make_edited(pcb_board_t *pcb);

/* apply a single patch record on [PCB_NETLIST_EDITED]; returns non-zero on failure */
int pcb_ratspatch_apply(pcb_board_t *pcb, pcb_ratspatch_line_t * patch);

/**** exporter ****/

/* Special text exporter:
   save all patch lines as an ordered list of text lines
   if fmt is non-zero, generate pcb savefile compatible lines, else generate a back annotation patch */
int pcb_ratspatch_fexport(pcb_board_t *pcb, FILE * f, int fmt_pcb);

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
   if need_info_lines is true; the pcb pointer is used for looking up connections */
int pcb_rats_patch_export(pcb_board_t *pcb, pcb_ratspatch_line_t *pat, rnd_bool need_info_lines, void (*cb)(void *ctx, pcb_rats_patch_export_ev_t ev, const char *netn, const char *key, const char *val), void *ctx);

#endif
