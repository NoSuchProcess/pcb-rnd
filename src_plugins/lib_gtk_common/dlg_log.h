#ifndef PCB_GTK_DLG_LOG_H
#define PCB_GTK_DLG_LOG_H

#include "pcb_bool.h"
#include "error.h"

/** Creates or raises the _log_ window dialog.
    \param raise    if `TRUE`, presents the window.
 */
void pcb_gtk_dlg_log_show(pcb_bool raise);

/** Logs a new message.
    \param level    The message level (warning, etc...)
    \param fmt      A PCB format used to format \p args.
    \param args     A variable list of arguments.
 */
void pcb_gtk_logv(int hid_active, enum pcb_message_level level, const char *fmt, va_list args);


/* Actions */
extern const char pcb_gtk_acts_logshowonappend[];
extern const char pcb_gtk_acth_logshowonappend[];
int pcb_gtk_act_logshowonappend(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y);

#endif /* PCB_GTK_DLG_LOG_H */
