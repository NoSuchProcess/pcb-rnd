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

#ifndef PCB_HID_INLINES
#define PCB_HID_INLINES

#include "hid.h"

PCB_INLINE pcb_hid_gc_t pcb_hid_make_gc(void)
{
	pcb_hid_gc_t res;
	pcb_core_gc_t *hc;
	res = pcb_gui->make_gc(pcb_gui);
	hc = (pcb_core_gc_t *)res; /* assumes first field is pcb_core_gc_t */
	hc->width = PCB_MAX_COORD;
	hc->cap = pcb_cap_invalid;
	hc->xor = 0;
	hc->faded = 0;
	return res;
}

PCB_INLINE void pcb_hid_destroy_gc(pcb_hid_gc_t gc)
{
	pcb_gui->destroy_gc(pcb_gui, gc);
}

PCB_INLINE void pcb_hid_set_line_cap(pcb_hid_gc_t gc, pcb_cap_style_t style)
{
	pcb_core_gc_t *hc = (pcb_core_gc_t *)gc;
	if (hc->cap != style) {
		hc->cap = style;
		pcb_gui->set_line_cap(gc, style);
	}
}

PCB_INLINE void pcb_hid_set_line_width(pcb_hid_gc_t gc, pcb_coord_t width)
{
	pcb_core_gc_t *hc = (pcb_core_gc_t *)gc;
	if (hc->width != width) {
		hc->width = width;
		pcb_gui->set_line_width(gc, width);
	}
}

PCB_INLINE void pcb_hid_set_draw_xor(pcb_hid_gc_t gc, int xor)
{
	pcb_core_gc_t *hc = (pcb_core_gc_t *)gc;
	if (hc->xor != xor) {
		hc->xor = xor;
		pcb_gui->set_draw_xor(gc, xor);
	}
}

PCB_INLINE void pcb_hid_set_draw_faded(pcb_hid_gc_t gc, int faded)
{
	pcb_core_gc_t *hc = (pcb_core_gc_t *)gc;
	if (hc->faded != faded) {
		hc->faded = faded;
		pcb_gui->set_draw_faded(gc, faded);
	}
}

PCB_INLINE const char *pcb_hid_command_entry(const char *ovr, int *cursor)
{
	if ((pcb_gui == NULL) || (pcb_gui->command_entry == NULL)) {
		if (cursor != NULL)
			*cursor = -1;
		return NULL;
	}
	return pcb_gui->command_entry(ovr, cursor);
}


#endif
