#include <gtk/gtk.h>

#include "compat_nls.h"
#include "plug_io.h"

#include "util_str.h"

gchar *ghid_dialog_file_select_open(GtkWidget *top_window, const gchar *title, gchar **path, const gchar *shortcuts);

gchar *ghid_dialog_file_select_save(GtkWidget *top_window, const gchar *title, gchar **path, const gchar *file, const gchar *shortcuts, const char **formats, const char **extensions, int *format);

gchar *pcb_gtk_fileselect(GtkWidget *top_window, const char *title, const char *descr, const char *default_file, const char *default_ext, const char *history_tag, pcb_hid_fsd_flags_t flags);
