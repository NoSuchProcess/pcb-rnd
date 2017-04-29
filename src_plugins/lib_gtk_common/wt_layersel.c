/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *  Copyright (C) 2017 Alain Vigne
 *
 *  This module is subject to the GNU GPL as described below.
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

#include "config.h"

#include <gtk/gtk.h>

#include "layer.h"
#include "layer_grp.h"
#include "layer_vis.h"
#include "board.h"
#include "data.h"
#include "conf_core.h"
#include "hid_actions.h"

#include "wt_layersel.h"
#include "compat.h"

/*** Layer visibility widget rendering ***/
#define set_pixel(dst, r, g, b, a) \
do { \
	p[0] = r; \
	p[1] = g; \
	p[2] = b; \
	p[3] = a; \
} while(0)

static guint hex2bin_(char c)
{
	if ((c >= '0') && (c <= '9')) return c - '0';
	if ((c >= 'a') && (c <= 'z')) return c - 'a' + 10;
	if ((c >= 'A') && (c <= 'Z')) return c - 'A' + 10;
	return 0; /* syntax error... */
}

#define hex2bin(str) ((hex2bin_((str)[0])) << 4 | hex2bin_((str)[1]))

/* draw a visibility box: filled or partially filled with layer color */
static GtkWidget *layer_vis_box(int filled, const char *rgb)
{
	GdkPixbuf *pixbuf;
	GtkWidget *image;
	gint width, height, max_height;
	guchar *pixels;
	guint r, g, b;
	guint w;
	guchar *p;

	pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 16, 16);
	width = gdk_pixbuf_get_width(pixbuf);	/* 16 here, obviously */
	max_height = height = gdk_pixbuf_get_height(pixbuf);
	pixels = gdk_pixbuf_get_pixels(pixbuf);

	/* Fill the whole rectangle with color */
	r = hex2bin(rgb+1);
	g = hex2bin(rgb+3);
	b = hex2bin(rgb+5);

	while (height--) {
		w = width;
		p = pixels;
		while (w--) {
			if ((height == 0) || (height == max_height-1) || (w == 0) || (w == width-1))
				set_pixel(p, 0, 0, 0, 0xff); /* frame */
			else if ((width-w+5 < height) || (filled))
				set_pixel(p, r, g, b, 0xff); /* layer color fill (full or up-left triangle) */
			else
				set_pixel(p, 0xff, 0xff, 0xff, 0x0); /* the unfilled part when triangle should be transparent */
			p += 4;
		}
		pixels += gdk_pixbuf_get_rowstride(pixbuf);
	}

	image = gtk_image_new_from_pixbuf(pixbuf);
	g_object_unref(pixbuf);

	return image;
}

/*** Sync: make the GUI look like internal states ***/

static void layersel_lyr_vis_sync(pcb_gtk_ls_lyr_t *lsl)
{
	pcb_layer_t *l = pcb_get_layer(lsl->lid);
	if (l != NULL) {
		if (l->On) {
			gtk_widget_show(lsl->vis_on);
			gtk_widget_hide(lsl->vis_off);
		}
		else {
			gtk_widget_show(lsl->vis_off);
			gtk_widget_hide(lsl->vis_on);
		}
	}
	if ((lsl->lid != -1) && (lsl->lid == pcb_layer_id(PCB->Data, LAYER_ON_STACK(0))))
		pcb_gtk_set_selected(lsl->name_box, 1);
	else
		pcb_gtk_set_selected(lsl->name_box, 0);
}

static void group_vis_sync(pcb_gtk_ls_grp_t *lsg)
{
	if (lsg->grp->open) {
		int n;
		pcb_gtk_widget_hide_all(lsg->grp_closed);
		gtk_widget_set_no_show_all(lsg->grp_open, 0);
		gtk_widget_show_all(lsg->grp_open);
		gtk_widget_set_no_show_all(lsg->grp_open, 1);
		for(n = 0; n < lsg->grp->len; n++)
			layersel_lyr_vis_sync(&lsg->layer[n]);
	}
	else {
		gtk_widget_set_no_show_all(lsg->grp_closed, 0);
		gtk_widget_show_all(lsg->grp_closed);
		gtk_widget_set_no_show_all(lsg->grp_closed, 1);
		pcb_gtk_widget_hide_all(lsg->grp_open);

		/* a closed group has visibility boxes displayed, pick one to show */
		if (lsg->vis_on != NULL) {
			if (lsg->grp->vis) {
				gtk_widget_show(lsg->vis_on);
				gtk_widget_hide(lsg->vis_off);
			}
			else {
				gtk_widget_show(lsg->vis_off);
				gtk_widget_hide(lsg->vis_on);
			}
		}
	}
}


/*** Event handling ***/

static int ev_lyr_no_select(pcb_gtk_ls_lyr_t *lsl)
{
	return 1; /* layer can not be selected ever */
}

static gboolean group_vis_press_cb(GtkWidget *widget, GdkEvent *event, pcb_gtk_ls_grp_t *lsg)
{
	switch(event->button.button) {
		case 1:
			lsg->grp->vis = !lsg->grp->vis;
			group_vis_sync(lsg);
			break;
		case 3:
			pcb_hid_actionl("Popup", "group", NULL);
			break;
	}
	return TRUE;
}

static gboolean layer_vis_press_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	pcb_gtk_ls_lyr_t *lsl = user_data;
	pcb_gtk_layersel_t *ls = lsl->lsg->ls;

	switch(event->button.button) {
		case 1:
		case 3:
			if ((lsl->ev_toggle_vis == NULL) || (lsl->ev_toggle_vis(lsl) == 0)) {
				pcb_layer_t *l = pcb_get_layer(lsl->lid);
				if (l != NULL) {
					int n, want_on = !l->On;
					pcb_layervis_change_group_vis(lsl->lid, want_on, 0);
					for(n = 0; n < lsl->lsg->grp->len; n++)
						layersel_lyr_vis_sync(&lsl->lsg->layer[n]);
				}
			}
			ls->com->invalidate_all();
			if (event->button.button == 3)
				pcb_hid_actionl("Popup", "layer", NULL);
			break;
	}
	return TRUE;
}

static gboolean layer_select_press_cb(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	pcb_gtk_ls_lyr_t *lsl = user_data;
	pcb_layer_id_t old_curr;
	switch(event->button.button) {
		case 1:
		case 3:
			if ((lsl->ev_selected == NULL) || (lsl->ev_selected(lsl) == 0)) {
				old_curr = pcb_layer_id(PCB->Data, LAYER_ON_STACK(0));
				pcb_layervis_change_group_vis(lsl->lid, 1, 1);
				if (old_curr >= 0) { /* need to find old layer by lid */
					int gi, li;
					pcb_gtk_layersel_t *ls = lsl->lsg->ls;

					for(gi = 0; gi < pcb_max_group(PCB); gi++)
						if (ls->grp[gi].grp != NULL)
							for(li = 0; li < ls->grp[gi].grp->len; li++)
								if (ls->grp[gi].layer[li].lid == old_curr)
									layersel_lyr_vis_sync(&ls->grp[gi].layer[li]);
					ls->com->invalidate_all();
				}
				layersel_lyr_vis_sync(lsl);
			}
			if (event->button.button == 3)
				pcb_hid_actionl("Popup", "layer", NULL);
			break;
	}
	return TRUE;
}

static gboolean group_any_press_cb(GtkWidget *widget, GdkEvent *event, pcb_gtk_ls_grp_t *lsg, int openval)
{
	switch(event->button.button) {
		case 1:
			if (lsg->grp->len != 0) /* don't let empty group open, that'd make the open group row unusable */
				lsg->grp->open = openval;
			else
				lsg->grp->open = 0;
			group_vis_sync(lsg);
			break;
		case 3:
			pcb_hid_actionl("Popup", "group", NULL);
			break;
	}
	return TRUE;
}

static gboolean group_close_press_cb(GtkWidget *widget, GdkEvent *event, pcb_gtk_ls_grp_t *lsg)
{
	return group_any_press_cb(widget, event, lsg, 0);
}

static gboolean group_open_press_cb(GtkWidget *widget, GdkEvent *event, pcb_gtk_ls_grp_t *lsg)
{
	return group_any_press_cb(widget, event, lsg, 1);
}

/** Wrap w so that clicks on it are triggering a callback */
static GtkWidget *wrap_bind_click(GtkWidget *w, GCallback cb, void *cb_data)
{
	GtkWidget *event_box = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(event_box), w);
	g_signal_connect(event_box, "button-press-event", G_CALLBACK(cb), cb_data);

	return event_box;
}

/*** Row builder ***/

#define hardwired_colors(typ) \
do { \
	unsigned long __typ__ = (typ); \
	if (__typ__ & PCB_LYT_SILK)   return conf_core.appearance.color.element; \
	if (__typ__ & PCB_LYT_MASK)   return conf_core.appearance.color.mask; \
	if (__typ__ & PCB_LYT_PASTE)  return conf_core.appearance.color.paste; \
} while (0)

static const char *lyr_color(pcb_layer_id_t lid)
{
	if (lid < 0) return "#aaaa00";
	hardwired_colors(pcb_layer_flags(PCB, lid));
	return conf_core.appearance.color.layer[lid];
}

static const char *grp_color(pcb_layer_group_t *g)
{
	hardwired_colors(g->type);
	/* normal mechanism: first layer's color or yellow */
	if (g->len == 0) return "#ffff00";
	return lyr_color(g->lid[0]);
}

/* Create a hbox with on/off visibility boxes packed in, pointers returned in *on, *off */
static GtkWidget *build_visbox(const char *color, GtkWidget **on, GtkWidget **off)
{
	GtkWidget *vis_box = gtkc_hbox_new(0, 0);
	*on = layer_vis_box(1, color);
	gtk_box_pack_start(GTK_BOX(vis_box), *on, FALSE, FALSE, 0);
	*off = layer_vis_box(0, color);
	gtk_box_pack_start(GTK_BOX(vis_box), *off, FALSE, FALSE, 0);
	return vis_box;
}

/* Create a hbox of a layer within an expanded group */
static GtkWidget *build_layer(pcb_gtk_ls_grp_t *lsg, pcb_gtk_ls_lyr_t *lsl, const char *name, pcb_layer_id_t lid, char * const *force_color)
{
	GtkWidget *vis_box, *vis_ebox, *ly_name_bx, *lab;
	const char *color;

	lsl->lsg = lsg;
	lsl->force_color = force_color;
	lsl->box = gtkc_hbox_new(0, 0);

	if (force_color == NULL)
		color = lyr_color(lid);
	else
		color = *force_color;

	/* sensitive layer visibility widgets */
	vis_box = build_visbox(color, &lsl->vis_on, &lsl->vis_off);
	vis_ebox = wrap_bind_click(vis_box, G_CALLBACK(layer_vis_press_cb), lsl);
	gtk_box_pack_start(GTK_BOX(lsl->box), vis_ebox, FALSE, FALSE, 0);

	/* selectable layer name*/
	ly_name_bx = gtkc_hbox_new(0, 0);
	lsl->name_box = wrap_bind_click(ly_name_bx, G_CALLBACK(layer_select_press_cb), lsl);
	gtkc_widget_selectable(lsl->name_box, "layersel");
	lab = gtk_label_new(name);
	gtk_box_pack_start(GTK_BOX(ly_name_bx), lab, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(lsl->box), lsl->name_box, TRUE, TRUE, 10);
	gtk_misc_set_alignment(GTK_MISC(lab), 0, 0.5);

	layersel_lyr_vis_sync(lsl);

	return lsl->box;
}

/* Creating a group enrty (both open and closed state); after layers are added, finish() needs to be called */
static GtkWidget *build_group_start(pcb_gtk_layersel_t *ls, pcb_gtk_ls_grp_t *lsg, const char *gname, int has_group_vis, pcb_layer_group_t *grp)
{
	GtkWidget *gn_vert, *vlabel, *opn, *cld;

/* Layout:
  +--------------------------+<- grp_row
  | grp_closed               |
  +--------------------------+
  |+-----------------------+<--- grp_open
  || gn | layer1           | |
  || _v | layer2           | |
  || er | layer3           | |
  || t  | layer4           | |
  |+-----------------------+ |
  +--------------------------+
*/
	lsg->ls = ls;
	lsg->grp = grp;
	lsg->grp_row = gtkc_vbox_new(0, 0);
	lsg->grp_closed = gtkc_hbox_new(0, 0);
	lsg->grp_open = gtkc_hbox_new(0, 0);
	gtk_box_pack_start(GTK_BOX(lsg->grp_row), lsg->grp_closed, FALSE, FALSE, 1);
	gtk_box_pack_start(GTK_BOX(lsg->grp_row), lsg->grp_open, FALSE, FALSE, 5);

	gn_vert = gtkc_vbox_new(0, 0);
	lsg->layers = gtkc_vbox_new(0, 0);
	opn = wrap_bind_click(gn_vert, G_CALLBACK(group_close_press_cb), lsg);
	gtk_box_pack_start(GTK_BOX(lsg->grp_open), opn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(lsg->grp_open), lsg->layers, TRUE, TRUE, 0);

	/* install group name - vertical (for when the group is open) */
	vlabel = gtk_label_new(gname);
	gtk_label_set_angle(GTK_LABEL(vlabel), 90);
	gtk_box_pack_start(GTK_BOX(gn_vert), vlabel, TRUE, TRUE, 0);
	gtk_misc_set_alignment(GTK_MISC(vlabel), 0, 1);
	gtk_widget_set_size_request(vlabel, 32, 1);

	/* install group name - vertical (for when the group is closed) */
	if (has_group_vis) {
		GtkWidget *vis;
		vis = wrap_bind_click(build_visbox(grp_color(grp), &lsg->vis_on, &lsg->vis_off), G_CALLBACK(group_vis_press_cb), lsg);
		gtk_box_pack_start(GTK_BOX(lsg->grp_closed), vis, FALSE, FALSE, 0);
	}
	cld = wrap_bind_click(gtk_label_new(gname), G_CALLBACK(group_open_press_cb), lsg);
	gtk_box_pack_start(GTK_BOX(lsg->grp_closed), cld, FALSE, FALSE, 2);

	return lsg->grp_row;
}

/* finish creating a group - call after layers are added */
static void build_group_finish(pcb_gtk_ls_grp_t *lsg)
{
	/* Make sure gtk_show_all() won't mess with our hardwired hide/show */
	group_vis_sync(lsg);
	gtk_widget_set_no_show_all(lsg->grp_closed, 1);
	gtk_widget_set_no_show_all(lsg->grp_open, 1);
}

/* Create a group that has a real layer group in core, add all layers */
static GtkWidget *build_group_real(pcb_gtk_layersel_t *ls, pcb_gtk_ls_grp_t *lsg, pcb_layer_group_t *grp)
{
	int n;
	GtkWidget *wg = build_group_start(ls, lsg, grp->name, 1, grp);

	lsg->layer = calloc(sizeof(pcb_gtk_ls_lyr_t), grp->len);

	/* install layers */
	for(n = 0; n < grp->len; n++) {
		pcb_layer_t *l = pcb_get_layer(grp->lid[n]);
		if (l != NULL) {
			GtkWidget *wl = build_layer(lsg, &lsg->layer[n], l->Name, grp->lid[n], NULL);
			gtk_box_pack_start(GTK_BOX(lsg->layers), wl, TRUE, TRUE, 1);
			lsg->layer[n].lid = grp->lid[n];
		}
	}

	build_group_finish(lsg);

	return wg;
}


/*** Layer selector widget building function ***/
static void layersel_populate(pcb_gtk_layersel_t *ls)
{
	GtkWidget *spring;
	pcb_layergrp_id_t gid;

	for(gid = 0; gid < pcb_max_group(PCB); gid++) {
		pcb_layer_group_t *g = &PCB->LayerGroups.grp[gid];
		if (g->type & PCB_LYT_SUBSTRATE)
			continue;
		gtk_box_pack_start(GTK_BOX(ls->grp_box), build_group_real(ls, &ls->grp[gid], g), FALSE, FALSE, 0);
	}

	gtk_box_pack_start(GTK_BOX(ls->grp_box), gtk_hseparator_new(), FALSE, FALSE, 0);

	{ /* build hardwired virtual layers */
		pcb_gtk_ls_grp_t *lsg = &ls->lsg_virt;
		int n;

		ls->grp_virt.len = 5;
		lsg->layer = calloc(sizeof(pcb_gtk_ls_lyr_t), ls->grp_virt.len);
		gtk_box_pack_start(GTK_BOX(ls->grp_box), build_group_start(ls, lsg, "Virtual", 0, &ls->grp_virt), FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(lsg->layers), build_layer(lsg, &lsg->layer[0], "Pins/Pads", -1, &conf_core.appearance.color.pin), FALSE, FALSE, 1);
		gtk_box_pack_start(GTK_BOX(lsg->layers), build_layer(lsg, &lsg->layer[1], "Vias", -1, &conf_core.appearance.color.via), FALSE, FALSE, 1);
		gtk_box_pack_start(GTK_BOX(lsg->layers), build_layer(lsg, &lsg->layer[2], "Far side", -1, &conf_core.appearance.color.invisible_objects), FALSE, FALSE, 1);
		gtk_box_pack_start(GTK_BOX(lsg->layers), build_layer(lsg, &lsg->layer[3], "Rats", -1, &conf_core.appearance.color.rat), FALSE, FALSE, 1);
		gtk_box_pack_start(GTK_BOX(lsg->layers), build_layer(lsg, &lsg->layer[4], "All-silk", -1, &conf_core.appearance.color.element), FALSE, FALSE, 1);

		for(n = 0; n < ls->grp_virt.len; n++) {
			lsg->layer[n].ev_selected = ev_lyr_no_select;
			lsg->layer[n].lid = ls->grp_virt.lid[n] = -1;
		}

		build_group_finish(lsg);
	}

	spring = gtkc_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(ls->grp_box), spring, TRUE, TRUE, 0);
}

GtkWidget *pcb_gtk_layersel_build(pcb_gtk_common_t *com, pcb_gtk_layersel_t *ls)
{
	GtkWidget *scrolled;

	ls->grp_box = gtkc_vbox_new(FALSE, 0);
	ls->grp_virt.open = 1;
	ls->com = com;
	layersel_populate(ls);

	/* get the whole box vertically scrolled, if needed */
	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), ls->grp_box);

	return scrolled;
}
