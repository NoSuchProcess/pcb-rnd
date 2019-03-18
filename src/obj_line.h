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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* Drawing primitive: line segment */

#ifndef PCB_OBJ_LINE_H
#define PCB_OBJ_LINE_H

#include <genlist/gendlist.h>
#include "obj_common.h"

struct pcb_line_s {            /* holds information about one line */
	PCB_ANYLINEFIELDS;
	gdl_elem_t link;             /* a line is in a list: either on a layer */
};

/* crosshair: */
typedef struct {								/* current marked line */
	pcb_point_t Point1,							/* start- and end-position */
	  Point2;
	long int State;
	pcb_bool draw;
} pcb_attached_line_t;


pcb_line_t *pcb_line_alloc(pcb_layer_t * layer);
pcb_line_t *pcb_line_alloc_id(pcb_layer_t *layer, long int id);
void pcb_line_free(pcb_line_t * data);

pcb_line_t *pcb_line_new_merge(pcb_layer_t *Layer, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_coord_t Thickness, pcb_coord_t Clearance, pcb_flag_t Flags);
pcb_line_t *pcb_line_new(pcb_layer_t *Layer, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_coord_t Thickness, pcb_coord_t Clearance, pcb_flag_t Flags);
pcb_line_t *pcb_line_dup(pcb_layer_t *Layer, pcb_line_t *src);
pcb_line_t *pcb_line_dup_at(pcb_layer_t *dst, pcb_line_t *src, pcb_coord_t dx, pcb_coord_t dy);
void *pcb_line_destroy(pcb_layer_t *dst, pcb_line_t *src);

void pcb_line_reg(pcb_layer_t *layer, pcb_line_t *line);
void pcb_line_unreg(pcb_line_t *line);

/* Add objects without creating them or making any "sanity modifications" to them */
void pcb_add_line_on_layer(pcb_layer_t *Layer, pcb_line_t *Line);

void pcb_line_bbox(pcb_line_t *Line);
void pcb_line_rotate90(pcb_line_t *Line, pcb_coord_t X, pcb_coord_t Y, unsigned Number);
void pcb_line_rotate(pcb_layer_t *layer, pcb_line_t *line, pcb_coord_t X, pcb_coord_t Y, double cosa, double sina);
void pcb_line_mirror(pcb_layer_t *layer, pcb_line_t *line, pcb_coord_t y_offs);
void pcb_line_flip_side(pcb_layer_t *layer, pcb_line_t *line);
void pcb_line_scale(pcb_line_t *line, double sx, double sy, double sth);

pcb_coord_t pcb_line_length(const pcb_line_t *line);
double pcb_line_area(const pcb_line_t *line);

/* Convert a square cap line (e.g. a gEDA/pcb pad) to 4 corner points of a rectangle */
void pcb_sqline_to_rect(const pcb_line_t *line, pcb_coord_t *x, pcb_coord_t *y);

/* hash and eq */
int pcb_line_eq(const pcb_host_trans_t *tr1, const pcb_line_t *l1, const pcb_host_trans_t *tr2, const pcb_line_t *l2);
unsigned int pcb_line_hash(const pcb_host_trans_t *tr, const pcb_line_t *l);

/* un-administrate a line; call before changing geometry */
void pcb_line_pre(pcb_line_t *line);

/* re-administrate a line; call after changing geometry */
void pcb_line_post(pcb_line_t *line);

/*** DRC enforcement (obj_line_drcenf.c) ***/

/* Adjust the attached line to 45 degrees if necessary */
void pcb_line_adjust_attached(void);

/* adjusts the insert lines to make them 45 degrees as necessary */
void pcb_line_adjust_attached_2lines(pcb_bool);

/* makes the attached line fit into a 45 degree direction */
void pcb_line_45(pcb_attached_line_t *);

void pcb_line_enforce_drc(void);

/* Calculate a pair of refractioned (ortho-45) lines between 'start' and 'end'.
   If 'mid_out' is not NULL, load it with the coords of the middle point.
   If way is false it checks an ortho start line with one 45 refraction to
   reach the endpoint, otherwise it checks a 45 start, with a ortho refraction
   to reach endpoint.

   Checks for intersectors against two lines.

   If optimize is false, return straight-line distance between start and end
   on success or -1 if the pair-of-lines has hit a blocker object.

   If optimize is true, keep on looking for a mid-way solution, adjusting
   the fields of 'end' needed to find the closest point to the original target
   that still won't hit any object. Returns the straigh-line distance between
   start and the new end. */
double pcb_drc_lines(const pcb_point_t *start, pcb_point_t *end, pcb_point_t *mid_out, pcb_bool way, pcb_bool optimize);


/* Rather than mode the line bounding box, we set it so the point bounding
 * boxes are updated too.
 */
#define pcb_line_move(l,dx,dy) \
	do { \
		pcb_coord_t __dx__ = (dx), __dy__ = (dy); \
		pcb_line_t *__l__ = (l); \
		PCB_MOVE_POINT((__l__)->Point1.X,(__l__)->Point1.Y,(__dx__),(__dy__)); \
		PCB_MOVE_POINT((__l__)->Point2.X,(__l__)->Point2.Y,(__dx__),(__dy__)); \
		pcb_line_bbox(__l__); \
	} while(0)

#define PCB_LINE_LOOP(layer) do {                                       \
  pcb_line_t *line;                                                   \
  gdl_iterator_t __it__;                                            \
  linelist_foreach(&(layer)->Line, &__it__, line) {

#define PCB_LINE_ALL_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	for (l = 0; l < ((top)->LayerN > 0 ? (top)->LayerN : PCB->Data->LayerN); l++, layer++)	\
	{ \
		PCB_LINE_LOOP(layer)

#define PCB_LINE_COPPER_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	for (l = 0; l < ((top)->LayerN > 0 ? (top)->LayerN : PCB->Data->LayerN); l++, layer++)	\
	{ \
		if (!(pcb_layer_flags(PCB, l) & PCB_LYT_COPPER)) continue; \
		PCB_LINE_LOOP(layer)

#define PCB_LINE_SILK_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	for (l = 0; l < ((top)->LayerN > 0 ? (top)->LayerN : PCB->Data->LayerN); l++, layer++)	\
	{ \
		if (!(pcb_layer_flags(PCB, l) & PCB_LYT_SILK)) continue; \
		PCB_LINE_LOOP(layer)

#define PCB_LINE_VISIBLE_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	for (l = 0; l < ((top)->LayerN > 0 ? (top)->LayerN : PCB->Data->LayerN); l++, layer++)	\
	{ \
		if (layer->meta.real.vis) \
			PCB_LINE_LOOP(layer)

#endif
