/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2013..2023 Tibor 'Igor2' Palinkas
 *
 *  (Supported by NLnet NGI0 Entrust in 2023)
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
	RATP_CHANGE_COMP_ATTRIB,
	RATP_CHANGE_NET_ATTRIB,
	RATP_COMP_ADD,
	RATP_COMP_DEL
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

/* Clean up rats patch list: remove entries that are already solved on the
   input netlist. Returns number of entries removed or -1 on error. */
int pcb_rats_patch_cleanup_patches(pcb_board_t *pcb);

/* Unlink n from the list; if do_free is non-zero, also free fields and n */
void rats_patch_remove(pcb_board_t *pcb, pcb_ratspatch_line_t *n, int do_free);

/**** high level calls for connection add/remove ****/

long pcb_ratspatch_addconn_term(pcb_board_t *pcb, pcb_net_t *net, pcb_any_obj_t *obj);
long pcb_ratspatch_addconn_selected(pcb_board_t *pcb, pcb_data_t *parent, pcb_net_t *net);

long pcb_ratspatch_delconn_term(pcb_board_t *pcb, pcb_any_obj_t *obj);
long pcb_ratspatch_delconn_selected(pcb_board_t *pcb, pcb_data_t *parent);


/**** high level calls for subc add/remove ****/

/* Add a subc to the netlist patch of pcb if subc is not already on it.
   Returns 0 on sucess.*/
int rats_patch_add_subc(pcb_board_t *pcb, pcb_subc_t *subc, int undoable);

/* Remove a subc from the netlist patch of pcb if subc is referenced from the
   netlist. Returns 0 on sucess.*/
int rats_patch_del_subc(pcb_board_t *pcb, pcb_subc_t *subc, int force, int undoable);

/* Back annotate removal of all connections of a subc */
void rats_patch_break_subc_conns(pcb_board_t *pcb, pcb_subc_t *subc, int undoable);

/* Returns non-zero if subc is on the input netlist or on the net list patch
   (scheduled for back annotation for creating the subc) */
int rats_patch_is_subc_referenced(pcb_board_t *pcb, const char *refdes);


/**** exporter ****/

typedef enum pcb_ratspatch_fmt_e {
	PCB_RPFM_PCB,         /* original .pcb format from the early days of pcb-rnd; feature-compatible with tEDAx backann v1 */
	PCB_RPFM_BAP,         /* plain text format for gschem and old versions of sch-rnd; feature-compatible with tEDAx backann v1 */
	PCB_RPFM_BACKANN_V1,  /* tEDAx backann v1 block */
	PCB_RPFM_BACKANN_V2   /* tEDAx backann v2 block */
} pcb_ratspatch_fmt_t;

/* Special text exporter: save all patch lines as an ordered list */
int pcb_ratspatch_fexport(pcb_board_t *pcb, FILE * f, pcb_ratspatch_fmt_t fmt);

/* Generic, callback based exporter: */

                          /* EVENT ARGUMENTS */
typedef enum {            /* netn is always the net name, unless specified otherwise */
	PCB_RPE_INFO_BEGIN,     /* netn */
	PCB_RPE_INFO_TERMINAL,  /* netn; val is the terminal pin/pad name */
	PCB_RPE_INFO_END,       /* netn */

	PCB_RPE_CONN_ADD,       /* netn; val is the terminal pin/pad name */
	PCB_RPE_CONN_DEL,       /* netn; val is the terminal pin/pad name */
	PCB_RPE_COMP_ATTR_CHG,  /* netn is component name; key is the attribute name, val is the new attribute value */
	PCB_RPE_NET_ATTR_CHG,   /* netn is net name; key is the attribute name, val is the new attribute value */
	PCB_RPE_COMP_ADD,       /* netn is component name */
	PCB_RPE_COMP_DEL        /* netn is component name */
} pcb_rats_patch_export_ev_t;

/* Call cb() for each item to output; PCB_PTRE_INFO* is calculated/called only
   if need_info_lines is true; the pcb pointer is used for looking up connections */
int pcb_rats_patch_export(pcb_board_t *pcb, pcb_ratspatch_line_t *pat, rnd_bool need_info_lines, void (*cb)(void *ctx, pcb_rats_patch_export_ev_t ev, const char *netn, const char *key, const char *val), void *ctx);

#endif
