/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
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
	unsigned keep_id:1;
	unsigned removing:1;
	pcb_data_t *dst, *src;

	/* internal */
	pcb_subc_t *post_subc; /* remember the extobj parent subc while removing a floater */
} pcb_opctx_buffer_t;

typedef struct {
	pcb_board_t *pcb;
	int is_primary;  /* whether the primary parameter should be changed */
	rnd_bool is_absolute;
	rnd_coord_t value;     /* delta or absolute (depending on is_absoulute) */
} pcb_opctx_chgsize_t;

typedef struct {
	pcb_board_t *pcb;
	int is_primary;                 /* whether the primary parameter should be changed */
	rnd_bool is_absolute;
	rnd_angle_t value;     /* same as above, but for angles */
} pcb_opctx_chgangle_t;

typedef struct {
	pcb_board_t *pcb;
	char *new_name;
} pcb_opctx_chgname_t;

typedef struct {
	pcb_board_t *pcb;
	int style;             /* the new bits */
	unsigned long lid;     /* the layer to operate on */
} pcb_opctx_chgtherm_t;

typedef struct {
	pcb_board_t *pcb;
	rnd_coord_t DeltaX, DeltaY; /* movement vector */
	int from_outside;
	int keep_id;
} pcb_opctx_copy_t;

typedef struct {
	pcb_board_t *pcb;
	rnd_coord_t x, y;
	rnd_cardinal_t idx; /* poly point idx */
	rnd_bool last;
	rnd_bool forcible;
} pcb_opctx_insert_t;

typedef struct {
	pcb_board_t *pcb;
	rnd_coord_t dx, dy;         /* used by local routines as offset */
	pcb_layer_t *dst_layer;
	rnd_bool more_to_come;
} pcb_opctx_move_t;

typedef struct {
	pcb_board_t *pcb;
	pcb_data_t *destroy_target;
} pcb_opctx_remove_t;

typedef struct {
	pcb_board_t *pcb;
	rnd_coord_t center_x, center_y; /* center of rotation */
	unsigned number;                /* number of rotations, for 90 deg rotation */
	double cosa, sina, angle;       /* for arbitrary angle rotation */
} pcb_opctx_rotate_t;

typedef struct {
	pcb_board_t *pcb;
	unsigned long how; /* pcb_change_flag_t */
	unsigned long flag; /* pcb_flag_values_t */
} pcb_opctx_chgflag_t;

typedef struct {
	pcb_board_t *pcb;
} pcb_opctx_noarg_t;

typedef struct {
	pcb_board_t *pcb;
	int restore;
	int clear;
} pcb_opctx_clip_t;

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
	pcb_opctx_chgflag_t chgflag;
	pcb_opctx_noarg_t noarg;
	pcb_opctx_clip_t clip;
} pcb_opctx_t;

/* pointer to low-level operation (copy, move and rotate) functions */
typedef struct {
	int (*common_pre)(pcb_opctx_t *ctx, pcb_any_obj_t *ptr2, void *ptr3);  /* called before anything else (not object-type-dependent); skip the rest of the call for the object if returns 1 */
	void (*common_post)(pcb_opctx_t *ctx, pcb_any_obj_t *ptr2, void *ptr3); /* called after everything else (not object-type-dependent) */

	void *(*Line)(pcb_opctx_t *ctx, pcb_layer_t *, pcb_line_t *);
	void *(*Text)(pcb_opctx_t *ctx, pcb_layer_t *, pcb_text_t *);
	void *(*Polygon)(pcb_opctx_t *ctx, pcb_layer_t *, pcb_poly_t *);
/*5*/
	void *(*LinePoint)(pcb_opctx_t *ctx, pcb_layer_t *, pcb_line_t *, rnd_point_t *);
	void *(*Point)(pcb_opctx_t *ctx, pcb_layer_t *, pcb_poly_t *, rnd_point_t *);
	void *(*Arc)(pcb_opctx_t *ctx, pcb_layer_t *, pcb_arc_t *);
	void *(*Gfx)(pcb_opctx_t *ctx, pcb_layer_t *, pcb_gfx_t *); 
	void *(*Rat)(pcb_opctx_t *ctx, pcb_rat_t *);
	void *(*ArcPoint)(pcb_opctx_t *ctx, pcb_layer_t *, pcb_arc_t *, int *end_id);
	void *(*subc)(pcb_opctx_t *ctx, pcb_subc_t *);
	void *(*padstack)(pcb_opctx_t *ctx, pcb_pstk_t *);

	unsigned int extobj_inhibit_regen:1;
} pcb_opfunc_t;

void *pcb_object_operation(pcb_opfunc_t *F, pcb_opctx_t *ctx, int Type, void *Ptr1, void *Ptr2, void *Ptr3);
rnd_bool pcb_selected_operation(pcb_board_t *pcb, pcb_data_t *data, pcb_opfunc_t *F, pcb_opctx_t *ctx, rnd_bool Reset, int type, rnd_bool on_locked_too);

#endif
