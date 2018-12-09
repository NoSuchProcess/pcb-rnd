#ifndef PCB_DLG_PREF_CONF_H
#define PCB_DLG_PREF_CONF_H

typedef struct {
	int wtree, wintree, wmemtree, wdesc, wname, wmainp;
} pref_conf_t;

void pcb_dlg_pref_conf_close(pref_ctx_t *ctx);
void pcb_dlg_pref_conf_create(pref_ctx_t *ctx);
void pcb_dlg_pref_conf_open(pref_ctx_t *ctx);

#endif
