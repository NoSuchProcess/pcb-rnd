#ifndef PCB_DLG_PREF_COLOR_H
#define PCB_DLG_PREF_COLOR_H

typedef struct {
	int *wgen, *wlayer;
} pref_color_t;

void pcb_dlg_pref_color_create(pref_ctx_t *ctx);

#endif
