#ifndef PCB_DLG_PREF_CONF_H
#define PCB_DLG_PREF_CONF_H

#include <librnd/core/conf.h>

typedef struct {
	int wtree, wintree, wdesc, wname, wmainp, wnattype, wfilter;
	int wnatval[RND_CFN_max+1], wsrc[RND_CFN_max+1];
	rnd_conf_native_t *selected_nat;
	int selected_idx;
} pref_conf_t;

void pcb_dlg_pref_conf_close(pref_ctx_t *ctx);
void pcb_dlg_pref_conf_create(pref_ctx_t *ctx);
void pcb_dlg_pref_conf_open(pref_ctx_t *ctx, const char *tabarg);

void pcb_pref_dlg_conf_changed_cb(pref_ctx_t *ctx, rnd_conf_native_t *cfg, int arr_idx); /* global conf change */

#endif
