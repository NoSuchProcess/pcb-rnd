/* Open a padstack library dialog box for a subc (or for the board if subc_id
   is 0). If modal, returns the selected prototype or PCB_PADSTACK_INVALID when
   nothing is selected or the operation cancelled. In non-modal mode return
   0 on success and PCB_PADSTACK_INVALID on error. */
pcb_cardinal_t pcb_dlg_pstklib(pcb_board_t *pcb, long subc_id, pcb_bool modal, const char *hint);
