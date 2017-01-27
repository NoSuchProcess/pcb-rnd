/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Alain Vigne
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* This code was originally written by Bill Wilson for the PCB Gtk port. */

#include "config.h"

#include "in_keyboard.h"
#include <gdk/gdkkeysyms.h>

gboolean ghid_is_modifier_key_sym(gint ksym)
{
	if (ksym == GDK_KEY_Shift_R || ksym == GDK_KEY_Shift_L || ksym == GDK_KEY_Control_R || ksym == GDK_KEY_Control_L)
		return TRUE;
	return FALSE;
}

ModifierKeysState ghid_modifier_keys_state(GdkModifierType * state)
{
	GdkModifierType mask;
	ModifierKeysState mk;
	gboolean shift, control, mod1;
	GHidPort *out = &ghid_port;

	if (!state)
		gdk_window_get_pointer(gtk_widget_get_window(out->drawing_area), NULL, NULL, &mask);
	else
		mask = *state;

	shift = (mask & GDK_SHIFT_MASK);
	control = (mask & GDK_CONTROL_MASK);
	mod1 = (mask & GDK_MOD1_MASK);

	if (shift && !control && !mod1)
		mk = SHIFT_PRESSED;
	else if (!shift && control && !mod1)
		mk = CONTROL_PRESSED;
	else if (!shift && !control && mod1)
		mk = MOD1_PRESSED;
	else if (shift && control && !mod1)
		mk = SHIFT_CONTROL_PRESSED;
	else if (shift && !control && mod1)
		mk = SHIFT_MOD1_PRESSED;
	else if (!shift && control && mod1)
		mk = CONTROL_MOD1_PRESSED;
	else if (shift && control && mod1)
		mk = SHIFT_CONTROL_MOD1_PRESSED;
	else
		mk = NONE_PRESSED;

	return mk;
}
