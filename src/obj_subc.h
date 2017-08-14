/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

void pcb_subc_free(pcb_subc_t *sc);

void pcb_add_subc_to_data(pcb_data_t *dt, pcb_subc_t *sc);

void pcb_subc_select(pcb_board_t *pcb, pcb_subc_t *sc, pcb_change_flag_t how, int redraw);

void pcb_subc_bbox(pcb_subc_t *sc);

/* convert buffer contents into a subcircuit, in-place; returns 0 on success */
int pcb_subc_convert_from_buffer(pcb_buffer_t *buffer);
pcb_bool pcb_subc_smash_buffer(pcb_buffer_t *buff);

void pcb_subc_mirror(pcb_data_t *data, pcb_subc_t *subc, pcb_coord_t y_offs);

/* changes the side of the board an element is on; returns pcb_true if done */
pcb_bool pcb_subc_change_side(pcb_subc_t *subc, pcb_coord_t yoff);


void XORDrawSubc(pcb_subc_t *sc, pcb_coord_t DX, pcb_coord_t DY);

#include "rtree.h"
pcb_r_dir_t draw_subc_mark_callback(const pcb_box_t *b, void *cl);
void DrawSubc(pcb_subc_t *sc);
void EraseSubc(pcb_subc_t *sc);

/* calculate geometrical properties using the aux layer; return 0 on success */
int pcb_subc_get_origin(pcb_subc_t *sc, pcb_coord_t *x, pcb_coord_t *y);
int pcb_subc_get_rotation(pcb_subc_t *sc, double *rot);

/*** loops ***/

#define PCB_SUBC_LOOP(top) do {                                     \
	pcb_subc_t *subc;                                                 \
	gdl_iterator_t __it__;                                            \
	subclist_foreach(&(top)->subc, &__it__, subc) {


#endif
