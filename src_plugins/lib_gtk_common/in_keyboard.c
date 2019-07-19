/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* This code was originally written by Bill Wilson for the PCB Gtk port. */

#include "config.h"
#include "in_keyboard.h"
#include <gdk/gdkkeysyms.h>
#include <ctype.h>
#include <string.h>

#include "event.h"
#include "compat_misc.h"
#include "glue_common.h"

pcb_hid_cfg_keys_t ghid_keymap;
GdkModifierType pcb_gtk_glob_mask;

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
		gdkc_window_get_pointer(drawing_area, NULL, NULL, &mask);
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
	if (ghid_is_modifier_key_sym(kev->keyval))
		return FALSE;

	if (kev->keyval <= 0xffff) {
		GdkModifierType state = (GdkModifierType) (kev->state);
		int slen, mods = 0;
		unsigned short int key_raw = 0;
		unsigned short int kv = kev->keyval;
		guint *keyvals;
		GdkKeymapKey *keys;
		gint n_entries;

		pcb_gtk_note_event_location(NULL);

		pcb_gtk_glob_mask = state;

		if (state & GDK_MOD1_MASK)    mods |= PCB_M_Alt;
		if (state & GDK_CONTROL_MASK) mods |= PCB_M_Ctrl;
		if (state & GDK_SHIFT_MASK)   mods |= PCB_M_Shift;

		/* Retrieve the basic character (level 0) corresponding to physical key stroked. */
		if (gdk_keymap_get_entries_for_keycode(gdk_keymap_get_default(), kev->hardware_keycode, &keys, &keyvals, &n_entries)) {
			key_raw = keyvals[0];
			g_free(keys);
			g_free(keyvals);
		}

		if (kv == GDK_KEY_ISO_Left_Tab) kv = GDK_KEY_Tab;
		if (kv == GDK_KEY_KP_Add) key_raw = kv = '+';
		if (kv == GDK_KEY_KP_Subtract) key_raw = kv = '-';
		if (kv == GDK_KEY_KP_Multiply) key_raw = kv = '*';
		if (kv == GDK_KEY_KP_Divide) key_raw = kv = '/';
		if (kv == GDK_KEY_KP_Enter) key_raw = kv = GDK_KEY_Return;

		slen = pcb_hid_cfg_keys_input(&ghid_keymap, mods, key_raw, kv);
		if (slen > 0) {
			pcb_hid_cfg_keys_action(&ghid_keymap);
			return TRUE;
		}
	}

	return FALSE;
}

unsigned short int ghid_translate_key(const char *desc, int len)
{
	guint key;

	if (pcb_strcasecmp(desc, "enter") == 0)
		desc = "Return";

	key = gdk_keyval_from_name(desc);
	if (key > 0xffff) {
		pcb_message(PCB_MSG_WARNING, "Ignoring invalid/exotic key sym: '%s'\n", desc);
		return 0;
	}
	return key;
}

int ghid_key_name(unsigned short int key_char, char *out, int out_len)
{
	char *name = gdk_keyval_name(key_char);
	if (name == NULL)
		return -1;
	strncpy(out, name, out_len);
	out[out_len - 1] = '\0';
	return 0;
}
