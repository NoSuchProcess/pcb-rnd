/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2004 Thomas Nau
 *  15 Oct 2008 Ineiev: add different crosshair shapes
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

#ifndef PCB_OPERATION_H
#define PCB_OPERATION_H

#include "global_typedefs.h"

/* Each object operation gets an operation-context with some operation-specific
   configuration, and the board to operate on. Optionally this is the place to
   hold temporary states of a multi-call operation too. */

typedef struct {
	pcb_board_t *pcb;
	int extraflg;
	pcb_data_t *dst, *src;
} pcb_opctx_buffer_t;

typedef struct {
	pcb_board_t *pcb;
	int is_primary;  /* whether the primary parameter should be changed */
	pcb_coord_t delta;     /* change of size */
	pcb_coord_t absolute;  /* Absolute size */
} pcb_opctx_chgsize_t;

typedef struct {
	pcb_board_t *pcb;
	int is_primary;                 /* whether the primary parameter should be changed */
	pcb_angle_t delta, absolute;    /* same as above, but for angles */
} pcb_opctx_chgangle_t;

typedef struct {
	pcb_board_t *pcb;
	char *new_name;
} pcb_opctx_chgname_t;

typedef struct {
	pcb_board_t *pcb;
	int style;
} pcb_opctx_chgtherm_t;

typedef struct {
	pcb_board_t *pcb;
	pcb_coord_t DeltaX, DeltaY; /* movement vector */
} pcb_opctx_copy_t;

typedef struct {
	pcb_board_t *pcb;
	pcb_coord_t x, y;
	pcb_cardinal_t idx; /* poly point idx */
	pcb_bool last;
	pcb_bool forcible;
} pcb_opctx_insert_t;

typedef struct {
	pcb_board_t *pcb;
	pcb_coord_t dx, dy;         /* used by local routines as offset */
	pcb_layer_t *dst_layer;
	pcb_bool more_to_come;
} pcb_opctx_move_t;

typedef struct {
	pcb_board_t *pcb;
	pcb_data_t *destroy_target;
	pcb_bool bulk;                /* don't draw if part of a bulk operation */
} pcb_opctx_remove_t;

typedef struct {
	pcb_board_t *pcb;
	pcb_coord_t center_x, center_y;    /* center of rotation */
	unsigned number;             /* number of rotations */
} pcb_opctx_rotate_t;

typedef union {
	pcb_opctx_buffer_t buffer;
	pcb_opctx_chgname_t chgname;
	pcb_opctx_chgsize_t chgsize;
	pcb_opctx_chgangle_t chgangle;
	pcb_opctx_chgtherm_t chgtherm;
	pcb_opctx_copy_t copy;
	pcb_opctx_insert_t insert;
	pcb_opctx_move_t move;
	pcb_opctx_remove_t remove;
	pcb_opctx_rotate_t rotate;
} pcb_opctx_t;

/* pointer to low-level operation (copy, move and rotate) functions */
typedef struct {
	void *(*Line)(pcb_opctx_t *ctx, pcb_layer_t *, pcb_line_t *);
	void *(*Text)(pcb_opctx_t *ctx, pcb_layer_t *, pcb_text_t *);
	void *(*Polygon)(pcb_opctx_t *ctx, pcb_layer_t *, pcb_polygon_t *);
	void *(*Via)(pcb_opctx_t *ctx, pcb_pin_t *);
	void *(*Element)(pcb_opctx_t *ctx, pcb_element_t *);
	void *(*ElementName)(pcb_opctx_t *ctx, pcb_element_t *);
	void *(*Pin)(pcb_opctx_t *ctx, pcb_element_t *, pcb_pin_t *);
	void *(*Pad)(pcb_opctx_t *ctx, pcb_element_t *, pcb_pad_t *);
	void *(*LinePoint)(pcb_opctx_t *ctx, pcb_layer_t *, pcb_line_t *, pcb_point_t *);
	void *(*Point)(pcb_opctx_t *ctx, pcb_layer_t *, pcb_polygon_t *, pcb_point_t *);
	void *(*Arc)(pcb_opctx_t *ctx, pcb_layer_t *, pcb_arc_t *);
	void *(*Rat)(pcb_opctx_t *ctx, pcb_rat_t *);
} pcb_opfunc_t;

#endif
