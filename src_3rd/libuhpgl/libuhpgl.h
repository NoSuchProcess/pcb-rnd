/*
    libuhpgl - the micro HP-GL library
    Copyright (C) 2017  Tibor 'Igor2' Palinkas

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA


    This library is part of pcb-rnd: http://repo.hu/projects/pcb-rnd
*/

#ifndef LIBUHPGL_H
#define LIBUHPGL_H

#include <stddef.h>

/*** low level types ***/
typedef long int uhpgl_coord_t;  /* 1 unit is 0.025 mm */
typedef double uhpgl_angle_t;    /* in degrees; absolute angles are [0..360], relative angles are [-360..+360] */

typedef enum uhpgl_obj_type_s {
	UHPGL_OBJ_INVALID,
	UHPGL_OBJ_LINE,
	UHPGL_OBJ_ARC,
	UHPGL_OBJ_WEDGE,
	UHPGL_OBJ_RECT,
	UHPGL_OBJ_POLY
} uhpgl_obj_type_t;

typedef enum uhpgl_fill_type_s {
	UHPGL_FILL_NONE,
	UHPGL_FILL_PAINT1,
	UHPGL_FILL_PAINT2,
	UHPGL_FILL_SINGLE,
	UHPGL_FILL_CROSS
} uhpgl_fill_type_t;

typedef struct uhpgl_fill_stroke_s {
	uhpgl_fill_type_t  type;       /* UHPGL_FILL_NONE if no fill */
	uhpgl_coord_t      spacing;
	uhpgl_angle_t      angle;
	double             pen_thick;
	unsigned           stroke:1;   /* 1 if stroked */
} uhpgl_fill_stroke_t;

typedef struct uhpgl_point_s {
	uhpgl_coord_t x, y;
} uhpgl_point_t;

typedef struct uhpgl_obj_s uhpgl_obj_t;

/*** drawing objects ***/
typedef struct uhpgl_line_s {
	int pen;
	uhpgl_point_t p1, p2;
} uhpgl_line_t;

typedef struct uhpgl_arc_s {
	int pen;
	uhpgl_point_t center;
	uhpgl_coord_t r;
	uhpgl_point_t startp, endp;
	uhpgl_angle_t starta, enda, deltaa;
} uhpgl_arc_t;

typedef struct uhpgl_wedge_s {
	int pen;
	uhpgl_arc_t arc;
	uhpgl_fill_stroke_t fill_stroke;
} uhpgl_wedge_t;

typedef struct uhpgl_rect_s {
	int pen;
	uhpgl_point_t p1, p2; /* p1 is the minimal x;y and p2 is the maximal x;y */
	uhpgl_fill_stroke_t fill_stroke;
} uhpgl_rect_t;

typedef struct uhpgl_poly_s {
	int pen;
	uhpgl_point_t bb1, bb2; /* bounding box; bb1 is the minimal x;y and bb2 is the maximal x;y */
	uhpgl_obj_t *outline;   /* outer contour (linked list of non-poly objects) */
	int num_holes;
	uhpgl_obj_t **holes;    /* inner/hole contours (each is a linked list of non-poly objects) */
	uhpgl_fill_stroke_t fill_stroke;
} uhpgl_poly_t;

/*** common object and context ***/
struct uhpgl_obj_s {
	uhpgl_obj_type_t type;
	union {
		uhpgl_line_t line;
		uhpgl_arc_t arc;
		uhpgl_wedge_t wedge;
		uhpgl_rect_t rect;
		uhpgl_poly_t poly;
	} obj;
	uhpgl_obj_t *next; /* singly linked list of objects, NULL terminates the list */
};

typedef struct uhpgl_ctx_s uhpgl_ctx_t;
struct uhpgl_ctx_s {
	/* configuration: read-only for the lib, written by the caller */
	struct {
		unsigned approx_curve_in_poly:1; /* if 1, use line approx in polygons even if arc() is non-NULL */

		/* callbacks for objects found; if returns non-zero, parsing is abandoned */
		int (*line)(uhpgl_ctx_t *ctx, uhpgl_line_t *line);    /* must NOT be NULL */
		int (*arc)(uhpgl_ctx_t *ctx, uhpgl_arc_t *arc);       /* if NULL, use line() approx */
		int (*circ)(uhpgl_ctx_t *ctx, uhpgl_arc_t *circ);     /* if NULL, use arc() */
		int (*poly)(uhpgl_ctx_t *ctx, uhpgl_poly_t *poly);    /* if NULL, use draw the outline only (even for filled polygons) */
		int (*wedge)(uhpgl_ctx_t *ctx, uhpgl_wedge_t *wedge); /* if NULL, use polygon() */
		int (*rect)(uhpgl_ctx_t *ctx, uhpgl_rect_t *rect);    /* if NULL, use polygon() */
	} conf;

	/* current state: read-only for the caller, written by the lib */
	struct {
		int pen;              /* selected pen [0..255] */
		int pen_speed;
		unsigned pen_down:1;  /* whether pen is down (drawing) */
		uhpgl_point_t at;     /* last known coordinate of the pen */
		int ct;               /* Chord Tolerance */
		uhpgl_fill_stroke_t fill;

		/* last char parsed */
		size_t offs;
		size_t line, col;
	} state;

	/* parser error report: written by the lib, read-only for the caller */
	struct {
		size_t offs;
		size_t line, col;
		const char *msg;
	} error;

	void *parser;    /* opaque parser data */
	void *user_data; /* only used by the caller */
};

#endif
