#ifndef PCB_DLG_PREF_MENU_H
#define PCB_DLG_PREF_MENU_H

typedef struct {
	int wlist, wdesc, wload, wunload, wreload, wexport;
} pref_menu_t;

void pcb_dlg_pref_menu_create(pref_ctx_t *ctx);

#endif
