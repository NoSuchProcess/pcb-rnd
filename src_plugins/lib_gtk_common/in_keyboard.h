#include <gtk/gtk.h>

/*FIXME :( Needed for  ModifierKeysState */
#include "../hid_gtk/gui.h"

/** */
gboolean ghid_is_modifier_key_sym(gint ksym);

/** Returns key modifier state */
ModifierKeysState ghid_modifier_keys_state(GdkModifierType * state);
