/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996, 2004 Thomas Nau
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
 *
 */

#ifndef PCB_DRAW_H
#define PCB_DRAW_H

#include "config.h"
#include "hid.h"
#include "layer.h"

/* holds information about output window */
typedef struct {
	pcb_hid_t *hid;
	pcb_hid_gc_t drillGC, fgGC, pmGC;                      /* changed from some routines */
	pcb_hid_gc_t active_padGC, backpadGC, padGC, padselGC; /* pads are drawn with this gc */
	unsigned direct:1;                                     /* starts as 1 and becomes 0 before the first compositing layer group is reset */
} pcb_output_t;

extern pcb_output_t pcb_draw_out;

typedef enum {
	PCB_PHOLE_PLATED = 1,
	PCB_PHOLE_UNPLATED = 2,
	PCB_PHOLE_BB = 4
} pcb_pstk_draw_hole_t;

/* Some low level draw callback depend on this in their void *cl */
typedef struct pcb_draw_info_s {
	pcb_board_t *pcb;
	const pcb_box_t *drawn_area;
	pcb_xform_t *xform_caller;             /* the extra transformation the caller requested (the one who has initiated the rendering, e.g. throuh pcb_draw_everything()) */
	pcb_xform_t *xform_exporter;           /* the extra transformation the exporter requested (e.g. because of cam) */
	pcb_xform_t *xform;                    /* the final transformation applied on objects */

	const pcb_layer_t *layer;

	union { /* fields used for specific object callbacks */
		struct {
			pcb_layergrp_id_t gid;
			int is_current;
			pcb_pstk_draw_hole_t holetype;
			pcb_layer_combining_t comb;
			const pcb_layer_t *layer1; /* first (real) layer in the target group */

			pcb_layer_type_t shape_mask; /* when gid is invalid, use this for the shapes */
		} pstk;
		struct {
			int per_side;
		} subc;
	} objcb;
} pcb_draw_info_t;

/* Temporarily inhibid drawing if this is non-zero. A function that calls a
   lot of other functions that would call pcb_draw() a lot in turn may increase
   this value before the calls, then decrease it at the end and call pcb_draw().
   This makes sure the whole block is redrawn only once at the end. */
extern pcb_cardinal_t pcb_draw_inhibit;

#define pcb_draw_inhibit_inc() pcb_draw_inhibit++
#define pcb_draw_inhibit_dec() \
do { \
	if (pcb_draw_inhibit > 0) \
		pcb_draw_inhibit--; \
} while(0) \

/* When true, do not draw terminals but put them on the obj delay list
   using pcb_draw_delay_obj_add() - used to delay heavy terminal drawing
   to get proper overlaps */
extern pcb_bool delayed_terms_enabled;
void pcb_draw_delay_obj_add(pcb_any_obj_t *obj);

/* Append an object to the annotatation list; annotations are drawn at the
   end, after labels */
void pcb_draw_annotation_add(pcb_any_obj_t *obj);

/* the minimum box that needs to be redrawn */
extern pcb_box_t pcb_draw_invalidated;

/* Adds the update rect to the invalidated region. This schedules the object
   for redraw (by pcb_draw()). obj is anything that can be casted to pcb_box_t */
#define pcb_draw_invalidate(obj) \
do { \
	pcb_box_t *box = (pcb_box_t *)obj; \
	pcb_draw_invalidated.X1 = MIN(pcb_draw_invalidated.X1, box->X1); \
	pcb_draw_invalidated.X2 = MAX(pcb_draw_invalidated.X2, box->X2); \
	pcb_draw_invalidated.Y1 = MIN(pcb_draw_invalidated.Y1, box->Y1); \
	pcb_draw_invalidated.Y2 = MAX(pcb_draw_invalidated.Y2, box->Y2); \
} while(0)

extern int pcb_draw_force_termlab; /* force drawing terminal lables - useful for pinout previews */
extern pcb_bool pcb_draw_doing_assy;

void pcb_lighten_color(const pcb_color_t *orig, pcb_color_t *dst, double factor);

/* Draw a series of short line segments emulating a dashed line. segs is
   the number of on/off segment pairs. It is guaranteed that the line starts
   and ends with an "on" line segment. If cheap is true, allow drawing less
   segments if the line is short */
void pcb_draw_dashed_line(pcb_draw_info_t *info, pcb_hid_gc_t GC, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, unsigned int segs, pcb_bool_t cheap);


void pcb_draw(void);
void pcb_draw_obj(pcb_any_obj_t *obj);
void pcb_draw_layer(pcb_draw_info_t *info, const pcb_layer_t *ly);
void pcb_draw_layer_noxform(pcb_board_t *pcb, const pcb_layer_t *ly, const pcb_box_t *screen);

/* Same as pcb_draw_layer(), but never draws an implicit outline and ignores
   objects that are not in the subtree of data - useful for drawing a subtree,
   e.g. a subc only */
void pcb_draw_layer_under(pcb_board_t *pcb, const pcb_layer_t *Layer, const pcb_box_t *screen, pcb_data_t *data);

/* Composite draw all layer groups matching lyt/purpi/purpose */
void pcb_draw_groups(pcb_board_t *pcb, pcb_layer_type_t lyt, int purpi, char *purpose, const pcb_box_t *screen, const pcb_color_t *default_color, pcb_layer_type_t pstk_lyt_match, int thin_draw, int invert);


void pcb_erase_obj(int, void *, void *);
void pcb_draw_pstk_names(pcb_draw_info_t *info, pcb_layergrp_id_t group, const pcb_box_t *drawn_area);

/*#define PCB_BBOX_DEBUG*/

#ifdef PCB_BBOX_DEBUG
#define PCB_DRAW_BBOX(obj) \
	do { \
		pcb_hid_set_line_width(pcb_draw_out.fgGC, 0); \
		pcb_gui->draw_rect(pcb_draw_out.fgGC, obj->BoundingBox.X1, obj->BoundingBox.Y1, obj->BoundingBox.X2, obj->BoundingBox.Y2); \
	} while(0)
#else
#define PCB_DRAW_BBOX(obj)
#endif


/* Returns whether lay_id is part of a group that is composite-drawn */
int pcb_draw_layer_is_comp(pcb_layer_id_t lay_id);

/* Returns whether a group is composite-drawn */
int pcb_draw_layergrp_is_comp(const pcb_layergrp_t *g);

/* Draw (render) or invalidate a terminal label */
void pcb_term_label_draw(pcb_draw_info_t *info, pcb_coord_t x, pcb_coord_t y, double scale, pcb_bool vert, pcb_bool centered, const pcb_any_obj_t *obj);
void pcb_term_label_invalidate(pcb_coord_t x, pcb_coord_t y, double scale, pcb_bool vert, pcb_bool centered, const pcb_any_obj_t *obj);
void pcb_label_draw(pcb_draw_info_t *info, pcb_coord_t x, pcb_coord_t y, double scale, pcb_bool vert, pcb_bool centered, const char *label);
void pcb_label_invalidate(pcb_coord_t x, pcb_coord_t y, double scale, pcb_bool vert, pcb_bool centered, const char *label);


/* Schedule an object to be called again at the end for drawing its labels 
   on top of everything. */
void pcb_draw_delay_label_add(pcb_any_obj_t *obj);

/* Return non-zero if extra terminal graphics should be drawn */
#define pcb_draw_term_need_gfx(obj) \
	(((obj)->term != NULL) && !PCB_FLAG_TEST(PCB_FLAG_FOUND, (obj)) && !PCB_FLAG_TEST(PCB_FLAG_WARN, (obj)) && !PCB_FLAG_TEST(PCB_FLAG_SELECTED, (obj)))

#define pcb_draw_term_hid_permission() (pcb_gui->heavy_term_layer_ind)

#define PCB_DRAW_TERM_GFX_WIDTH (-3)

#endif
