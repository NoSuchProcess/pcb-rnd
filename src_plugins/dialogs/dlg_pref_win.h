#ifndef PCB_DLG_PREF_WIN_H
#define PCB_DLG_PREF_WIN_H

typedef struct {
	int wmaster, wboard, wproject, wuser;
} pref_win_t;

void pcb_dlg_pref_win_create(pref_ctx_t *ctx);

#endif
