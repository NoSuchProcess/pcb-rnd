#include "hid.h"

pcb_hidval_t ghid_add_timer(void (*func) (pcb_hidval_t user_data), unsigned long milliseconds, pcb_hidval_t user_data);
void ghid_stop_timer(pcb_hidval_t timer);
