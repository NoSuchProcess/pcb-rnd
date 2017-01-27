/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,1997,1998,1999 Thomas Nau
 *  pcb-rnd Copyright (C) 2017, Tibor 'Igor2' Palinkas
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* This file was originally written by Bill Wilson for the PCB Gtk port;
   refactored for pcb-rnd by Tibor 'Igor2' Palinkas */

#include <gtk/gtk.h>
#include "in_mouse.h"

/*FIXME sad :( Needed for ButtonState, GHidPort */
#include "../hid_gtk/gui.h"

pcb_hid_cfg_mouse_t ghid_mouse;
int ghid_wheel_zoom = 0;


pcb_hid_cfg_mod_t ghid_mouse_button(int ev_button)
{
	/* GDK numbers buttons from 1..5, there seem to be no symbolic names */
	return (PCB_MB_LEFT << (ev_button-1));
}

/*FIXME: Is this function used ? */
ButtonState ghid_button_state(GdkModifierType * state)
{
	GdkModifierType mask;
	ButtonState bs;
	gboolean button1, button2, button3;
	GHidPort *out = &ghid_port;

	if (!state) {
		gdk_window_get_pointer(gtk_widget_get_window(out->drawing_area), NULL, NULL, &mask);
	}
	else
		mask = *state;

	extern GdkModifierType ghid_glob_mask;
	ghid_glob_mask = mask;

	button1 = (mask & GDK_BUTTON1_MASK);
	button2 = (mask & GDK_BUTTON2_MASK);
	button3 = (mask & GDK_BUTTON3_MASK);

	if (button1)
		bs = BUTTON1_PRESSED;
	else if (button2)
		bs = BUTTON2_PRESSED;
	else if (button3)
		bs = BUTTON3_PRESSED;
	else
		bs = NO_BUTTON_PRESSED;

	return bs;
}
