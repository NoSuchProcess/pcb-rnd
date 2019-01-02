/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  ttf low level loader
 *  pcb-rnd Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

#ifndef PCB_TTF_LOAD_H
#define PCB_TTF_LOAD_H

#include <ft2build.h>
#include <freetype.h>

typedef struct pcb_ttf_s {
	FT_Library library;
	FT_Face face;
} pcb_ttf_t;

/* Stroker backend; this low level lib does not know how to draw anything but
   calls back the stroker for outline objects */
typedef struct pcb_ttf_stroke_s pcb_ttf_stroke_t;
struct pcb_ttf_stroke_s {
	FT_Outline_Funcs funcs;
	void (*init)(pcb_ttf_stroke_t *s);
	void (*start)(pcb_ttf_stroke_t *s, int chr);
	void (*finish)(pcb_ttf_stroke_t *s);
	void (*uninit)(pcb_ttf_stroke_t *s);

	double x, y;
	double dx, dy, scale_x, scale_y;
};

/* Load the ttf font from fn; return 0 on success */
FT_Error pcb_ttf_load(pcb_ttf_t *ttf, const char *fn);

int pcb_ttf_unload(pcb_ttf_t *ctx);

/* Use str to trace the outline of a glyph; returns 0 on success */
FT_Error pcb_ttf_trace(pcb_ttf_t *ctx, FT_ULong ttf_chr, FT_ULong out_chr, pcb_ttf_stroke_t *str, unsigned short int scale);

/* Convert an error code into a human readable error message */
const char *pcb_ttf_errmsg(FT_Error errnum);


#endif
