#include "event.h"
#include "conf.h"

void pcb_toolbar_gui_init_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[]);
void pcb_toolbar_reg_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[]);
void pcb_toolbar_update_conf(conf_native_t *cfg, int arr_idx);

