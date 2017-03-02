#include <gtk/gtk.h>
#include "hid.h"
#include "conf_hid.h"
#include "../src_plugins/lib_gtk_common/glue.h"

extern conf_hid_id_t ghid_conf_id;

/* called before the first conf access, from hid_gtk* */
void pcb_gtk_conf_init(void);

void ghid_config_window_show(pcb_gtk_common_t *com);

void ghid_config_handle_units_changed(pcb_gtk_common_t *com);

void config_color_button_update(pcb_gtk_common_t *com, conf_native_t *cfg, int idx);
