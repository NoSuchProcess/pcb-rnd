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
void pcb_gtk_zoom_view_abs(pcb_gtk_view_t *v, pcb_coord_t center_x, pcb_coord_t center_y, double new_zoom)
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
	pcb_gtk_zoom_view_abs(v, center_x, center_y, v->coord_per_px * factor);
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
