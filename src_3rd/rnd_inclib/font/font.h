/*
 *                            COPYRIGHT
 *
 *  librnd, modular 2D CAD framework - vector font rendering
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

#ifndef RND_FONT_H
#define RND_FONT_H

#include <genht/htip.h>
#include <librnd/core/global_typedefs.h>
#include <librnd/core/box.h>
#include <rnd_inclib/font/glyph.h>

#ifndef RND_FONT_DRAW_API
#define RND_FONT_DRAW_API
#endif

#define RND_FONT_MAX_GLYPHS 255

typedef long int rnd_font_id_t;      /* safe reference */

typedef struct rnd_font_s {          /* complete set of symbols */
	rnd_coord_t max_height, max_width; /* maximum glyph width and height */
	rnd_box_t unknown_glyph;           /* drawn when a glyph is not found (filled box) */
	rnd_glyph_t glyph[RND_FONT_MAX_GLYPHS+1];
	char *name;                        /* not unique */
	rnd_font_id_t id;                  /* unique for safe reference */
} rnd_font_t;


typedef enum { /* bitfield - order matters for backward compatibility */
	RND_TXT_MIRROR_NO = 0,
	RND_TXT_MIRROR_Y = 1, /* change Y coords (mirror over the X axis) */
	RND_TXT_MIRROR_X = 2  /* change X coords (mirror over the Y axis) */
} rnd_font_mirror_t;

void rnd_font_set_info(rnd_font_t *dst);

rnd_glyph_line_t *rnd_font_new_line_in_glyph(rnd_glyph_t *glyph, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t thickness);
rnd_glyph_arc_t *rnd_font_new_arc_in_glyph(rnd_glyph_t *glyph, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t r, rnd_angle_t start, rnd_angle_t delta, rnd_coord_t thickness);
rnd_glyph_poly_t *rnd_font_new_poly_in_glyph(rnd_glyph_t *glyph, int num_points);

void rnd_font_free(rnd_font_t *f);

/* Remove all content (atoms and geometry, except for ->height) of
   the glyph and mark it invalid */
void rnd_font_clear_glyph(rnd_glyph_t *g);



typedef void (*rnd_font_draw_atom_cb)(void *cb_ctx, const rnd_glyph_atom_t *a);

RND_FONT_DRAW_API void rnd_font_draw_string(rnd_font_t *font, const unsigned char *string, rnd_coord_t x0, rnd_coord_t y0, double scx, double scy, double rotdeg, rnd_font_mirror_t mirror, rnd_coord_t thickness, rnd_coord_t min_line_width, int poly_thin, rnd_font_draw_atom_cb cb, void *cb_ctx);

#endif
