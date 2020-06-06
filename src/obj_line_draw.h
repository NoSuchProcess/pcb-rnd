/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/*** Standard draw of lines ***/

#include "draw.h"

/* Include rtree.h for this */
#ifdef RND_RTREE_H
rnd_r_dir_t pcb_line_draw_callback(const rnd_box_t * b, void *cl);
rnd_r_dir_t pcb_line_draw_term_callback(const rnd_box_t * b, void *cl);
#endif

void pcb_line_draw_(pcb_draw_info_t *info, pcb_line_t *line, int allow_term_gfx);
void pcb_line_invalidate_erase(pcb_line_t *Line);
void pcb_line_invalidate_draw(pcb_layer_t *Layer, pcb_line_t *Line);
void pcb_line_draw_label(pcb_draw_info_t *info, pcb_line_t *line);
void pcb_line_name_invalidate_draw(pcb_line_t *line);

