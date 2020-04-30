#ifndef PCB_GTK_IN_KEYBOARD_H
#define PCB_GTK_IN_KEYBOARD_H

#include <gtk/gtk.h>
#include <gdk/gdkevents.h>

#include <librnd/core/hid_cfg_input.h>
#include "ui_zoompan.h"

typedef enum {
	NONE_PRESSED = 0,
	SHIFT_PRESSED = RND_M_Shift,
	CONTROL_PRESSED = RND_M_Ctrl,
	MOD1_PRESSED = RND_M_Mod1,
	SHIFT_CONTROL_PRESSED = RND_M_Shift | RND_M_Ctrl,
	SHIFT_MOD1_PRESSED = RND_M_Shift | RND_M_Mod1,
	CONTROL_MOD1_PRESSED = RND_M_Ctrl | RND_M_Mod1,
	SHIFT_CONTROL_MOD1_PRESSED = RND_M_Shift | RND_M_Ctrl | RND_M_Mod1
} ModifierKeysState;

/* return TRUE ksym is SHIFT or CTRL modifier key. */
gboolean ghid_is_modifier_key_sym(gint ksym);

/* key modifier state of drawing_area if state is NULL, otherwise, corresponding state key modifier. */
ModifierKeysState ghid_modifier_keys_state(GtkWidget *drawing_area, GdkModifierType *state);

/* Handle user key events of the output drawing area. */
gboolean ghid_port_key_press_cb(GtkWidget *drawing_area, GdkEventKey *kev, gpointer data);

extern rnd_hid_cfg_keys_t ghid_keymap;

extern GdkModifierType pcb_gtk_glob_mask;

/* return the keyval corresponding to desc key name. len is not used. */
unsigned short int ghid_translate_key(const char *desc, int len);

/* Load out/put_len with the GDK name corresponding to key_char key value.
   Return 0 upon success. */
int ghid_key_name(unsigned short int key_char, char *out, int out_len);

#endif
