/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <gtk/gtk.h>

#include "bu_mode_btn.h"
#include "bu_mode_btn.data"

#include "crosshair.h"
#include "conf_core.h"
#include "tool.h"

#include "hid_gtk_conf.h"

#include "compat.h"

typedef struct {
	GtkWidget *button;
	GtkWidget *toolbar_button;
	guint button_cb_id;
	guint toolbar_button_cb_id;
	const gchar *name;
	gint mode;
	const gchar **xpm;
	const char *tooltip;
	pcb_gtk_mode_btn_t *mbb;
} ModeButton;


static ModeButton mode_buttons[] = {
	{NULL, NULL, 0, 0, "via", PCB_MODE_VIA, via, "place a via on the board"},
	{NULL, NULL, 0, 0, "line", PCB_MODE_LINE, line, "draw a line segment (trace) on the board"},
	{NULL, NULL, 0, 0, "arc", PCB_MODE_ARC, arc, "draw an arc segment (trace) on the board"},
	{NULL, NULL, 0, 0, "text", PCB_MODE_TEXT, text, "draw text on the board"},
	{NULL, NULL, 0, 0, "rectangle", PCB_MODE_RECTANGLE, rect, "draw a rectangular polygon on the board"},
	{NULL, NULL, 0, 0, "polygon", PCB_MODE_POLYGON, poly, "draw a polygon on the board"},
	{NULL, NULL, 0, 0, "polygonhole", PCB_MODE_POLYGON_HOLE, polyhole, "cut out a hole from existing polygons on the board"},
	{NULL, NULL, 0, 0, "buffer", PCB_MODE_PASTE_BUFFER, buf, "paste the current buffer on the board"},
	{NULL, NULL, 0, 0, "remove", PCB_MODE_REMOVE, del, "remove an object from the board"},
	{NULL, NULL, 0, 0, "rotate", PCB_MODE_ROTATE, rot, "rotate an object on the board"},
	{NULL, NULL, 0, 0, "insertPoint", PCB_MODE_INSERT_POINT, ins, "insert a point in a trace or polygon contour"},
	{NULL, NULL, 0, 0, "thermal", PCB_MODE_THERMAL, thrm, "change thermal connectivity of a pin or via"},
	{NULL, NULL, 0, 0, "arrow", PCB_MODE_ARROW, arr, "switch to arrow mode"},
	{NULL, NULL, 0, 0, "lock", PCB_MODE_LOCK, lock, "lock or unlock objects on the board"}
};

static gint n_mode_buttons = G_N_ELEMENTS(mode_buttons);

static void do_set_mode(pcb_gtk_mode_btn_t *mbb, int mode)
{
	pcb_tool_select_by_id(mode);
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
	gtk_widget_hide(mb->mode_buttons_frame);
	gtk_widget_show_all(mb->mode_toolbar);
}

void pcb_gtk_make_mode_buttons_and_toolbar(pcb_gtk_common_t *com, pcb_gtk_mode_btn_t *mb)
{
	GtkToolItem *tool_item;
	GtkWidget *vbox, *hbox = NULL;
	GtkWidget *pad_hbox, *pad_vbox;
	GtkWidget *image;
	GtkRequisition requisition;
	GdkPixbuf *pixbuf;
	GSList *group = NULL;
	GSList *toolbar_group = NULL;
	ModeButton *MB;
	int i, tb_width = 0;

	mb->com = com;
	mb->mode_toolbar = gtk_toolbar_new();

	mb->mode_buttons_frame = gtk_frame_new(NULL);
	vbox = gtkc_vbox_new(FALSE, 0);
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
		gtk_widget_set_tooltip_text(MB->toolbar_button, MB->tooltip);

		/* Pack mode-frame button into the frame */
		hbox = gtkc_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
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
		gtk_widget_get_requisition(image, &requisition);
		tb_width += requisition.width;

		if (strcmp(MB->name, "arrow") == 0) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(MB->button), TRUE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(MB->toolbar_button), TRUE);
		}

		MB->button_cb_id = g_signal_connect(MB->button, "toggled", G_CALLBACK(mode_button_toggled_cb), MB);
		MB->toolbar_button_cb_id = g_signal_connect(MB->toolbar_button, "toggled", G_CALLBACK(mode_toolbar_button_toggled_cb), MB);
	}

	mb->mode_toolbar_vbox = gtkc_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(mb->mode_toolbar_vbox), mb->mode_toolbar, FALSE, FALSE, 0);

	/* Pack an empty, wide hbox right below the toolbar and make it as wide
	   as the calculated width of the toolbar with some tuning. Toolbar icons
	   disappear if the container hbox is not wide enough. Without this hack
	   the width would be determined by the menu bar, and that could be short
	   if the user changes the menu layout. */
	pad_hbox = gtkc_hbox_new(FALSE, 0);
	pad_vbox = gtkc_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(pad_hbox), pad_vbox, FALSE, FALSE, tb_width * 3 / 4);
	gtk_box_pack_start(GTK_BOX(mb->mode_toolbar_vbox), pad_hbox, FALSE, FALSE, 0);
}
