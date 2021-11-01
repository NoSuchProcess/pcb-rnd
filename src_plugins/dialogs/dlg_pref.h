#ifndef PCB_DLG_PREF_H
#define PCB_DLG_PREF_H

typedef struct pref_ctx_s pref_ctx_t;

#include <librnd/core/conf.h>
#include <librnd/core/conf_hid.h>
#include "dlg_pref_win.h"
#include "dlg_pref_key.h"
#include "dlg_pref_menu.h"
#include "dlg_pref_conf.h"

typedef struct pref_conflist_s pref_confitem_t;
struct pref_conflist_s {
	const char *label;
	const char *confpath;
	int wid;
	pref_confitem_t *cnext; /* linked list for conf callback - should be NULL initially */
};

typedef enum Rnd_pref_tab_flag_e { /* bitfield */
	Rnd_PREFTAB_NEEDS_ROLE = 1,
	Rnd_PREFTAB_AUTO_FREE_DATA = 2       /* free tab data when plugin is unloaded */
} Rnd_pref_tab_flag_t;

typedef struct Rnd_pref_tab_hook_s Rnd_pref_tab_hook_t;
struct Rnd_pref_tab_hook_s {
	const char *tab_label;
	unsigned long flags;                /* bitfield of Rnd_pref_tab_flag_t */

	void (*open_cb)(pref_ctx_t *ctx);   /* called right after the dialog box is created */
	void (*close_cb)(pref_ctx_t *ctx);  /* called from the dialog box is close_cb event */
	void (*create_cb)(pref_ctx_t *ctx); /* called while the dialog box is being created: create widgets in current tab */

	void (*board_changed_cb)(pref_ctx_t *ctx); /* called if the board got replaced (e.g. new board loaded) */
	void (*meta_changed_cb)(pref_ctx_t *ctx);  /* called if the board metadata changed */

	void (*spare_f1)(); void (*spare_f2)(); void (*spare_f3)(); void (*spare_f4)();
	void *spare_p1, *spare_p2, *spare_p3, *spare_p4;
	long spare_l1, spare_l2, spare_l3, spare_l4;
};

#define Rnd_PREF_MAX_TAB 32

struct pref_ctx_s {
	RND_DAD_DECL_NOINIT(dlg)
	int wtab, wrole, wrolebox;
	int active; /* already open - allow only one instance */

	struct {
		const Rnd_pref_tab_hook_t *hooks;
		void *tabdata;
	} tab[Rnd_PREF_MAX_TAB];
	int tabs;       /* number of app-specific tabs used */
	int tabs_total; /* number of tabs used (app-specific and built-in combined) */

	/* builtin tabs */
	pref_win_t win;
	pref_key_t key;
	pref_menu_t menu;
	pref_conf_t conf;

	rnd_conf_role_t role; /* where changes are saved to */

	pref_confitem_t *pcb_conf_lock; /* the item being changed - should be ignored in a conf change callback */
	vtp0_t auto_free; /* free() each item on close */
};

extern pref_ctx_t pref_ctx;

/* Create label-input widget pair for editing a conf item, or create whole
   list of them */
void pcb_pref_create_conf_item(pref_ctx_t *ctx, pref_confitem_t *item, void (*change_cb)(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr));
void pcb_pref_create_conftable(pref_ctx_t *ctx, pref_confitem_t *list, void (*change_cb)(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr));

/* Set the config node from the current widget value of a conf item, or
   create whole list of them; the table version returns whether the item is found. */
void pcb_pref_dlg2conf_item(pref_ctx_t *ctx, pref_confitem_t *item, rnd_hid_attribute_t *attr);
rnd_bool pcb_pref_dlg2conf_table(pref_ctx_t *ctx, pref_confitem_t *list, rnd_hid_attribute_t *attr);

/* Remove conf change binding - shall be called when widgets are removed
   (i.e. on dialog box close) */
void pcb_pref_conflist_remove(pref_ctx_t *ctx, pref_confitem_t *list);

extern rnd_conf_hid_id_t pref_hid;

/*** pulbic API for the caller ***/
void pcb_dlg_pref_init(void);
void pcb_dlg_pref_uninit(void);

extern const char pcb_acts_Preferences[];
extern const char pcb_acth_Preferences[];
fgw_error_t pcb_act_Preferences(fgw_arg_t *res, int argc, fgw_arg_t *argv);

#endif
