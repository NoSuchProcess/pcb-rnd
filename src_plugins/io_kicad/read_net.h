#include <librnd/core/unit.h>
#include <libfungw/fungw.h>

extern const char pcb_acts_LoadeeschemaFrom[];
extern const char pcb_acth_LoadeeschemaFrom[];
fgw_error_t pcb_act_LoadeeschemaFrom(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv);


void pcb_eeschema_init(void);
void pcb_eeschema_uninit(void);

