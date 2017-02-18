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
#include <ctype.h>
#include <string.h>

pcb_hid_cfg_keys_t ghid_keymap;

gboolean ghid_is_modifier_key_sym(gint ksym)
{
	if (ksym == GDK_KEY_Shift_R || ksym == GDK_KEY_Shift_L || ksym == GDK_KEY_Control_R || ksym == GDK_KEY_Control_L)
		return TRUE;
	return FALSE;
}

ModifierKeysState ghid_modifier_keys_state(GtkWidget *drawing_area, GdkModifierType *state)
{
	GdkModifierType mask;
	ModifierKeysState mk;
	gboolean shift, control, mod1;

	if (!state)
		gdk_window_get_pointer(gtk_widget_get_window(drawing_area), NULL, NULL, &mask);
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

gboolean ghid_port_key_press_cb(GtkWidget *drawing_area, GdkEventKey *kev, gpointer data)
{
	pcb_gtk_view_t *view = data;

	if (ghid_is_modifier_key_sym(kev->keyval))
		return FALSE;

	if (kev->keyval <= 0xffff) {
		GdkModifierType state = (GdkModifierType) (kev->state);
		int slen, mods = 0;
		static pcb_hid_cfg_keyseq_t *seq[32];
		static int seq_len = 0;
		unsigned short int kv = kev->keyval;

		ghid_note_event_location(NULL);

		extern GdkModifierType ghid_glob_mask;
		ghid_glob_mask = state;

		if (state & GDK_MOD1_MASK)    mods |= PCB_M_Alt;
		if (state & GDK_CONTROL_MASK) mods |= PCB_M_Ctrl;
		if (state & GDK_SHIFT_MASK) {
/* TODO#3: this works only on US keyboard */
			static const char *ignore_shift = "~!@#$%^&*()_+{}|:\"<>?";
			if ((kv < 32) || (kv > 126) || (strchr(ignore_shift, kv) == NULL)) {
				mods |= PCB_M_Shift;
				if ((kv >= 'A') && (kv <= 'Z'))
					kv = tolower(kv);
			}
		}

		if (kv == GDK_KEY_ISO_Left_Tab) kv = GDK_KEY_Tab;
		slen = pcb_hid_cfg_keys_input(&ghid_keymap, mods, kv, seq, &seq_len);
		if (slen > 0) {
			view->has_entered  = 1;
			pcb_hid_cfg_keys_action(seq, slen);
			return TRUE;
		}
	}

	return FALSE;
}
