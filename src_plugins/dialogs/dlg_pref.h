#ifndef PCB_DLG_PREF_H
#define PCB_DLG_PREF_H

typedef struct pref_ctx_s pref_ctx_t;

#include "conf.h"
#include "conf_hid.h"
#include "dlg_pref_sizes.h"
#include "dlg_pref_board.h"
#include "dlg_pref_general.h"
#include "dlg_pref_lib.h"
#include "dlg_pref_color.h"
#include "dlg_pref_win.h"
#include "dlg_pref_conf.h"

typedef struct pref_conflist_s pref_confitem_t;
struct pref_conflist_s {
	const char *label;
	const char *confpath;
	int wid;
	pref_confitem_t *cnext; /* linked list for conf callback - should be NULL initially */
};

struct pref_ctx_s {
	PCB_DAD_DECL_NOINIT(dlg)
	int wtab, wrole;
	int active; /* already open - allow only one instance */

	pref_sizes_t sizes;
	pref_board_t board;
	pref_general_t general;
	pref_lib_t lib;
	pref_color_t color;
	pref_win_t win;
	pref_conf_t conf;

	conf_role_t role; /* where changes are saved to */

	pref_confitem_t *pcb_conf_lock; /* the item being changed - should be ignored in a conf change callback */
};

extern pref_ctx_t pref_ctx;

/* Create label-input widget pair for editing a conf item, or create whole
   list of them */
void pcb_pref_create_conf_item(pref_ctx_t *ctx, pref_confitem_t *item, void (*change_cb)(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr));
void pcb_pref_create_conftable(pref_ctx_t *ctx, pref_confitem_t *list, void (*change_cb)(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr));

/* Set the config node from the current widget value of a conf item, or
   create whole list of them; the table version returns whether the item is found. */
void pcb_pref_dlg2conf_item(pref_ctx_t *ctx, pref_confitem_t *item, pcb_hid_attribute_t *attr);
pcb_bool pcb_pref_dlg2conf_table(pref_ctx_t *ctx, pref_confitem_t *list, pcb_hid_attribute_t *attr);

/* Remove conf change binding - shall be called when widgets are removed
   (i.e. on dialog box close) */
void pcb_pref_conflist_remove(pref_ctx_t *ctx, pref_confitem_t *list);

conf_hid_id_t pref_hid;

/*** pulbic API for the caller ***/
void pcb_dlg_pref_init(void);
void pcb_dlg_pref_uninit(void);

extern const char pcb_acts_Preferences[];
extern const char pcb_acth_Preferences[];
fgw_error_t pcb_act_Preferences(fgw_arg_t *res, int argc, fgw_arg_t *argv);

#endif
