/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* Drawing primitive: line segment */

#ifndef PCB_OBJ_LINE_H
#define PCB_OBJ_LINE_H

#include "obj_common.h"

struct pcb_line_s {            /* holds information about one line */
	PCB_ANYLINEFIELDS;
	char *Number;
	gdl_elem_t link;             /* a line is in a list: either on a layer or in an element */
};

/* crosshair: */
typedef struct {								/* current marked line */
	pcb_point_t Point1,							/* start- and end-position */
	  Point2;
	long int State;
	pcb_bool draw;
} pcb_attached_line_t;


pcb_line_t *pcb_line_alloc(pcb_layer_t * layer);
void pcb_line_free(pcb_line_t * data);

pcb_line_t *pcb_line_new_merge(pcb_layer_t *Layer, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_coord_t Thickness, pcb_coord_t Clearance, pcb_flag_t Flags);
pcb_line_t *pcb_line_new(pcb_layer_t *Layer, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_coord_t Thickness, pcb_coord_t Clearance, pcb_flag_t Flags);
void *pcb_line_destroy(pcb_layer_t *Layer, pcb_line_t *Line);


/* Add objects without creating them or making any "sanity modifications" to them */
void pcb_add_line_on_layer(pcb_layer_t *Layer, pcb_line_t *Line);

void pcb_line_bbox(pcb_line_t *Line);
void pcb_line_rotate90(pcb_line_t *Line, pcb_coord_t X, pcb_coord_t Y, unsigned Number);
void pcb_line_rotate(pcb_layer_t *layer, pcb_line_t *line, pcb_coord_t X, pcb_coord_t Y, double cosa, double sina);
void pcb_line_mirror(pcb_layer_t *layer, pcb_line_t *line);
void pcb_line_flip_side(pcb_layer_t *layer, pcb_line_t *line);

pcb_coord_t pcb_line_length(const pcb_line_t *line);
double pcb_line_area(const pcb_line_t *line);


/* hash */
int pcb_line_eq(const pcb_element_t *e1, const pcb_line_t *l1, const pcb_element_t *e2, const pcb_line_t *l2);
unsigned int pcb_line_hash(const pcb_element_t *e, const pcb_line_t *l);


/*** DRC enforcement (obj_line_drcenf.c) ***/
void pcb_line_adjust_attached(void);
void pcb_line_adjust_attached_2lines(pcb_bool);
void pcb_line_45(pcb_attached_line_t *);
void pcb_line_enforce_drc(void);

/* Rather than mode the line bounding box, we set it so the point bounding
 * boxes are updated too.
 */
#define pcb_line_move(l,dx,dy) \
	do { \
		pcb_coord_t __dx__ = (dx), __dy__ = (dy); \
		pcb_line_t *__l__ = (l); \
		PCB_MOVE((__l__)->Point1.X,(__l__)->Point1.Y,(__dx__),(__dy__)) \
		PCB_MOVE((__l__)->Point2.X,(__l__)->Point2.Y,(__dx__),(__dy__)) \
		pcb_line_bbox(__l__); \
	} while(0)


#define PCB_LINE_LOOP(layer) do {                                       \
  pcb_line_t *line;                                                   \
  gdl_iterator_t __it__;                                            \
  linelist_foreach(&(layer)->Line, &__it__, line) {

#define PCB_LINE_ALL_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	for (l = 0; l < pcb_max_copper_layer + 2; l++, layer++)	\
	{ \
		PCB_LINE_LOOP(layer)

#define PCB_LINE_COPPER_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	for (l = 0; l < pcb_max_copper_layer; l++, layer++)	\
	{ \
		PCB_LINE_LOOP(layer)

#define PCB_LINE_SILK_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	layer += pcb_max_copper_layer;			\
	for (l = 0; l < 2; l++, layer++)		\
	{ \
		PCB_LINE_LOOP(layer)

#define PCB_LINE_VISIBLE_LOOP(top) do	{		\
	pcb_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	for (l = 0; l < pcb_max_copper_layer + 2; l++, layer++)	\
	{ \
		if (layer->On)				\
			PCB_LINE_LOOP(layer)

#endif
