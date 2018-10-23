#include "dlg_pref_sizes.h"

typedef struct{
	PCB_DAD_DECL_NOINIT(dlg)
	int active; /* already open - allow only one instance */
	pref_sizes_t sizes;
} pref_ctx_t;
