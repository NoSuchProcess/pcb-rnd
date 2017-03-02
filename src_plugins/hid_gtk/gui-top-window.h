#ifndef PCB_GTK_TOPWIN_H
#define PCB_GTK_TOPWIN_H

/* OPTIONAL top window implementation */

#include <gtk/gtk.h>

#include "../src_plugins/lib_gtk_common/util_ext_chg.h"
#include "../src_plugins/lib_gtk_common/bu_info_bar.h"
#include "../src_plugins/lib_gtk_common/bu_menu.h"
#include "../src_plugins/lib_gtk_common/bu_mode_btn.h"
#include "../src_plugins/lib_gtk_common/bu_cursor_pos.h"
#include "../src_plugins/lib_gtk_common/glue.h"
#include "../src_plugins/lib_gtk_common/dlg_command.h"
#include "hid_cfg.h"

typedef struct {
	/* util/builder states */
	pcb_gtk_common_t *com;
	pcb_gtk_ext_chg_t ext_chg;
	pcb_gtk_info_bar_t ibar;
	pcb_gtk_menu_ctx_t menu;
	pcb_hid_cfg_t *ghid_cfg;
	pcb_gtk_mode_btn_t mode_btn;
	pcb_gtk_cursor_pos_t cps;
	pcb_gtk_command_t cmd;

	/* own widgets */
	GtkWidget *drawing_area;
	GtkWidget *status_line_label, *status_line_hbox;

	GtkWidget *top_hbox, *top_bar_background, *menu_hbox, *position_hbox, *menubar_toolbar_vbox;
	GtkWidget *left_toolbar;
	GtkWidget *layer_selector, *route_style_selector;
	GtkWidget *vbox_middle;

	GtkWidget *h_range, *v_range;
	GtkObject *h_adjustment, *v_adjustment;

	/* own internal states */
	gchar *name_label_string;
	gboolean adjustment_changed_holdoff, in_popup;
	gboolean small_label_markup;
	int active; /* 0 before init finishes */
} pcb_gtk_topwin_t;

void ghid_update_toggle_flags(pcb_gtk_topwin_t *tw);
void ghid_install_accel_groups(GtkWindow *window, pcb_gtk_topwin_t *tw);
void ghid_remove_accel_groups(GtkWindow *window, pcb_gtk_topwin_t *tw);
void ghid_create_pcb_widgets(pcb_gtk_topwin_t *tw, GtkWidget *in_top_window);
void ghid_sync_with_new_layout(pcb_gtk_topwin_t *tw);
void ghid_fullscreen_apply(pcb_gtk_topwin_t *tw);
void ghid_layer_buttons_update(pcb_gtk_topwin_t *tw);
void ghid_handle_units_changed(pcb_gtk_topwin_t *tw);

void pcb_gtk_tw_route_styles_edited_cb(pcb_gtk_topwin_t *tw);
void pcb_gtk_tw_notify_save_pcb(pcb_gtk_topwin_t *tw, const char *filename, pcb_bool done);
void pcb_gtk_tw_notify_filename_changed(pcb_gtk_topwin_t *tw);
void pcb_gtk_tw_interface_set_sensitive(pcb_gtk_topwin_t *tw, gboolean sensitive);
void pcb_gtk_tw_window_set_name_label(pcb_gtk_topwin_t *tw, gchar *name);
void pcb_gtk_tw_layer_buttons_color_update(pcb_gtk_topwin_t *tw);


/* output events */
void ghid_port_ranges_changed(pcb_gtk_topwin_t *tw);
gboolean ghid_idle_cb(void *topwin);
void pcb_gtk_tw_ranges_scale(pcb_gtk_topwin_t *tw);
void ghid_note_event_location(GdkEventButton *ev);

gboolean ghid_port_key_release_cb(GtkWidget * drawing_area, GdkEventKey * kev, pcb_gtk_topwin_t *tw);

gint ghid_port_window_motion_cb(GtkWidget * widget, GdkEventMotion * ev, void * out);

#endif

