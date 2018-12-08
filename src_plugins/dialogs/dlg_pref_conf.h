#ifndef PCB_DLG_PREF_CONF_H
#define PCB_DLG_PREF_CONF_H

typedef struct {
	int wtree, wintree, wdesc, wname;
} pref_conf_t;

void pcb_dlg_pref_conf_close(pref_ctx_t *ctx);
void pcb_dlg_pref_conf_create(pref_ctx_t *ctx);

#endif
