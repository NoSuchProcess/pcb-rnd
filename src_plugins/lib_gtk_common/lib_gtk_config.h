#include <gtk/gtk.h>
#include "hid.h"
#include "event.h"
#include "conf_hid.h"
#include "../src_plugins/lib_gtk_common/glue.h"

extern conf_hid_id_t ghid_conf_id;

void pcb_gtk_conf_init(void);
void pcb_gtk_conf_uninit(void);

/* Parses string_path to expand and select the corresponding path in tree view. */
void pcb_gtk_config_set_cursor(const char *string_path);



