/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

/*** Standard draw of gfxs ***/

#include "draw.h"

/* Include rtree.h for this */
#ifdef RND_RTREE_H
rnd_r_dir_t pcb_gfx_draw_callback(const rnd_rnd_box_t * b, void *cl);
#endif

void pcb_gfx_draw_(pcb_draw_info_t *info, pcb_gfx_t *gfx, int allow_term_gfx);
void pcb_gfx_draw(pcb_draw_info_t *info, pcb_gfx_t *gfx, int allow_term_gfx);
void pcb_gfx_invalidate_erase(pcb_gfx_t *gfx);
void pcb_gfx_invalidate_draw(pcb_layer_t *Layer, pcb_gfx_t *gfx);
void pcb_gfx_name_invalidate_draw(pcb_gfx_t *gfx);
void pcb_gfx_draw_label(pcb_draw_info_t *info, pcb_gfx_t *gfx);

void pcb_gfx_draw_xor(pcb_gfx_t *gfx, rnd_coord_t x, rnd_coord_t y);
