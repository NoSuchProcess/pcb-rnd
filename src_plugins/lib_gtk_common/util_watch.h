#include "hid.h"
#include "pcb_gtk.h"

pcb_hidval_t pcb_gtk_watch_file(pcb_gtk_impl_t *com, int fd, unsigned int condition,
								pcb_bool (*func)(pcb_hidval_t watch, int fd, unsigned int condition, pcb_hidval_t user_data),
								pcb_hidval_t user_data);

void pcb_gtk_unwatch_file(pcb_hidval_t data);

