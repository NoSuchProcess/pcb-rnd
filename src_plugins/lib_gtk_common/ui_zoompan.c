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
#include <glib.h>

#warning TODO: this can not be done like this:
#include "../src_plugins/hid_gtk/gui.h"

#warning TODO: defined in the hid for now:
void ghid_zoom_view_abs(pcb_coord_t center_x, pcb_coord_t center_y, double new_zoom);
void pan_common(GHidPort * port);


static void ghid_zoom_view_rel(pcb_coord_t center_x, pcb_coord_t center_y, double factor)
{
	ghid_zoom_view_abs(center_x, center_y, gport->view.coord_per_px * factor);
}

void ghid_zoom_view_fit(void)
{
	ghid_pan_view_abs(SIDE_X(0), SIDE_Y(0), 0, 0);
	ghid_zoom_view_abs(SIDE_X(0), SIDE_Y(0), MAX(PCB->MaxWidth / gport->width, PCB->MaxHeight / gport->height));
}

void ghid_flip_view(pcb_coord_t center_x, pcb_coord_t center_y, pcb_bool flip_x, pcb_bool flip_y)
{
	int widget_x, widget_y;

	pcb_draw_inhibit_inc();

	/* Work out where on the screen the flip point is */
	ghid_pcb_to_event_coords(center_x, center_y, &widget_x, &widget_y);

	conf_set_design("editor/view/flip_x", "%d", conf_core.editor.view.flip_x != flip_x);
	conf_set_design("editor/view/flip_y", "%d", conf_core.editor.view.flip_y != flip_y);

	/* Pan the board so the center location remains in the same place */
	ghid_pan_view_abs(center_x, center_y, widget_x, widget_y);

	pcb_draw_inhibit_dec();

	ghid_invalidate_all();
}

void ghid_pan_view_abs(pcb_coord_t pcb_x, pcb_coord_t pcb_y, int widget_x, int widget_y)
{
	gport->view.x0 = SIDE_X(pcb_x) - widget_x * gport->view.coord_per_px;
	gport->view.y0 = SIDE_Y(pcb_y) - widget_y * gport->view.coord_per_px;

	pan_common(gport);
}

void ghid_pan_view_rel(pcb_coord_t dx, pcb_coord_t dy)
{
	gport->view.x0 += dx;
	gport->view.y0 += dy;

	pan_common(gport);
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

int Zoom(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *vp;
	double v;

	if (argc > 1)
		PCB_AFAIL(zoom);

	if (argc < 1) {
		ghid_zoom_view_fit();
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
		ghid_zoom_view_rel(x, y, 1 / v);
		break;
	default:
	case '+':
		ghid_zoom_view_rel(x, y, v);
		break;
	case '=':
		ghid_zoom_view_abs(x, y, v);
		break;
	}

	return 0;
}
