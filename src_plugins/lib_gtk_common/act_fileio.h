#include <gtk/gtk.h>
#include <libfungw/fungw.h>
#include "unit.h"

fgw_error_t pcb_gtk_act_load(GtkWidget *top_window, int oargc, const char **oargv);

extern const char pcb_gtk_acts_save[];
extern const char pcb_gtk_acth_save[];
fgw_error_t pcb_gtk_act_save(GtkWidget *top_window, int oargc, const char **oargv);

extern const char pcb_gtk_acts_importgui[];
extern const char pcb_gtk_acth_importgui[];
fgw_error_t pcb_gtk_act_importgui(GtkWidget *top_window, int oargc, const char **oargv);
