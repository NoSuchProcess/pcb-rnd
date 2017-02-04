#include <gtk/gtk.h>

/*FIXME :( Needed for  ModifierKeysState */
#include "../hid_gtk/gui.h"

/** */
gboolean ghid_is_modifier_key_sym(gint ksym);

/** Returns key modifier state */
ModifierKeysState ghid_modifier_keys_state(GdkModifierType * state);

/** Handle user keys in the output drawing area. */
gboolean ghid_port_key_press_cb(GtkWidget *drawing_area, GdkEventKey *kev, gpointer data);

