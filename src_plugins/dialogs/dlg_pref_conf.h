#ifndef PCB_DLG_PREF_CONF_H
#define PCB_DLG_PREF_CONF_H

#include "conf.h"

typedef struct {
	int wtree, wintree, wmemtree, wdesc, wname, wmainp;
	conf_native_t *selected_nat;
} pref_conf_t;

void pcb_dlg_pref_conf_close(pref_ctx_t *ctx);
void pcb_dlg_pref_conf_create(pref_ctx_t *ctx);
void pcb_dlg_pref_conf_open(pref_ctx_t *ctx);

void pcb_pref_dlg_conf_changed_cb(pref_ctx_t *ctx, conf_native_t *cfg, int arr_idx); /* global conf change */

#endif
