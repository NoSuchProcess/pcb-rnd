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

/* drawing the fab layer is a plugin now */

#ifndef PCB_STUB_DRAW_FAB_H
#define PCB_STUB_DRAW_FAB_H

#include "hid.h"
#include "pcb_bool.h"
#include "global_typedefs.h"
#include "draw.h"

/* fab */
extern int (*pcb_stub_draw_fab_overhang)(void);
extern void (*pcb_stub_draw_fab)(pcb_draw_info_t *info, pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e);

/* csect */
extern void (*pcb_stub_draw_csect)(pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e);
extern pcb_bool (*pcb_stub_draw_csect_mouse_ev)(pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y);

/* fontsel */
extern void (*pcb_stub_draw_fontsel)(pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e, pcb_text_t *txt);
extern pcb_bool (*pcb_stub_draw_fontsel_mouse_ev)(pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y, pcb_text_t *txt);

#endif
