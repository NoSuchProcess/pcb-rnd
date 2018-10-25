#ifndef PCB_DLG_PREF_BOARD_H
#define PCB_DLG_PREF_BOARD_H

typedef struct {
	int wname, wthermscale, wtype;
} pref_board_t;

void pcb_dlg_pref_board_close(pref_ctx_t *ctx);
void pcb_dlg_pref_board_create(pref_ctx_t *ctx);

#endif
