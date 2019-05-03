#include "conf.h"
#include "event.h"
#include "actions.h"

void pcb_rst_update_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[]);
void pcb_rst_gui_init_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[]);
void pcb_rst_update_conf(conf_native_t *cfg, int arr_idx);


extern const char pcb_acts_AdjustStyle[];
extern const char pcb_acth_AdjustStyle[];
fgw_error_t pcb_act_AdjustStyle(fgw_arg_t *res, int argc, fgw_arg_t *argv);
