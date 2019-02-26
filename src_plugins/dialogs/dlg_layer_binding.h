#ifndef PCB_DLG_LAYRE_BINDING_H
#define PCB_DLG_LAYRE_BINDING_H

extern const char pcb_acts_LayerBinding[];
extern const char pcb_acth_LayerBinding[];
fgw_error_t pcb_act_LayerBinding(fgw_arg_t *res, int argc, fgw_arg_t *argv);

extern const char *pcb_lb_comp[];
extern const char *pcb_lb_types[];
extern const char *pcb_lb_side[];

int pcb_ly_type2enum(pcb_layer_type_t type);
void pcb_get_ly_type_(int combo_type, pcb_layer_type_t *type);

#endif
