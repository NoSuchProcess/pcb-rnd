#include <gtk/gtk.h>
#include <libfungw/fungw.h>

extern const char pcb_gtk_acts_print[];
extern const char pcb_gtk_acth_print[];
fgw_error_t pcb_gtk_act_print(GtkWidget *top_window, fgw_arg_t *ores, int oargc, fgw_arg_t *oargv);

extern const char pcb_gtk_acts_printcalibrate[];
extern const char pcb_gtk_acth_printcalibrate[];
fgw_error_t pcb_gtk_act_printcalibrate(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv);

