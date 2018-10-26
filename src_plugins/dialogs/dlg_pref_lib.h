#ifndef PCB_DLG_PREF_LIB_H
#define PCB_DLG_PREF_LIB_H

typedef struct pref_libhelp_ctx_s {
	PCB_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */
} pref_libhelp_ctx_t;

typedef struct {
	int wlist, whsbutton;
	int lock; /* a change in on the dialog box causes a change on the board but this shouldn't in turn casue a changein the dialog */
	char *cursor_path;
	pref_libhelp_ctx_t help;
} pref_lib_t;

void pcb_dlg_pref_lib_close(pref_ctx_t *ctx);
void pcb_dlg_pref_lib_create(pref_ctx_t *ctx);
void pcb_dlg_pref_lib_init(pref_ctx_t *ctx);
void pcb_dlg_pref_lib_open(pref_ctx_t *ctx);


#endif
