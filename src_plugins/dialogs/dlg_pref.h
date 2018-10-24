#ifndef PCB_DLG_PREF_H
#define PCB_DLG_PREF_H

typedef struct pref_ctx_s pref_ctx_t;

#include "dlg_pref_sizes.h"
#include "dlg_pref_general.h"

typedef struct pref_conflist_s pref_confitem_t;
struct pref_conflist_s {
	const char *label;
	const char *confpath;
	int wid;
	pref_confitem_t *cnext; /* linked list for conf callback - should be NULL initially */
};

struct pref_ctx_s {
	PCB_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */

	pref_sizes_t sizes;
	pref_general_t general;

	pref_confitem_t *conf_lock; /* the item being changed - should be ignored in a conf change callback */
};

/* Create label-input widget pair for editing a conf item, or create whole
   list of them */
void pcb_pref_create_conf_item(pref_ctx_t *ctx, pref_confitem_t *item, void (*change_cb)(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr));
void pcb_pref_create_conftable(pref_ctx_t *ctx, pref_confitem_t *list, void (*change_cb)(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr));

/* Set the config node from the current widget value of a conf item, or
   create whole list of them */
void pcb_pref_dlg2conf_item(pref_ctx_t *ctx, pref_confitem_t *item, pcb_hid_attribute_t *attr);
void pcb_pref_dlg2conf_table(pref_ctx_t *ctx, pref_confitem_t *list, pcb_hid_attribute_t *attr);

/* Remove conf change binding - shall be called when widgets are removed
   (i.e. on dialog box close) */
void pcb_pref_conflist_remove(pref_ctx_t *ctx, pref_confitem_t *list);

#endif
