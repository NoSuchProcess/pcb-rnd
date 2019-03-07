#include <gtk/gtk.h>
#include <libfungw/fungw.h>
#include "unit.h"


extern const char pcb_gtk_acts_importgui[];
extern const char pcb_gtk_acth_importgui[];
fgw_error_t pcb_gtk_act_importgui(GtkWidget *top_window, fgw_arg_t *res, int argc, fgw_arg_t *argv);
