#ifndef PCB_GTK_TOPWIN_H
#define PCB_GTK_TOPWIN_H

/* OPTIONAL top window implementation */

#include <gtk/gtk.h>

#include "hid_cfg.h"
#include "hid_dad.h"

#include "util_ext_chg.h"
#include "bu_info_bar.h"
#include "bu_menu.h"
#include "glue.h"
#include "bu_command.h"
#include "wt_layersel.h"

typedef struct {
	/* util/builder states */
	pcb_gtk_common_t *com;
	pcb_gtk_ext_chg_t ext_chg;
	pcb_gtk_info_bar_t ibar;
	pcb_gtk_menu_ctx_t menu;
	pcb_hid_cfg_t *ghid_cfg;
	pcb_gtk_command_t cmd;

	/* own widgets */
	GtkWidget *drawing_area;
	GtkWidget *bottom_hbox;

	GtkWidget *top_hbox, *top_bar_background, *menu_hbox, *position_hbox, *menubar_toolbar_vbox;
	GtkWidget *left_toolbar;
	GtkWidget *layer_selector;
	GtkWidget *vbox_middle;

	GtkWidget *h_range, *v_range;
	GObject *h_adjustment, *v_adjustment;

	/* own internal states */
	gboolean adjustment_changed_holdoff;
	gboolean small_label_markup;
	int active; /* 0 before init finishes */
	pcb_gtk_layersel_t layersel;

	/* docking */
	GtkWidget *dockbox[PCB_HID_DOCK_max];
	gdl_list_t dock[PCB_HID_DOCK_max];
} pcb_gtk_topwin_t;

void ghid_update_toggle_flags(pcb_gtk_topwin_t *tw, const char *cookie);
void ghid_install_accel_groups(GtkWindow *window, pcb_gtk_topwin_t *tw);
void ghid_remove_accel_groups(GtkWindow *window, pcb_gtk_topwin_t *tw);
void ghid_create_pcb_widgets(pcb_gtk_topwin_t *tw, GtkWidget *in_top_window);
void ghid_sync_with_new_layout(pcb_gtk_topwin_t *tw);
void ghid_fullscreen_apply(pcb_gtk_topwin_t *tw);
void pcb_gtk_tw_layer_buttons_update(pcb_gtk_topwin_t *tw);
void pcb_gtk_tw_layer_vis_update(pcb_gtk_topwin_t *tw);

void pcb_gtk_tw_notify_save_pcb(pcb_gtk_topwin_t *tw, const char *filename, pcb_bool done);
void pcb_gtk_tw_notify_filename_changed(pcb_gtk_topwin_t *tw);
void pcb_gtk_tw_interface_set_sensitive(pcb_gtk_topwin_t *tw, gboolean sensitive);
void pcb_gtk_tw_window_set_name_label(pcb_gtk_topwin_t *tw, const char *name);

int pcb_gtk_tw_dock_enter(pcb_gtk_topwin_t *tw, pcb_hid_dad_subdialog_t *sub, pcb_hid_dock_t where, const char *id);
void pcb_gtk_tw_dock_leave(pcb_gtk_topwin_t *tw, pcb_hid_dad_subdialog_t *sub);


gboolean ghid_idle_cb(void *topwin);
gboolean ghid_port_key_release_cb(GtkWidget * drawing_area, GdkEventKey * kev, pcb_gtk_topwin_t *tw);

#endif

