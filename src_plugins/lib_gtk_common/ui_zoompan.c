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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
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
#include "bu_status_line.h"

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
	v->x0 = MIN(v->x0, PCB->MaxWidth);
	v->y0 = MIN(v->y0, PCB->MaxHeight);

	/* Fix up noted event coordinates to match where we clamped. Alternatively
	 * we could call ghid_note_event_location (NULL); to get a new pointer
	 * location, but this costs us an xserver round-trip (on X11 platforms)
	 */
	pcb_gtk_coords_event2pcb(v, event_x, event_y, &v->pcb_x, &v->pcb_y);

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

	xtmp = (SIDE_X(center_x) - v->x0) / (double) v->width;
	ytmp = (SIDE_Y(center_y) - v->y0) / (double) v->height;

	v->coord_per_px = new_zoom;
	pcb_pixel_slop = new_zoom;
	v->com->port_ranges_scale();

	v->x0 = SIDE_X(center_x) - xtmp * v->width;
	v->y0 = SIDE_Y(center_y) - ytmp * v->height;

	pcb_gtk_pan_common(v);
	v->com->set_status_line_label();
}



void pcb_gtk_zoom_view_rel(pcb_gtk_view_t *v, pcb_coord_t center_x, pcb_coord_t center_y, double factor)
{
	ghid_zoom_view_abs(v, center_x, center_y, v->coord_per_px * factor);
}

#warning TODO: remove this and make the side-correct version the default (rename that to this short name); check when looking from the bottom: library window, drc window
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
static void pcb_gtk_zoom_view_win_side(pcb_gtk_view_t *v, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	double xf, yf;

	if ((v->canvas_width < 1) || (v->canvas_height < 1))
		return;

	xf = (x2 - x1) / v->canvas_width;
	yf = (y2 - y1) / v->canvas_height;
	v->coord_per_px = (xf > yf ? xf : yf);

	v->x0 = SIDE_X(conf_core.editor.view.flip_x ? x2 : x1);
	v->y0 = SIDE_Y(conf_core.editor.view.flip_y ? y2 : y1);

	pcb_gtk_pan_common(v);
}

void pcb_gtk_zoom_view_fit(pcb_gtk_view_t *v)
{
	pcb_gtk_pan_view_abs(v, SIDE_X(0), SIDE_Y(0), 0, 0);
	ghid_zoom_view_abs(v, SIDE_X(0), SIDE_Y(0), MAX(PCB->MaxWidth / v->canvas_width, PCB->MaxHeight / v->canvas_height));
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
	v->x0 = SIDE_X(pcb_x) - widget_x * v->coord_per_px;
	v->y0 = SIDE_Y(pcb_y) - widget_y * v->coord_per_px;

	pcb_gtk_pan_common(v);
}

void pcb_gtk_pan_view_rel(pcb_gtk_view_t *v, pcb_coord_t dx, pcb_coord_t dy)
{
	v->x0 += dx;
	v->y0 += dy;

	pcb_gtk_pan_common(v);
}


/* ------------------------------------------------------------ */

const char pcb_acts_zoom[] = "Zoom()\n" "Zoom(factor)\n" "Zoom(x1, y1, x2, y2)\n";

const char pcb_acth_zoom[] = "Various zoom factor changes.";

/* %start-doc actions Zoom
Changes the zoom (magnification) of the view of the board.  If no
arguments are passed, the view is scaled such that the board just fits
inside the visible window (i.e. ``view all'').  Otherwise,
@var{factor} specifies a change in zoom factor.  It may be prefixed by
@code{+}, @code{-}, or @code{=} to change how the zoom factor is
modified.  The @var{factor} is a floating point number, such as
@code{1.5} or @code{0.75}.

@table @code

@item +@var{factor}
Values greater than 1.0 cause the board to be drawn smaller; more of
the board will be visible.  Values between 0.0 and 1.0 cause the board
to be drawn bigger; less of the board will be visible.

@item -@var{factor}
Values greater than 1.0 cause the board to be drawn bigger; less of
the board will be visible.  Values between 0.0 and 1.0 cause the board
to be drawn smaller; more of the board will be visible.

@item =@var{factor}

The @var{factor} is an absolute zoom factor; the unit for this value
is "PCB units per screen pixel".  Since PCB units are 0.01 mil, a
@var{factor} of 1000 means 10 mils (0.01 in) per pixel, or 100 DPI,
about the actual resolution of most screens - resulting in an "actual
size" board.  Similarly, a @var{factor} of 100 gives you a 10x actual
size.

@item @var{x1}, @var{y1}, @var{x2}, @var{y2}

Zoom and pan to the box specified by the coords.

@item ?

Print the current zoom level in the message log (as an info line)

@end table

Note that zoom factors of zero are silently ignored.

%end-doc */

int pcb_gtk_zoom(pcb_gtk_view_t *vw, int argc, const char **argv)
{
	const char *vp;
	double v;
	pcb_coord_t x, y;

	if (argc < 1) {
		pcb_gtk_zoom_view_fit(vw);
		return 0;
	}

	if (argc == 4) {
		pcb_coord_t x1, y1, x2, y2;
		pcb_bool succ;

		x1 = pcb_get_value(argv[0], NULL, NULL, &succ);
		if (!succ)
			PCB_ACT_FAIL(zoom);

		y1 = pcb_get_value(argv[1], NULL, NULL, &succ);
		if (!succ)
			PCB_ACT_FAIL(zoom);

		x2 = pcb_get_value(argv[2], NULL, NULL, &succ);
		if (!succ)
			PCB_ACT_FAIL(zoom);

		y2 = pcb_get_value(argv[3], NULL, NULL, &succ);
		if (!succ)
			PCB_ACT_FAIL(zoom);

		pcb_gtk_zoom_view_win_side(vw, x1, y1, x2, y2);
		return 0;
	}

	if (argc > 1)
		PCB_ACT_FAIL(zoom);

	vp = argv[0];
	if (*vp == '?') {
		pcb_message(PCB_MSG_INFO, "Current gtk zoom level: %f\n", vw->coord_per_px);
		return 0;
	}

	if (*vp == '+' || *vp == '-' || *vp == '=')
		vp++;
	v = g_ascii_strtod(vp, 0);
	if (v <= 0)
		return 1;

	pcb_hid_get_coords("Select zoom center", &x, &y);
	switch (argv[0][0]) {
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

	return 0;
}

/* ------------------------------------------------------------ */

const char pcb_acts_center[] = "Center()\n";
const char pcb_acth_center[] = "Moves the pointer to the center of the window.";

/* %start-doc actions Center

Move the pointer to the center of the window, but only if it's
currently within the window already.

%end-doc */

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
const char pcb_acts_swapsides[] = "SwapSides(|v|h|r)";
const char pcb_acth_swapsides[] = "Swaps the side of the board you're looking at.";

/* %start-doc actions SwapSides

This action changes the way you view the board.

@table @code

@item v
Flips the board over vertically (up/down).

@item h
Flips the board over horizontally (left/right), like flipping pages in
a book.

@item r
Rotates the board 180 degrees without changing sides.

@end table

If no argument is given, the board isn't moved but the opposite side
is shown.

Normally, this action changes which pads and silk layer are drawn as
pcb_true silk, and which are drawn as the "invisible" layer.  It also
determines which solder mask you see.

As a special case, if the layer group for the side you're looking at
is visible and currently active, and the layer group for the opposite
is not visible (i.e. disabled), then this action will also swap which
layer group is visible and active, effectively swapping the ``working
side'' of the board.

%end-doc */


int pcb_gtk_swap_sides(pcb_gtk_view_t *vw, int argc, const char **argv)
{
	pcb_layergrp_id_t active_group = pcb_layer_get_group(PCB, pcb_layer_stack[0]);
	pcb_layergrp_id_t comp_group = -1, solder_group = -1;
	pcb_bool comp_on = pcb_false, solder_on = pcb_false;

	if (pcb_layergrp_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_COPPER, &solder_group, 1) > 0)
		solder_on = LAYER_PTR(PCB->LayerGroups.grp[solder_group].lid[0])->meta.real.vis;

	if (pcb_layergrp_list(PCB, PCB_LYT_TOP | PCB_LYT_COPPER, &comp_group, 1) > 0)
		comp_on = LAYER_PTR(PCB->LayerGroups.grp[comp_group].lid[0])->meta.real.vis;

	pcb_draw_inhibit_inc();
	if (argc > 0) {
		switch (argv[0][0]) {
		case 'h':
		case 'H':
			pcb_gtk_flip_view(vw, vw->pcb_x, vw->pcb_y, pcb_true, pcb_false);
			break;
		case 'v':
		case 'V':
			pcb_gtk_flip_view(vw, vw->pcb_x, vw->pcb_y, pcb_false, pcb_true);
			break;
		case 'r':
		case 'R':
			pcb_gtk_flip_view(vw, vw->pcb_x, vw->pcb_y, pcb_true, pcb_true);
			conf_toggle_editor(show_solder_side); /* Swapped back below */
			break;
		default:
			pcb_draw_inhibit_dec();
			return 1;
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

	return 0;
}

/* ------------------------------------------------------------ */
const char pcb_acts_scroll[] = "Scroll(up|down|left|right, [div])";
const char pcb_acth_scroll[] = "Scroll the viewport.";

/* % start-doc actions Scroll

@item up|down|left|right
Specifies the direction to scroll

@item div
Optional.  Specifies how much to scroll by.  The viewport is scrolled
by 1/div of what is visible, so div = 1 scrolls a whole page. If not
default is given, div=40.

%end-doc */

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

/* ------------------------------------------------------------ */
const char pcb_acts_pan[] = "Pan([thumb], Mode)";
const char pcb_acth_pan[] = "Start or stop panning (Mode = 1 to start, 0 to stop)\n" "Optional thumb argument is ignored for now in gtk hid.\n";

/* %start-doc actions Pan

Start or stop panning.  To start call with Mode = 1, to stop call with
Mode = 0.

%end-doc */

fgw_error_t pcb_gtk_act_pan(pcb_gtk_view_t *vw, fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int mode;

	switch(argc) {
		case 2:
			PCB_ACT_CONVARG(1, FGW_INT, pan, mode = argv[1].val.str);
			break;
		case 3:
			PCB_ACT_CONVARG(2, FGW_INT, pan, mode = argv[2].val.str);
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


void pcb_gtk_get_coords(pcb_gtk_mouse_t *mouse, pcb_gtk_view_t *vw, const char *msg, pcb_coord_t * x, pcb_coord_t * y)
{
	if (!vw->has_entered && msg)
		ghid_get_user_xy(mouse, msg);
	if (vw->has_entered) {
		*x = vw->pcb_x;
		*y = vw->pcb_y;
	}
}
