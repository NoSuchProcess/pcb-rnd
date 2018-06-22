#ifndef PCB_GTK_DLG_LOG_H
#define PCB_GTK_DLG_LOG_H

#include "pcb_bool.h"
#include "error.h"
#include <libfungw/fungw.h>

/* Creates or raises the _log_ window dialog. */
void pcb_gtk_dlg_log_show(pcb_bool raise);

/* Logs a new message. */
void pcb_gtk_logv(int hid_active, enum pcb_message_level level, const char *fmt, va_list args);


/* Actions */
extern const char pcb_gtk_acts_logshowonappend[];
extern const char pcb_gtk_acth_logshowonappend[];
fgw_error_t pcb_gtk_act_logshowonappend(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv);

#endif /* PCB_GTK_DLG_LOG_H */
