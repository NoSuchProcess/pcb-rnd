#include "conf.h"
#include "event.h"

void pcb_rst_update_ev(void *user_data, int argc, pcb_event_arg_t argv[]);
void pcb_rst_gui_init_ev(void *user_data, int argc, pcb_event_arg_t argv[]);
void pcb_rst_update_conf(conf_native_t *cfg, int arr_idx);

