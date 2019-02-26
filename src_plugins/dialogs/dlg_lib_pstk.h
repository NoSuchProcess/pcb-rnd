/* Open a padstack library dialog box for a subc (or for the board if subc_id
   is 0). If modal, returns the selected prototype or PCB_PADSTACK_INVALID when
   nothing is selected or the operation cancelled. In non-modal mode return
   0 on success and PCB_PADSTACK_INVALID on error. */
pcb_cardinal_t pcb_dlg_pstklib(pcb_board_t *pcb, long subc_id, pcb_bool modal, const char *hint);

extern const char pcb_acts_pstklib[];
extern const char pcb_acth_pstklib[];
fgw_error_t pcb_act_pstklib(fgw_arg_t *res, int argc, fgw_arg_t *argv);

void pcb_dlg_pstklib_init(void);
void pcb_dlg_pstklib_uninit(void);
