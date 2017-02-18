#include <glib.h>
#include "error.h"

void ghid_log_window_create();
void ghid_log_window_show(gboolean raise);
void ghid_log(const char *fmt, ...);
void ghid_logv(enum pcb_message_level level, const char *fmt, va_list args);
