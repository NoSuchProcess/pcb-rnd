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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"
#include "ui_zoompan.h"

#include "unit.h"
#include "actions.h"
#include "error.h"
#include "conf_core.h"
#include "board.h"
#include "compat_misc.h"
#include "draw.h"
#include "data.h"
#include "layer_vis.h"

#include "../src_plugins/lib_hid_pcbui/util.h"

double pcb_gtk_clamp_zoom(const pcb_gtk_view_t *vw, double coord_per_px)
{
	double min_zoom, max_zoom, max_zoom_w, max_zoom_h, out_zoom;

	min_zoom = 200;

	/* max zoom is calculated so that zoom * canvas_size * 2 doesn't overflow pcb_coord_t */
	max_zoom_w = (double)COORD_MAX / (double)vw->canvas_width;
	max_zoom_h = (double)COORD_MAX / (double)vw->canvas_height;
	max_zoom = MIN(max_zoom_w, max_zoom_h) / 2.0;

	out_zoom = coord_per_px;
	if (out_zoom < min_zoom)
		out_zoom = min_zoom;
	if (out_zoom > max_zoom)
		out_zoom = max_zoom;

	return out_zoom;
}


static void pcb_gtk_pan_common(pcb_gtk_view_t *v)
{
	int event_x, event_y;

	/* We need to fix up the PCB coordinates corresponding to the last
	 * event so convert it back to event coordinates temporarily. */
	pcb_gtk_coords_pcb2event(v, v->pcb_x, v->pcb_y, &event_x, &event_y);

	/* Don't pan so far the board is completely off the screen */
	v->x0 = MAX(-v->width, v->x0);
	v->y0 = MAX(-v->height, v->y0);

	if (v->use_max_pcb) {
		v->x0 = MIN(v->x0, v->com->hidlib->size_x);
		v->y0 = MIN(v->y0, v->com->hidlib->size_y);
	}
	else {
		assert(v->max_width > 0);
		assert(v->max_height > 0);
		v->x0 = MIN(v->x0, v->max_width);
		v->y0 = MIN(v->y0, v->max_height);
	}

	/* Fix up noted event coordinates to match where we clamped. Alternatively
	 * we could call ghid_note_event_location (NULL); to get a new pointer
	 * location, but this costs us an xserver round-trip (on X11 platforms)
	 */
	pcb_gtk_coords_event2pcb(v, event_x, event_y, &v->pcb_x, &v->pcb_y);

	if (v->com->pan_common != NULL)
		v->com->pan_common();
}

pcb_bool pcb_gtk_coords_pcb2event(const pcb_gtk_view_t *v, pcb_coord_t pcb_x, pcb_coord_t pcb_y, int *event_x, int *event_y)
{
	*event_x = DRAW_X(v, pcb_x);
	*event_y = DRAW_Y(v, pcb_y);

	return pcb_true;
}

pcb_bool pcb_gtk_coords_event2pcb(const pcb_gtk_view_t *v, int event_x, int event_y, pcb_coord_t * pcb_x, pcb_coord_t * pcb_y)
{
	*pcb_x = pcb_round(EVENT_TO_PCB_X(v, event_x));
	*pcb_y = pcb_round(EVENT_TO_PCB_Y(v, event_y));

	return pcb_true;
}

void pcb_gtk_zoom_post(pcb_gtk_view_t *v)
{
	v->coord_per_px = pcb_gtk_clamp_zoom(v, v->coord_per_px);
	v->width = v->canvas_width * v->coord_per_px;
	v->height = v->canvas_height * v->coord_per_px;
}

/* gport->view.coord_per_px:
 * zoom value is PCB units per screen pixel.  Larger numbers mean zooming
 * out - the largest value means you are looking at the whole board.
 *
 * gport->view_width and gport->view_height are in PCB coordinates
 */
static void ghid_zoom_view_abs(pcb_gtk_view_t *v, pcb_coord_t center_x, pcb_coord_t center_y, double new_zoom)
{
	double clamped_zoom;
	double xtmp, ytmp;
	pcb_coord_t cmaxx, cmaxy;

	clamped_zoom = pcb_gtk_clamp_zoom(v, new_zoom);
	if (clamped_zoom != new_zoom)
		return;

	if (v->coord_per_px == new_zoom)
		return;

	/* Do not allow zoom level that'd overflow the coord type */
	cmaxx = v->canvas_width  * (new_zoom / 2.0);
	cmaxy = v->canvas_height * (new_zoom / 2.0);
	if ((cmaxx >= COORD_MAX/2) || (cmaxy >= COORD_MAX/2)) {
		return;
	}

	xtmp = (SIDE_X(v, center_x) - v->x0) / (double) v->width;
	ytmp = (SIDE_Y(v, center_y) - v->y0) / (double) v->height;

	v->coord_per_px = new_zoom;
	pcb_pixel_slop = new_zoom;
	v->com->port_ranges_scale();

	v->x0 = SIDE_X(v, center_x) - xtmp * v->width;
	v->y0 = SIDE_Y(v, center_y) - ytmp * v->height;

	pcb_gtk_pan_common(v);
}



void pcb_gtk_zoom_view_rel(pcb_gtk_view_t *v, pcb_coord_t center_x, pcb_coord_t center_y, double factor)
{
	ghid_zoom_view_abs(v, center_x, center_y, v->coord_per_px * factor);
}

TODO(": remove this and make the side-correct version the default (rename that to this short name); check when looking from the bottom: library window, drc window")
void pcb_gtk_zoom_view_win(pcb_gtk_view_t *v, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	double xf, yf;

	if ((v->canvas_width < 1) || (v->canvas_height < 1))
		return;

	xf = (x2 - x1) / v->canvas_width;
	yf = (y2 - y1) / v->canvas_height;
	v->coord_per_px = (xf > yf ? xf : yf);

	v->x0 = x1;
	v->y0 = y1;

	pcb_gtk_pan_common(v);
}

/* Side-correct version - long term this will be kept and the other is removed */
void pcb_gtk_zoom_view_win_side(pcb_gtk_view_t *v, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, int setch)
{
	double xf, yf;

	if ((v->canvas_width < 1) || (v->canvas_height < 1))
		return;

	xf = (x2 - x1) / v->canvas_width;
	yf = (y2 - y1) / v->canvas_height;
	v->coord_per_px = (xf > yf ? xf : yf);

	v->x0 = SIDE_X(v, conf_core.editor.view.flip_x ? x2 : x1);
	v->y0 = SIDE_Y(v, conf_core.editor.view.flip_y ? y2 : y1);

	pcb_gtk_pan_common(v);
	if (setch) {
		v->pcb_x = (x1+x2)/2;
		v->pcb_y = (y1+y2)/2;
		pcb_crosshair_move_absolute(v->pcb_x, v->pcb_y);
		pcb_notify_crosshair_change(pcb_true);
	}
}

static void pcb_gtk_flip_view(pcb_gtk_view_t *v, pcb_coord_t center_x, pcb_coord_t center_y, pcb_bool flip_x, pcb_bool flip_y)
{
	int widget_x, widget_y;

	pcb_draw_inhibit_inc();

	/* Work out where on the screen the flip point is */
	pcb_gtk_coords_pcb2event(v, center_x, center_y, &widget_x, &widget_y);

	conf_set_design("editor/view/flip_x", "%d", conf_core.editor.view.flip_x != flip_x);
	conf_set_design("editor/view/flip_y", "%d", conf_core.editor.view.flip_y != flip_y);

	/* Pan the board so the center location remains in the same place */
	pcb_gtk_pan_view_abs(v, center_x, center_y, widget_x, widget_y);

	pcb_draw_inhibit_dec();

	v->com->invalidate_all();
}

void pcb_gtk_pan_view_abs(pcb_gtk_view_t *v, pcb_coord_t pcb_x, pcb_coord_t pcb_y, int widget_x, int widget_y)
{
	v->x0 = pcb_round((double)SIDE_X(v, pcb_x) - (double)widget_x * v->coord_per_px);
	v->y0 = pcb_round((double)SIDE_Y(v, pcb_y) - (double)widget_y * v->coord_per_px);

	pcb_gtk_pan_common(v);
}

void pcb_gtk_pan_view_rel(pcb_gtk_view_t *v, pcb_coord_t dx, pcb_coord_t dy)
{
	v->x0 += dx;
	v->y0 += dy;

	pcb_gtk_pan_common(v);
}


const char pcb_acts_zoom[] = "Zoom()\n" "Zoom(factor)\n" "Zoom(x1, y1, x2, y2)\n";
const char pcb_acth_zoom[] = "Various zoom factor changes.";
/* DOC: zoom.html */
fgw_error_t pcb_gtk_act_zoom(pcb_gtk_view_t *vw, fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *vp, *ovp;
	double v;
	pcb_coord_t x, y;

	if (argc < 2) {
		pcb_gtk_zoom_view_win_side(vw, 0, 0, PCB->hidlib.size_x, PCB->hidlib.size_y, 1);
		return 0;
	}

	if (argc == 5) {
		pcb_coord_t x1, y1, x2, y2;

		PCB_ACT_CONVARG(1, FGW_COORD, zoom, x1 = fgw_coord(&argv[1]));
		PCB_ACT_CONVARG(2, FGW_COORD, zoom, y1 = fgw_coord(&argv[2]));
		PCB_ACT_CONVARG(3, FGW_COORD, zoom, x2 = fgw_coord(&argv[3]));
		PCB_ACT_CONVARG(4, FGW_COORD, zoom, y2 = fgw_coord(&argv[4]));

		pcb_gtk_zoom_view_win_side(vw, x1, y1, x2, y2, 1);
		return 0;
	}

	if (argc > 2)
		PCB_ACT_FAIL(zoom);

	PCB_ACT_CONVARG(1, FGW_STR, zoom, ovp = vp = argv[1].val.str);

	if (pcb_strcasecmp(vp, "selected") == 0) {
		pcb_box_t sb;
		if (pcb_get_selection_bbox(&sb, PCB->Data) > 0)
			pcb_gtk_zoom_view_win_side(vw, sb.X1, sb.Y1, sb.X2, sb.Y2, 1);
		else
			pcb_message(PCB_MSG_ERROR, "Can't zoom to selection: nothing selected\n");
		return 0;
	}

	if (pcb_strcasecmp(vp, "found") == 0) {
		pcb_box_t sb;
		if (pcb_get_found_bbox(&sb, PCB->Data) > 0)
			pcb_gtk_zoom_view_win_side(vw, sb.X1, sb.Y1, sb.X2, sb.Y2, 1);
		else
			pcb_message(PCB_MSG_ERROR, "Can't zoom to 'found': nothing found\n");
		return 0;
	}

	if (*vp == '?') {
		pcb_message(PCB_MSG_INFO, "Current gtk zoom level: %f\n", vw->coord_per_px);
		return 0;
	}

	if (pcb_strcasecmp(argv[1].val.str, "get") == 0) {
		res->type = FGW_DOUBLE;
		res->val.nat_double = vw->coord_per_px;
		return 0;
	}

	if (*vp == '+' || *vp == '-' || *vp == '=')
		vp++;
	v = g_ascii_strtod(vp, 0);
	if (v <= 0)
		return 1;

	pcb_hid_get_coords("Select zoom center", &x, &y, 0);
	switch (ovp[0]) {
	case '-':
		pcb_gtk_zoom_view_rel(vw, x, y, 1 / v);
		break;
	default:
	case '+':
		pcb_gtk_zoom_view_rel(vw, x, y, v);
		break;
	case '=':
		ghid_zoom_view_abs(vw, x, y, v);
		break;
	}

	PCB_ACT_IRES(0);
	return 0;
}

const char pcb_acts_center[] = "Center()\n";
const char pcb_acth_center[] = "Moves the pointer to the center of the window.";
/* DOC: center.html */
fgw_error_t pcb_gtk_act_center(pcb_gtk_view_t *vw, fgw_arg_t *res, int argc, fgw_arg_t *argv, pcb_coord_t pcb_x, pcb_coord_t pcb_y, int offset_x, int offset_y, int *out_pointer_x, int *out_pointer_y)
{
	int widget_x, widget_y;

	if (argc != 1)
		PCB_ACT_FAIL(center);

	/* Aim to put the given x, y PCB coordinates in the center of the widget */
	widget_x = vw->canvas_width / 2;
	widget_y = vw->canvas_height / 2;

	pcb_gtk_pan_view_abs(vw, pcb_x, pcb_y, widget_x, widget_y);

	/* Now move the mouse pointer to the place where the board location
	 * actually ended up.
	 *
	 * XXX: Should only do this if we confirm we are inside our window?
	 */

	pcb_gtk_coords_pcb2event(vw, pcb_x, pcb_y, &widget_x, &widget_y);

	*out_pointer_x = offset_x + widget_x;
	*out_pointer_y = offset_y + widget_y;

	PCB_ACT_IRES(0);
	return 0;
}

/* ---------------------------------------------------------------------- */
const char pcb_acts_swapsides[] = "SwapSides(|v|h|r, [S])";
const char pcb_acth_swapsides[] = "Swaps the side of the board you're looking at.";
/* DOC: swapsides.html */
fgw_error_t pcb_gtk_swap_sides(pcb_gtk_view_t *vw, fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_layergrp_id_t active_group = pcb_layer_get_group(PCB, pcb_layer_stack[0]);
	pcb_layergrp_id_t comp_group = -1, solder_group = -1;
	pcb_bool comp_on = pcb_false, solder_on = pcb_false;

	if (pcb_layergrp_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_COPPER, &solder_group, 1) > 0)
		solder_on = LAYER_PTR(PCB->LayerGroups.grp[solder_group].lid[0])->meta.real.vis;

	if (pcb_layergrp_list(PCB, PCB_LYT_TOP | PCB_LYT_COPPER, &comp_group, 1) > 0)
		comp_on = LAYER_PTR(PCB->LayerGroups.grp[comp_group].lid[0])->meta.real.vis;

	pcb_draw_inhibit_inc();
	if (argc > 1) {
		const char *a, *b = "";
		pcb_layer_id_t lid;
		pcb_layer_type_t lyt;

		PCB_ACT_CONVARG(1, FGW_STR, swapsides, a = argv[1].val.str);
		PCB_ACT_MAY_CONVARG(2, FGW_STR, swapsides, b = argv[2].val.str);
		switch (a[0]) {
		case 'h':
		case 'H':
			pcb_gtk_flip_view(vw, vw->crosshair_x, vw->crosshair_y, pcb_true, pcb_false);
			break;
		case 'v':
		case 'V':
			pcb_gtk_flip_view(vw, vw->crosshair_x, vw->crosshair_y, pcb_false, pcb_true);
			break;
		case 'r':
		case 'R':
			pcb_gtk_flip_view(vw, vw->crosshair_x, vw->crosshair_y, pcb_true, pcb_true);
			conf_toggle_editor(show_solder_side); /* Swapped back below */
			break;
		default:
			pcb_draw_inhibit_dec();
			return 1;
		}
		switch (b[0]) {
			case 'S':
			case 's':
				lyt = (pcb_layer_flags_(CURRENT) & PCB_LYT_ANYTHING) | (!conf_core.editor.show_solder_side ?  PCB_LYT_BOTTOM : PCB_LYT_TOP);
				lid = pcb_layer_vis_last_lyt(lyt);
				if (lid >= 0)
					pcb_layervis_change_group_vis(lid, 1, 1);
		}
	}

	conf_toggle_editor(show_solder_side);

	if ((active_group == comp_group && comp_on && !solder_on) || (active_group == solder_group && solder_on && !comp_on)) {
		pcb_bool new_solder_vis = conf_core.editor.show_solder_side;

		if (comp_group >= 0)
			pcb_layervis_change_group_vis(PCB->LayerGroups.grp[comp_group].lid[0], !new_solder_vis, !new_solder_vis);
		if (solder_group >= 0)
			pcb_layervis_change_group_vis(PCB->LayerGroups.grp[solder_group].lid[0], new_solder_vis, new_solder_vis);
	}

	pcb_draw_inhibit_dec();

	vw->com->invalidate_all();

	PCB_ACT_IRES(0);
	return 0;
}

const char pcb_acts_scroll[] = "Scroll(up|down|left|right, [div])";
const char pcb_acth_scroll[] = "Scroll the viewport.";
/* DOC: scroll.html */
fgw_error_t pcb_gtk_act_scroll(pcb_gtk_view_t *vw, fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *op;
	gdouble dx = 0.0, dy = 0.0;
	int div = 40;

	PCB_ACT_CONVARG(1, FGW_STR, scroll, op = argv[1].val.str);
	PCB_ACT_MAY_CONVARG(2, FGW_INT, scroll, div = argv[2].val.nat_int);

	if (pcb_strcasecmp(op, "up") == 0)
		dy = -vw->height / div;
	else if (pcb_strcasecmp(op, "down") == 0)
		dy = vw->height / div;
	else if (pcb_strcasecmp(op, "right") == 0)
		dx = vw->width / div;
	else if (pcb_strcasecmp(op, "left") == 0)
		dx = -vw->width / div;
	else
		PCB_ACT_FAIL(scroll);

	pcb_gtk_pan_view_rel(vw, dx, dy);

	PCB_ACT_IRES(0);
	return 0;
}

const char pcb_acts_pan[] = "Pan([thumb], Mode)";
const char pcb_acth_pan[] = "Start or stop panning (Mode = 1 to start, 0 to stop)\n" "Optional thumb argument is ignored for now in gtk hid.\n";
/* DOC: pan.html */
fgw_error_t pcb_gtk_act_pan(pcb_gtk_view_t *vw, fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int mode;

	switch(argc) {
		case 2:
			PCB_ACT_CONVARG(1, FGW_INT, pan, mode = argv[1].val.nat_int);
			break;
		case 3:
			PCB_ACT_CONVARG(2, FGW_INT, pan, mode = argv[2].val.nat_int);
			pcb_message(PCB_MSG_WARNING, "The gtk gui currently ignores the optional first argument to the Pan action.\nFeel free to provide patches.\n");
			PCB_ACT_IRES(1);
			return 0;
		default:
			PCB_ACT_FAIL(pan);
	}

	vw->panning = mode;

	PCB_ACT_IRES(0);
	return 0;
}


void pcb_gtk_get_coords(pcb_gtk_mouse_t *mouse, pcb_gtk_view_t *vw, const char *msg, pcb_coord_t *x, pcb_coord_t *y, int force)
{
	if ((force || !vw->has_entered) && msg)
		if (!ghid_get_user_xy(mouse, msg))
			return;
	if (vw->has_entered) {
		*x = vw->pcb_x;
		*y = vw->pcb_y;
	}
}
