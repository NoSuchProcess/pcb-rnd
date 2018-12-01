extern const char pcb_acts_DrcDialog[];
extern const char pcb_acth_DrcDialog[];
fgw_error_t pcb_act_DrcDialog(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv);

extern const char pcb_acts_IOIncompatList[];
extern const char pcb_acth_IOIncompatList[];
fgw_error_t pcb_act_IOIncompatList(fgw_arg_t *res, int argc, fgw_arg_t *argv);

void pcb_view_dlg_uninit(void);
void pcb_view_dlg_init(void);

