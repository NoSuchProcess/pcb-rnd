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

/** \return TRUE if \p ksym is SHIFT or CTRL modifier key. */
gboolean ghid_is_modifier_key_sym(gint ksym);

/** \return key modifier state of \p drawing_area if \p state is NULL, otherwise, corresponding \p state key modifier. */
ModifierKeysState ghid_modifier_keys_state(GtkWidget *drawing_area, GdkModifierType *state);

/** Handle user key events of the output drawing area. */
gboolean ghid_port_key_press_cb(GtkWidget *drawing_area, GdkEventKey *kev, gpointer data);

extern pcb_hid_cfg_keys_t ghid_keymap;

extern GdkModifierType pcb_gtk_glob_mask;

/** \return the keyval corresponding to \p desc key name. \p len is not used. */
unsigned short int ghid_translate_key(const char *desc, int len);

/** Return in \p out -a string of max. \p out_len chars-, the GDK name corresponding to \p key_char key value.
    \return 0 upon successful completion, -1 otherwise.
*/
int ghid_key_name(unsigned short int key_char, char *out, int out_len);

#endif
