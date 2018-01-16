/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *
 */

/* Find galvanic/geometrical connections */

#ifndef	PCB_FIND_H
#define	PCB_FIND_H

#include <stdio.h>							/* needed to define 'FILE *' */
#include "config.h"
#include "obj_common.h"

typedef enum {
	PCB_FCT_COPPER   = 1, /* copper connection */
	PCB_FCT_INTERNAL = 2, /* element-internal connection */
	PCB_FCT_RAT      = 4, /* connected by a rat line */
	PCB_FCT_ELEMENT  = 8, /* pin/pad is part of an element whose pins/pads are being listed */
	PCB_FCT_START    = 16 /* starting object of a query */
} pcb_found_conn_type_t;

typedef void (*pcb_find_callback_t)(int current_type, void *current_ptr, int from_type, void *from_ptr,
																pcb_found_conn_type_t conn_type);


/* if not NULL, this function is called whenever something is found
   (in LookupConnections for example). The caller should save the original
   value and set that back around the call, if the callback needs to be changed.
   */
extern pcb_find_callback_t pcb_find_callback;

#define PCB_LOOKUP_FIRST	\
	(PCB_TYPE_PIN | PCB_TYPE_PAD | PCB_TYPE_PSTK)
#define PCB_LOOKUP_MORE	\
	(PCB_TYPE_VIA | PCB_TYPE_LINE | PCB_TYPE_RATLINE | PCB_TYPE_POLY | PCB_TYPE_ARC | PCB_TYPE_SUBC_PART)
#define PCB_SILK_TYPE	\
	(PCB_TYPE_LINE | PCB_TYPE_ARC | PCB_TYPE_POLY)

pcb_bool pcb_intersect_line_line(pcb_line_t *, pcb_line_t *);
pcb_bool pcb_intersect_line_arc(pcb_line_t *, pcb_arc_t *);
pcb_bool pcb_intersect_line_pin(pcb_pin_t *, pcb_line_t *);
pcb_bool pcb_intersect_line_pad(pcb_line_t *, pcb_pad_t *);
pcb_bool pcb_intersect_arc_pad(pcb_arc_t *, pcb_pad_t *);
pcb_bool pcb_is_poly_in_poly(pcb_poly_t *, pcb_poly_t *);
void pcb_lookup_element_conns(pcb_element_t *, FILE *);
void pcb_lookup_conns_to_all_elements(FILE *);
void pcb_lookup_conn(pcb_coord_t, pcb_coord_t, pcb_bool, pcb_coord_t, int);
void pcb_lookup_conn_by_pin(int type, void *ptr1);
void pcb_lookup_unused_pins(FILE *);
pcb_bool pcb_reset_found_lines_polys(pcb_bool);
pcb_bool pcb_reset_found_pins_vias_pads(pcb_bool);
pcb_bool pcb_reset_conns(pcb_bool);
void pcb_conn_lookup_init(void);
void pcb_component_lookup_init(void);
void pcb_layout_lookup_init(void);
void pcb_conn_lookup_uninit(void);
void pcb_component_lookup_uninit(void);
void pcb_layout_lookup_uninit(void);
void pcb_rat_find_hook(pcb_any_obj_t *obj, pcb_bool undo, pcb_bool AndRats);
void pcb_save_find_flag(int);
void pcb_restore_find_flag(void);
int pcb_drc_all(void);
pcb_bool pcb_is_line_in_poly(pcb_line_t *, pcb_poly_t *);
pcb_bool pcb_is_arc_in_poly(pcb_arc_t *, pcb_poly_t *);
pcb_bool pcb_is_pad_in_poly(pcb_pad_t *, pcb_poly_t *);

pcb_cardinal_t pcb_lookup_conn_by_obj(void *ctx, pcb_any_obj_t *obj, pcb_bool AndDraw, pcb_cardinal_t (*cb)(void *ctx, pcb_any_obj_t *obj));

/* find_clear.c */
pcb_bool pcb_clear_flag_on_pins_vias_pads(pcb_bool AndDraw, int flag);
pcb_bool pcb_clear_flag_on_lines_polys(pcb_bool AndDraw, int flag);

#endif
