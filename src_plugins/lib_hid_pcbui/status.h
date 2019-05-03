#include "event.h"
#include "conf.h"
#include "actions.h"

void pcb_status_gui_init_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[]);
void pcb_status_st_update_conf(conf_native_t *cfg, int arr_idx);
void pcb_status_rd_update_conf(conf_native_t *cfg, int arr_idx);
void pcb_status_st_update_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[]);
void pcb_status_rd_update_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[]);

extern const char pcb_acts_StatusSetText[];
extern const char pcb_acth_StatusSetText[];
fgw_error_t pcb_act_StatusSetText(fgw_arg_t *res, int argc, fgw_arg_t *argv);

extern const char pcb_acts_DescribeLocation[];
extern const char pcb_acth_DescribeLocation[];
fgw_error_t pcb_act_DescribeLocation(fgw_arg_t *res, int argc, fgw_arg_t *argv);
