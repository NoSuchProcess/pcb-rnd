#include <librnd/core/hid.h>
#include "pcb_gtk.h"

rnd_hidval_t pcb_gtk_add_timer(struct pcb_gtk_s *gctx, void (*func)(rnd_hidval_t user_data), unsigned long milliseconds, rnd_hidval_t user_data);
void ghid_stop_timer(rnd_hid_t *hid, rnd_hidval_t timer);
