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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#ifndef PCB_TOPOLY_H
#define PCB_TOPOLY_H

#include "board.h"
#include "obj_common.h"
#include <libfungw/fungw.h>

typedef enum pcb_topoly_e {
	PCB_TOPOLY_KEEP_ORIG = 1,   /* keep original objects */
	PCB_TOPOLY_FLOATING = 2     /* do not add the new polygon on any layer */
} pcb_topoly_t;

typedef struct pcb_topoly_cutout_opts_s {
	rnd_coord_t pstk_min_drill_dia;   /* do not export drilled holes smaller than this */
	rnd_coord_t pstk_min_line_thick;
	unsigned pstk_omit_slot_poly:1;   /* do not export polygon shaped slots in padstacks */
	unsigned omit_pstks:1;            /* ignore padstacks */
} pcb_topoly_cutout_opts_t;

/* Convert a loop of connected objects into a polygon (with no holes); the first
   object is named in start. The _with version uses a caller provided dynamic
   flag that is not cleared within the call so multiple loops can be mapped. */
pcb_poly_t *pcb_topoly_conn(pcb_board_t *pcb, pcb_any_obj_t *start, pcb_topoly_t how);
pcb_poly_t *pcb_topoly_conn_with(pcb_board_t *pcb, pcb_any_obj_t *start, pcb_topoly_t how, pcb_dynf_t df);


/* Find the first line/arc on the outline layer from top-left */
pcb_any_obj_t *pcb_topoly_find_1st_outline(pcb_board_t *pcb);

/* Construct a polygon of the first line/arc on the outline layer from
   top-left; if df is not -1, lines and arcs are marked with df. Furthermore
   padstacks with hole/slot that cross the contour of the polygon are marked
   with df (if df is not -1) */
pcb_poly_t *pcb_topoly_1st_outline_with(pcb_board_t *pcb, pcb_topoly_t how, pcb_dynf_t df);

/* Construct a cutout (multi-island) polyarea using boundary lines/arcs and
   padstack holes/slots that are in contour (mapped using
   pcb_topoly_1st_outline_with() before this call) */
rnd_polyarea_t *pcb_topoly_cutouts_in(pcb_board_t *pcb, pcb_dynf_t df, pcb_poly_t *contour, const pcb_topoly_cutout_opts_t *opts);


/* Construct a polygon of the first line/arc on the outline layer from top-left */
pcb_poly_t *pcb_topoly_1st_outline(pcb_board_t *pcb, pcb_topoly_t how);




extern const char pcb_acts_topoly[];
extern const char pcb_acth_topoly[];
fgw_error_t pcb_act_topoly(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv);

#endif
