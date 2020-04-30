#include <librnd/core/event.h>
#include <librnd/core/conf.h>

/* Automatic initialization (event and conf change bindings) */
void rnd_toolbar_init(void);
void rnd_toolbar_uninit(void);


/* Alternatively, the caller can bind these */
void pcb_toolbar_gui_init_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[]);
void pcb_toolbar_reg_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[]);
void pcb_toolbar_update_conf(rnd_conf_native_t *cfg, int arr_idx);


