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

	/* Go from from the grid units in use (millimeters or mils) to PCB units
	   |  and back again.
	   |  PCB keeps values internally higher precision, but gui
	   |  widgets (spin buttons, labels, etc) need mils or millimeters.
	 */
#define	FROM_PCB_UNITS(v)	pcb_coord_to_unit(conf_core.editor.grid_unit, v)
#define	TO_PCB_UNITS(v)		pcb_unit_to_coord(conf_core.editor.grid_unit, v)

#define SIDE_X(x)         ((conf_core.editor.view.flip_x ? PCB->MaxWidth - (x) : (x)))
#define SIDE_Y(y)         ((conf_core.editor.view.flip_y ? PCB->MaxHeight - (y) : (y)))

#define	DRAW_X(x)         (gint)((SIDE_X(x) - gport->view.x0) / gport->view.coord_per_px)
#define	DRAW_Y(y)         (gint)((SIDE_Y(y) - gport->view.y0) / gport->view.coord_per_px)

#define	EVENT_TO_PCB_X(x) SIDE_X((gint)((x) * gport->view.coord_per_px + gport->view.x0))
#define	EVENT_TO_PCB_Y(y) SIDE_Y((gint)((y) * gport->view.coord_per_px + gport->view.y0))


void ghid_flip_view(pcb_coord_t center_x, pcb_coord_t center_y, pcb_bool flip_x, pcb_bool flip_y);
void ghid_pan_view_abs(pcb_coord_t pcb_x, pcb_coord_t pcb_y, int widget_x, int widget_y);
void ghid_zoom_view_fit(void);


extern const char zoom_syntax[];
extern const char zoom_help[];
int Zoom(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y);


