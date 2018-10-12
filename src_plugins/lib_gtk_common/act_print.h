#include <gtk/gtk.h>
#include <libfungw/fungw.h>
#include "glue.h"

extern const char pcb_gtk_acts_print[];
extern const char pcb_gtk_acth_print[];
fgw_error_t pcb_gtk_act_print(pcb_gtk_common_t *com, fgw_arg_t *ores, int oargc, fgw_arg_t *oargv);

extern const char pcb_gtk_acts_printcalibrate[];
extern const char pcb_gtk_acth_printcalibrate[];
fgw_error_t pcb_gtk_act_printcalibrate(fgw_arg_t *res, int argc, fgw_arg_t *argv);

