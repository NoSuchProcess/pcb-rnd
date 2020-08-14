#ifndef PCB_DLG_PREF_KEY_H
#define PCB_DLG_PREF_KEY_H

typedef struct {
	int wlist;
	int lock;
} pref_key_t;

void pcb_dlg_pref_key_create(pref_ctx_t *ctx);

#endif
