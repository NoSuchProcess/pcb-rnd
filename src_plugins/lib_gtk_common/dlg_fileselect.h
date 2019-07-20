#include "global_typedefs.h"
#include <gtk/gtk.h>

char *pcb_gtk_fileselect(pcb_gtk_impl_t *com, const char *title, const char *descr, const char *default_file, const char *default_ext, const pcb_hid_fsd_filter_t *flt, const char *history_tag, pcb_hid_fsd_flags_t flags, pcb_hid_dad_subdialog_t *sub);
