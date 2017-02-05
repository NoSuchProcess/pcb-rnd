#include <glib.h>
#include "hid.h"

pcb_hidval_t ghid_add_block_hook(void (*func) (pcb_hidval_t data), pcb_hidval_t user_data);
void ghid_stop_block_hook(pcb_hidval_t mlpoll);

gboolean ghid_block_hook_prepare(GSource * source, gint * timeout);
gboolean ghid_block_hook_check(GSource * source);
gboolean ghid_block_hook_dispatch(GSource * source, GSourceFunc callback, gpointer user_data);
