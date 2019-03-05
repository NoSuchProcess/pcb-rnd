#include <gtk/gtk.h>

#include "compat_nls.h"
#include "plug_io.h"

#include "util_str.h"

gchar *ghid_dialog_file_select_open(GtkWidget *top_window, const gchar *title, gchar **path, const gchar *shortcuts);

gchar *ghid_dialog_file_select_save(GtkWidget *top_window, const gchar *title, gchar **path, const gchar *file, const gchar *shortcuts, const char **formats, const char **extensions, int *format);


