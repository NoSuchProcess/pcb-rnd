#include <librnd/core/global_typedefs.h>
#include <gtk/gtk.h>

char *pcb_gtk_fileselect(pcb_gtk_t *gctx, const char *title, const char *descr, const char *default_file, const char *default_ext, const rnd_hid_fsd_filter_t *flt, const char *history_tag, rnd_hid_fsd_flags_t flags, rnd_hid_dad_subdialog_t *sub);
