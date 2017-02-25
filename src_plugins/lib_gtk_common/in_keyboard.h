#ifndef PCB_GTK_IN_KEYBOARD_H
#define PCB_GTK_IN_KEYBOARD_H

#include <gtk/gtk.h>
#include <gdk/gdkevents.h>

#include "hid_cfg_input.h"
#include "ui_zoompan.h"

typedef enum {
	NONE_PRESSED = 0,
	SHIFT_PRESSED = PCB_M_Shift,
	CONTROL_PRESSED = PCB_M_Ctrl,
	MOD1_PRESSED = PCB_M_Mod1,
	SHIFT_CONTROL_PRESSED = PCB_M_Shift | PCB_M_Ctrl,
	SHIFT_MOD1_PRESSED = PCB_M_Shift | PCB_M_Mod1,
	CONTROL_MOD1_PRESSED = PCB_M_Ctrl | PCB_M_Mod1,
	SHIFT_CONTROL_MOD1_PRESSED = PCB_M_Shift | PCB_M_Ctrl | PCB_M_Mod1
} ModifierKeysState;

/** */
gboolean ghid_is_modifier_key_sym(gint ksym);

/** Returns key modifier state */
ModifierKeysState ghid_modifier_keys_state(GtkWidget *drawing_area, GdkModifierType *state);

/** Handle user keys in the output drawing area. */
gboolean ghid_port_key_press_cb(GtkWidget *drawing_area, GdkEventKey *kev, gpointer data);

extern pcb_hid_cfg_keys_t ghid_keymap;

/* Temporary glue: */
extern GdkModifierType ghid_glob_mask;

#endif
