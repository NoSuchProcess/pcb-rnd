#ifndef PCB_GTK_DLG_LOG_H
#define PCB_GTK_DLG_LOG_H

#include "pcb_bool.h"
#include "error.h"

/** Creates or raises the _log_ window dialog.
    \param raise    if `TRUE`, presents the window.
 */
void pcb_gtk_dlg_log_show(pcb_bool raise);

/** A vararg function (variable number of arguments), calling \ref pcb_gtk_logv ().
    \param fmt  The format used.
 */
void pcb_gtk_log(const char *fmt, ...);

/** Logs a new message.
    \param level    The message level (warning, etc...)
    \param fmt      A PCB format used to format \p args.
    \param args     A variable list of arguments.
 */
void pcb_gtk_logv(enum pcb_message_level level, const char *fmt, va_list args);

/* Temporary back reference to hid_gtk */
extern const char *ghid_cookie;

#endif /* PCB_GTK_DLG_LOG_H */
