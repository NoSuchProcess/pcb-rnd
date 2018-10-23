#ifndef PCB_DLG_PREF_H
#define PCB_DLG_PREF_H

#include "dlg_pref_sizes.h"

typedef struct {
	const char *label;
	const char *confpath;
	int wid;
} pref_conflist_t;

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */
	pref_sizes_t sizes;
} pref_ctx_t;

/* Create label-input widget pair for editing a conf item, or create whole
   list of them */
void pcb_pref_create_conf_item(pref_ctx_t *ctx, pref_conflist_t *item, void (*change_cb)(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr));
void pcb_pref_create_conftable(pref_ctx_t *ctx, pref_conflist_t *list, void (*change_cb)(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr));

/* Set the config node from the current widget value of a conf item, or
   create whole list of them */
void pcb_pref_dlg2conf_item(pref_ctx_t *ctx, pref_conflist_t *item, pcb_hid_attribute_t *attr);
void pcb_pref_dlg2conf_table(pref_ctx_t *ctx, pref_conflist_t *list, pcb_hid_attribute_t *attr);

#endif
