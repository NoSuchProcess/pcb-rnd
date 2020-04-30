#include <librnd/core/conf.h>
void pcb_wplc_load(rnd_conf_role_t role);


/*** for internal use ***/
void pcb_dialog_place_uninit(void);
void pcb_dialog_place_init(void);
void pcb_dialog_resize(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[]);
void pcb_dialog_place(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[]);

