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
#define RND_FONT_DEFAULT_CELLSIZE 50

typedef long int rnd_font_id_t;      /* safe reference */

typedef struct rnd_font_s {          /* complete set of symbols */
	rnd_coord_t max_height, max_width; /* maximum glyph width and height; calculated per glyph */
	rnd_coord_t height;                /* total height, font-wise (distance between lowest point of the lowest glyph and highest point of the tallest glyph) */
	rnd_box_t unknown_glyph;           /* drawn when a glyph is not found (filled box) */
	rnd_glyph_t glyph[RND_FONT_MAX_GLYPHS+1];
	char *name;                        /* not unique */
	rnd_font_id_t id;                  /* unique for safe reference */
} rnd_font_t;


typedef enum {                 /* bitfield - order matters for backward compatibility */
	RND_FONT_MIRROR_NO = 0,
	RND_FONT_MIRROR_Y = 1,       /* change Y coords (mirror over the X axis) */
	RND_FONT_MIRROR_X = 2        /* change X coords (mirror over the Y axis) */
} rnd_font_mirror_t;

typedef enum rnd_font_tiny_e { /* How to draw text that is too tiny to be readable */
	RND_FONT_TINY_HIDE,          /* do not draw it at all */
	RND_FONT_TINY_CHEAP,         /* draw a cheaper, simplified approximation that shows there's something there */
	RND_FONT_TINY_ACCURATE       /* always draw text accurately, even if it will end up unreadable */
} rnd_font_tiny_t;

rnd_glyph_line_t *rnd_font_new_line_in_glyph(rnd_glyph_t *glyph, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t thickness);
rnd_glyph_arc_t *rnd_font_new_arc_in_glyph(rnd_glyph_t *glyph, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t r, rnd_angle_t start, rnd_angle_t delta, rnd_coord_t thickness);
rnd_glyph_poly_t *rnd_font_new_poly_in_glyph(rnd_glyph_t *glyph, long num_points);

/* Free all font content in f; doesn't free f itself */
void rnd_font_free(rnd_font_t *f);

/* Remove all content (atoms and geometry, except for ->height) of
   the glyph and mark it invalid; doesn't free g itself */
void rnd_font_free_glyph(rnd_glyph_t *g);


/* Very rough (but quick) estimation on the full size of the text */
rnd_coord_t rnd_font_string_width(rnd_font_t *font, double scx, const unsigned char *string);
rnd_coord_t rnd_font_string_height(rnd_font_t *font, double scy, const unsigned char *string);


typedef void (*rnd_font_draw_atom_cb)(void *cb_ctx, const rnd_glyph_atom_t *a);

RND_FONT_DRAW_API void rnd_font_draw_string(rnd_font_t *font, const unsigned char *string, rnd_coord_t x0, rnd_coord_t y0, double scx, double scy, double rotdeg, rnd_font_mirror_t mirror, rnd_coord_t thickness, rnd_coord_t min_line_width, int poly_thin, rnd_font_tiny_t tiny, rnd_font_draw_atom_cb cb, void *cb_ctx);

/* Calculate all 4 corners of the transformed (e.g. rotated) box in cx;cy */
void rnd_font_string_bbox(rnd_coord_t cx[4], rnd_coord_t cy[4], rnd_font_t *font, const unsigned char *string, rnd_coord_t x0, rnd_coord_t y0, double scx, double scy, double rotdeg, rnd_font_mirror_t mirror, rnd_coord_t thickness, rnd_coord_t min_line_width);
void rnd_font_string_bbox_pcb_rnd(rnd_coord_t cx[4], rnd_coord_t cy[4], rnd_font_t *font, const unsigned char *string, rnd_coord_t x0, rnd_coord_t y0, double scx, double scy, double rotdeg, rnd_font_mirror_t mirror, rnd_coord_t thickness, rnd_coord_t min_line_width, int scale);

/* transforms symbol coordinates so that the left edge of each symbol
   is at the zero position. The y coordinates are moved so that min(y) = 0 */
void rnd_font_normalize(rnd_font_t *f);

/* Count the number of characters in s that wouldn't render with the given font */
int rnd_font_invalid_chars(rnd_font_t *font, const unsigned char *s);

void rnd_font_copy(rnd_font_t *dst, const rnd_font_t *src);

/*** embedded (internal) font ***/

typedef struct embf_line_s {
	int x1, y1, x2, y2, th;
} embf_line_t;

typedef struct embf_font_s {
	int delta;
	embf_line_t *lines;
	int num_lines;
} embf_font_t;

void rnd_font_load_internal(rnd_font_t *font, embf_font_t *embf_font, int len, rnd_coord_t embf_minx, rnd_coord_t embf_miny, rnd_coord_t embf_maxx, rnd_coord_t embf_maxy);

#endif

