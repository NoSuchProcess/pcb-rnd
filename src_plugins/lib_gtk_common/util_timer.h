#include "hid.h"
#include "pcb_gtk.h"

pcb_hidval_t pcb_gtk_add_timer(struct pcb_gtk_s *gctx, void (*func) (pcb_hidval_t user_data), unsigned long milliseconds, pcb_hidval_t user_data);
void ghid_stop_timer(pcb_hidval_t timer);
