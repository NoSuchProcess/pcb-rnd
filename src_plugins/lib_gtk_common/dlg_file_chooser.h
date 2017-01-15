#include <gtk/gtk.h>

#include "compat_nls.h"
#include "plug_io.h"

#include "util_str.h"


/* This needs to be changed */
#include "../hid_gtk/gui.h"

gchar *ghid_dialog_file_select_open(const gchar * title, gchar ** path, const gchar * shortcuts);
gchar *ghid_dialog_file_select_save(const gchar * title, gchar ** path, const gchar * file, const gchar * shortcuts,
																		const char **formats, const char **extensions, int *format);
gchar *ghid_fileselect(const char *title, const char *descr,
											 const char *default_file, const char *default_ext, const char *history_tag, int flags);
