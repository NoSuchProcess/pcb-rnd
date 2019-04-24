#include "conf.h"
void pcb_wplc_load(conf_role_t role);


/*** for internal use ***/
void pcb_dialog_place_uninit(void);
void pcb_dialog_place_init(void);
void pcb_dialog_resize(void *user_data, int argc, pcb_event_arg_t argv[]);
void pcb_dialog_place(void *user_data, int argc, pcb_event_arg_t argv[]);

