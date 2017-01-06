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


#ifndef PCB_LIB_GTK_COMMON_UI_ZOOMPAN_H
#define PCB_LIB_GTK_COMMON_UI_ZOOMPAN_H

#include <glib.h>

	/* Go from from the grid units in use (millimeters or mils) to PCB units
	   |  and back again.
	   |  PCB keeps values internally higher precision, but gui
	   |  widgets (spin buttons, labels, etc) need mils or millimeters.
	 */
#define	FROM_PCB_UNITS(v)	pcb_coord_to_unit(conf_core.editor.grid_unit, v)
#define	TO_PCB_UNITS(v)		pcb_unit_to_coord(conf_core.editor.grid_unit, v)

#define SIDE_X(x)         ((conf_core.editor.view.flip_x ? PCB->MaxWidth - (x) : (x)))
#define SIDE_Y(y)         ((conf_core.editor.view.flip_y ? PCB->MaxHeight - (y) : (y)))

#define	DRAW_X(view, x)         (gint)((SIDE_X(x) - (view)->x0) / (view)->coord_per_px)
#define	DRAW_Y(view, y)         (gint)((SIDE_Y(y) - (view)->y0) / (view)->coord_per_px)

#define	EVENT_TO_PCB_X(view, x) SIDE_X((gint)((x) * (view)->coord_per_px + (view)->x0))
#define	EVENT_TO_PCB_Y(view, y) SIDE_Y((gint)((y) * (view)->coord_per_px + (view)->y0))


typedef struct {
	double coord_per_px;					/* Zoom level described as PCB units per screen pixel */

	pcb_coord_t x0;
	pcb_coord_t y0;
	pcb_coord_t width;
	pcb_coord_t height;

	gint canvas_width, canvas_height;

	gboolean has_entered;
	gboolean panning;
	pcb_coord_t pcb_x, pcb_y;						/* PCB coordinates of the mouse pointer */
	pcb_coord_t crosshair_x, crosshair_y;	/* PCB coordinates of the crosshair     */
} pcb_gtk_view_t;

void pcb_gtk_pan_view_abs(pcb_gtk_view_t *v, pcb_coord_t pcb_x, pcb_coord_t pcb_y, int widget_x, int widget_y);
void pcb_gtk_pan_view_rel(pcb_gtk_view_t *v, pcb_coord_t dx, pcb_coord_t dy);
void pcb_gtk_zoom_view_fit(pcb_gtk_view_t *v);

pcb_bool pcb_gtk_coords_pcb2event(const pcb_gtk_view_t *v, pcb_coord_t pcb_x, pcb_coord_t pcb_y, int *event_x, int *event_y);
pcb_bool pcb_gtk_coords_event2pcb(const pcb_gtk_view_t *v, int event_x, int event_y, pcb_coord_t * pcb_x, pcb_coord_t * pcb_y);


extern const char zoom_syntax[];
extern const char zoom_help[];
int pcb_gtk_zoom(pcb_gtk_view_t *v, int argc, const char **argv, pcb_coord_t x, pcb_coord_t y);

extern const char center_syntax[];
extern const char center_help[];
int pcb_gtk_act_center(pcb_gtk_view_t *vw, int argc, const char **argv, pcb_coord_t pcb_x, pcb_coord_t pcb_y, int offset_x, int offset_y, int *out_pointer_x, int *out_pointer_y);

extern const char swapsides_syntax[];
extern const char swapsides_help[];
int pcb_gtk_swap_sides(pcb_gtk_view_t *vw, int argc, const char **argv, pcb_coord_t x, pcb_coord_t y);

extern const char scroll_syntax[];
extern const char scroll_help[];
int pcb_gtk_act_scroll(pcb_gtk_view_t *vw, int argc, const char **argv, pcb_coord_t x, pcb_coord_t y);

extern const char pan_syntax[];
extern const char pan_help[];
int pcb_gtk_act_pan(pcb_gtk_view_t *vw, int argc, const char **argv, pcb_coord_t x, pcb_coord_t y);

void pcb_gtk_get_coords(pcb_gtk_view_t *vw, const char *msg, pcb_coord_t * x, pcb_coord_t * y);

#endif
