/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

#ifndef RND_COLOR_H
#define RND_COLOR_H

#include <librnd/core/global_typedefs.h>
#include <stddef.h>

struct rnd_color_s {
	unsigned char r, g, b, a; /* primary storage; alpha is not really supported at the moment */
	unsigned long packed;     /* cache: 32 bit portable (byte-order-safe) packed version used for lookups */
	float fr, fg, fb, fa;     /* cache: 0..1 version; using float to save memory^1 */
	char str[10];             /* cache: "#rrggbb[aa]" \0 terminated string version */
};

extern const rnd_color_t *rnd_color_black;
extern const rnd_color_t *rnd_color_white;
extern const rnd_color_t *rnd_color_cyan;
extern const rnd_color_t *rnd_color_red;
extern const rnd_color_t *rnd_color_blue;
extern const rnd_color_t *rnd_color_grey33;
extern const rnd_color_t *rnd_color_magenta;
extern const rnd_color_t *rnd_color_golden;
extern const rnd_color_t *rnd_color_drill;

/* Convert a color from various formats to a pcb color; returns 0 on success */
int rnd_color_load_int(rnd_color_t *dst, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
int rnd_color_load_packed(rnd_color_t *dst, unsigned long p);
int rnd_color_load_float(rnd_color_t *dst, float r, float g, float b, float a);
int rnd_color_load_str(rnd_color_t *dst, const char *src);

/* Same as strdup(), but for colors */
rnd_color_t *rnd_clrdup(const rnd_color_t *src);

void rnd_color_init(void);


/* temporary hack */
#define rnd_color_is_drill(clr) (strcmp((clr)->str, "drill") == 0)

/*** color vector ***/

#define GVT(x) vtclr_ ## x
#define GVT_ELEM_TYPE rnd_color_t
#define GVT_SIZE_TYPE size_t
#define GVT_DOUBLING_THRS 512
#define GVT_START_SIZE 16
#define GVT_FUNC
#define GVT_SET_NEW_BYTES_TO 0

#include <genvector/genvector_impl.h>
#define GVT_REALLOC(vect, ptr, size)  realloc(ptr, size)
#define GVT_FREE(vect, ptr)           free(ptr)
#include <genvector/genvector_undef.h>

/* Note ^1: openGL uses GLfloat which is guaranteed to be at least 32 bits;
but at the end for each color component it's unreasonable to use more than 8
bits and it is unlikely to encounter a system that is capable of doing opengl
but having a float type with less integer bits than 8. */

#endif
