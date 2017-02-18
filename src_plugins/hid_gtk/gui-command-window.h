#include <glib.h>

void ghid_handle_user_command(gboolean raise);
void ghid_command_window_show(gboolean raise);
gchar *ghid_command_entry_get(const gchar * prompt, const gchar * command);
void ghid_command_use_command_window_sync(void);
