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

#ifndef PCB_OBJ_PSTK_SHAPE_H
#define PCB_OBJ_PSTK_SHAPE_H

#include <librnd/core/unit.h>
#include "polygon.h"
#include "layer.h"

typedef struct pcb_pstk_poly_s {
	unsigned int len;             /* number of points in polygon */
	rnd_coord_t *x;               /* ordered list of points, X coord */
	rnd_coord_t *y;               /* ordered list of points, Y coord */
	rnd_polyarea_t *pa;           /* cache for the poly code */
	char inverted;                /* 1 if x;y has the opposite direction as pa */
} pcb_pstk_poly_t;

typedef struct pcb_pstk_line_s {
	rnd_coord_t x1, y1, x2, y2, thickness;
	unsigned square:1;
} pcb_pstk_line_t;

typedef struct pcb_pstk_circ_s {
	rnd_coord_t dia;             /* diameter of the filled circle */
	rnd_coord_t x, y;            /* assymetric pads */
} pcb_pstk_circ_t;

typedef struct pcb_pstk_shape_s {
	pcb_layer_type_t layer_mask;
	pcb_layer_combining_t comb;
	union {
		pcb_pstk_poly_t poly;
		pcb_pstk_line_t line;
		pcb_pstk_circ_t circ;
	} data;
	enum {
		PCB_PSSH_POLY,
		PCB_PSSH_LINE,
		PCB_PSSH_CIRC,               /* filled circle */
		PCB_PSSH_HSHADOW             /* for clearance: pretend the shape is the same as the drill's or slot's; but do not add anything positive to the target layer */
	} shape;
	rnd_coord_t clearance;         /* per layer clearance: internal layer clearance is sometimes different for production or insulation reasons (IPC2221A) */
} pcb_pstk_shape_t;

/* transformed prototype */
typedef struct pcb_pstk_tshape_s {
	double rot;
	unsigned xmirror:1;
	unsigned smirror:1;

	unsigned char len;             /* number of shapes (PCB_PADSTACK_MAX_SHAPES) */
	pcb_pstk_shape_t *shape;   /* list of layer-shape pairs */
} pcb_pstk_tshape_t;

void pcb_pstk_shape_rot(pcb_pstk_shape_t *sh, double sina, double cosa, double angle);

#endif
