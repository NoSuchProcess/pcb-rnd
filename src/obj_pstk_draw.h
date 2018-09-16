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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "obj_pstk.h"

#ifndef PCB_OBJ_PSTK_DRAW_H
#define PCB_OBJ_PSTK_DRAW_H

/* Include rtree.h for these */
#ifdef PCB_RTREE_H

#include "board.h"
#include "layer_grp.h"
#include "draw.h"

pcb_r_dir_t pcb_pstk_draw_callback(const pcb_box_t *b, void *cl);
pcb_r_dir_t pcb_pstk_draw_hole_callback(const pcb_box_t *b, void *cl);
pcb_r_dir_t pcb_pstk_draw_slot_callback(const pcb_box_t *b, void *cl);
pcb_r_dir_t pcb_pstk_clear_callback(const pcb_box_t *b, void *cl);
#endif

void pcb_pstk_draw(pcb_pstk_t *ps, pcb_bool draw_hole);
pcb_r_dir_t pcb_pstk_draw_mark_callback(const pcb_box_t *b, void *cl);
pcb_r_dir_t pcb_pstk_draw_label_callback(const pcb_box_t *b, void *cl);
void pcb_pstk_draw_label(pcb_draw_info_t *info, pcb_pstk_t *ps);
void pcb_pstk_invalidate_erase(pcb_pstk_t *ps);
void pcb_pstk_invalidate_draw(pcb_pstk_t *ps);

void pcb_pstk_thindraw(pcb_draw_info_t *info, pcb_hid_gc_t gc, pcb_pstk_t *ps);


#endif
