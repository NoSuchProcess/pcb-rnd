#include <librnd/core/actions.h>

double pcb_impedance_microstrip(rnd_coord_t trace_width, rnd_coord_t trace_height, rnd_coord_t subst_height, double dielect);

extern const char pcb_acts_impedance_microstrip[];
extern const char pcb_acth_impedance_microstrip[];
fgw_error_t pcb_act_impedance_microstrip(fgw_arg_t *res, int argc, fgw_arg_t *argv);
