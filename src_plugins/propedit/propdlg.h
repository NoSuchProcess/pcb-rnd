#ifndef PCB_PROPDLG_H
#define PCB_PROPDLG_H

#include <librnd/core/actions.h>

extern const char pcb_acts_propedit[];
extern const char pcb_acth_propedit[];
fgw_error_t pcb_act_propedit(fgw_arg_t *res, int argc, fgw_arg_t *argv);

void pcb_propdlg_init(void);
void pcb_propdlg_uninit(void);

#endif
