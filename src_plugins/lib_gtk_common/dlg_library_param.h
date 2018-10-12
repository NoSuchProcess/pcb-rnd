#include "plug_footprint.h"
#include "dlg_library.h"

typedef struct pcb_gtk_library_param_cb_ctx_s pcb_gtk_library_param_cb_ctx_t;

typedef void (*pcb_gtk_library_param_cb_t)(pcb_gtk_library_param_cb_ctx_t *param_ctx);

struct pcb_gtk_library_param_cb_ctx_s {
	pcb_gtk_library_t *library_window;
	pcb_fplibrary_t *entry;
	pcb_gtk_library_param_cb_t cb;

	/* private */
	pcb_hid_attribute_t *attrs;
	pcb_hid_attr_val_t *res;
	int *numattr;
	int first_optional;
};

/* Parse the "--help" of a parametric footprint, offer GUI to change parameters.
   If filter_txt is non-NULL, it contains initial parameters from the user.
   Returns a dynamically allocated parametric footprint command */
char *pcb_gtk_library_param_ui(void *com, pcb_gtk_library_t *library_window, pcb_fplibrary_t *entry, const char *filter_txt, pcb_gtk_library_param_cb_t cb);


/* Called from a library_param_cb to return the current command line (built from
   the dialog box fields) */
char *pcb_gtk_library_param_snapshot(pcb_gtk_library_param_cb_ctx_t *param_ctx);
