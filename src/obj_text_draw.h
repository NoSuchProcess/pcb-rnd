/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

/*** Standard draw of text ***/

#include "draw.h"

/* Include rtree.h for these */
#ifdef RND_RTREE_H
rnd_r_dir_t pcb_text_draw_callback(const rnd_rnd_box_t * b, void *cl);
rnd_r_dir_t pcb_text_draw_term_callback(const rnd_rnd_box_t * b, void *cl);
#endif

typedef enum pcb_text_tiny_e { /* How to draw text that is too tiny to be readable */
	PCB_TXT_TINY_HIDE,         /* do not draw it at all */
	PCB_TXT_TINY_CHEAP,        /* draw a cheaper, simplified approximation that shows there's something there */
	PCB_TXT_TINY_ACCURATE      /* always draw text accurately, even if it will end up unreadable */
} pcb_text_tiny_t;

void pcb_text_draw_(pcb_draw_info_t *info, pcb_text_t *Text, rnd_coord_t min_line_width, int allow_term_gfx, pcb_text_tiny_t tiny);
void pcb_text_invalidate_erase(pcb_layer_t *Layer, pcb_text_t *Text);
void pcb_text_invalidate_draw(pcb_layer_t *Layer, pcb_text_t *Text);
void pcb_text_draw_xor(pcb_text_t *text, rnd_coord_t x, rnd_coord_t y);
void pcb_text_name_invalidate_draw(pcb_text_t *txt);
void pcb_text_draw_label(pcb_draw_info_t *info, pcb_text_t *text);

/* lowlevel drawing routine for text strings */
void pcb_text_draw_string(pcb_draw_info_t *info, pcb_font_t *font, const unsigned char *string, rnd_coord_t x0, rnd_coord_t y0, int scale, double rotdeg, int mirror, rnd_coord_t thickness, rnd_coord_t min_line_width, int xordraw, rnd_coord_t xordx, rnd_coord_t xordy, pcb_text_tiny_t tiny);

/* lowlevel text bounding box calculation */
rnd_coord_t pcb_text_width(pcb_font_t *font, int scale, const unsigned char *string);
rnd_coord_t pcb_text_height(pcb_font_t *font, int scale, const unsigned char *string);

/* Call cb(ctx, ...) for every object in a rendered string or text; obj will
   be disacrded after the call, can be modified by the callback */
typedef void (*pcb_draw_text_cb)(void *ctx, pcb_any_obj_t *obj);
void pcb_text_decompose_string(pcb_draw_info_t *info, pcb_font_t *font, const unsigned char *string, rnd_coord_t x0, rnd_coord_t y0, int scale, double rotdeg, int mirror, rnd_coord_t thickness, pcb_draw_text_cb cb, void *cb_ctx);
void pcb_text_decompose_text(pcb_draw_info_t *info, pcb_text_t *text, pcb_draw_text_cb cb, void *cb_ctx);

