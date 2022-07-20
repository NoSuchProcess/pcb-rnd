/*
 *                            COPYRIGHT
 *
 *  librnd, modular 2D CAD framework
 *  librnd Copyright (C) 2022 Tibor 'Igor2' Palinkas
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
 *    Project page: http://repo.hu/projects/librnd
 *    lead developer: http://repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#ifndef RND_GLYPH_ATOM_H
#define RND_GLYPH_ATOM_H

#include <genht/htip.h>
#include <librnd/core/global_typedefs.h>
#include <librnd/core/box.h>
#include <librnd/core/vtc0.h>

typedef enum rnd_glyph_atom_type_e {
	RND_GLYPH_LINE,
	RND_GLYPH_ARC,
	RND_GLYPH_POLY
} rnd_glyph_atom_type_t;

typedef struct rnd_glyph_line_s {
	rnd_glyph_atom_type_t type;
	rnd_coord_t x1, y1, x2, y2;
	rnd_coord_t thickness;
} rnd_glyph_line_t;

typedef struct rnd_glyph_arc_s {
	rnd_glyph_atom_type_t type;
	rnd_coord_t cx, cy, r;
	rnd_coord_t thickness;
	rnd_angle_t start, delta;      /* in deg */
} rnd_glyph_arc_t;

typedef struct rnd_glyph_poly_s {
	rnd_glyph_atom_type_t type;
	vtc0_t xy;                /* odd: x, even: y */
} rnd_glyph_poly_t;

typedef union rnd_glyph_atom_s {
		rnd_glyph_atom_type_t type;
		rnd_glyph_line_t line;
		rnd_glyph_arc_t arc;
		rnd_glyph_poly_t poly;
} rnd_glyph_atom_t;


#include <rnd_inclib/font/vtgla.h>

typedef struct rnd_glyph_s {
	unsigned valid:1;
	rnd_coord_t width, height; /* total size of glyph */
	rnd_coord_t xdelta;        /* distance to next symbol */
	vtgla_t atoms;
} rnd_glyph_t;


#endif

