/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#ifndef PCB_OBJ_SUBC_H
#define PCB_OBJ_SUBC_H

#include <libminuid/libminuid.h>
#include <genht/htsp.h>
#include "obj_common.h"
#include "global_typedefs.h"


typedef enum pcb_subc_cached_s {
	PCB_SUBCH_ORIGIN,
	PCB_SUBCH_X,
	PCB_SUBCH_Y,
	PCB_SUBCH_max
} pcb_subc_cached_t;

struct pcb_subc_s {
	PCB_ANYOBJECTFIELDS;
	minuid_bin_t uid;
	pcb_data_t *data;
	htsp_t terminals;
	pcb_line_t *aux_cache[PCB_SUBCH_max];
	pcb_layer_t *aux_layer;
	const char *refdes; /* cached from attributes for fast lookup */
	gdl_elem_t link;
};

pcb_subc_t *pcb_subc_alloc(void);
pcb_subc_t *pcb_subc_new(void);
void pcb_subc_free(pcb_subc_t *sc);

void pcb_add_subc_to_data(pcb_data_t *dt, pcb_subc_t *sc);

void pcb_subc_select(pcb_board_t *pcb, pcb_subc_t *sc, pcb_change_flag_t how, int redraw);

void pcb_subc_bbox(pcb_subc_t *sc);

/* convert buffer contents into a subcircuit, in-place; returns 0 on success */
int pcb_subc_convert_from_buffer(pcb_buffer_t *buffer);
pcb_bool pcb_subc_smash_buffer(pcb_buffer_t *buff);

void pcb_subc_mirror(pcb_data_t *data, pcb_subc_t *subc, pcb_coord_t y_offs, pcb_bool smirror);

/* changes the side of the board an element is on; returns pcb_true if done */
pcb_bool pcb_subc_change_side(pcb_subc_t **subc, pcb_coord_t yoff);

void pcb_subc_rotate(pcb_subc_t *subc, pcb_coord_t cx, pcb_coord_t cy, double cosa, double sina, double angle);
void pcb_subc_rotate90(pcb_subc_t *subc, pcb_coord_t cx, pcb_coord_t cy, int steps);

/* High level move (op wrapper; no undo) */
void pcb_subc_move(pcb_subc_t *sc, pcb_coord_t dx, pcb_coord_t dy, pcb_bool more_to_come);

/* changes the side of all selected and visible subcs; returns pcb_true if anything has changed */
pcb_bool pcb_selected_subc_change_side(void);

/* Draw a subcircuit for a preview (silk, copper and outline only) */
void pcb_subc_draw_preview(const pcb_subc_t *sc, const pcb_box_t *drawn_area);

void XORDrawSubc(pcb_subc_t *sc, pcb_coord_t DX, pcb_coord_t DY, int use_curr_side);

/* Redo the binding after the layer binding recipe changed in sc */
int pcb_subc_rebind(pcb_board_t *pcb, pcb_subc_t *sc);

#include "rtree.h"
pcb_r_dir_t draw_subc_mark_callback(const pcb_box_t *b, void *cl);
void DrawSubc(pcb_subc_t *sc);
void EraseSubc(pcb_subc_t *sc);

/* calculate geometrical properties using the aux layer; return 0 on success */
int pcb_subc_get_origin(pcb_subc_t *sc, pcb_coord_t *x, pcb_coord_t *y);
int pcb_subc_get_rotation(pcb_subc_t *sc, double *rot);
int pcb_subc_get_side(pcb_subc_t *sc, int *on_bottom);

/* Search for the named subc; name is relative path in hierarchy. Returns
   NULL if not found */
pcb_subc_t *pcb_subc_by_refdes(pcb_data_t *base, const char *name);

/* Search subc, "recursively", by ID */
pcb_subc_t *pcb_subc_by_id(pcb_data_t *base, long int ID);

/* undoable remove */
void *pcb_subc_remove(pcb_subc_t *sc);

/* In board mode return brd_layer; in footprint edit mode, return the subcircuit
   layer that matches brd_layer or brd_layer if not found. */
pcb_layer_t *pcb_loose_subc_layer(pcb_board_t *pcb, pcb_layer_t *brd_layer);

/* Returns whether there's no object in the subc */
pcb_bool pcb_subc_is_empty(pcb_subc_t *subc);

/*** subc creation helpers ***/

/* Create the aux layer for a subc, set origin to ox;oy and rotation to rot */
void pcb_subc_create_aux(pcb_subc_t *sc, pcb_coord_t ox, pcb_coord_t oy, double rot);

/* Create a new point on the aux layer using a given role string in attribute */
void pcb_subc_create_aux_point(pcb_subc_t *sc, pcb_coord_t x, pcb_coord_t y, const char *role);

/*** loops ***/

#define PCB_SUBC_LOOP(top) do {                                     \
	pcb_subc_t *subc;                                                 \
	gdl_iterator_t __it__;                                            \
	subclist_foreach(&(top)->subc, &__it__, subc) {


#endif
