#include <librnd/core/hid.h>
#include "pcb_gtk.h"

rnd_hidval_t pcb_gtk_watch_file(pcb_gtk_t *gctx, int fd, unsigned int condition,
	rnd_bool (*func)(rnd_hidval_t watch, int fd, unsigned int condition, rnd_hidval_t user_data),
	rnd_hidval_t user_data);

void pcb_gtk_unwatch_file(rnd_hid_t *hid, rnd_hidval_t data);

