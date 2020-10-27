/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2020 Tibor 'Igor2' Palinkas
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

/* Follow galvanic connections through overlapping objects */

#ifndef PCB_FIND2_H
#define PCB_FIND2_H

#include <genht/htpp.h>
#include <genvector/vtp0.h>
#include "global_typedefs.h"
#include "flag.h"

typedef enum {
	PCB_FCT_COPPER   = 1, /* copper connection */
	PCB_FCT_INTCONN  = 2, /* subc internal connection, using the intconn attrib */
	PCB_FCT_RAT      = 4, /* connected between a rat line and anything else */
	PCB_FCT_START    = 8  /* starting object of a query */
} pcb_found_conn_type_t;

#define PCB_FIND_DROP_THREAD 4242

typedef struct pcb_find_s pcb_find_t;
struct pcb_find_s {
	/* public config - all-zero uses the original method, except for flag set */
	/* low level (will work with isc calls): */
	rnd_coord_t bloat;              /* perform intersection tests with one object bloated up by this amount (can be negative for shrinking) */
	unsigned ignore_clearance:1;    /* a flag dictated clearance is no excuse for intersection - useful for overlap calculation between objects on different layers */
	unsigned allow_noncopper_pstk:1;/* when 1, even non-copper shapes of padstacks will "conduct" in a pstk-pstk check (useful for the drc) */
	unsigned pstk_anylayer:1;       /* when 1, check every shape of the padstack for intersection, don't require it to be on the same layer */

	/* high level (ignored by isc calls): */
	unsigned stay_layergrp:1;       /* do not leave the layer (no padstack hop) */
	unsigned allow_noncopper:1;     /* also run on non-copper objects */
	unsigned list_found:1;          /* allow adding objects in the ->found vector */
	unsigned ignore_intconn:1;      /* do not jump terminals on subc intconn */
	unsigned consider_rats:1;       /* don't ignore rat lines, don't consider physical objects only */
	unsigned only_mark_rats:1;      /* don't ignore rat lines, find them, but do not jump over them */
	unsigned flag_chg_undoable:1;   /* when set, and flag_set or flag_clr is non-zero, put all found objects on the flag-undo before the flag change */
	unsigned long flag_set;         /* when non-zero, set the static flag bits on objects found */
	unsigned long flag_clr;         /* when non-zero, remove the static flag bits on objects found */

	/* if non-NULL, call after an object is found; if returns non-zero,
	   set ->aborted and stop the search. When search started from an object,
	   it is called for the starting object as well. All object data and ctx
	   fields are updated for new_obj before the call. arrived_from is
	   the previous object (that already triggered a callback) from which
	   new_obj was first found; can be NULL for the starting object. ctype
	   describes the relation between arrived_from and new_obj.
	   If return PCB_FIND_DROP_THREAD, stop searching this thread of objects
	   and continue elsewhere.
	   */
	int (*found_cb)(pcb_find_t *ctx, pcb_any_obj_t *new_obj, pcb_any_obj_t *arrived_from, pcb_found_conn_type_t ctype);

	/* public state/result */
	vtp0_t found;                   /* objects found, when list_found is 1 - of (pcb_any_obj_t *) */
	void *user_data;                /* filled in by caller, not read or modified by find code */

	/* private */
	vtp0_t open;                    /* objects already found but need checking for conns of (pcb_any_obj_t *) */
	pcb_data_t *data;
	pcb_board_t *pcb;
	pcb_layergrp_t *start_layergrp;

	/* marks if the object is ever found; in some cases this can be partial:
	   e.g. in some padstacks not all copper shapes are connected and there's
	   an extra bitfield for per-layer found bool; the extra per-component
	   found flags are stored in multimark */
	pcb_dynf_t mark;
	htpp_t multimark; /* key=(pcb_any_obj_t *), value=(pcb_find_mm_t *), private to find.c */

	unsigned long nfound;
	unsigned in_use:1;
	unsigned aborted:1;
	unsigned multimark_inited:1;
};

extern const pcb_find_t *pcb_find0; /* nop-configuration for calling isc functions */

unsigned long pcb_find_from_xy(pcb_find_t *ctx, pcb_data_t *data, rnd_coord_t x, rnd_coord_t y);

/* Find connections starting from an object. The obj() variant initializes the
   search; if from is NULL, don't do the initial search. The obj_next() variant
   assumes ctx is already initialized (by an obj()) and continues from that state */
unsigned long pcb_find_from_obj(pcb_find_t *ctx, pcb_data_t *data, pcb_any_obj_t *from);
unsigned long pcb_find_from_obj_next(pcb_find_t *ctx, pcb_data_t *data, pcb_any_obj_t *from);

void pcb_find_free(pcb_find_t *ctx);

/* High level intersection function: returns if a and b intersect (overlap) */
rnd_bool pcb_intersect_obj_obj(const pcb_find_t *ctx, pcb_any_obj_t *a, pcb_any_obj_t *b);

/* Low level intersection functions: */
rnd_bool pcb_isc_line_line(const pcb_find_t *ctx, pcb_line_t *Line1, pcb_line_t *Line2);
rnd_bool pcb_isc_line_arc(const pcb_find_t *ctx, pcb_line_t *Line, pcb_arc_t *Arc);
rnd_bool pcb_isc_arc_poly(const pcb_find_t *ctx, pcb_arc_t *Arc, pcb_poly_t *Polygon);
rnd_bool pcb_isc_arc_polyarea(const pcb_find_t *ctx, pcb_arc_t *Arc, rnd_polyarea_t *pa);
rnd_bool pcb_isc_line_poly(const pcb_find_t *ctx, pcb_line_t *Line, pcb_poly_t *Polygon);
rnd_bool pcb_isc_poly_poly(const pcb_find_t *ctx, pcb_poly_t *P1, pcb_poly_t *P2);
rnd_bool_t pcb_isc_pstk_line(const pcb_find_t *ctx, pcb_pstk_t *ps, pcb_line_t *line, rnd_bool anylayer);
#ifdef PCB_OBJ_PSTK_STRUCT_DECLARED
rnd_bool_t pcb_isc_pstk_line_shp(const pcb_find_t *ctx, pcb_pstk_t *ps, pcb_line_t *line, pcb_pstk_shape_t *shape);
rnd_polyarea_t *pcb_pstk_shape2polyarea(pcb_pstk_t *ps, pcb_pstk_shape_t *shape);
#endif

/* Return whether obj is marked as already visited by the current search context */
#define PCB_FIND_IS_MARKED(ctx, obj) PCB_DFLAG_TEST(&((obj)->Flags), (ctx)->mark)

#define PCB_LOOKUP_FIRST	\
	(PCB_OBJ_PSTK | PCB_OBJ_SUBC_PART)
#define PCB_LOOKUP_MORE	\
	(PCB_OBJ_LINE | PCB_OBJ_RAT | PCB_OBJ_POLY | PCB_OBJ_ARC | PCB_OBJ_GFX | PCB_OBJ_SUBC_PART)

#endif
