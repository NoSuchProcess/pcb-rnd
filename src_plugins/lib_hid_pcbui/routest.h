#include <librnd/core/conf.h>
#include <librnd/core/event.h>
#include <librnd/core/actions.h>

void pcb_rst_update_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[]);
void pcb_rst_gui_init_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[]);
void pcb_rst_update_conf(rnd_conf_native_t *cfg, int arr_idx);
void pcb_rst_menu_batch_timer_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[]);


extern const char pcb_acts_AdjustStyle[];
extern const char pcb_acth_AdjustStyle[];
fgw_error_t pcb_act_AdjustStyle(fgw_arg_t *res, int argc, fgw_arg_t *argv);
