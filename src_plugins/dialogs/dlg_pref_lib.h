#ifndef PCB_DLG_PREF_LIB_H
#define PCB_DLG_PREF_LIB_H

typedef struct pref_libhelp_ctx_s {
	RND_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */
} pref_libhelp_ctx_t;

typedef struct {
	int wlist, whsbutton, wmoveup, wmovedown, wedit, wremove;
	int lock; /* a change in on the dialog box causes a change on the board but this shouldn't in turn casue a changein the dialog */
	char *cursor_path;
	pref_libhelp_ctx_t help;
} pref_lib_t;

#endif
