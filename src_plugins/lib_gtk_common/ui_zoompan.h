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


#ifndef PCB_LIB_GTK_COMMON_UI_ZOOMPAN_H
#define PCB_LIB_GTK_COMMON_UI_ZOOMPAN_H

#include <glib.h>
#include <libfungw/fungw.h>
#include "unit.h"
#include "pcb_bool.h"
#include "in_mouse.h"
#include "glue.h"

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

#define	EVENT_TO_PCB_X(view, x) SIDE_X((pcb_coord_t)((x) * (view)->coord_per_px + (view)->x0))
#define	EVENT_TO_PCB_Y(view, y) SIDE_Y((pcb_coord_t)((y) * (view)->coord_per_px + (view)->y0))


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

	pcb_gtk_common_t *com;
} pcb_gtk_view_t;

/* take coord_per_px and validate it against other view parameters. Return
    coord_per_px or the clamped value if it was too small or too large. */
double pcb_gtk_clamp_zoom(const pcb_gtk_view_t *vw, double coord_per_px);

void pcb_gtk_pan_view_abs(pcb_gtk_view_t *v, pcb_coord_t pcb_x, pcb_coord_t pcb_y, int widget_x, int widget_y);
void pcb_gtk_pan_view_rel(pcb_gtk_view_t *v, pcb_coord_t dx, pcb_coord_t dy);
void pcb_gtk_zoom_view_fit(pcb_gtk_view_t *v);
void pcb_gtk_zoom_view_rel(pcb_gtk_view_t *v, pcb_coord_t center_x, pcb_coord_t center_y, double factor);
void pcb_gtk_zoom_view_win(pcb_gtk_view_t *v, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2);

/* Updates width and heigth using the new zoom level; call after a call
   to pcb_gtk_zoom_*() */
void pcb_gtk_zoom_post(pcb_gtk_view_t *v);

pcb_bool pcb_gtk_coords_pcb2event(const pcb_gtk_view_t *v, pcb_coord_t pcb_x, pcb_coord_t pcb_y, int *event_x, int *event_y);
pcb_bool pcb_gtk_coords_event2pcb(const pcb_gtk_view_t *v, int event_x, int event_y, pcb_coord_t * pcb_x, pcb_coord_t * pcb_y);


extern const char pcb_acts_zoom[];
extern const char pcb_acth_zoom[];
fgw_error_t pcb_gtk_act_zoom(pcb_gtk_view_t *v, fgw_arg_t *res, int argc, fgw_arg_t *argv);

extern const char pcb_acts_center[];
extern const char pcb_acth_center[];
fgw_error_t pcb_gtk_act_center(pcb_gtk_view_t *vw, fgw_arg_t *res, int argc, fgw_arg_t *argv, pcb_coord_t pcb_x, pcb_coord_t pcb_y, int offset_x, int offset_y, int *out_pointer_x, int *out_pointer_y);

extern const char pcb_acts_swapsides[];
extern const char pcb_acth_swapsides[];
int pcb_gtk_swap_sides(pcb_gtk_view_t *vw, int argc, const char **argv);

extern const char pcb_acts_scroll[];
extern const char pcb_acth_scroll[];
fgw_error_t pcb_gtk_act_scroll(pcb_gtk_view_t *vw, fgw_arg_t *res, int argc, fgw_arg_t *argv);

extern const char pcb_acts_pan[];
extern const char pcb_acth_pan[];
fgw_error_t pcb_gtk_act_pan(pcb_gtk_view_t *vw, fgw_arg_t *res, int argc, fgw_arg_t *argv);

void pcb_gtk_get_coords(pcb_gtk_mouse_t *mouse, pcb_gtk_view_t *vw, const char *msg, pcb_coord_t * x, pcb_coord_t * y);

/* Update adj limits to match the current zoom level */
static inline void pcb_gtk_zoom_adjustment(GtkAdjustment *adj, pcb_coord_t view_size, pcb_coord_t board_size)
{
	gdouble page_size;

	page_size = MIN(view_size, board_size);
	gtk_adjustment_configure(adj, gtk_adjustment_get_value(adj), /* value */
													 -view_size,              /* lower */
													 board_size + page_size,  /* upper */
													 page_size / 100.0,       /* step_increment */
													 page_size / 10.0,        /* page_increment */
													 page_size);              /* page_size */
}

#endif
