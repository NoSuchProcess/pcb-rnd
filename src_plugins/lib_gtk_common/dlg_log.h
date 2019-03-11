#ifndef PCB_GTK_DLG_LOG_H
#define PCB_GTK_DLG_LOG_H

#include "pcb_bool.h"
#include "error.h"
#include <libfungw/fungw.h>

/* Actions */
extern const char pcb_gtk_acts_logshowonappend[];
extern const char pcb_gtk_acth_logshowonappend[];
fgw_error_t pcb_gtk_act_logshowonappend(fgw_arg_t *res, int argc, fgw_arg_t *argv);

#endif /* PCB_GTK_DLG_LOG_H */
