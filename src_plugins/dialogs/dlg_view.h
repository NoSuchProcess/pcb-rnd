extern const char pcb_acts_DrcDialog[];
extern const char pcb_acth_DrcDialog[];
fgw_error_t pcb_act_DrcDialog(fgw_arg_t *ores, int oargc, fgw_arg_t *oargv);

extern const char pcb_acts_IOIncompatListDialog[];
extern const char pcb_acth_IOIncompatListDialog[];
fgw_error_t pcb_act_IOIncompatListDialog(fgw_arg_t *res, int argc, fgw_arg_t *argv);

extern const char pcb_acts_ViewList[];
extern const char pcb_acth_ViewList[];
fgw_error_t pcb_act_ViewList(fgw_arg_t *res, int argc, fgw_arg_t *argv);

void pcb_view_dlg_uninit(void);
void pcb_view_dlg_init(void);

