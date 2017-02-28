/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include "config.h"

#include <gtk/gtk.h>

#include "bu_mode_btn.h"
#include "bu_mode_btn.data"

#include "const.h"
#include "crosshair.h"
#include "conf_core.h"

#include "../src_plugins/lib_gtk_config/hid_gtk_conf.h"


typedef struct {
	GtkWidget *button;
	GtkWidget *toolbar_button;
	guint button_cb_id;
	guint toolbar_button_cb_id;
	const gchar *name;
	gint mode;
	const gchar **xpm;
	pcb_gtk_mode_btn_t *mbb;
} ModeButton;


static ModeButton mode_buttons[] = {
	{NULL, NULL, 0, 0, "via", PCB_MODE_VIA, via},
	{NULL, NULL, 0, 0, "line", PCB_MODE_LINE, line},
	{NULL, NULL, 0, 0, "arc", PCB_MODE_ARC, arc},
	{NULL, NULL, 0, 0, "text", PCB_MODE_TEXT, text},
	{NULL, NULL, 0, 0, "rectangle", PCB_MODE_RECTANGLE, rect},
	{NULL, NULL, 0, 0, "polygon", PCB_MODE_POLYGON, poly},
	{NULL, NULL, 0, 0, "polygonhole", PCB_MODE_POLYGON_HOLE, polyhole},
	{NULL, NULL, 0, 0, "buffer", PCB_MODE_PASTE_BUFFER, buf},
	{NULL, NULL, 0, 0, "remove", PCB_MODE_REMOVE, del},
	{NULL, NULL, 0, 0, "rotate", PCB_MODE_ROTATE, rot},
	{NULL, NULL, 0, 0, "insertPoint", PCB_MODE_INSERT_POINT, ins},
	{NULL, NULL, 0, 0, "thermal", PCB_MODE_THERMAL, thrm},
	{NULL, NULL, 0, 0, "select", PCB_MODE_ARROW, sel},
	{NULL, NULL, 0, 0, "lock", PCB_MODE_LOCK, lock}
};

static gint n_mode_buttons = G_N_ELEMENTS(mode_buttons);

static void do_set_mode(pcb_gtk_mode_btn_t *mbb, int mode)
{
	pcb_crosshair_set_mode(mode);
	mbb->com->mode_cursor_main(mode);
	mbb->settings_mode = mode;
}

static void mode_toolbar_button_toggled_cb(GtkToggleButton * button, ModeButton * mb)
{
	gboolean active = gtk_toggle_button_get_active(button);

	if (mb->button != NULL) {
		g_signal_handler_block(mb->button, mb->button_cb_id);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mb->button), active);
		g_signal_handler_unblock(mb->button, mb->button_cb_id);
	}

	if (active)
		do_set_mode(mb->mbb, mb->mode);
}

static void mode_button_toggled_cb(GtkWidget * widget, ModeButton * mb)
{
	gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

	if (mb->toolbar_button != NULL) {
		g_signal_handler_block(mb->toolbar_button, mb->toolbar_button_cb_id);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mb->toolbar_button), active);
		g_signal_handler_unblock(mb->toolbar_button, mb->toolbar_button_cb_id);
	}

	if (active)
		do_set_mode(mb->mbb, mb->mode);
}

void ghid_mode_buttons_update(void)
{
	ModeButton *mb;
	gint i;

	for (i = 0; i < n_mode_buttons; ++i) {
		mb = &mode_buttons[i];
		if (conf_core.editor.mode == mb->mode) {
			g_signal_handler_block(mb->button, mb->button_cb_id);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mb->button), TRUE);
			g_signal_handler_unblock(mb->button, mb->button_cb_id);

			g_signal_handler_block(mb->toolbar_button, mb->toolbar_button_cb_id);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mb->toolbar_button), TRUE);
			g_signal_handler_unblock(mb->toolbar_button, mb->toolbar_button_cb_id);
			break;
		}
	}
}

void pcb_gtk_pack_mode_buttons(pcb_gtk_mode_btn_t *mb)
{
	if (conf_hid_gtk.plugins.hid_gtk.compact_vertical) {
		gtk_widget_hide(mb->mode_buttons_frame);
		gtk_widget_show_all(mb->mode_toolbar);
	}
	else {
		gtk_widget_hide(mb->mode_toolbar);
		gtk_widget_show_all(mb->mode_buttons_frame);
	}
}

void pcb_gtk_make_mode_buttons_and_toolbar(pcb_gtk_common_t *com, pcb_gtk_mode_btn_t *mb)
{
	GtkToolItem *tool_item;
	GtkWidget *vbox, *hbox = NULL;
	GtkWidget *pad_hbox, *pad_vbox;
	GtkWidget *image;
	GdkPixbuf *pixbuf;
	GSList *group = NULL;
	GSList *toolbar_group = NULL;
	ModeButton *MB;
	int n_mb, i, tb_width = 0;

	mb->com = com;
	mb->mode_toolbar = gtk_toolbar_new();

	mb->mode_buttons_frame = gtk_frame_new(NULL);
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(mb->mode_buttons_frame), vbox);

	for (i = 0; i < n_mode_buttons; ++i) {
		MB = &mode_buttons[i];
		MB->mbb = mb;

		/* Create tool button for mode frame */
		MB->button = gtk_radio_button_new(group);
		group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(MB->button));
		gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(MB->button), FALSE);

		/* Create tool button for toolbar */
		MB->toolbar_button = gtk_radio_button_new(toolbar_group);
		toolbar_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(MB->toolbar_button));
		gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(MB->toolbar_button), FALSE);

		/* Pack mode-frame button into the frame */
		n_mb = conf_hid_gtk.plugins.hid_gtk.n_mode_button_columns;
		if ((n_mb < 1) || (n_mb > 10))
			n_mb = 3;
		if ((i % n_mb) == 0) {
			hbox = gtk_hbox_new(FALSE, 0);
			gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
		}
		gtk_box_pack_start(GTK_BOX(hbox), MB->button, FALSE, FALSE, 0);

		/* Create a container for the toolbar button and add that */
		tool_item = gtk_tool_item_new();
		gtk_container_add(GTK_CONTAINER(tool_item), MB->toolbar_button);
		gtk_toolbar_insert(GTK_TOOLBAR(mb->mode_toolbar), tool_item, -1);

		/* Load the image for the button, create GtkImage widgets for both
		 * the grid button and the toolbar button, then pack into the buttons
		 */
		pixbuf = gdk_pixbuf_new_from_xpm_data((const char **) MB->xpm);
		image = gtk_image_new_from_pixbuf(pixbuf);
		gtk_container_add(GTK_CONTAINER(MB->button), image);
		image = gtk_image_new_from_pixbuf(pixbuf);
		gtk_container_add(GTK_CONTAINER(MB->toolbar_button), image);
		g_object_unref(pixbuf);
		tb_width += image->requisition.width;

		if (strcmp(MB->name, "select") == 0) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(MB->button), TRUE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(MB->toolbar_button), TRUE);
		}

		MB->button_cb_id = g_signal_connect(MB->button, "toggled", G_CALLBACK(mode_button_toggled_cb), MB);
		MB->toolbar_button_cb_id = g_signal_connect(MB->toolbar_button, "toggled", G_CALLBACK(mode_toolbar_button_toggled_cb), MB);
	}

	mb->mode_toolbar_vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(mb->mode_toolbar_vbox), mb->mode_toolbar, FALSE, FALSE, 0);

	/* Pack an empty, wide hbox right below the toolbar and make it as wide
	   as the calculated width of the toolbar with some tuning. Toolbar icons
	   disappear if the container hbox is not wide enough. Without this hack
	   the width would be determined by the menu bar, and that could be short
	   if the user changes the menu layout. */
	pad_hbox = gtk_hbox_new(FALSE, 0);
	pad_vbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pad_hbox), pad_vbox, FALSE, FALSE, tb_width * 3 / 4);
	gtk_box_pack_start(GTK_BOX(mb->mode_toolbar_vbox), pad_hbox, FALSE, FALSE, 0);
}
