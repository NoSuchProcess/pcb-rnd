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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

/* This file written by Bill Wilson for the PCB Gtk port.
*/

#include "config.h"
#include "conf_core.h"

#include <stdlib.h>
#include <genht/hash.h>

#include "gui.h"
#include "win_place.h"
#include "hid.h"
#include "gtkhid.h"

#include "global.h"
#include "action_helper.h"
#include "change.h"
#include "plug_io.h"
#include "error.h"
#include "draw.h"
#include "misc.h"
#include "pcb-printf.h"
#include "set.h"
#include "hid_attrib.h"
#include "conf.h"
#include "misc_util.h"
#include "hid_gtk_conf.h"
#include "gtk_conf_list.h"
#include "paths.h"
#include "plug_footprint.h"
#include "compat_misc.h"
#include <liblihata/tree.h>

#if 0
#include <locale.h>
#endif

extern int MoveLayerAction(int argc, char **argv, int x, int y);
conf_hid_gtk_t conf_hid_gtk;
window_geometry_t hid_gtk_wgeo, hid_gtk_wgeo_old;

#define hid_gtk_wgeo_save_(field, dest_role) \
	conf_setf(dest_role, "plugins/hid_gtk/window_geometry/" #field, -1, "%d", hid_gtk_wgeo.field); \

#define hid_gtk_wgeo_update_(field, dest_role) \
	if (hid_gtk_wgeo.field != hid_gtk_wgeo_old.field) { \
		hid_gtk_wgeo_old.field = hid_gtk_wgeo.field; \
		hid_gtk_wgeo_save_(field, dest_role); \
	}

void hid_gtk_wgeo_update(void)
{
	if (conf_hid_gtk.plugins.hid_gtk.auto_save_window_geometry.to_design)
		GHID_WGEO_ALL(hid_gtk_wgeo_update_, CFR_DESIGN);
	if (conf_hid_gtk.plugins.hid_gtk.auto_save_window_geometry.to_project)
		GHID_WGEO_ALL(hid_gtk_wgeo_update_, CFR_PROJECT);
	if (conf_hid_gtk.plugins.hid_gtk.auto_save_window_geometry.to_user)
		GHID_WGEO_ALL(hid_gtk_wgeo_update_, CFR_USER);
}


void ghid_wgeo_save(int save_to_file, int skip_user)
{
	if (conf_hid_gtk.plugins.hid_gtk.auto_save_window_geometry.to_design) {
		GHID_WGEO_ALL(hid_gtk_wgeo_save_, CFR_DESIGN);
		if (save_to_file)
			conf_save_file(NULL, (PCB == NULL ? NULL : PCB->Filename), CFR_DESIGN, NULL);
	}

	if (conf_hid_gtk.plugins.hid_gtk.auto_save_window_geometry.to_project) {
		GHID_WGEO_ALL(hid_gtk_wgeo_save_, CFR_PROJECT);
		if (save_to_file)
			conf_save_file(NULL, (PCB == NULL ? NULL : PCB->Filename), CFR_PROJECT, NULL);
	}

	if (!skip_user) {
		if (conf_hid_gtk.plugins.hid_gtk.auto_save_window_geometry.to_user) {
			GHID_WGEO_ALL(hid_gtk_wgeo_save_, CFR_USER);
			if (save_to_file)
				conf_save_file(NULL, (PCB == NULL ? NULL : PCB->Filename), CFR_USER, NULL);
		}
	}
}

static void wgeo_save_direct(GtkButton *widget, const char *ctx)
{
	if (ctx == NULL) {
		GtkWidget *fcd = gtk_file_chooser_dialog_new("Save window geometry to...", NULL, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
		if (gtk_dialog_run(GTK_DIALOG(fcd)) == GTK_RESPONSE_ACCEPT) {
			char *fn = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fcd));
			conf_reset(CFR_file, "<wgeo_save_direct>");
			GHID_WGEO_ALL(hid_gtk_wgeo_save_, CFR_file);
			conf_export_to_file(fn, CFR_file, "/");
			gtk_widget_destroy(fcd);
			g_free(fn);
			conf_reset(CFR_file, "<internal>");
		}
	}
	else if (*ctx == '*') {
		switch(ctx[1]) {
			case 'd':
				GHID_WGEO_ALL(hid_gtk_wgeo_save_, CFR_DESIGN);
				conf_save_file(NULL, (PCB == NULL ? NULL : PCB->Filename), CFR_DESIGN, NULL);
				break;
			case 'p':
				GHID_WGEO_ALL(hid_gtk_wgeo_save_, CFR_PROJECT);
				conf_save_file(NULL, (PCB == NULL ? NULL : PCB->Filename), CFR_PROJECT, NULL);
				break;
			case 'u':
				GHID_WGEO_ALL(hid_gtk_wgeo_save_, CFR_USER);
				conf_save_file(NULL, (PCB == NULL ? NULL : PCB->Filename), CFR_USER, NULL);
				break;
		}
	}
}

#undef hid_gtk_wgeo_update_
#undef hid_gtk_wgeo_save_

/* event handler that runs before the current pcb is saved - save win geo
   in the corresponding lihata trees if the checkbox is selected. */
void ghid_conf_save_pre_wgeo(void *user_data, int argc, event_arg_t * argv[])
{
	ghid_wgeo_save(1, 1);
}


static int just_loaded(const char *path)
{
	conf_native_t *n = conf_get_field(path);
	conf_role_t r;
	if ((n == NULL) || (n->used == 0))
		return 0;
	r = conf_lookup_role(n->prop[0].src);
	return (r == CFR_DESIGN) || (r == CFR_PROJECT);
}

#define path_prefix "plugins/hid_gtk/window_geometry/"
#define did_just_load_geo(win, id) \
	if (just_loaded(path_prefix #win "_width") || just_loaded(path_prefix #win "_height") \
	 || just_loaded(path_prefix #win "_x") || just_loaded(path_prefix #win "_y")) { \
		hid_gtk_wgeo.win ## _width  = conf_hid_gtk.plugins.hid_gtk.window_geometry.win ## _width; \
		hid_gtk_wgeo.win ## _height = conf_hid_gtk.plugins.hid_gtk.window_geometry.win ## _height; \
		hid_gtk_wgeo.win ## _x      = conf_hid_gtk.plugins.hid_gtk.window_geometry.win ## _x; \
		hid_gtk_wgeo.win ## _y      = conf_hid_gtk.plugins.hid_gtk.window_geometry.win ## _y; \
		wplc_place(id, NULL); \
	}

void ghid_conf_load_post_wgeo(void *user_data, int argc, event_arg_t * argv[])
{
	did_just_load_geo(top, WPLC_TOP);
	did_just_load_geo(log, WPLC_LOG);
	did_just_load_geo(drc, WPLC_DRC);
	did_just_load_geo(library, WPLC_LIBRARY);
	did_just_load_geo(netlist, WPLC_NETLIST);
	did_just_load_geo(keyref, WPLC_KEYREF);
	did_just_load_geo(pinout, WPLC_PINOUT);
}

#undef path_prefix


#undef just_loaded_geo

enum ConfigType {
	CONFIG_Boolean,
	CONFIG_Integer,
	CONFIG_Coord,
	CONFIG_Real,
	CONFIG_String,
	CONFIG_Unused
};

typedef struct {
	gchar *name;
	enum ConfigType type;
	void *value;
} ConfigAttribute;

extern void ghid_set_special_colors(conf_native_t *cfg);

void ghid_config_init(void)
{
	hid_gtk_wgeo_old = hid_gtk_wgeo = conf_hid_gtk.plugins.hid_gtk.window_geometry;
#warning CONF TODO: inject the internal part here?
}

typedef struct {
	conf_role_t dst_role;
	conf_role_t src_role;
} save_ctx_t;

static void ghid_config_window_close(void);

/* Replace a list of paths in dst with src; each path must be either:
   - a path to a config filed that presents in the hash (not a ha:subtree)
   - a subtree or multiple subtrees matching a prefix (e.g. "*appearance/color")
*/
void config_any_replace(save_ctx_t *ctx, const char **paths)
{
	const char **p;
	int need_update = 0;

	for(p = paths; *p != NULL; p++) {
		/* wildcards - match subtree */
		if (**p == '*') {
			const char *wildp = (*p)+1;
			int pl = strlen(wildp);
			htsp_entry_t *e;
			conf_fields_foreach(e) {
				if (strncmp(e->key, wildp, pl) == 0) {
					if (conf_replace_subtree(ctx->dst_role, e->key, ctx->src_role, e->key) != 0)
						Message(PCB_MSG_DEFAULT, "Error: failed to save config item %s\n", *p);
					if (ctx->dst_role < CFR_max_real) {
						conf_update(e->key);
						need_update++;
					}
				}
			}
		}
		else {
			/* plain node */
			if (conf_replace_subtree(ctx->dst_role, *p, ctx->src_role, *p) != 0)
					Message(PCB_MSG_DEFAULT, "Error: failed to save config item %s\n", *p);
			if (ctx->dst_role < CFR_max_real) {
				conf_update(*p);
				need_update++;
			}
		}
	}

	/* present a file choose dialog box on save to custom file */
	if (ctx->dst_role == CFR_file) {
		GtkWidget *fcd = gtk_file_chooser_dialog_new("Save config settings to...", NULL, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
		if (gtk_dialog_run(GTK_DIALOG(fcd)) == GTK_RESPONSE_ACCEPT) {
			char *fn = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fcd));
			conf_export_to_file(fn, CFR_file, "/");
			conf_reset(CFR_file, "<internal>");
			g_free(fn);
		}
		gtk_widget_destroy(fcd);
	}

	if (need_update) {
		/* do all the gui updates */
		ghid_set_status_line_label();
		ghid_pack_mode_buttons();

		/* need to reopen the preferences dialog to show the new settings */
		ghid_config_window_close();
		ghid_config_window_show();
	}

	if ((ctx->dst_role == CFR_USER) || (ctx->dst_role == CFR_PROJECT))
		conf_save_file(NULL, (PCB == NULL ? NULL : PCB->Filename), ctx->dst_role, NULL);
}

/* =================== OK, now the gui stuff ======================
*/
static GtkWidget *config_window;

static void config_user_role_section(GtkWidget * vbox, void (*save_cb)(GtkButton *widget, save_ctx_t *sctx))
{
	GtkWidget *config_color_warn_label, *button, *hbox, *vbox2;
	static save_ctx_t ctx_all2project = { CFR_PROJECT, CFR_binary };
	static save_ctx_t ctx_all2user    = { CFR_USER, CFR_binary };
	static save_ctx_t ctx_all2file    = { CFR_file, CFR_binary };
	static save_ctx_t ctx_int2design  = { CFR_DESIGN, CFR_INTERNAL };

	hbox = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);

	config_color_warn_label = gtk_label_new("");
	gtk_label_set_use_markup(GTK_LABEL(config_color_warn_label), TRUE);
	gtk_label_set_markup(GTK_LABEL(config_color_warn_label),
											 _("<small>The above are <i>design-level</i>\nconfiguration, <u>saved</u> with the\npcb file. Use these buttons\nto save all the above settings:</small>"));
	gtk_box_pack_start(GTK_BOX(hbox), config_color_warn_label, FALSE, FALSE, 4);

	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox2, FALSE, FALSE, 0);

	button = gtk_button_new_with_label("Save in project config");
	gtk_box_pack_start(GTK_BOX(vbox2), button, FALSE, FALSE, 0);
	g_signal_connect(GTK_OBJECT(button), "clicked", G_CALLBACK(save_cb), &ctx_all2project);

	button = gtk_button_new_with_label("Save in user config");
	gtk_box_pack_start(GTK_BOX(vbox2), button, FALSE, FALSE, 0);
	g_signal_connect(GTK_OBJECT(button), "clicked", G_CALLBACK(save_cb), &ctx_all2user);

	vbox2 = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox2, FALSE, FALSE, 0);

	button = gtk_button_new_with_label("Save to file");
	gtk_box_pack_start(GTK_BOX(vbox2), button, FALSE, FALSE, 0);
	g_signal_connect(GTK_OBJECT(button), "clicked", G_CALLBACK(save_cb), &ctx_all2file);

	button = gtk_button_new_with_label("Restore factory defaults");
	gtk_box_pack_start(GTK_BOX(vbox2), button, FALSE, FALSE, 0);
	g_signal_connect(GTK_OBJECT(button), "clicked", G_CALLBACK(save_cb), &ctx_int2design);
}

	/* -------------- The General config page ----------------
	 */

static void config_command_window_toggle_cb(GtkToggleButton * button, gpointer data)
{
/*	gboolean active =*/ gtk_toggle_button_get_active(button);
	static gboolean holdoff;

	if (holdoff)
		return;

	/* Can't toggle into command window mode if the status line command
	   |  entry is active.
	 */
	if (ghidgui->command_entry_status_line_active) {
		holdoff = TRUE;
		gtk_toggle_button_set_active(button, FALSE);
		holdoff = FALSE;
		return;
	}
	conf_set(CFR_DESIGN, "plugins/hid_gtk/use_command_window", -1, "1", POL_OVERWRITE);
	ghid_command_use_command_window_sync();
}

static void config_compact_horizontal_toggle_cb(GtkToggleButton * button, gpointer data)
{
	gboolean active = gtk_toggle_button_get_active(button);

	conf_setf(CFR_DESIGN, "plugins/hid_gtk/compact_horizontal", -1, "%d", active);
	ghid_set_status_line_label();
}

static void config_compact_vertical_toggle_cb(GtkToggleButton * button, gpointer data)
{
	gboolean active = gtk_toggle_button_get_active(button);

	conf_setf(CFR_DESIGN, "plugins/hid_gtk/compact_vertical", -1, "%d", active);
	ghid_pack_mode_buttons();
}

static GtkWidget *pref_auto_place_lab;
static void pref_auto_place_update(void)
{
	const char *warn;
	if (conf_core.editor.auto_place)
		warn = "Restoring window geometry is <b>enabled</b>:\npcb-rnd will attempt to move and resize windows.\nIt can be disabled in General/";
	else
		warn = "Restoring window geometry is <b>disabled</b>:\npcb-rnd will <b>not</b> attempt to move and resize windows.\nConsider changing it in General/";
	gtk_label_set_markup(GTK_LABEL(pref_auto_place_lab), _(warn));
}

static void config_auto_place_toggle_cb(GtkToggleButton * button, gpointer data)
{
	gboolean active = gtk_toggle_button_get_active(button);

	conf_setf(CFR_DESIGN, "editor/auto_place", -1, "%d", active);
	pref_auto_place_update();
}

static void config_general_toggle_cb(GtkToggleButton * button, void *setting)
{
	*(gint *) setting = gtk_toggle_button_get_active(button);
}

static void config_backup_spin_button_cb(GtkSpinButton * spin_button, gpointer data)
{
	int i;
	char s[32];
	i = gtk_spin_button_get_value_as_int(spin_button);
	sprintf(s, "%d", i);
	conf_set(CFR_DESIGN, "rc/backup_interval", -1, s, POL_OVERWRITE);
	EnableAutosave();
}

static void config_history_spin_button_cb(GtkSpinButton * spin_button, gpointer data)
{
	conf_setf(CFR_DESIGN, "plugins/hid_gtk/history_size", -1, "%d", gtk_spin_button_get_value_as_int(spin_button));
}

void config_general_save(GtkButton *widget, save_ctx_t *ctx)
{
	const char *paths[] = {
		"plugins/hid_gtk/use_command_window",
		"plugins/hid_gtk/compact_horizontal",
		"plugins/hid_gtk/compact_vertical",
		"plugins/hid_gtk/history_size",
		"rc/backup_interval",
		"editor/auto_place",
		"editor/save_in_tmp",
		NULL
	};

	config_any_replace(ctx, paths);
}

static void config_general_tab_create(GtkWidget * tab_vbox)
{
	GtkWidget *vbox, *content_vbox;

	content_vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(tab_vbox), content_vbox, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(content_vbox), 6);

	vbox = ghid_category_vbox(content_vbox, _("Enables"), 4, 2, TRUE, TRUE);

	ghid_check_button_connected(vbox, NULL, conf_hid_gtk.plugins.hid_gtk.use_command_window,
															TRUE, FALSE, FALSE, 2,
															config_command_window_toggle_cb, NULL, _("Use separate window for command entry"));

	ghid_check_button_connected(vbox, NULL, conf_hid_gtk.plugins.hid_gtk.compact_horizontal,
															TRUE, FALSE, FALSE, 2,
															config_compact_horizontal_toggle_cb, NULL,
															_("Alternate window layout to allow smaller horizontal size"));

	ghid_check_button_connected(vbox, NULL, conf_hid_gtk.plugins.hid_gtk.compact_vertical,
															TRUE, FALSE, FALSE, 2,
															config_compact_vertical_toggle_cb, NULL,
															_("Alternate window layout to allow smaller vertical size"));

	ghid_check_button_connected(vbox, NULL, conf_core.editor.auto_place,
															TRUE, FALSE, FALSE, 2,
															config_auto_place_toggle_cb, NULL,
															_("Restore window geometry (when saved geometry is available)"));

	vbox = ghid_category_vbox(content_vbox, _("Backups"), 4, 2, TRUE, TRUE);
	ghid_check_button_connected(vbox, NULL, conf_core.editor.save_in_tmp,
															TRUE, FALSE, FALSE, 2,
															config_general_toggle_cb, (void *)&conf_core.editor.save_in_tmp,
															_("If layout is modified at exit, save into PCB.%i.save"));
	ghid_spin_button(vbox, NULL, conf_core.rc.backup_interval, 0.0, 60 * 60, 60.0,
									 600.0, 0, 0, config_backup_spin_button_cb, NULL, FALSE,
									 _("Seconds between auto backups\n" "(set to zero to disable auto backups)"));

	vbox = ghid_category_vbox(content_vbox, _("Misc"), 4, 2, TRUE, TRUE);
	ghid_spin_button(vbox, NULL, conf_hid_gtk.plugins.hid_gtk.history_size,
									 5.0, 25.0, 1.0, 1.0, 0, 0,
									 config_history_spin_button_cb, NULL, FALSE, _("Number of commands to remember in the history list"));


	vbox = gtk_vbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(tab_vbox), vbox, TRUE, TRUE, 0);
	config_user_role_section(tab_vbox, config_general_save);
}

	/* -------------- The Sizes config page ----------------
	 */
static GtkWidget *config_sizes_vbox, *config_sizes_tab_vbox, *config_text_spin_button;

static Coord new_board_width, new_board_height;

static void config_sizes_apply(void)
{
	conf_setf(CFR_DESIGN, "design/max_width", -1, "%$mS", new_board_width);
	conf_setf(CFR_DESIGN, "design/max_height", -1, "%$mS", new_board_height);

	conf_set_design("design/bloat", "%$mS", PCB->Bloat);
	conf_set_design("design/shrink", "%$mS", PCB->Shrink);
	conf_set_design("design/min_wid", "%$mS", PCB->minWid);
	conf_set_design("design/min_slk", "%$mS", PCB->minSlk);
	conf_set_design("design/poly_isle_area", "%f", PCB->IsleArea);
	conf_set_design("design/min_drill", "%$mS", PCB->minDrill);
	conf_set_design("design/min_ring", "%$mS", PCB->minRing);

	if (PCB->MaxWidth != conf_core.design.max_width || PCB->MaxHeight != conf_core.design.max_height)
		ChangePCBSize(conf_core.design.max_width, conf_core.design.max_height);
}

static void text_spin_button_cb(GtkSpinButton * spin, void *dst)
{
	conf_setf(CFR_DESIGN, "design/text_scale", -1, "%d", gtk_spin_button_get_value_as_int(spin));
	ghid_set_status_line_label();
}

static void coord_entry_cb(GHidCoordEntry * ce, void *dst)
{
	*(Coord *) dst = ghid_coord_entry_get_value(ce);
}

void config_sizes_save(GtkButton *widget, save_ctx_t *ctx)
{
	const char *paths[] = {
		"design/max_width",
		"design/max_height",
		"design/bloat",
		"design/shrink",
		"design/min_wid",
		"design/min_slk",
		"design/poly_isle_area",
		"design/min_drill",
		"design/min_ring",
		"design/text_scale",
		NULL
	};

	config_sizes_apply();
	config_any_replace(ctx, paths);
}


static void config_sizes_tab_create(GtkWidget * tab_vbox)
{
	GtkWidget *table, *vbox, *hbox, *content_vbox;

	content_vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(tab_vbox), content_vbox, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(content_vbox), 6);

	/* Need a vbox we can destroy if user changes grid units.
	 */
	if (!config_sizes_vbox) {
		vbox = gtk_vbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(content_vbox), vbox, FALSE, FALSE, 0);
		gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
		config_sizes_vbox = vbox;
		config_sizes_tab_vbox = content_vbox;
	}

	/* ---- Board Size ---- */
	vbox = ghid_category_vbox(config_sizes_vbox, _("Board Size"), 4, 2, TRUE, TRUE);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	table = gtk_table_new(2, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 0);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);

	new_board_width = PCB->MaxWidth;
	new_board_height = PCB->MaxHeight;
	ghid_table_coord_entry(table, 0, 0, NULL,
												 PCB->MaxWidth, MIN_SIZE, MAX_COORD, CE_LARGE, 0, coord_entry_cb, &new_board_width, FALSE, _("Width"));

	ghid_table_coord_entry(table, 1, 0, NULL,
												 PCB->MaxHeight, MIN_SIZE, MAX_COORD,
												 CE_LARGE, 0, coord_entry_cb, &new_board_height, FALSE, _("Height"));

	/* ---- Text Scale ---- */
	vbox = ghid_category_vbox(config_sizes_vbox, _("Text Scale"), 4, 2, TRUE, TRUE);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	table = gtk_table_new(4, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 0);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	ghid_table_spin_button(table, 0, 0, &config_text_spin_button,
												 conf_core.design.text_scale,
												 MIN_TEXTSCALE, MAX_TEXTSCALE, 10.0, 10.0, 0, 0, text_spin_button_cb, NULL, FALSE, "%");


	/* ---- DRC Sizes ---- */
	vbox = ghid_category_vbox(config_sizes_vbox, _("Design Rule Checking"), 4, 2, TRUE, TRUE);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	table = gtk_table_new(4, 2, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 0);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);

	ghid_table_coord_entry(table, 0, 0, NULL,
												 PCB->Bloat, MIN_DRC_VALUE, MAX_DRC_VALUE,
												 CE_SMALL, 0, coord_entry_cb, &PCB->Bloat, FALSE, _("Minimum copper spacing"));

	ghid_table_coord_entry(table, 1, 0, NULL,
												 PCB->minWid, MIN_DRC_VALUE, MAX_DRC_VALUE,
												 CE_SMALL, 0, coord_entry_cb, &PCB->minWid, FALSE, _("Minimum copper width"));

	ghid_table_coord_entry(table, 2, 0, NULL,
												 PCB->Shrink, MIN_DRC_VALUE, MAX_DRC_VALUE,
												 CE_SMALL, 0, coord_entry_cb, &PCB->Shrink, FALSE, _("Minimum touching copper overlap"));

	ghid_table_coord_entry(table, 3, 0, NULL,
												 PCB->minSlk, MIN_DRC_VALUE, MAX_DRC_VALUE,
												 CE_SMALL, 0, coord_entry_cb, &PCB->minSlk, FALSE, _("Minimum silk width"));

	ghid_table_coord_entry(table, 4, 0, NULL,
												 PCB->minDrill, MIN_DRC_VALUE, MAX_DRC_VALUE,
												 CE_SMALL, 0, coord_entry_cb, &PCB->minDrill, FALSE, _("Minimum drill diameter"));

	ghid_table_coord_entry(table, 5, 0, NULL,
												 PCB->minRing, MIN_DRC_VALUE, MAX_DRC_VALUE,
												 CE_SMALL, 0, coord_entry_cb, &PCB->minRing, FALSE, _("Minimum annular ring"));

	vbox = gtk_vbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(tab_vbox), vbox, TRUE, TRUE, 0);
	config_user_role_section(tab_vbox, config_sizes_save);

	gtk_widget_show_all(config_sizes_vbox);
}


	/* -------------- The window config page ----------------
	 */
static GtkWidget *config_window_vbox;

static void config_window_toggle_cb(GtkToggleButton * button, gpointer data)
{
	gboolean active = gtk_toggle_button_get_active(button);
	const char *path = NULL, *ctx = data;

	if (ctx == NULL)
		return;

	if (*ctx == '*') {
		switch(ctx[1]) {
			case 'd': path = "plugins/hid_gtk/auto_save_window_geometry/to_design"; break;
			case 'p': path = "plugins/hid_gtk/auto_save_window_geometry/to_project"; break;
			case 'u': path = "plugins/hid_gtk/auto_save_window_geometry/to_user"; break;
		}
		if (path != NULL)
			conf_set(CFR_USER, path, -1, (active ? "1" : "0"), POL_OVERWRITE);
	}
}


static void config_window_row(GtkWidget *parent, const char *desc, int load, const char *wgeo_save_str, CFT_BOOLEAN chk)
{
	GtkWidget *hbox, *lab, *button;
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);

	lab = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), lab, TRUE, FALSE, 0);
	gtk_misc_set_alignment(GTK_MISC(lab), 0.0, 1.0);

	lab = gtk_label_new(desc);
	gtk_box_pack_start(GTK_BOX(hbox), lab, FALSE, FALSE, 4);

	button = gtk_button_new_with_label(_("now"));
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	g_signal_connect(GTK_OBJECT(button), "clicked", G_CALLBACK(wgeo_save_direct), (void *)wgeo_save_str);

	if (load) {
		button = gtk_button_new_with_label(_("Load from file"));
		gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	}
	else {
		GtkWidget *btn;
		ghid_check_button_connected(hbox, &btn, chk,
															TRUE, FALSE, FALSE, 2,
															config_window_toggle_cb, (void *)wgeo_save_str, _("every time pcb-rnd exits"));
		if (wgeo_save_str == NULL)
			gtk_widget_set_sensitive(btn, FALSE);
	}
}

static void config_window_tab_create(GtkWidget *tab_vbox)
{
	GtkWidget *lab;

	config_window_vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(tab_vbox), config_window_vbox, FALSE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(config_window_vbox), 6);

	lab = gtk_label_new("");
	gtk_label_set_use_markup(GTK_LABEL(lab), TRUE);
	gtk_label_set_markup(GTK_LABEL(lab),
											 _("<b>Save window geometry...</b>"));
	gtk_box_pack_start(GTK_BOX(config_window_vbox), lab, FALSE, FALSE, 4);
	gtk_misc_set_alignment(GTK_MISC(lab), 0.0, 0.5);

	config_window_row(config_window_vbox, "... in the design (.pcb) file", 0, "*d", conf_hid_gtk.plugins.hid_gtk.auto_save_window_geometry.to_design);
	config_window_row(config_window_vbox, "... in the project file", 0, "*p", conf_hid_gtk.plugins.hid_gtk.auto_save_window_geometry.to_project);
	config_window_row(config_window_vbox, "... in the central user configuration", 0, "*u", conf_hid_gtk.plugins.hid_gtk.auto_save_window_geometry.to_user);
	config_window_row(config_window_vbox, "... in a custom file", 0, NULL, 0);




	lab = gtk_label_new("");
	gtk_label_set_use_markup(GTK_LABEL(lab), TRUE);
	gtk_label_set_markup(GTK_LABEL(lab),
											 _("<small>Note: the above checkbox values are saved in the user config</small>"));
	gtk_box_pack_start(GTK_BOX(config_window_vbox), lab, FALSE, FALSE, 4);
	gtk_misc_set_alignment(GTK_MISC(lab), 0.0, 0.5);

	lab = gtk_label_new("");
	gtk_label_set_use_markup(GTK_LABEL(lab), TRUE);
	pref_auto_place_lab = lab;
	pref_auto_place_update();
	gtk_box_pack_start(GTK_BOX(config_window_vbox), lab, FALSE, FALSE, 4);
	gtk_misc_set_alignment(GTK_MISC(lab), 0.0, 0.5);

	gtk_widget_show_all(config_window_vbox);
}


	/* -------------- The Increments config page ----------------
	 */
	/* Increment/decrement values are kept in mil and mm units and not in
	   |  PCB units.
	 */
GtkWidget *config_increments_tbl[4][4]; /* [col][row] */

static GtkWidget *config_increments_vbox, *config_increments_tab_vbox;

static void increment_tbl_update_cell(GtkLabel *lab, Coord val, const char *fmt)
{
	char s[128];
	pcb_snprintf(s, sizeof(s), fmt, val);
	gtk_label_set_text(lab, s);
}

static void increment_tbl_update_row(int row, Coord edit_in_mm, Coord edit_in_mil)
{
	increment_tbl_update_cell(GTK_LABEL(config_increments_tbl[0][row]), edit_in_mm, "%$mm");
	increment_tbl_update_cell(GTK_LABEL(config_increments_tbl[1][row]), edit_in_mil, "%$mm");
	increment_tbl_update_cell(GTK_LABEL(config_increments_tbl[2][row]), edit_in_mm, "%$ml");
	increment_tbl_update_cell(GTK_LABEL(config_increments_tbl[3][row]), edit_in_mil, "%$ml");
}

static void increment_tbl_update()
{
	increment_tbl_update_row(0, conf_core.editor.increments_mm.grid, conf_core.editor.increments_mil.grid);
	increment_tbl_update_row(1, conf_core.editor.increments_mm.size, conf_core.editor.increments_mil.size);
	increment_tbl_update_row(2, conf_core.editor.increments_mm.line, conf_core.editor.increments_mil.line);
	increment_tbl_update_row(3, conf_core.editor.increments_mm.clear, conf_core.editor.increments_mil.clear);
}

static void increment_spin_button_cb(GHidCoordEntry * ce, void *dst)
{
	const char *path = dst;
	conf_setf(CFR_DESIGN, path, -1, "%mr", (Coord)ghid_coord_entry_get_value(ce));
	increment_tbl_update();
}

static void config_increments_sect_create(GtkWidget * vbox)
{
	GtkWidget * hbox, *label;
	const int width = 128;
	char pathmm[256], *pemm;
	char pathmil[256], *pemil;
	const Unit *umm = get_unit_struct("mm");
	const Unit *umil = get_unit_struct("mil");
	const char *base_pathmm = "editor/increments_mm";
	const char *base_pathmil = "editor/increments_mil";
	int lmm = strlen(base_pathmm);
	int lmil = strlen(base_pathmil);

	memcpy(pathmm, base_pathmm, lmm);
	pemm = pathmm+lmm;
	*pemm = '/';
	pemm++;

	memcpy(pathmil, base_pathmil, lmil);
	pemil = pathmil+lmil;
	*pemil = '/';
	pemil++;

#warning leak: pcb_strdup(path) never free()d
	/* ---- Grid Increment/Decrement ---- */
	strcpy(pemm, "grid");
	strcpy(pemil, "grid");
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);
	ghid_coord_entry(hbox, NULL,
									 conf_core.editor.increments_mm.grid,
									 conf_core.editor.increments_mm.grid_min,
									 conf_core.editor.increments_mm.grid_max,
									 CE_SMALL, umm, width, increment_spin_button_cb, pcb_strdup(pathmm), _("Grid:"), NULL);

	ghid_coord_entry(hbox, NULL,
									 conf_core.editor.increments_mil.grid,
									 conf_core.editor.increments_mil.grid_min,
									 conf_core.editor.increments_mil.grid_max,
									 CE_SMALL, umil, width, increment_spin_button_cb, pcb_strdup(pathmil), NULL, NULL);

	label = gtk_label_new("");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), _("<small>For 'g' and '&lt;shift&gt;g' grid change actions</small>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 2);

	/* ---- Size Increment/Decrement ---- */
	strcpy(pemm, "size");
	strcpy(pemil, "size");
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

	ghid_coord_entry(hbox, NULL,
									 conf_core.editor.increments_mm.size,
									 conf_core.editor.increments_mm.size_min,
									 conf_core.editor.increments_mm.size_max,
									 CE_SMALL, umm, width, increment_spin_button_cb,
									 pcb_strdup(pathmm), _("Size:"), NULL);

	ghid_coord_entry(hbox, NULL,
									 conf_core.editor.increments_mil.size,
									 conf_core.editor.increments_mil.size_min,
									 conf_core.editor.increments_mil.size_max,
									 CE_SMALL, umil, width, increment_spin_button_cb,
									 pcb_strdup(pathmil), NULL, NULL);

	label = gtk_label_new("");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), _("For <small>'s' and '&lt;shift&gt;s' on lines, pads, text; '&lt;ctrl&gt;s' and '&lt;shift&gt;&lt;ctrl&gt;s' on holes.</small>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 2);

	/* ---- Line Increment/Decrement ---- */
	strcpy(pemm, "line");
	strcpy(pemil, "line");
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

	ghid_coord_entry(hbox, NULL,
									 conf_core.editor.increments_mm.line,
									 conf_core.editor.increments_mm.line_min,
									 conf_core.editor.increments_mm.line_max,
									 CE_SMALL, umm, width, increment_spin_button_cb,
									 pcb_strdup(pathmm), _("Line:"), NULL);

	ghid_coord_entry(hbox, NULL,
									 conf_core.editor.increments_mil.line,
									 conf_core.editor.increments_mil.line_min,
									 conf_core.editor.increments_mil.line_max,
									 CE_SMALL, umil, width, increment_spin_button_cb,
									 pcb_strdup(pathmil), NULL, NULL);

	label = gtk_label_new("");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), _("<small>For 'l' and '&lt;shift&gt;l' routing line width change actions</small>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 2);

	/* ---- Clear Increment/Decrement ---- */
	strcpy(pemm, "clear");
	strcpy(pemil, "clear");
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

	ghid_coord_entry(hbox, NULL,
									 conf_core.editor.increments_mm.clear,
									 conf_core.editor.increments_mm.clear_min,
									 conf_core.editor.increments_mm.clear_max,
									 CE_SMALL, umm, width, increment_spin_button_cb,
									 pcb_strdup(pathmm), _("Clear:"), NULL);

	ghid_coord_entry(hbox, NULL,
									 conf_core.editor.increments_mil.clear,
									 conf_core.editor.increments_mil.clear_min,
									 conf_core.editor.increments_mil.clear_max,
									 CE_SMALL, umil, width, increment_spin_button_cb,
									 pcb_strdup(pathmil), NULL, NULL);

	label = gtk_label_new("");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label),_("<small>For 'k' and '&lt;shift&gt;k' line clearance inside polygon size change actions</small>"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 2);


	gtk_widget_show_all(config_increments_vbox);
}

static GtkWidget *config_increments_table_attach(GtkWidget *table, int x, int y, int colspan, const char *text)
{
	GtkWidget *box, *label;

	box = gtk_vbox_new(FALSE, 0);
	label = gtk_label_new(text);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
	gtk_table_attach(GTK_TABLE(table), box,  x,x+colspan,y,y+1,   0,0,10,2);
	return label;
}

void config_increments_save(GtkButton *widget, save_ctx_t *ctx)
{
	const char *paths[] = {
		"editor/increments_mm",
		"editor/increments_mil",
		NULL
	};

	config_any_replace(ctx, paths);
}

static void config_increments_tab_create(GtkWidget * tab_vbox)
{
	GtkWidget *vbox, *catvbox, *content_vbox;

	content_vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(tab_vbox), content_vbox, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(content_vbox), 6);

	if (!config_increments_vbox)
		gtk_widget_destroy(GTK_BOX(config_increments_vbox));

	/* the actual content */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(content_vbox), vbox, FALSE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
	config_increments_vbox = vbox;
	config_increments_tab_vbox = content_vbox;


	catvbox = ghid_category_vbox (config_increments_vbox, "Increment Settings", 0, 0, TRUE, TRUE);
	config_increments_sect_create(catvbox);

	catvbox = ghid_category_vbox (config_increments_vbox, _("Comparison table"), 0, 0, TRUE, TRUE);

	/* increment summary table */
	{
		GtkWidget *table;
		int y, x;

		table = gtk_table_new(7, 3, 0);
		gtk_box_pack_start(GTK_BOX(catvbox), table, FALSE, FALSE, 0);

		config_increments_table_attach(table, 1, 0, 2, "converter to mm");
		config_increments_table_attach(table, 1, 1, 1, "metric editing");
		config_increments_table_attach(table, 2, 1, 1, "imperial editing");

		config_increments_table_attach(table, 3, 0, 2, "converter to mil");
		config_increments_table_attach(table, 3, 1, 1, "metric editing");
		config_increments_table_attach(table, 4, 1, 1, "imperial editing");

		config_increments_table_attach(table, 0, 2, 1, "grid");
		config_increments_table_attach(table, 0, 3, 1, "size");
		config_increments_table_attach(table, 0, 4, 1, "line");
		config_increments_table_attach(table, 0, 5, 1, "clear");

		for(y = 0; y < 4; y++)
			for(x = 0; x < 4; x++)
				config_increments_tbl[x][y] = config_increments_table_attach(table, x+1, y+2, 1, "n/a");
		increment_tbl_update();
	}

	vbox = gtk_vbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(tab_vbox), vbox, TRUE, TRUE, 0);
	config_user_role_section(tab_vbox, config_increments_save);
}

	/* -------------- The Library config page ----------------
	 */
static gtk_conf_list_t library_cl;

static void config_library_apply(void)
{
	fp_rehash();
}

static char *get_misc_col_data(int row, int col, lht_node_t *nd)
{
	if ((nd != NULL) && (col == 1)) {
		char *out;
		resolve_path(nd->data.text.value, &out, 0);
		return out;
	}
	return NULL;
}

#warning TODO: leak: this is never free()d
lht_doc_t *config_library_lst_doc;
lht_node_t *config_library_lst;

static void lht_clean_list(lht_node_t *lst)
{
	lht_node_t *n;
	while(lst->data.list.first != NULL) {
		n = lst->data.list.first;
		if (n->doc == NULL) {
			if (lst->data.list.last == n)
				lst->data.list.last = NULL;
			lst->data.list.first = n->next;
		}
		else
			lht_tree_unlink(n);
		lht_dom_node_free(n);
	}
	lst->data.list.last = NULL;
}

/* Create a lihata list of the current library paths - to be tuned into CFR design upon the first modification */
static lht_node_t *config_library_list()
{
	conf_listitem_t *i;
	int idx;
	const char *s;

	if (config_library_lst_doc == NULL) {
		/* create a new list */
		config_library_lst_doc = lht_dom_init();
		config_library_lst = lht_dom_node_alloc(LHT_LIST, "library_search_paths");
		config_library_lst_doc->root = config_library_lst;
		config_library_lst->doc = config_library_lst_doc;
	}
	else {
		/* clean the old list */
		lht_clean_list(config_library_lst);
	}

	conf_loop_list_str(&conf_core.rc.library_search_paths, i, s, idx) {
		lht_node_t *txt;
		const char *sfn;
		if (i->prop.src->file_name != NULL) {
			lht_dom_loc_newfile(config_library_lst_doc, i->prop.src->file_name);
			lht_dom_loc_active(config_library_lst_doc, &sfn, NULL, NULL);
		}
		else
			sfn = NULL;

		txt = lht_dom_node_alloc(LHT_TEXT, "");
		txt->data.text.value = pcb_strdup(i->payload);
		txt->file_name = sfn;
		txt->doc = config_library_lst_doc;
		printf("append: '%s' '%s'\n", txt->data.text.value, sfn);
		lht_dom_list_append(config_library_lst, txt);
	}
	return config_library_lst;
}

static void pre_rebuild(gtk_conf_list_t *cl)
{
	lht_node_t *m;
	lht_clean_list(config_library_lst);

	m = conf_lht_get_first(CFR_DESIGN);
	
	cl->lst = lht_tree_path_(m->doc, m, "rc/library_search_paths", 1, 0, NULL);
	if (cl->lst == NULL) {
		conf_set(CFR_DESIGN, "rc/library_search_paths", 0, "", POL_OVERWRITE);
	}
	cl->lst = lht_tree_path_(m->doc, m, "rc/library_search_paths", 1, 0, NULL);
	assert(cl->lst != NULL);

	lht_clean_list(cl->lst);
}

static void post_rebuild(gtk_conf_list_t *cl)
{
	conf_update("rc/library_search_paths");
}


static GtkWidget *config_library_append_paths(int post_sep)
{
	GtkWidget *hbox, *vbox_key, *vbox_val, *vbox_sep, *label;
	htsp_entry_t *e;

	hbox = gtk_hbox_new(FALSE, 0);

	vbox_key = gtk_vbox_new(FALSE, 0);
	vbox_sep = gtk_vbox_new(FALSE, 0);
	vbox_val = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox_key, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox_sep, FALSE, FALSE, 16);
	gtk_box_pack_start(GTK_BOX(hbox), vbox_val, FALSE, FALSE, 0);

	conf_fields_foreach(e) {
		if (strncmp(e->key, "rc/path/", 8) == 0) {
			conf_native_t *nat = e->value;
			char tmp[256];

			sprintf(tmp, "  $(rc.path.%s)", e->key+8);
			label = gtk_label_new(tmp);
			gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
			gtk_box_pack_start(GTK_BOX(vbox_key), label, FALSE, FALSE, 0);

			label = gtk_label_new(nat->val.string[0]);
			gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
			gtk_box_pack_start(GTK_BOX(vbox_val), label, FALSE, FALSE, 0);
		}
	}
	if (post_sep) {
		label = gtk_label_new("\n\n");
		gtk_box_pack_start(GTK_BOX(vbox_val), label, FALSE, FALSE, 0);
	}

	return hbox;
}

void config_library_save(GtkButton *widget, save_ctx_t *ctx)
{
	const char *paths[] = {
		"rc/library_search_paths",
		NULL
	};

	config_any_replace(ctx, paths);
}

static void config_library_tab_create(GtkWidget * tab_vbox)
{
	GtkWidget *vbox, *label, *entry, *content_vbox, *paths_box;
	const char *cnames[] = {"configured path", "actual path on the filesystem", "config source"};

	library_cl.num_cols = 3;
	library_cl.col_names = cnames;
	library_cl.col_data = 0;
	library_cl.col_src = 2;
	library_cl.reorder = 1;
	library_cl.get_misc_col_data = NULL;
	library_cl.file_chooser_title = "Select footprint library directory";
	library_cl.file_chooser_postproc = NULL;
	library_cl.get_misc_col_data = get_misc_col_data;
	library_cl.lst = config_library_list();
	library_cl.pre_rebuild = pre_rebuild;
	library_cl.post_rebuild = post_rebuild;

	content_vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(tab_vbox), content_vbox, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(content_vbox), 6);

	gtk_container_set_border_width(GTK_CONTAINER(content_vbox), 6);
	vbox = ghid_category_vbox(content_vbox, _("Element Directories"), 4, 2, TRUE, FALSE);

	label = gtk_label_new(_("Ordered list of footprint library search directories; use drag&drop to reorder.\nThe following $(variables) can be used in the path:\n\n"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	paths_box = config_library_append_paths(1);
	gtk_box_pack_start(GTK_BOX(vbox), paths_box, FALSE, FALSE, 0);

	entry = gtk_conf_list_widget(&library_cl);
	gtk_box_pack_start(GTK_BOX(vbox), entry, TRUE, TRUE, 4);

	config_user_role_section(tab_vbox, config_library_save);
}


	/* -------------- The Layers Group config page ----------------
	 */
static GtkWidget *config_groups_table, *config_groups_vbox, *config_groups_window;

static GtkWidget *layer_entry[MAX_LAYER];
static GtkWidget *group_button[MAX_LAYER + 2][MAX_LAYER];

static gint config_layer_group[MAX_LAYER + 2];

static LayerGroupType layer_groups,	/* Working copy */
 *lg_monitor;										/* Keep track if our working copy */
										/* needs to be changed (new layout) */

static gboolean groups_modified, groups_holdoff, layers_applying;

static gchar *layer_info_text[] = {
	N_("<h>Layer Names\n"),
	N_("You may enter layer names for the layers drawn on the screen.\n"
		 "The special 'component side' and 'solder side' are layers which\n"
		 "will be printed out, so they must have in their group at least one\n"
		 "of the other layers that are drawn on the screen.\n"),
	"\n",
	N_("<h>Layer Groups\n"),
	N_("Each layer on the screen may be in its own group which allows the\n"
		 "maximum number of board layers.  However, for boards with fewer\n"
		 "layers, you may group layers together which will then print as a\n"
		 "single layer on a printout.  This allows a visual color distinction\n"
		 "to be displayed on the screen for signal groups which will print as\n" "a single layer\n"),
	"\n",
	N_("For example, for a 4 layer board a useful layer group arrangement\n"
		 "can be to have 3 screen displayed layers grouped into the same group\n"
		 "as the 'component side' and 'solder side' printout layers.  Then\n"
		 "groups such as signals, ground, and supply traces can be color\n"
		 "coded on the screen while printing as a single layer.  For this\n"
		 "you would select buttons and enter names on the Setup page to\n" "structure four layer groups similar to this:\n"),
	"\n",
	N_("<b>Group 1:"),
	"\n\t",
	N_("solder"),
	"\n\t",
	N_("GND-solder"),
	"\n\t",
	N_("Vcc-solder"),
	"\n\t",
	N_("solder side"),
	"\n",
	N_("<b>Group 2:"),
	"\n\t",
	N_("component"),
	"\n\t",
	N_("GND-component"),
	"\n\t",
	N_("Vcc-component"),
	"\n\t",
	N_("component side"),
	"\n",
	N_("<b>Group 3:"),
	"\n\t",
	N_("signal1"),
	"\n",
	N_("<b>Group 4:"),
	"\n\t",
	N_("signal2"),
	"\n"
};

static void config_layer_groups_radio_button_cb(GtkToggleButton * button, gpointer data)
{
	gint layer = GPOINTER_TO_INT(data) >> 8;
	gint group = GPOINTER_TO_INT(data) & 0xff;

	if (!gtk_toggle_button_get_active(button) || groups_holdoff)
		return;
	config_layer_group[layer] = group;
	groups_modified = TRUE;
}

	/* Construct a layer group string.  Follow logic in WritePCBDataHeader(),
	   |  but use g_string functions.
	 */
static gchar *make_layer_group_string(LayerGroupType * lg)
{
	GString *string;
	gint group, entry, layer;

	string = g_string_new("");

	for (group = 0; group < max_group; group++) {
		if (lg->Number[group] == 0)
			continue;
		for (entry = 0; entry < lg->Number[group]; entry++) {
			layer = lg->Entries[group][entry];
			if (layer == component_silk_layer)
				string = g_string_append(string, "c");
			else if (layer == solder_silk_layer)
				string = g_string_append(string, "s");
			else
				g_string_append_printf(string, "%d", layer + 1);

			if (entry != lg->Number[group] - 1)
				string = g_string_append(string, ",");
		}
		if (group != max_group - 1)
			string = g_string_append(string, ":");
	}
	return g_string_free(string, FALSE);	/* Don't free string->str */
}

static void config_layers_apply(void)
{
	LayerType *layer;
	gchar *s;
	gint group, i;
	gint componentgroup = 0, soldergroup = 0;
	gboolean layers_modified = FALSE;

	/* Get each layer name entry and dup if modified into the PCB layer names
	   |  and, if to use as default, the Settings layer names.
	 */
	for (i = 0; i < max_copper_layer; ++i) {
		layer = &PCB->Data->Layer[i];
		s = ghid_entry_get_text(layer_entry[i]);
		if (dup_string(&layer->Name, s))
			layers_modified = TRUE;
	}
	/* Layer names can be changed from the menus and that can update the
	   |  config.  So holdoff the loop.
	 */
	layers_applying = TRUE;
	if (layers_modified)
		ghid_layer_buttons_update();
	layers_applying = FALSE;

	if (groups_modified) {				/* If any group radio buttons were toggled. */
		/* clear all entries and read layer by layer
		 */
		for (group = 0; group < max_group; group++)
			layer_groups.Number[group] = 0;

		for (i = 0; i < max_copper_layer + 2; i++) {
			group = config_layer_group[i] - 1;
			layer_groups.Entries[group][layer_groups.Number[group]++] = i;

			if (i == component_silk_layer)
				componentgroup = group;
			else if (i == solder_silk_layer)
				soldergroup = group;
		}

		/* do some cross-checking
		   |  solder-side and component-side must be in different groups
		   |  solder-side and component-side must not be the only one in the group
		 */
		if (layer_groups.Number[soldergroup] <= 1 || layer_groups.Number[componentgroup] <= 1) {
			Message(PCB_MSG_DEFAULT, _("Both 'solder side' or 'component side' layers must have at least\n" "\tone other layer in their group.\n"));
			return;
		}
		else if (soldergroup == componentgroup) {
			Message(PCB_MSG_DEFAULT, _("The 'solder side' and 'component side' layers are not allowed\n" "\tto be in the same layer group #\n"));
			return;
		}
		PCB->LayerGroups = layer_groups;
		ghid_invalidate_all();
		groups_modified = FALSE;
	}
}

static void config_layer_group_button_state_update(void)
{
	gint g, i;

	/* Set button active corresponding to layer group state.
	 */
	groups_holdoff = TRUE;
	for (g = 0; g < max_group; g++)
		for (i = 0; i < layer_groups.Number[g]; i++) {
/*			printf("layer %d in group %d\n", layer_groups.Entries[g][i], g +1); */
			config_layer_group[layer_groups.Entries[g][i]] = g + 1;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(group_button[layer_groups.Entries[g][i]][g]), TRUE);
		}
	groups_holdoff = FALSE;
}

static void layer_name_entry_cb(GtkWidget * entry, gpointer data)
{
	gint i = GPOINTER_TO_INT(data);
	LayerType *layer;
	gchar *name;

	layer = &PCB->Data->Layer[i];
	name = ghid_entry_get_text(entry);
	if (dup_string(&layer->Name, name))
		ghid_layer_buttons_update();
}

void ghid_config_groups_changed(void)
{
	GtkWidget *vbox, *table, *button, *label, *scrolled_window;
	GSList *group;
	gchar buf[32], *name;
	gint layer, i;

	if (!config_groups_vbox)
		return;
	vbox = config_groups_vbox;

	if (config_groups_table)
		gtk_widget_destroy(config_groups_table);
	if (config_groups_window)
		gtk_widget_destroy(config_groups_window);

	config_groups_window = scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(scrolled_window, (max_group + 1)*34, 300);
	gtk_container_set_border_width(GTK_CONTAINER(scrolled_window), 3);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
	gtk_widget_show(scrolled_window);


	table = gtk_table_new(max_copper_layer + 3, max_group + 1, FALSE);
	config_groups_table = table;
	gtk_table_set_row_spacings(GTK_TABLE(table), 3);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), table);
	gtk_widget_show(table);

	layer_groups = PCB->LayerGroups;	/* working copy */
	lg_monitor = &PCB->LayerGroups;	/* So can know if PCB changes on us */

	label = gtk_label_new(_("Group #"));
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
	gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);

	for (i = 1; i < max_group + 1; ++i) {
		snprintf(buf, sizeof(buf), "% 3d", i);
		label = gtk_label_new(buf);
		gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
		gtk_table_attach_defaults(GTK_TABLE(table), label, i, i + 1, 0, 1);
	}

	/* Create a row of radio toggle buttons for layer.  So each layer
	   |  can have an active radio button set for the group it needs to be in.
	 */
	for (layer = 0; layer < max_copper_layer + 2; ++layer) {
		if (layer == component_silk_layer)
			name = _("component side");
		else if (layer == solder_silk_layer)
			name = _("solder side");
		else
			name = (gchar *) UNKNOWN(PCB->Data->Layer[layer].Name);

		if (layer >= max_copper_layer) {
			label = gtk_label_new(name);
			gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
			gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, layer + 1, layer + 2);
		}
		else {
			layer_entry[layer] = gtk_entry_new();
			gtk_entry_set_text(GTK_ENTRY(layer_entry[layer]), name);
			gtk_table_attach_defaults(GTK_TABLE(table), layer_entry[layer], 0, 1, layer + 1, layer + 2);
			g_signal_connect(G_OBJECT(layer_entry[layer]), "activate", G_CALLBACK(layer_name_entry_cb), GINT_TO_POINTER(layer));
		}

		group = NULL;
		for (i = 0; i < max_group; ++i) {
			button = gtk_radio_button_new(group);

			group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
			gtk_table_attach_defaults(GTK_TABLE(table), button, i + 1, i + 2, layer + 1, layer + 2);
			g_signal_connect(G_OBJECT(button), "toggled",
											 G_CALLBACK(config_layer_groups_radio_button_cb), GINT_TO_POINTER((layer << 8) | (i + 1)));
			group_button[layer][i] = button;
		}
	}
	gtk_widget_show_all(config_groups_vbox);
	config_layer_group_button_state_update();
}


static void edit_layer_button_cb(GtkWidget * widget, gchar * data)
{
	gchar **argv;

	if (PCB->RatDraw || PCB->SilkActive)
		return;

	argv = g_strsplit(data, ",", -1);
	MoveLayerAction(2, argv, 0, 0);
	g_strfreev(argv);
}

void config_layers_save(GtkButton *widget, save_ctx_t *ctx)
{
	gchar *s;
	int n;
	const char *paths[] = {
		"design/groups",
		"design/default_layer_name",
		NULL
	};


	config_layers_apply();
	s = make_layer_group_string(&PCB->LayerGroups);

	/* change default layer names to the current ones in dest */
	for (n = 0; n < max_copper_layer; n++) {
		LayerType *layer;
		char lnp[128];
		lht_node_t *nd;
		sprintf(lnp, "design/default_layer_name[%d]", n);
		nd = conf_lht_get_at(ctx->dst_role, lnp, 1);
		layer = &PCB->Data->Layer[n];
		if (strcmp(layer->Name, nd->data.text.value) != 0) {
			free(nd->data.text.value);
			nd->data.text.value = pcb_strdup(layer->Name);
			conf_makedirty(ctx->dst_role);
		}
	}
	conf_update("design/default_layer_name");
	conf_set(CFR_DESIGN, "design/groups", -1, s, POL_OVERWRITE);
	g_free(s);
	config_any_replace(ctx, paths);
}

static void config_layers_tab_create(GtkWidget * tab_vbox)
{
	GtkWidget *tabs, *vbox, *vbox1, *button, *text, *content_vbox;
	GtkWidget *hbox, *arrow;
	gint i;

	content_vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(tab_vbox), content_vbox, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(content_vbox), 6);

	tabs = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(content_vbox), tabs, TRUE, TRUE, 0);

/* -- Change tab */
	vbox = ghid_notebook_page(tabs, _("Change"), 0, 6);
	vbox1 = ghid_category_vbox(vbox, _("Operations on currently selected layer:"), 4, 2, TRUE, TRUE);

	button = gtk_button_new();
	arrow = gtk_arrow_new(GTK_ARROW_UP, GTK_SHADOW_ETCHED_IN);
	gtk_container_add(GTK_CONTAINER(button), arrow);
	g_signal_connect(G_OBJECT(button), (gchar *) "clicked", G_CALLBACK(edit_layer_button_cb), (gchar *) "c,up");
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

	button = gtk_button_new();
	arrow = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_ETCHED_IN);
	gtk_container_add(GTK_CONTAINER(button), arrow);
	g_signal_connect(G_OBJECT(button), (gchar *) "clicked", G_CALLBACK(edit_layer_button_cb), (gchar *) "c,down");
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

	button = gtk_button_new_from_stock(GTK_STOCK_DELETE);
	g_signal_connect(G_OBJECT(button), (gchar *) "clicked", G_CALLBACK(edit_layer_button_cb), (gchar *) "c,-1");
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

	vbox1 = ghid_category_vbox(vbox, _("Add new layer above currently selected layer:"), 4, 2, TRUE, TRUE);
	button = gtk_button_new_from_stock(GTK_STOCK_ADD);
	g_signal_connect(G_OBJECT(button), (gchar *) "clicked", G_CALLBACK(edit_layer_button_cb), (gchar *) "-1,c");
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

/* -- Groups tab */
	vbox = ghid_notebook_page(tabs, _("Groups"), 0, 6);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

	config_groups_vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), config_groups_vbox, TRUE, TRUE, 0);
	ghid_config_groups_changed();

	/* A dummy vbox on the right to take up excess horizontal space so the layer list is not scretched horizontally */
	vbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

/* -- Info tab */
	vbox = ghid_notebook_page(tabs, _("Info"), 0, 6);

	text = ghid_scrolled_text_view(vbox, NULL, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	for (i = 0; i < sizeof(layer_info_text) / sizeof(gchar *); ++i)
		ghid_text_view_append(text, _(layer_info_text[i]));

/* -- common */
	vbox = gtk_vbox_new(TRUE, 0);
	gtk_box_pack_start(GTK_BOX(tab_vbox), vbox, TRUE, TRUE, 0);
	config_user_role_section(tab_vbox, config_layers_save);
}

void ghid_config_layer_name_update(gchar * name, gint layer)
{
	if (!config_window || layers_applying || !name)
		return;
	gtk_entry_set_text(GTK_ENTRY(layer_entry[layer]), name);

	/* If we get a config layer name change because a new PCB is loaded
	   |  or new layout started, need to change our working layer group copy.
	 */
	if (lg_monitor != &PCB->LayerGroups) {
		layer_groups = PCB->LayerGroups;
		lg_monitor = &PCB->LayerGroups;
		config_layer_group_button_state_update();
		groups_modified = FALSE;
	}
}

	/* -------------- The Colors config page ----------------
	 */
static GtkWidget *config_colors_vbox,
	*config_colors_tab_vbox;

static void config_colors_tab_create(GtkWidget * tab_vbox);

typedef struct {
	conf_native_t *cfg;
	int idx;
	GdkColor *color;
	GtkWidget *button;
} cfg_color_idx_t;

static void config_color_set_cb(GtkWidget * button, cfg_color_idx_t *ci)
{
	GdkColor new_color;
	const char *str;

	gtk_color_button_get_color(GTK_COLOR_BUTTON(button), &new_color);
	str = ghid_get_color_name(&new_color);

	if (conf_set(CFR_DESIGN, ci->cfg->hash_path, ci->idx, str, POL_OVERWRITE) == 0) {
		ghid_set_special_colors(ci->cfg);
		ghid_layer_buttons_color_update();
		ghid_invalidate_all();
	}
}

static void config_color_button_create(GtkWidget * box, conf_native_t *cfg, int idx)
{
	GtkWidget *hbox, *label;
	gchar *title;
	cfg_color_idx_t *ci = conf_hid_get_data(cfg, ghid_conf_id);

	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 0);

	if (ci == NULL) {
#warning LEAK: this is never free()d
		ci = malloc(sizeof(cfg_color_idx_t));
		ci->cfg = cfg;
		ci->idx = idx;
		ci->color = calloc(sizeof(GdkColor), cfg->array_size);
		conf_hid_set_data(cfg, ghid_conf_id, ci);
	}

	ghid_map_color_string(cfg->val.color[idx], &(ci->color[idx]));

	title = g_strdup_printf(_("pcb-rnd %s Color"), cfg->description);
	ci->button = gtk_color_button_new_with_color(&(ci->color[idx]));
	gtk_color_button_set_title(GTK_COLOR_BUTTON(ci->button), title);
	g_free(title);

	gtk_box_pack_start(GTK_BOX(hbox), ci->button, FALSE, FALSE, 0);
	label = gtk_label_new(cfg->description);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(ci->button), "color-set", G_CALLBACK(config_color_set_cb), ci);
}

void config_color_button_update(conf_native_t *cfg, int idx)
{
	if (idx < 0) {
		for(idx = 0; idx < cfg->array_size; idx++)
			config_color_button_update(cfg, idx);
	}
	else {
		cfg_color_idx_t *ci = conf_hid_get_data(cfg, ghid_conf_id);

		ghid_map_color_string(cfg->val.color[idx], &(ci->color[idx]));
		gtk_color_button_set_color(GTK_COLOR_BUTTON(ci->button), &(ci->color[idx]));
	}
}

void config_colors_tab_create_scalar(GtkWidget *parent_vbox, const char *path_prefix, int selected)
{
	htsp_entry_t *e;
	int pl = strlen(path_prefix);

	conf_fields_foreach(e) {
		conf_native_t *cfg = e->value;
		if ((strncmp(e->key, path_prefix, pl) == 0) && (cfg->type == CFN_COLOR) && (cfg->array_size == 1)) {
			int is_selected = (strstr(e->key, "_selected") != NULL);
			if (is_selected == selected)
				config_color_button_create(parent_vbox, cfg, 0);
		}
	}
}

void config_colors_tab_create_array(GtkWidget *parent_vbox, const char *path)
{
	conf_native_t *cfg = conf_get_field(path);
	int n;
	if (cfg->type != CFN_COLOR)
		return;

	for(n = 0; n < cfg->used; n++)
		config_color_button_create(parent_vbox, cfg, n);
}

void config_colors_save(GtkButton *widget, save_ctx_t *ctx)
{
	const char *paths[] = {
		"*appearance/color/",
		NULL
	};

	config_any_replace(ctx, paths);
}

static void config_colors_tab_create(GtkWidget * tab_vbox)
{
	GtkWidget *scrolled_vbox, *vbox, *expander;

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(tab_vbox), vbox, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

	config_colors_vbox = vbox;		/* can be destroyed if color file loaded */
	config_colors_tab_vbox = tab_vbox;

	scrolled_vbox = ghid_scrolled_vbox(config_colors_vbox, NULL, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	/* ---- Main colors ---- */
	expander = gtk_expander_new(_("Main colors"));
	gtk_box_pack_start(GTK_BOX(scrolled_vbox), expander, FALSE, FALSE, 2);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(expander), vbox);
	vbox = ghid_category_vbox(vbox, NULL, 0, 2, TRUE, FALSE);

	config_colors_tab_create_scalar(vbox, "appearance/color", 0);

	/* ---- Layer colors ---- */
	expander = gtk_expander_new(_("Layer colors"));
	gtk_box_pack_start(GTK_BOX(scrolled_vbox), expander, FALSE, FALSE, 2);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(expander), vbox);
	vbox = ghid_category_vbox(vbox, NULL, 0, 2, TRUE, FALSE);

	config_colors_tab_create_array(vbox, "appearance/color/layer");

	/* ---- Selected colors ---- */
	expander = gtk_expander_new(_("Selected colors"));
	gtk_box_pack_start(GTK_BOX(scrolled_vbox), expander, FALSE, FALSE, 2);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(expander), vbox);
	vbox = ghid_category_vbox(vbox, NULL, 0, 2, TRUE, FALSE);

	config_colors_tab_create_scalar(vbox, "appearance/color", 1);

	config_colors_tab_create_array(vbox, "appearance/color/layer_selected");
	config_user_role_section(config_colors_vbox, config_colors_save);

	gtk_widget_show_all(config_colors_vbox);
}


	/* --------------- The main config page -----------------
	 */
enum {
	CONFIG_NAME_COLUMN,
	CONFIG_PAGE_COLUMN,
	N_CONFIG_COLUMNS
};

static GtkNotebook *config_notebook;

static GtkWidget *config_page_create(GtkTreeStore * tree, GtkTreeIter * iter, GtkNotebook * notebook)
{
	GtkWidget *vbox;
	gint page;

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_notebook_append_page(notebook, vbox, NULL);
	page = gtk_notebook_get_n_pages(notebook) - 1;
	gtk_tree_store_set(tree, iter, CONFIG_PAGE_COLUMN, page, -1);
	return vbox;
}

void ghid_config_handle_units_changed(void)
{
	char *text = pcb_strdup_printf("<b>%s</b>",
																		conf_core.editor.grid_unit->in_suffix);
	ghid_set_cursor_position_labels();
	gtk_label_set_markup(GTK_LABEL(ghidgui->grid_units_label), text);
	free(text);

	if (config_sizes_vbox) {
		gtk_widget_destroy(config_sizes_vbox);
		config_sizes_vbox = NULL;
		config_sizes_tab_create(config_sizes_tab_vbox);
	}
}

void ghid_config_text_scale_update(void)
{
	if (config_window)
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(config_text_spin_button), (gdouble) conf_core.design.text_scale);
}

static void config_close_cb(gpointer data)
{
	/* Config pages may need to check for modified entries, use as default
	   |  options, etc when the config window is closed.
	 */
	config_sizes_apply();
	config_layers_apply();
	config_library_apply();

	config_sizes_vbox = NULL;
	config_increments_vbox = NULL;

	config_groups_vbox = config_groups_table = NULL;
	config_groups_window = NULL;

	gtk_widget_destroy(config_window);
	config_window = NULL;
}

static void config_destroy_cb(gpointer data)
{
	config_sizes_vbox = NULL;
	config_increments_vbox = NULL;
	config_groups_vbox = config_groups_table = NULL;
	config_groups_window = NULL;
	gtk_widget_destroy(config_window);
	config_window = NULL;
}

static void config_selection_changed_cb(GtkTreeSelection * selection, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	gint page;

	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		return;
	gtk_tree_model_get(model, &iter, CONFIG_PAGE_COLUMN, &page, -1);
	gtk_notebook_set_current_page(config_notebook, page);
}

/* Create a root (e.g. Config PoV) top level in the preference tree; iter is output and acts as a parent for further nodes */
static void config_tree_sect(GtkTreeStore *model, GtkTreeIter *parent, GtkTreeIter *iter, const char *name, const char *desc)
{
	GtkWidget *vbox, *label;
	gtk_tree_store_append(model, iter, parent);
	gtk_tree_store_set(model, iter, CONFIG_NAME_COLUMN, name, -1);
	vbox = config_page_create(model, iter, config_notebook);
	label = gtk_label_new("");
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
	gtk_label_set_markup(GTK_LABEL(label), desc);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
}

/* Create a leaf node with a custom tab */
static GtkWidget *config_tree_leaf(GtkTreeStore *model, GtkTreeIter *parent, const char *name, void (*tab_create)(GtkWidget *tab_vbox))
{
	GtkTreeIter iter;
	GtkWidget *vbox;

	gtk_tree_store_append(model, &iter, parent);
	gtk_tree_store_set(model, &iter, CONFIG_NAME_COLUMN, name, -1);
	vbox = config_page_create(model, &iter, config_notebook);
	if (tab_create != NULL)
		tab_create(vbox);
	return vbox;
}

/***** auto *****/
static void config_auto_tab_create(GtkWidget * tab_vbox, const char *basename, conf_native_t *item)
{
	GtkWidget *vbox, *label;
	GtkWidget *w;
	GtkObject *o;
	char *tmp, *so;
	const char *si;
	int l;

	gtk_container_set_border_width(GTK_CONTAINER(tab_vbox), 6);
	vbox = ghid_category_vbox(tab_vbox, basename, 4, 2, TRUE, TRUE);

	/* split long description strings at whitepsace to make the preferences window narrower */
	tmp = malloc(strlen(item->description) * 2);
	for(si = item->description, so = tmp, l = 0; *si != '\0'; si++,so++,l++) {
		if (isspace(*si) && (l > 64))
			*so = '\n';
		else
			*so = *si;

		if (*so == '\n')
			l = 0;
	}
	*so = '\0';

	label = gtk_label_new(tmp);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	free(tmp);

	switch(item->type) {
		case CFN_STRING:
			w = gtk_entry_new();
			gtk_entry_set_text(GTK_ENTRY(w), *item->val.string);
			gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
			break;
		case CFN_COORD:
			w = ghid_coord_entry_new(10, 15000, 2000000, conf_core.editor.grid_unit, CE_TINY);
			gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
			break;
		case CFN_INTEGER:
			o = gtk_adjustment_new(15,
																							10, /* min */
																							20,
																							1, 10, /* steps */
																							0.0);
			gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(o), FALSE, FALSE, 4);
			break;
		case CFN_REAL:
			o = gtk_adjustment_new(15.5,
																							10, /* min */
																							20,
																							1, 10, /* steps */
																							0.0);
			gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(o), FALSE, FALSE, 4);
			break;
		case CFN_BOOLEAN:
			ghid_check_button_connected(vbox, NULL, *item->val.boolean,
															TRUE, FALSE, FALSE, 2,
															config_command_window_toggle_cb, NULL, _("todo81"));
			break;
		case CFN_UNIT:
		case CFN_COLOR:
		case CFN_LIST:
		case CFN_INCREMENTS:

/*			gtk_entry_set_text(GTK_ENTRY(entry), *item->val.string);
			gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 4);*/
			break;
	}
}

static GtkTreeIter *config_tree_auto_mkdirp(GtkTreeStore *model, GtkTreeIter *main_parent, htsp_t *dirs, char *path)
{
	char *basename;
	GtkTreeIter *parent, *cwd;

	cwd = htsp_get(dirs, path);
	if (cwd != NULL)
		return cwd;

	cwd = malloc(sizeof(GtkTreeIter));
	htsp_set(dirs, path, cwd);

	basename = strrchr(path, '/');
	if (basename == NULL) {
		parent = main_parent;
		basename = path;
	}
	else {
		*basename = '\0';
		basename++;
		parent = config_tree_auto_mkdirp(model, main_parent, dirs, path);
	}

	config_tree_sect(model, parent, cwd, basename, "");
	return cwd;
}

static int config_tree_auto_cmp(const void *v1, const void *v2)
{
	const htsp_entry_t **e1 = (const htsp_entry_t **)v1, **e2 = (const htsp_entry_t **)v2;
	return strcmp((*e1)->key, (*e2)->key);
}

/* Automatically create a subtree using the central config field hash */
static void config_tree_auto(GtkTreeStore *model, GtkTreeIter *main_parent)
{
	htsp_t *dirs;
	htsp_entry_t *e;
	char path[1024];
	GtkTreeIter *parent;
	GtkWidget *tab;
	htsp_entry_t **sorted;
	int num_paths, n;

	/* remember the parent for each dir */
	dirs = htsp_alloc(strhash, strkeyeq);

	/* alpha sort keys for the more consistend UI */
	for (e = htsp_first(conf_fields), num_paths = 0; e; e = htsp_next(conf_fields, e))
		num_paths++;
	sorted = malloc(sizeof(htsp_entry_t *) * num_paths);
	for (e = htsp_first(conf_fields), n = 0; e; e = htsp_next(conf_fields, e), n++)
		sorted[n] = e;
	qsort(sorted, num_paths, sizeof(htsp_entry_t *), config_tree_auto_cmp);

	for (n = 0; n < num_paths; n++) {
		char *basename;
		e = sorted[n];
		if (strlen(e->key) > sizeof(path)-1) {
			Message(PCB_MSG_DEFAULT, "Warning: can't create config item for %s: path too long\n", e->key);
			continue;
		}
		strcpy(path, e->key);
		basename = strrchr(path, '/');
		if ((basename == NULL) || (basename == path)) {
			Message(PCB_MSG_DEFAULT, "Warning: can't create config item for %s: invalid path\n", e->key);
			continue;
		}
		*basename = '\0';
		basename++;
		parent = config_tree_auto_mkdirp(model, main_parent, dirs, path);
		tab = config_tree_leaf(model, parent, basename, NULL);
		config_auto_tab_create(tab, basename, e->value);
	}
	htsp_free(dirs);
	free(sorted);
}

void ghid_config_window_show(void)
{
	GtkWidget *widget, *main_vbox, *config_hbox, *hbox;
	GtkWidget *scrolled;
	GtkWidget *button;
	GtkTreeStore *model;
	GtkTreeView *treeview;
	GtkTreeIter user_pov, config_pov;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;

	if (config_window) {
		gtk_window_present(GTK_WINDOW(config_window));
		return;
	}

	config_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT(config_window), "delete_event", G_CALLBACK(config_destroy_cb), NULL);

	gtk_window_set_title(GTK_WINDOW(config_window), _("pcb-rnd Preferences"));
	gtk_window_set_wmclass(GTK_WINDOW(config_window), "Pcb_Conf", "PCB");
	gtk_container_set_border_width(GTK_CONTAINER(config_window), 2);

	config_hbox = gtk_hbox_new(FALSE, 4);
	gtk_container_add(GTK_CONTAINER(config_window), config_hbox);

	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
	gtk_widget_set_size_request(scrolled, 150, 0);
	gtk_box_pack_start(GTK_BOX(config_hbox), scrolled, TRUE, TRUE, 0);

	main_vbox = gtk_vbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(config_hbox), main_vbox, TRUE, TRUE, 0);

	widget = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(main_vbox), widget, TRUE, TRUE, 0);
	config_notebook = GTK_NOTEBOOK(widget);
	gtk_notebook_set_show_tabs(config_notebook, FALSE);

	/* build the tree */
	model = gtk_tree_store_new(N_CONFIG_COLUMNS, G_TYPE_STRING, G_TYPE_INT);

	config_tree_sect(model, NULL, &user_pov,   _("User PoV"),   _("\n<b>User PoV</b>\nA subset of configuration settings regroupped,\npresented in the User's Point of View."));
	config_tree_sect(model, NULL, &config_pov, _("Config PoV"), _("\n<b>Config PoV</b>\nAccess all configuration fields presented in\na tree that matches the configuration\nfile (lht) structure."));

	config_tree_leaf(model, &user_pov, _("General"), config_general_tab_create);
	config_tree_leaf(model, &user_pov, _("Window"), config_window_tab_create);
	config_tree_leaf(model, &user_pov, _("Sizes"), config_sizes_tab_create);
	config_tree_leaf(model, &user_pov, _("Increments"), config_increments_tab_create);
	config_tree_leaf(model, &user_pov, _("Library"), config_library_tab_create);
	config_tree_leaf(model, &user_pov, _("Layers"), config_layers_tab_create);
	config_tree_leaf(model, &user_pov, _("Colors"), config_colors_tab_create);

	config_tree_auto(model, &config_pov);

	/* Create the tree view */
	treeview = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(model)));
	g_object_unref(G_OBJECT(model));	/* Don't need the model anymore */

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(NULL, renderer, "text", CONFIG_NAME_COLUMN, NULL);
	gtk_tree_view_append_column(treeview, column);
	gtk_container_add(GTK_CONTAINER(scrolled), GTK_WIDGET(treeview));


	select = gtk_tree_view_get_selection(treeview);
	gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
	g_signal_connect(G_OBJECT(select), "changed", G_CALLBACK(config_selection_changed_cb), NULL);


	hbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing(GTK_BOX(hbox), 5);
	gtk_box_pack_start(GTK_BOX(main_vbox), hbox, FALSE, FALSE, 0);

	button = gtk_button_new_from_stock(GTK_STOCK_OK);
	gtk_widget_set_can_default(button, TRUE);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(config_close_cb), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
	gtk_widget_grab_default(button);

	gtk_widget_show_all(config_window);
}

static void ghid_config_window_close(void)
{
	gtk_widget_destroy(config_window);
	config_window = NULL;
}
