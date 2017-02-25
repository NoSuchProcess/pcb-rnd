#include <gtk/gtk.h>
#include "hid.h"
#include "conf_hid.h"
#include "../src_plugins/lib_gtk_common/glue.h"

extern conf_hid_id_t ghid_conf_id;

/* called before the first conf access, from hid_gtk* */
void pcb_gtk_conf_init(void);

void ghid_config_window_show(pcb_gtk_common_t *com);

void ghid_config_handle_units_changed(void *gport);


/* temporary back reference to hid_gtk */
void ghid_pack_mode_buttons(void);
int ghid_command_entry_is_active(void);
void ghid_init_drawing_widget(GtkWidget * widget, void *gport);
void ghid_command_use_command_window_sync(void);
gboolean ghid_preview_expose(GtkWidget * widget, GdkEventExpose * ev, pcb_hid_expose_t expcall, const pcb_hid_expose_ctx_t *ctx);
void ghid_layer_buttons_update(void);
void ghid_layer_buttons_color_update(void);
