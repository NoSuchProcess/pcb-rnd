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
#include "layer_ui.h"
#include "layer_vis.h"
#include "board.h"
#include "data.h"
#include "conf_core.h"
#include "hid_actions.h"
#include "math_helper.h"

#include "wt_layersel.h"
#include "compat.h"

extern pcb_layergrp_id_t pcb_actd_EditGroup_gid;

/*** Layer visibility widget rendering ***/

#define set_pixel(dst, r, g, b, a) \
	do { p[0] = r; p[1] = g; p[2] = b; p[3] = a; } while(0)

static guint hex2bin_(char c)
{
	if ((c >= '0') && (c <= '9')) return c - '0';
	if ((c >= 'a') && (c <= 'z')) return c - 'a' + 10;
	if ((c >= 'A') && (c <= 'Z')) return c - 'A' + 10;
	return 0; /* syntax error... */
}

#define hex2bin(str) ((hex2bin_((str)[0])) << 4 | hex2bin_((str)[1]))

/* draw a visibility box: filled or partially filled with layer color */
static GtkWidget *layer_vis_box(int filled, const char *rgb, int brd)
{
	GdkPixbuf *pixbuf;
	GtkWidget *image;
	gint width, height, max_height, hatch=0;
	guchar *pixels, *p;
	guint w, r, g, b;

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
			if ((height < brd) || (height >= max_height-brd) || (w < brd) || (w >= width-brd))
				set_pixel(p, 0, 0, 0, 0xff); /* frame */
			else if ((hatch) && (((w - height) % 4) == 0))
				set_pixel(p, 0x0, 0x0, 0x0, 0x0);
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
	int is_on = 0;

	if (lsl->ev_vis == NULL) {
		pcb_layer_t *l = pcb_get_layer(lsl->lid);
		if (l != NULL)
			is_on = l->On;
	}
	else
		lsl->ev_vis(lsl, 0, &is_on);

	if (is_on) {
		gtk_widget_show(lsl->vis_on);
		gtk_widget_hide(lsl->vis_off);
	}
	else {
		gtk_widget_show(lsl->vis_off);
		gtk_widget_hide(lsl->vis_on);
	}

	if ((lsl->lid < 0) ?
		((lsl->ev_selected != NULL) && (lsl->ev_selected(lsl, 0) == 1)) :
		((!lsl->lsg->ls->no_copper_sel) && (lsl->lid == pcb_layer_id(PCB->Data, LAYER_ON_STACK(0)))))
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
		for(n = 0; n < MAX(lsg->grp->len, 1); n++)
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

/* Toggle a virtual layer: flip the bool */
static int vis_virt(pcb_gtk_ls_lyr_t *lsl, int toggle, int *is_on)
{
	pcb_bool *b = (pcb_bool *)((char *)PCB + lsl->virt_data);
	if (toggle)
		*b = !*b;
	*is_on = *b;
	return 0;
}

static int vis_ui(pcb_gtk_ls_lyr_t *lsl, int toggle, int *is_on)
{
	pcb_layer_t *l = &pcb_uilayer.array[lsl->virt_data];
	if (toggle)
		l->On = !l->On;
	*is_on = l->On;
	return 0;
}

/* Toggle a implicit layer: toggle the parent group's */
static int vis_impl(pcb_gtk_ls_lyr_t *lsl, int toggle, int *is_on)
{
	if (toggle)
		lsl->lsg->grp->vis = !lsl->lsg->grp->vis;
	*is_on = lsl->lsg->grp->vis;
	return 0;
}

static int ev_lyr_no_select(pcb_gtk_ls_lyr_t *lsl, int do_select)
{
	return -1; /* layer can not be selected ever */
}

static int ev_lyr_select_rat(pcb_gtk_ls_lyr_t *lsl, int do_select)
{
	if (do_select)
		lsl->lsg->ls->no_copper_sel = PCB->RatOn = PCB->RatDraw = 1;
	return PCB->RatDraw;
}

static gboolean group_vis_press_cb(GtkWidget *widget, GdkEvent *event, pcb_gtk_ls_grp_t *lsg)
{
	pcb_gtk_layersel_t *ls = lsg->ls;
	int n;

	ls->running = 1;
	switch(event->button.button) {
		case 1:
			lsg->grp->vis = !lsg->grp->vis;
			if (lsg->grp->len > 0)
				pcb_layervis_change_group_vis(lsg->layer[0].lid, lsg->grp->vis, 0);
			for(n = 0; n < lsg->grp->len; n++)
				layersel_lyr_vis_sync(&lsg->layer[n]);
			group_vis_sync(lsg);
			ls->com->invalidate_all();
			break;
		case 3:
			if (lsg->grp != &lsg->ls->grp_virt) {
				pcb_actd_EditGroup_gid = pcb_layergrp_id(PCB, lsg->grp);
				pcb_hid_actionl("Popup", "group", NULL);
			}
			break;
	}
	ls->running = 0;
	return TRUE;
}

static void layer_popup(pcb_gtk_layersel_t *ls, int button, pcb_layer_id_t lid, pcb_layer_group_t *g)
{
	if (button != 3)
		return;
	if (lid < 0) {
		if ((g != NULL) && (g != &ls->grp_virt)) {
			pcb_actd_EditGroup_gid = pcb_layergrp_id(PCB, g);
			pcb_hid_actionl("Popup", "group", NULL);
		}
	}
	else
		pcb_hid_actionl("Popup", "layer", NULL);
}

static gboolean layer_vis_press_cb(GtkWidget *widget, GdkEvent *event, pcb_gtk_ls_lyr_t *lsl)
{
	pcb_gtk_layersel_t *ls = lsl->lsg->ls;
	int n, is_on, normal = 1;

	if (event->type != GDK_BUTTON_PRESS) /* ignore excess events on double or triple buttons */
		return TRUE;

	ls->running = 1;
	switch(event->button.button) {
		case 1:
		case 3:
			if (lsl->ev_vis != NULL) {
				if (lsl->ev_vis(lsl, 1, &is_on) == 0) {
					normal = 0;
					layersel_lyr_vis_sync(lsl);
				}
			}
			else {
				pcb_layer_t *l = pcb_get_layer(lsl->lid);
				if (l != NULL)
					is_on = !l->On;
				else
					normal = 0;
			}

			if (normal) {
				pcb_layervis_change_group_vis(lsl->lid, is_on, 0);
				for(n = 0; n < lsl->lsg->grp->len; n++)
					layersel_lyr_vis_sync(&lsl->lsg->layer[n]);
			}

			ls->com->invalidate_all();
			layer_popup(ls, event->button.button, lsl->lid, lsl->lsg->grp);
			break;
	}
	ls->running = 0;
	return TRUE;
}

static gboolean layer_select_press_cb(GtkWidget *widget, GdkEvent *event, pcb_gtk_ls_lyr_t *lsl)
{
	pcb_gtk_layersel_t *ls = lsl->lsg->ls;

	ls->running = 1;
	switch(event->button.button) {
		case 1:
			ls->no_copper_sel = PCB->RatDraw = 0;
		case 3:
			if ((lsl->ev_selected == NULL) || (lsl->ev_selected(lsl, 1) >= 0)) {
				pcb_layervis_change_group_vis(lsl->lid, 1, 1);
				ls->com->invalidate_all();
				pcb_gtk_layersel_vis_update(ls); /* need to do a full redraw because of side effects of special layer selections (e.g. Rats') */
			}
			layer_popup(ls, event->button.button, lsl->lid, lsl->lsg->grp);
			break;
	}
	ls->running = 0;
	return TRUE;
}

static gboolean group_any_press_cb(GtkWidget *widget, GdkEvent *event, pcb_gtk_ls_grp_t *lsg, int openval)
{
	switch(event->button.button) {
		case 1:
			lsg->grp->open = openval;
			group_vis_sync(lsg);
			break;
		case 3:
			if (lsg->grp != &lsg->ls->grp_virt) {
				pcb_actd_EditGroup_gid = pcb_layergrp_id(PCB, lsg->grp);
				pcb_hid_actionl("Popup", "group", NULL);
			}
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

static const char * const grp_color(pcb_layer_group_t *g)
{
	hardwired_colors(g->type);
	/* normal mechanism: first layer's color or yellow */
	if (g->len == 0) return "#ffff00";
	return lyr_color(g->lid[0]);
}

/* Create a hbox with on/off visibility boxes packed in, pointers returned in *on, *off */
static GtkWidget *build_visbox(const char *color, GtkWidget **on, GtkWidget **off, int brd)
{
	GtkWidget *vis_box = gtkc_hbox_new(0, 0);
	*on = layer_vis_box(1, color, brd);
	gtk_box_pack_start(GTK_BOX(vis_box), *on, FALSE, FALSE, 0);
	*off = layer_vis_box(0, color, brd);
	gtk_box_pack_start(GTK_BOX(vis_box), *off, FALSE, FALSE, 0);
	return vis_box;
}

/* Create a hbox of a layer within an expanded group */
static GtkWidget *build_layer(pcb_gtk_ls_grp_t *lsg, pcb_gtk_ls_lyr_t *lsl, const char *name, pcb_layer_id_t lid, const char * const *force_color)
{
	GtkWidget *vis_box, *vis_ebox, *ly_name_bx, *lab;
	pcb_layer_t *ly;
	const char *color;

	lsl->lsg = lsg;
	lsl->force_color = force_color;
	lsl->box = gtkc_hbox_new(0, 0);

	if (force_color == NULL)
		color = lyr_color(lid);
	else
		color = *force_color;

	ly = pcb_get_layer(lid);

	/* sensitive layer visibility widgets */
	vis_box = build_visbox(color, &lsl->vis_on, &lsl->vis_off, (((ly != NULL) && (ly->comb & PCB_LYC_SUB)) ? 2 : 1));
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
	gtk_box_pack_start(GTK_BOX(lsg->grp_open), gtk_vseparator_new(), FALSE, FALSE, 4);
	gtk_box_pack_start(GTK_BOX(lsg->grp_open), lsg->layers, TRUE, TRUE, 0);

	/* install group name - vertical (for when the group is open) */
	vlabel = gtk_label_new(gname);
	gtk_label_set_angle(GTK_LABEL(vlabel), 90);
	gtk_box_pack_start(GTK_BOX(gn_vert), vlabel, TRUE, TRUE, 0);
	gtk_misc_set_alignment(GTK_MISC(vlabel), 0, 1);
	gtk_widget_set_size_request(vlabel, 16, 1);
	gtk_widget_set_tooltip_text(gn_vert, gname);


	/* install group name - horizontal (for when the group is closed) */
	if (has_group_vis) {
		GtkWidget *vis;
		vis = wrap_bind_click(build_visbox(grp_color(grp), &lsg->vis_on, &lsg->vis_off, 1), G_CALLBACK(group_vis_press_cb), lsg);
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

	lsg->layer = calloc(sizeof(pcb_gtk_ls_lyr_t), (grp->len == 0 ? 1 : grp->len));

	/* install layers */
	if (grp->len == 0) {
		const char * const clr = grp_color(grp);
		char *name = pcb_strdup_printf("<%s>", lsg->grp->name);
		GtkWidget *wl = build_layer(lsg, &lsg->layer[0], name, -1, &clr);
		gtk_box_pack_start(GTK_BOX(lsg->layers), wl, TRUE, TRUE, 1);
		lsg->layer[0].lid = -1;
		lsg->layer[0].ev_selected = ev_lyr_no_select;
		lsg->layer[0].ev_vis = vis_impl;
		free(name);
	}
	else {
		for(n = 0; n < grp->len; n++) {
			pcb_layer_t *l = pcb_get_layer(grp->lid[n]);
			if (l != NULL) {
				GtkWidget *wl = build_layer(lsg, &lsg->layer[n], l->Name, grp->lid[n], NULL);
				gtk_box_pack_start(GTK_BOX(lsg->layers), wl, TRUE, TRUE, 1);
				lsg->layer[n].lid = grp->lid[n];
			}
		}
	}

	build_group_finish(lsg);

	return wg;
}

/*** Layer selector widget building function ***/
typedef struct {
	const char *name;
	char * const*force_color;
	int (*ev_vis)(pcb_gtk_ls_lyr_t *lsl, int toggle, int *is_on);
	int (*ev_selected)(pcb_gtk_ls_lyr_t *lsl, int do_select);
	int virt_data;
} virt_layers_t;

static const virt_layers_t virts[] = {
	{ "Pins/Pads",  &conf_core.appearance.color.pin,               vis_virt, ev_lyr_no_select,  offsetof(pcb_board_t, PinOn) },
	{ "Vias",       &conf_core.appearance.color.via,               vis_virt, ev_lyr_no_select,  offsetof(pcb_board_t, ViaOn) },
	{ "Far side",   &conf_core.appearance.color.invisible_objects, vis_virt, ev_lyr_no_select,  offsetof(pcb_board_t, InvisibleObjectsOn) },
	{ "Rats",       &conf_core.appearance.color.rat,               vis_virt, ev_lyr_select_rat, offsetof(pcb_board_t, RatOn) },
};

static void layersel_populate(pcb_gtk_layersel_t *ls)
{
	GtkWidget *spring;
	pcb_layergrp_id_t gid;
		int n;

	for(gid = 0; gid < pcb_max_group(PCB); gid++) {
		pcb_layer_group_t *g = &PCB->LayerGroups.grp[gid];
		if (g->type & PCB_LYT_SUBSTRATE)
			continue;
		gtk_box_pack_start(GTK_BOX(ls->grp_box), build_group_real(ls, &ls->grp[gid], g), FALSE, FALSE, 0);
	}

	gtk_box_pack_start(GTK_BOX(ls->grp_box), gtk_hseparator_new(), FALSE, FALSE, 0);

	{ /* build hardwired virtual layers */
		pcb_gtk_ls_grp_t *lsg = &ls->lsg_virt;

		ls->grp_virt.len = sizeof(virts) / sizeof(virts[0]);
		lsg->layer = calloc(sizeof(pcb_gtk_ls_lyr_t), ls->grp_virt.len);
		gtk_box_pack_start(GTK_BOX(ls->grp_box), build_group_start(ls, lsg, "Virtual", 0, &ls->grp_virt), FALSE, FALSE, 0);

		for(n = 0; n < ls->grp_virt.len; n++) {
			gtk_box_pack_start(GTK_BOX(lsg->layers), build_layer(lsg, &lsg->layer[n], virts[n].name, -1, (const char *const *)virts[n].force_color), FALSE, FALSE, 1);
			lsg->layer[n].ev_selected = virts[n].ev_selected;
			lsg->layer[n].ev_vis = virts[n].ev_vis;
			lsg->layer[n].virt_data = virts[n].virt_data;
			lsg->layer[n].lid = ls->grp_virt.lid[n] = -1;
		}

		build_group_finish(lsg);
	}

	if (vtlayer_len(&pcb_uilayer) > 0) { /* UI layers */
		pcb_gtk_ls_grp_t *lsg = &ls->lsg_ui;

		ls->grp_ui.len = vtlayer_len(&pcb_uilayer);
		lsg->layer = calloc(sizeof(pcb_gtk_ls_lyr_t), ls->grp_ui.len);
		gtk_box_pack_start(GTK_BOX(ls->grp_box), build_group_start(ls, lsg, "UI", 0, &ls->grp_ui), FALSE, FALSE, 0);

		for(n = 0; n < vtlayer_len(&pcb_uilayer); n++) {
			gtk_box_pack_start(GTK_BOX(lsg->layers), build_layer(lsg, &lsg->layer[n], pcb_uilayer.array[n].Name, -1, &pcb_uilayer.array[n].Color), FALSE, FALSE, 1);
			lsg->layer[n].ev_selected = ev_lyr_no_select;
			lsg->layer[n].ev_vis = vis_ui;
			lsg->layer[n].virt_data = n;
			lsg->layer[n].lid = -1;
		}
		build_group_finish(lsg);
	}


	spring = gtkc_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(ls->grp_box), spring, TRUE, TRUE, 0);
}

GtkWidget *pcb_gtk_layersel_build(pcb_gtk_common_t *com, pcb_gtk_layersel_t *ls)
{
	GtkWidget *scrolled;

	ls->grp_box_outer = gtkc_vbox_new(FALSE, 0);
	ls->grp_virt.open = 1;
	ls->grp_ui.open = 1;
	ls->com = com;

	ls->grp_box = gtkc_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(ls->grp_box_outer), ls->grp_box, FALSE, FALSE, 0);
	layersel_populate(ls);

	/* get the whole box vertically scrolled, if needed */
	scrolled = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled), ls->grp_box_outer);

	return scrolled;
}

static void layersel_destroy(pcb_gtk_common_t *com, pcb_gtk_layersel_t *ls)
{
	pcb_layergrp_id_t gid;

	for(gid = 0; gid < pcb_max_group(PCB); gid++)
		free(ls->grp[gid].layer);

	free(ls->lsg_virt.layer);

	memset(&ls->grp, 0, sizeof(ls->grp));
	memset(&ls->lsg_virt, 0, sizeof(ls->lsg_virt));

	gtk_widget_destroy(ls->grp_box);
	ls->grp_box = gtkc_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(ls->grp_box_outer), ls->grp_box, FALSE, FALSE, 0);
}

void pcb_gtk_layersel_update(pcb_gtk_common_t *com, pcb_gtk_layersel_t *ls)
{
	layersel_destroy(com, ls);
	layersel_populate(ls);
	gtk_widget_show_all(ls->grp_box);
}

void pcb_gtk_layersel_vis_update(pcb_gtk_layersel_t *ls)
{
	pcb_layergrp_id_t gid;

	ls->no_copper_sel = PCB->RatDraw; /* someone may have turned RatDraw on - turn off selecting copper */

	for(gid = 0; gid < pcb_max_group(PCB); gid++)
		if (ls->grp[gid].grp != NULL)
			group_vis_sync(&ls->grp[gid]);

	group_vis_sync(&ls->lsg_virt);
}

