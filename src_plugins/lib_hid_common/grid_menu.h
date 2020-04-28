#include <librnd/core/conf.h>
#include <librnd/core/event.h>

void pcb_grid_update_conf(rnd_conf_native_t *cfg, int arr_idx);
void pcb_grid_update_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[]);
