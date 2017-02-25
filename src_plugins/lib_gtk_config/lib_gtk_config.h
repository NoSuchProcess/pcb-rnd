#include <gtk/gtk.h>
#include "hid.h"
#include "conf_hid.h"
#include "../src_plugins/lib_gtk_common/glue.h"

extern conf_hid_id_t ghid_conf_id;

/* called before the first conf access, from hid_gtk* */
void pcb_gtk_conf_init(void);

void ghid_config_window_show(pcb_gtk_common_t *com);

void ghid_config_handle_units_changed(void *gport);

void config_color_button_update(pcb_gtk_common_t *com, conf_native_t *cfg, int idx);

/* temporary back reference to hid_gtk */
void ghid_pack_mode_buttons(void);
int ghid_command_entry_is_active(void);
void ghid_command_use_command_window_sync(void);

