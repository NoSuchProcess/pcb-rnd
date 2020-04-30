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

#ifndef RND_HID_INLINES
#define RND_HID_INLINES

#include <librnd/core/hid.h>
#include <librnd/core/globalconst.h>

RND_INLINE rnd_hid_gc_t rnd_hid_make_gc(void)
{
	rnd_hid_gc_t res;
	rnd_core_gc_t *hc;
	res = rnd_render->make_gc(rnd_gui);
	hc = (rnd_core_gc_t *)res; /* assumes first field is rnd_core_gc_t */
	hc->width = RND_MAX_COORD;
	hc->cap = rnd_cap_invalid;
	hc->xor = 0;
	hc->faded = 0;
	hc->hid = rnd_gui;
	return res;
}

RND_INLINE void rnd_hid_destroy_gc(rnd_hid_gc_t gc)
{
	rnd_render->destroy_gc(gc);
}

RND_INLINE void rnd_hid_set_line_cap(rnd_hid_gc_t gc, rnd_cap_style_t style)
{
	rnd_core_gc_t *hc = (rnd_core_gc_t *)gc;
	if (hc->cap != style) {
		hc->cap = style;
		rnd_render->set_line_cap(gc, style);
	}
}

RND_INLINE void rnd_hid_set_line_width(rnd_hid_gc_t gc, rnd_coord_t width)
{
	rnd_core_gc_t *hc = (rnd_core_gc_t *)gc;
	if (hc->width != width) {
		hc->width = width;
		rnd_render->set_line_width(gc, width);
	}
}

RND_INLINE void rnd_hid_set_draw_xor(rnd_hid_gc_t gc, int xor)
{
	rnd_core_gc_t *hc = (rnd_core_gc_t *)gc;
	if (hc->xor != xor) {
		hc->xor = xor;
		rnd_render->set_draw_xor(gc, xor);
	}
}

RND_INLINE void rnd_hid_set_draw_faded(rnd_hid_gc_t gc, int faded)
{
	rnd_core_gc_t *hc = (rnd_core_gc_t *)gc;
	if (hc->faded != faded) {
		hc->faded = faded;
		rnd_render->set_draw_faded(gc, faded);
	}
}

RND_INLINE const char *rnd_hid_command_entry(const char *ovr, int *cursor)
{
	if ((rnd_gui == NULL) || (rnd_gui->command_entry == NULL)) {
		if (cursor != NULL)
			*cursor = -1;
		return NULL;
	}
	return rnd_gui->command_entry(rnd_gui, ovr, cursor);
}


#endif
