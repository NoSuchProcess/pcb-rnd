#include <genht/htsi.h>

typedef struct {
	int pactive; /* already open - allow only one instance */
	int pwdesc;
	RND_DAD_DECL_NOINIT(pdlg)
	library_ent_t *last_l;
	char *example, *help_params;
	htsi_t param_names;     /* param_name -> param_idx */
	int pwid[MAX_PARAMS];   /* param_idx -> widget_idx (for the input field widget) */
	char *pnames[MAX_PARAMS]; /* param_idx -> parameter_name (also key stored in the param_names hash */
	int num_params, first_optional;
	gds_t descr;
	library_ctx_t *lib_ctx;
} library_param_ctx_t;
