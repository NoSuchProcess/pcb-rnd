#include <gtk/gtk.h>
#include "gui.h"

void ghid_update_toggle_flags(void);
void ghid_notify_save_pcb(const char *file, pcb_bool done);
void ghid_notify_filename_changed(void);
void ghid_install_accel_groups(GtkWindow * window, GhidGui * gui);
void ghid_remove_accel_groups(GtkWindow * window, GhidGui * gui);
