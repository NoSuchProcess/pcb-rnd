#include "plug_footprint.h"
#include "dlg_library.h"

/** Parse the help of a parametric footprint, offer GUI to change parameters.
    If filter_txt is non-NULL, it contains initial parameters from the user.
    Returns a dynamically allocated parametric footprint command */
char *pcb_gtk_library_param_ui(pcb_gtk_library_t *library_window, pcb_fplibrary_t *entry, const char *filter_txt);

