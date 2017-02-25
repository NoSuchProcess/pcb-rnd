#include "hid.h"
#include "glue.h"

pcb_hidval_t pcb_gtk_add_timer(pcb_gtk_common_t *com, void (*func) (pcb_hidval_t user_data), unsigned long milliseconds, pcb_hidval_t user_data);
void ghid_stop_timer(pcb_hidval_t timer);
