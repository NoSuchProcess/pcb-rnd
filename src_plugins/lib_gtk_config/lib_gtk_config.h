#include <gtk/gtk.h>
#include "hid.h"
#include "event.h"
#include "conf_hid.h"
#include "../src_plugins/lib_gtk_common/glue.h"

extern conf_hid_id_t ghid_conf_id;

/* called before the first conf access, from hid_gtk* */
void pcb_gtk_conf_init(void);

void pcb_gtk_config_window_show(pcb_gtk_common_t *com, gboolean raise);

void ghid_config_handle_units_changed(pcb_gtk_common_t *com);

/** Parses \p string_path to expand and select the corresponding path in tree view. */
void pcb_gtk_config_set_cursor(const char *string_path);

void config_color_button_update(pcb_gtk_common_t *com, conf_native_t *cfg, int idx);

void ghid_wgeo_save(int save_to_file, int skip_user);
void ghid_conf_save_pre_wgeo(void *user_data, int argc, pcb_event_arg_t argv[]);
void ghid_conf_load_post_wgeo(void *user_data, int argc, pcb_event_arg_t argv[]);

void ghid_config_init(void);

