#include "hid.h"

pcb_hidval_t ghid_watch_file(int fd, unsigned int condition,
								void (*func) (pcb_hidval_t watch, int fd, unsigned int condition, pcb_hidval_t user_data),
								pcb_hidval_t user_data);

void ghid_unwatch_file(pcb_hidval_t data);


