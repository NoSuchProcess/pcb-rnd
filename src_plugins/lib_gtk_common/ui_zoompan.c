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
#include "compat_nls.h"
#include "draw.h"
#include "ui_zoompan.h"

void ghid_set_status_line_label(void);

#warning TODO: defined in the hid for now:
void pcb_gtk_pan_common();
void ghid_port_ranges_scale(void);
void ghid_invalidate_all();

pcb_bool ghid_pcb_to_event_coords(const pcb_gtk_view_t *v, pcb_coord_t pcb_x, pcb_coord_t pcb_y, int *event_x, int *event_y)
{
	*event_x = DRAW_X(v, pcb_x);
	*event_y = DRAW_Y(v, pcb_y);

	return pcb_true;
}

pcb_bool ghid_event_to_pcb_coords(const pcb_gtk_view_t *v, int event_x, int event_y, pcb_coord_t * pcb_x, pcb_coord_t * pcb_y)
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

	pcb_gtk_pan_common();

	ghid_set_status_line_label();
}



static void ghid_zoom_view_rel(pcb_gtk_view_t *v, pcb_coord_t center_x, pcb_coord_t center_y, double factor)
{
	ghid_zoom_view_abs(v, center_x, center_y, v->coord_per_px * factor);
}

void ghid_zoom_view_fit(pcb_gtk_view_t *v)
{
	ghid_pan_view_abs(v, SIDE_X(0), SIDE_Y(0), 0, 0);
	ghid_zoom_view_abs(v, SIDE_X(0), SIDE_Y(0), MAX(PCB->MaxWidth / v->canvas_width, PCB->MaxHeight / v->canvas_height));
}

void ghid_flip_view(pcb_gtk_view_t *v, pcb_coord_t center_x, pcb_coord_t center_y, pcb_bool flip_x, pcb_bool flip_y)
{
	int widget_x, widget_y;

	pcb_draw_inhibit_inc();

	/* Work out where on the screen the flip point is */
	ghid_pcb_to_event_coords(v, center_x, center_y, &widget_x, &widget_y);

	conf_set_design("editor/view/flip_x", "%d", conf_core.editor.view.flip_x != flip_x);
	conf_set_design("editor/view/flip_y", "%d", conf_core.editor.view.flip_y != flip_y);

	/* Pan the board so the center location remains in the same place */
	ghid_pan_view_abs(v, center_x, center_y, widget_x, widget_y);

	pcb_draw_inhibit_dec();

	ghid_invalidate_all();
}

void ghid_pan_view_abs(pcb_gtk_view_t *v, pcb_coord_t pcb_x, pcb_coord_t pcb_y, int widget_x, int widget_y)
{
	v->x0 = SIDE_X(pcb_x) - widget_x * v->coord_per_px;
	v->y0 = SIDE_Y(pcb_y) - widget_y * v->coord_per_px;

	pcb_gtk_pan_common();
}

void ghid_pan_view_rel(pcb_gtk_view_t *v, pcb_coord_t dx, pcb_coord_t dy)
{
	v->x0 += dx;
	v->y0 += dy;

	pcb_gtk_pan_common();
}


/* ------------------------------------------------------------ */

const char zoom_syntax[] = "Zoom()\n" "Zoom(factor)";

const char zoom_help[] = N_("Various zoom factor changes.");

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
		PCB_AFAIL(zoom);

	if (argc < 1) {
		ghid_zoom_view_fit(vw);
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
