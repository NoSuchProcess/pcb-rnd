/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* drawing the fab layer is a plugin now */

#ifndef PCB_STUB_DRAW_FAB_H
#define PCB_STUB_DRAW_FAB_H

#include "hid.h"
#include "pcb_bool.h"
#include "global_typedefs.h"

/* fab */
extern int (*pcb_stub_draw_fab_overhang)(void);
extern void (*pcb_stub_draw_fab)(pcb_hid_gc_t gc);

/* csect */
extern void (*pcb_stub_draw_csect)(pcb_hid_gc_t gc);
extern pcb_bool (*pcb_stub_draw_csect_mouse_ev)(void *widget, pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y);
extern void (*pcb_stub_draw_csect_overlay)(pcb_hid_t *hid, const pcb_hid_expose_ctx_t *ctx);

/* fontsel */
extern void (*pcb_stub_draw_fontsel)(pcb_hid_gc_t gc);
extern pcb_bool (*pcb_stub_draw_fontsel_mouse_ev)(void *widget, pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y);
extern pcb_text_t **pcb_stub_draw_fontsel_text_obj;
extern pcb_layer_t **pcb_stub_draw_fontsel_layer_obj;
extern int *pcb_stub_draw_fontsel_text_type;

#endif
