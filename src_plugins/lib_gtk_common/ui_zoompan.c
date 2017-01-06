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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"
#include "unit.h"
#include "action_helper.h"
#include "error.h"
#include "conf_core.h"
#include "board.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "draw.h"
#include "data.h"
#include "layer_vis.h"
#include "ui_zoompan.h"

/* defined by the hid (gtk version or render specific): */
void ghid_set_status_line_label(void);
void ghid_pan_common(void);
void ghid_port_ranges_scale(void);
void ghid_invalidate_all();
void ghid_get_user_xy(const gchar *msg);

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

	ghid_pan_common();
}

pcb_bool pcb_gtk_coords_pcb2event(const pcb_gtk_view_t *v, pcb_coord_t pcb_x, pcb_coord_t pcb_y, int *event_x, int *event_y)
{
	*event_x = DRAW_X(v, pcb_x);
	*event_y = DRAW_Y(v, pcb_y);

	return pcb_true;
}

pcb_bool pcb_gtk_coords_event2pcb(const pcb_gtk_view_t *v, int event_x, int event_y, pcb_coord_t * pcb_x, pcb_coord_t * pcb_y)
{
	*pcb_x = EVENT_TO_PCB_X(v, event_x);
	*pcb_y = EVENT_TO_PCB_Y(v, event_y);

	return pcb_true;
}

/* gport->view.coord_per_px:
 * zoom value is PCB units per screen pixel.  Larger numbers mean zooming
 * out - the largest value means you are looking at the whole board.
 *
 * gport->view_width and gport->view_height are in PCB coordinates
 */

#define ALLOW_ZOOM_OUT_BY 10		/* Arbitrary, and same as the lesstif HID MAX_ZOOM_SCALE */
static void ghid_zoom_view_abs(pcb_gtk_view_t *v, pcb_coord_t center_x, pcb_coord_t center_y, double new_zoom)
{
	double min_zoom, max_zoom;
	double xtmp, ytmp;
	pcb_coord_t cmaxx, cmaxy;

	/* Limit the "minimum" zoom constant (maximum zoom), at 1 pixel per PCB
	 * unit, and set the "maximum" zoom constant (minimum zoom), such that
	 * the entire board just fits inside the viewport
	 */
	min_zoom = 200;
	max_zoom = MAX(PCB->MaxWidth / v->canvas_width, PCB->MaxHeight / v->canvas_height) * ALLOW_ZOOM_OUT_BY;
	new_zoom = MIN(MAX(min_zoom, new_zoom), max_zoom);

	if ((new_zoom > max_zoom) || (new_zoom < min_zoom))
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
	ghid_port_ranges_scale();

	v->x0 = SIDE_X(center_x) - xtmp * v->width;
	v->y0 = SIDE_Y(center_y) - ytmp * v->height;

	pcb_gtk_pan_common(v);

	ghid_set_status_line_label();
}



static void ghid_zoom_view_rel(pcb_gtk_view_t *v, pcb_coord_t center_x, pcb_coord_t center_y, double factor)
{
	ghid_zoom_view_abs(v, center_x, center_y, v->coord_per_px * factor);
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

	ghid_invalidate_all();
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

const char pcb_acts_zoom[] = "Zoom()\n" "Zoom(factor)";

const char pcb_acth_zoom[] = N_("Various zoom factor changes.");

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

@end table

Note that zoom factors of zero are silently ignored.



%end-doc */

int pcb_gtk_zoom(pcb_gtk_view_t *vw, int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *vp;
	double v;

	if (argc > 1)
		PCB_ACT_FAIL(zoom);

	if (argc < 1) {
		pcb_gtk_zoom_view_fit(vw);
		return 0;
	}

	vp = argv[0];
	if (*vp == '+' || *vp == '-' || *vp == '=')
		vp++;
	v = g_ascii_strtod(vp, 0);
	if (v <= 0)
		return 1;
	switch (argv[0][0]) {
	case '-':
		ghid_zoom_view_rel(vw, x, y, 1 / v);
		break;
	default:
	case '+':
		ghid_zoom_view_rel(vw, x, y, v);
		break;
	case '=':
		ghid_zoom_view_abs(vw, x, y, v);
		break;
	}

	return 0;
}

/* ------------------------------------------------------------ */

const char pcb_acts_center[] = "Center()\n";
const char pcb_acth_center[] = N_("Moves the pointer to the center of the window.");

/* %start-doc actions Center

Move the pointer to the center of the window, but only if it's
currently within the window already.

%end-doc */

int pcb_gtk_act_center(pcb_gtk_view_t *vw, int argc, const char **argv, pcb_coord_t pcb_x, pcb_coord_t pcb_y, int offset_x, int offset_y, int *out_pointer_x, int *out_pointer_y)
{
	int widget_x, widget_y;

	if (argc != 0)
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

	return 0;
}

/* ---------------------------------------------------------------------- */
const char pcb_acts_swapsides[] = "SwapSides(|v|h|r)";
const char pcb_acth_swapsides[] = N_("Swaps the side of the board you're looking at.");

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


int pcb_gtk_swap_sides(pcb_gtk_view_t *vw, int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_layergrp_id_t active_group = pcb_layer_get_group(pcb_layer_stack[0]);
	pcb_layergrp_id_t comp_group = pcb_layer_get_group(pcb_component_silk_layer);
	pcb_layergrp_id_t solder_group = pcb_layer_get_group(pcb_solder_silk_layer);
	pcb_bool comp_on = LAYER_PTR(PCB->LayerGroups.Entries[comp_group][0])->On;
	pcb_bool solder_on = LAYER_PTR(PCB->LayerGroups.Entries[solder_group][0])->On;

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

		pcb_layervis_change_group_vis(PCB->LayerGroups.Entries[comp_group][0], !new_solder_vis, !new_solder_vis);
		pcb_layervis_change_group_vis(PCB->LayerGroups.Entries[solder_group][0], new_solder_vis, new_solder_vis);
	}

	pcb_draw_inhibit_dec();

	ghid_invalidate_all();

	return 0;
}

/* ------------------------------------------------------------ */
const char pcb_acts_scroll[] = "Scroll(up|down|left|right, [div])";
const char pcb_acth_scroll[] = N_("Scroll the viewport.");

/* % start-doc actions Scroll

@item up|down|left|right
Specifies the direction to scroll

@item div
Optional.  Specifies how much to scroll by.  The viewport is scrolled
by 1/div of what is visible, so div = 1 scrolls a whole page. If not
default is given, div=40.

%end-doc */

int pcb_gtk_act_scroll(pcb_gtk_view_t *vw, int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	gdouble dx = 0.0, dy = 0.0;
	int div = 40;

	if (argc != 1 && argc != 2)
		PCB_ACT_FAIL(scroll);

	if (argc == 2)
		div = atoi(argv[1]);

	if (pcb_strcasecmp(argv[0], "up") == 0)
		dy = -vw->height / div;
	else if (pcb_strcasecmp(argv[0], "down") == 0)
		dy = vw->height / div;
	else if (pcb_strcasecmp(argv[0], "right") == 0)
		dx = vw->width / div;
	else if (pcb_strcasecmp(argv[0], "left") == 0)
		dx = -vw->width / div;
	else
		PCB_ACT_FAIL(scroll);

	pcb_gtk_pan_view_rel(vw, dx, dy);

	return 0;
}

/* ------------------------------------------------------------ */
const char pcb_acts_pan[] = "Pan([thumb], Mode)";
const char pcb_acth_pan[] = N_("Start or stop panning (Mode = 1 to start, 0 to stop)\n" "Optional thumb argument is ignored for now in gtk hid.\n");

/* %start-doc actions Pan

Start or stop panning.  To start call with Mode = 1, to stop call with
Mode = 0.

%end-doc */

int pcb_gtk_act_pan(pcb_gtk_view_t *vw, int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	int mode;

	if (argc != 1 && argc != 2)
		PCB_ACT_FAIL(pan);

	if (argc == 1) {
		mode = atoi(argv[0]);
	}
	else {
		mode = atoi(argv[1]);
		pcb_message(PCB_MSG_WARNING, _("The gtk gui currently ignores the optional first argument to the Pan action.\nFeel free to provide patches.\n"));
	}

	vw->panning = mode;

	return 0;
}


void pcb_gtk_get_coords(pcb_gtk_view_t *vw, const char *msg, pcb_coord_t * x, pcb_coord_t * y)
{
	if (!vw->has_entered && msg)
		ghid_get_user_xy(msg);
	if (vw->has_entered) {
		*x = vw->pcb_x;
		*y = vw->pcb_y;
	}
}
