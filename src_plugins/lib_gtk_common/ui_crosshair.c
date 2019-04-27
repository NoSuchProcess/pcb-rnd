/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <gtk/gtk.h>

#include "ui_crosshair.h"

#include "hid_gtk_conf.h"
#include "ui_zoompan.h"
#include "conf_core.h"

void pcb_gtk_crosshair_set(pcb_coord_t x, pcb_coord_t y, int action, int offset_x, int offset_y, pcb_gtk_view_t *view)
{
	GdkDisplay *display;
	GdkScreen *screen;
	int widget_x, widget_y;
	int pointer_x, pointer_y;
	pcb_coord_t pcb_x, pcb_y;

	if (view->crosshair_x != x || view->crosshair_y != y) {
		view->crosshair_x = x;
		view->crosshair_y = y;
	}

	if (action != HID_SC_PAN_VIEWPORT && action != HID_SC_WARP_POINTER)
		return;

	/* Find out where the drawing area is on the screen. gdk_display_get_pointer
	 * and gdk_display_warp_pointer work relative to the whole display, whilst
	 * our coordinates are relative to the drawing area origin.
	 */
	display = gdk_display_get_default();

	switch (action) {
	case HID_SC_PAN_VIEWPORT:
		/* Pan the board in the viewport so that the crosshair (who's location
		 * relative on the board was set above) lands where the pointer is.
		 * We pass the request to pan a particular point on the board to a
		 * given widget coordinate of the viewport into the rendering code
		 */

		/* Find out where the pointer is relative to the display */
		gdk_display_get_pointer(display, NULL, &pointer_x, &pointer_y, NULL);

		widget_x = pointer_x - offset_x;
		widget_y = pointer_y - offset_y;

		pcb_gtk_coords_event2pcb(view, widget_x, widget_y, &pcb_x, &pcb_y);
		pcb_gtk_pan_view_abs(view, pcb_x, pcb_y, widget_x, widget_y);

		/* Just in case we couldn't pan the board the whole way,
		 * we warp the pointer to where the crosshair DID land.
		 */
		/* Fall through */

	case HID_SC_WARP_POINTER:
		screen = gdk_display_get_default_screen(display);

		pcb_gtk_coords_pcb2event(view, x, y, &widget_x, &widget_y);

		pointer_x = offset_x + widget_x;
		pointer_y = offset_y + widget_y;

		gdk_display_warp_pointer(display, screen, pointer_x, pointer_y);

		break;
	}
}
