/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* FIXME - rename this file to ghid.h */

#ifndef PCB_HID_GTK_GHID_H
#define PCB_HID_GTK_GHID_H

#include "config.h"
#include "board.h"
#include "hid.h"
#include "hid_cfg.h"
#include "hid_cfg_input.h"

#include "data.h"

#include <gtk/gtk.h>
#include "ghid-main-menu.h"
#include "ghid-propedit.h"
#include "conf_core.h"
#include "event.h"
#include "compat_misc.h"
#include "colors.h"

#include "hid_gtk_conf.h"




/* TODO: REMOVE THIS */
#include "../src_plugins/lib_gtk_common/ui_zoompan.h"
#include "../src_plugins/lib_gtk_common/bu_box.h"
#include "../src_plugins/lib_gtk_common/bu_text_view.h"
#include "../src_plugins/lib_gtk_common/bu_check_button.h"
#include "../src_plugins/lib_gtk_common/bu_spin_button.h"
#include "../src_plugins/lib_gtk_common/wt_coord_entry.h"


	/* Silk and rats lines are the two additional selectable to draw on.
	   |  gui code in gui-top-window.c and group code in misc.c must agree
	   |  on what layer is what!
	 */
#define	LAYER_BUTTON_SILK			PCB_MAX_LAYER
#define	LAYER_BUTTON_RATS			(PCB_MAX_LAYER + 1)
#define	N_SELECTABLE_LAYER_BUTTONS	(LAYER_BUTTON_RATS + 1)

#define LAYER_BUTTON_PINS			(PCB_MAX_LAYER + 2)
#define LAYER_BUTTON_VIAS			(PCB_MAX_LAYER + 3)
#define LAYER_BUTTON_FARSIDE		(PCB_MAX_LAYER + 4)
#define LAYER_BUTTON_MASK			(PCB_MAX_LAYER + 5)
#define LAYER_BUTTON_UI			(PCB_MAX_LAYER + 6)
#define N_LAYER_BUTTONS				(PCB_MAX_LAYER + 7)

/*
 * Used to intercept "special" hotkeys that gtk doesn't usually pass
 * on to the menu hotkeys.  We catch them and put them back where we
 * want them.
 */

/* The modifier keys */

/* The actual keys */
#define GHID_KEY_TAB      0x81
#define GHID_KEY_UP       0x82
#define GHID_KEY_DOWN     0x83
#define GHID_KEY_LEFT     0x84
#define GHID_KEY_RIGHT    0x85

typedef struct {
	GtkActionGroup *main_actions, *change_selected_actions, *displayed_name_actions;

	  GtkWidget
		* status_line_label,
		*cursor_position_relative_label, *cursor_position_absolute_label, *grid_units_label, *status_line_hbox, *command_combo_box;
	GtkEntry *command_entry;

	GtkWidget *top_hbox, *top_bar_background, *menu_hbox, *position_hbox, *menubar_toolbar_vbox, *mode_buttons_frame;
	GtkWidget *left_toolbar;
	GtkWidget *grid_units_button;
	GtkWidget *menu_bar, *layer_selector, *route_style_selector;
	GtkWidget *mode_toolbar;
	GtkWidget *mode_toolbar_vbox;
	GtkWidget *vbox_middle;

	GtkWidget *info_bar;
	GTimeVal our_mtime;
	GTimeVal last_seen_mtime;

	GtkWidget *h_range, *v_range;
	GtkObject *h_adjustment, *v_adjustment;

	GdkPixbuf *bg_pixbuf;

	gchar *name_label_string;

	gboolean adjustment_changed_holdoff, command_entry_status_line_active, in_popup;

	gboolean small_label_markup, creating;

	gint settings_mode;

	ghid_propedit_dialog_t propedit_dlg;
	GtkWidget *propedit_widget;
	const char *(*propedit_query)(void *pe, const char *cmd, const char *key, const char *val, int idx);
	void *propedit_pe;
} GhidGui;

extern GhidGui _ghidgui, *ghidgui;

	/* The output viewport
	 */
typedef struct {
	GtkWidget *top_window,				/* toplevel widget              */
	 *drawing_area;								/* and its drawing area */
	GdkPixmap *pixmap, *mask;
	GdkDrawable *drawable;				/* Current drawable for drawing routines */

	struct render_priv *render_priv;

	GdkColor bg_color, offlimits_color, grid_color;

	GdkColormap *colormap;

	GdkCursor *X_cursor;					/* used X cursor */
	GdkCursorType X_cursor_shape;	/* and its shape */

	pcb_gtk_view_t view;
} GHidPort;

extern GHidPort ghid_port, *gport;

typedef enum {
	NONE_PRESSED = 0,
	SHIFT_PRESSED = PCB_M_Shift,
	CONTROL_PRESSED = PCB_M_Ctrl,
	MOD1_PRESSED = PCB_M_Mod1,
	SHIFT_CONTROL_PRESSED = PCB_M_Shift | PCB_M_Ctrl,
	SHIFT_MOD1_PRESSED = PCB_M_Shift | PCB_M_Mod1,
	CONTROL_MOD1_PRESSED = PCB_M_Ctrl | PCB_M_Mod1,
	SHIFT_CONTROL_MOD1_PRESSED = PCB_M_Shift | PCB_M_Ctrl | PCB_M_Mod1
} ModifierKeysState;

typedef enum {
	NO_BUTTON_PRESSED,
	BUTTON1_PRESSED,
	BUTTON2_PRESSED,
	BUTTON3_PRESSED
} ButtonState;

/* Function prototypes
*/
void ghid_parse_arguments(gint * argc, gchar *** argv);
void ghid_do_export(pcb_hid_attr_val_t * options);
void ghid_do_exit(pcb_hid_t *hid);

void ghid_create_pcb_widgets(void);
void ghid_window_set_name_label(gchar * name);
void ghid_interface_set_sensitive(gboolean sensitive);
void ghid_interface_input_signals_connect(void);
void ghid_interface_input_signals_disconnect(void);

void ghid_pcb_saved_toggle_states_set(void);
void ghid_sync_with_new_layout(void);

void ghid_change_selected_update_menu_actions(void);

void ghid_config_window_show();
void ghid_config_handle_units_changed(void);
void ghid_config_start_backup_timer(void);
void ghid_config_text_scale_update(void);
void ghid_config_layer_name_update(gchar * name, gint layer);

void ghid_config_init(void);
void ghid_wgeo_save(int save_to_file, int skip_user);
void ghid_conf_save_pre_wgeo(void *user_data, int argc, pcb_event_arg_t argv[]);
void ghid_conf_load_post_wgeo(void *user_data, int argc, pcb_event_arg_t argv[]);

void ghid_mode_buttons_update(void);
void ghid_pack_mode_buttons(void);
void ghid_layer_buttons_update(void);
void ghid_layer_buttons_color_update(void);


/* gui-misc.c function prototypes
*/
void ghid_status_line_set_text(const gchar * text);
void ghid_cursor_position_label_set_text(gchar * text);
void ghid_cursor_position_relative_label_set_text(gchar * text);

/* in_mouse.c static variables
*/
extern GdkPixmap *XC_hand_source, *XC_hand_mask;
extern GdkPixmap *XC_lock_source, *XC_lock_mask;
extern GdkPixmap *XC_clock_source, *XC_clock_mask;

/* gui-output-events.c function prototypes.
*/
void ghid_port_ranges_changed(void);
void ghid_port_ranges_scale(void);

void ghid_note_event_location(GdkEventButton * ev);
gboolean ghid_port_key_press_cb(GtkWidget * drawing_area, GdkEventKey * kev, gpointer data);
gboolean ghid_port_key_release_cb(GtkWidget * drawing_area, GdkEventKey * kev, gpointer data);
gboolean ghid_port_button_press_cb(GtkWidget * drawing_area, GdkEventButton * ev, gpointer data);
gboolean ghid_port_button_release_cb(GtkWidget * drawing_area, GdkEventButton * ev, gpointer data);


gint ghid_port_window_enter_cb(GtkWidget * widget, GdkEventCrossing * ev, GHidPort * out);
gint ghid_port_window_leave_cb(GtkWidget * widget, GdkEventCrossing * ev, GHidPort * out);
gint ghid_port_window_motion_cb(GtkWidget * widget, GdkEventMotion * ev, GHidPort * out);
gint ghid_port_window_mouse_scroll_cb(GtkWidget * widget, GdkEventScroll * ev, GHidPort * out);


gint ghid_port_drawing_area_configure_event_cb(GtkWidget * widget, GdkEventConfigure * ev, GHidPort * out);


/* gui-dialog.c function prototypes.
*/
#define		GUI_DIALOG_RESPONSE_ALL	1

gchar *ghid_dialog_file_select_open(const gchar * title, gchar ** path, const gchar * shortcuts);
gchar *ghid_dialog_file_select_save(const gchar * title, gchar ** path, const gchar * file, const gchar * shortcuts, const char **formats, const char **extensions, int *format);
/*gboolean ghid_dialog_confirm(const gchar * message, const gchar * cancelmsg, const gchar * okmsg);
int ghid_dialog_close_confirm(void);
#define GUI_DIALOG_CLOSE_CONFIRM_CANCEL 0
#define GUI_DIALOG_CLOSE_CONFIRM_NOSAVE 1
#define GUI_DIALOG_CLOSE_CONFIRM_SAVE   2
gint ghid_dialog_confirm_all(gchar * message);
gchar *ghid_dialog_input(const char *prompt, const char *initial);*/

char *ghid_fileselect(const char *, const char *, const char *, const char *, const char *, int);

/* gui-drc-window.c */
void ghid_drc_window_show(gboolean raise);
void ghid_drc_window_reset_message(void);
void ghid_drc_window_append_violation(pcb_drc_violation_t * violation);
void ghid_drc_window_append_messagev(const char *fmt, va_list va);
int ghid_drc_window_throw_dialog(void);

/* In gui-top-window.c  */
void ghid_update_toggle_flags(void);
void ghid_notify_save_pcb(const char *file, pcb_bool done);
void ghid_notify_filename_changed(void);
void ghid_install_accel_groups(GtkWindow * window, GhidGui * gui);
void ghid_remove_accel_groups(GtkWindow * window, GhidGui * gui);
void make_route_style_buttons(GHidRouteStyleSelector * rss);

/* gui-utils.c
*/
gboolean ghid_control_is_pressed(void);
gboolean ghid_mod1_is_pressed(void);
gboolean ghid_shift_is_pressed(void);

void ghid_draw_area_update(GHidPort * out, GdkRectangle * rect);

void ghid_range_control(GtkWidget * box, GtkWidget ** scale_res,
												gboolean horizontal, GtkPositionType pos,
												gboolean set_draw_value, gint digits,
												gboolean pack_start, gboolean expand, gboolean fill,
												guint pad, gfloat value, gfloat low, gfloat high,
												gfloat step0, gfloat step1, void (*cb_func) (), gpointer data);

void ghid_set_cursor_position_labels(void);
void ghid_set_status_line_label(void);


/* gui-netlist-window.c */
void ghid_netlist_window_create(GHidPort * out);
void ghid_netlist_window_show(GHidPort * out, gboolean raise);
void ghid_netlist_window_update(gboolean init_nodes);

pcb_lib_menu_t *ghid_get_net_from_node_name(const gchar * name, gboolean);
void ghid_netlist_highlight_node(const gchar * name);


/* gui-command-window.c */
void ghid_handle_user_command(gboolean raise);
void ghid_command_window_show(gboolean raise);
gchar *ghid_command_entry_get(const gchar * prompt, const gchar * command);
void ghid_command_use_command_window_sync(void);

/* gui-keyref-window.c */
void ghid_keyref_window_show(gboolean raise);

/* gui-library-window.c */
void ghid_library_window_create(GHidPort * out);
void ghid_library_window_show(GHidPort * out, gboolean raise);


/* gui-log-window.c */
void ghid_log_window_create();
void ghid_log_window_show(gboolean raise);
void ghid_log(const char *fmt, ...);
void ghid_logv(enum pcb_message_level level, const char *fmt, va_list args);

/* gui-pinout-window.c */
void ghid_pinout_window_show(GHidPort * out, pcb_element_t *Element);

/* gtkhid-gdk.c AND gtkhid-gl.c */
int ghid_set_layer_group(pcb_layergrp_id_t group, pcb_layer_id_t layer, unsigned int flags, int is_empty);
pcb_hid_gc_t ghid_make_gc(void);
void ghid_destroy_gc(pcb_hid_gc_t);
void ghid_use_mask(int use_it);
void ghid_set_color(pcb_hid_gc_t gc, const char *name);
void ghid_set_line_cap(pcb_hid_gc_t gc, pcb_cap_style_t style);
void ghid_set_line_width(pcb_hid_gc_t gc, pcb_coord_t width);
void ghid_set_draw_xor(pcb_hid_gc_t gc, int _xor);
void ghid_draw_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2);
void ghid_draw_arc(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t xradius, pcb_coord_t yradius, pcb_angle_t start_angle, pcb_angle_t delta_angle);
void ghid_draw_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2);
void ghid_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t radius);
void ghid_fill_polygon(pcb_hid_gc_t gc, int n_coords, pcb_coord_t * x, pcb_coord_t * y);
void ghid_fill_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2);
void ghid_invalidate_lr(pcb_coord_t left, pcb_coord_t right, pcb_coord_t top, pcb_coord_t bottom);
void ghid_invalidate_all();
void ghid_notify_crosshair_change(pcb_bool changes_complete);
void ghid_notify_mark_change(pcb_bool changes_complete);
void ghid_init_renderer(int *, char ***, GHidPort *);
void ghid_shutdown_renderer(GHidPort *);
void ghid_init_drawing_widget(GtkWidget * widget, void *gport);
void ghid_drawing_area_configure_hook(GHidPort * port);
void ghid_screen_update(void);
gboolean ghid_drawing_area_expose_cb(GtkWidget *, GdkEventExpose *, GHidPort *);
void ghid_port_drawing_realize_cb(GtkWidget *, gpointer);
gboolean ghid_preview_expose(GtkWidget * widget, GdkEventExpose * ev, pcb_hid_expose_t expcall, const pcb_hid_expose_ctx_t *ctx);
gboolean ghid_preview_draw(GtkWidget * widget, pcb_hid_expose_t expcall, const pcb_hid_expose_ctx_t *ctx);
GdkPixmap *ghid_render_pixmap(int cx, int cy, double zoom, int width, int height, int depth);
pcb_hid_t *ghid_request_debug_draw(void);
void ghid_flush_debug_draw(void);
void ghid_finish_debug_draw(void);

void ghid_lead_user_to_location(pcb_coord_t x, pcb_coord_t y);
void ghid_cancel_lead_user(void);


/* Coordinate conversions */
/* Px converts view->pcb, Vx converts pcb->view */
static inline int Vx(pcb_coord_t x)
{
	double rv;
	if (conf_core.editor.view.flip_x)
		rv = (PCB->MaxWidth - x - gport->view.x0) / gport->view.coord_per_px + 0.5;
	else
		rv = (x - gport->view.x0) / gport->view.coord_per_px + 0.5;
	return pcb_round(rv);
}

static inline int Vy(pcb_coord_t y)
{
	double rv;
	if (conf_core.editor.view.flip_y)
		rv = (PCB->MaxHeight - y - gport->view.y0) / gport->view.coord_per_px + 0.5;
	else
		rv = (y - gport->view.y0) / gport->view.coord_per_px + 0.5;
	return pcb_round(rv);
}

static inline int Vz(pcb_coord_t z)
{
	return pcb_round((double)z / gport->view.coord_per_px + 0.5);
}

static inline pcb_coord_t Px(int x)
{
	pcb_coord_t rv = x * gport->view.coord_per_px + gport->view.x0;
	if (conf_core.editor.view.flip_x)
		rv = PCB->MaxWidth - (x * gport->view.coord_per_px + gport->view.x0);
	return rv;
}

static inline pcb_coord_t Py(int y)
{
	pcb_coord_t rv = y * gport->view.coord_per_px + gport->view.y0;
	if (conf_core.editor.view.flip_y)
		rv = PCB->MaxHeight - (y * gport->view.coord_per_px + gport->view.y0);
	return rv;
}

static inline pcb_coord_t Pz(int z)
{
	return (z * gport->view.coord_per_px);
}

extern const char *ghid_cookie;
extern const char *ghid_menu_cookie;
extern pcb_hid_cfg_keys_t ghid_keymap;

int ghid_usage(const char *topic);
void hid_gtk_wgeo_update(void);
void config_color_button_update(conf_native_t *cfg, int idx);

void ghid_confchg_line_refraction(conf_native_t *cfg);
void ghid_confchg_all_direction_lines(conf_native_t *cfg);
void ghid_confchg_fullscreen(conf_native_t *cfg);
void ghid_confchg_checkbox(conf_native_t *cfg);
void ghid_confchg_flip(conf_native_t *cfg);

void ghid_draw_grid_local(pcb_coord_t cx, pcb_coord_t cy);

void ghid_fullscreen_apply(void);

GMainLoop *ghid_entry_loop;

void ghid_LayersChanged(void *user_data, int argc, pcb_event_arg_t argv[]);

#endif /* PCB_HID_GTK_GHID_GUI_H */
