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


#ifndef PCB_LIB_GTK_COMMON_UI_ZOOMPAN_H
#define PCB_LIB_GTK_COMMON_UI_ZOOMPAN_H

#include "pcb_bool.h"
#include "glue.h"

#define SIDE_X_(hidlib, x)      ((pcbhl_conf.editor.view.flip_x ? hidlib->size_x - (x) : (x)))
#define SIDE_Y_(hidlib, y)      ((pcbhl_conf.editor.view.flip_y ? hidlib->size_y - (y) : (y)))
#define SIDE_X(v, x)            SIDE_X_((v)->com->hidlib, (x))
#define SIDE_Y(v, y)            SIDE_Y_((v)->com->hidlib, (y))

#define DRAW_X(view, x)         (gint)((SIDE_X((view), x) - (view)->x0) / (view)->coord_per_px)
#define DRAW_Y(view, y)         (gint)((SIDE_Y((view), y) - (view)->y0) / (view)->coord_per_px)

#define EVENT_TO_PCB_X_(hidlib, view, x) (pcb_coord_t)pcb_round(SIDE_X_((hidlib), (double)(x) * (view)->coord_per_px + (double)(view)->x0))
#define EVENT_TO_PCB_Y_(hidlib, view, y) (pcb_coord_t)pcb_round(SIDE_Y_((hidlib), (double)(y) * (view)->coord_per_px + (double)(view)->y0))

#define EVENT_TO_PCB_X(view, x)  EVENT_TO_PCB_X_((view)->com->hidlib, view, (x))
#define EVENT_TO_PCB_Y(view, y)  EVENT_TO_PCB_Y_((view)->com->hidlib, view, (y))

typedef struct {
	double coord_per_px;     /* Zoom level described as PCB units per screen pixel */

	pcb_coord_t x0;
	pcb_coord_t y0;
	pcb_coord_t width;
	pcb_coord_t height;

	unsigned use_max_pcb:1;  /* when 1, use PCB->Max*; when 0, use the following two: */
	pcb_coord_t max_width;
	pcb_coord_t max_height;

	int canvas_width, canvas_height;

	pcb_bool has_entered;
	pcb_bool panning;
	pcb_coord_t pcb_x, pcb_y;              /* PCB coordinates of the mouse pointer */
	pcb_coord_t crosshair_x, crosshair_y;  /* PCB coordinates of the crosshair     */

	pcb_gtk_common_t *com;
} pcb_gtk_view_t;

#include "in_mouse.h"

/* take coord_per_px and validate it against other view parameters. Return
    coord_per_px or the clamped value if it was too small or too large. */
double pcb_gtk_clamp_zoom(const pcb_gtk_view_t *vw, double coord_per_px);

void pcb_gtk_pan_view_abs(pcb_gtk_view_t *v, pcb_coord_t pcb_x, pcb_coord_t pcb_y, double widget_x, double widget_y);
void pcb_gtk_pan_view_rel(pcb_gtk_view_t *v, pcb_coord_t dx, pcb_coord_t dy);
void pcb_gtk_zoom_view_abs(pcb_gtk_view_t *v, pcb_coord_t center_x, pcb_coord_t center_y, double new_zoom);
void pcb_gtk_zoom_view_rel(pcb_gtk_view_t *v, pcb_coord_t center_x, pcb_coord_t center_y, double factor);
void pcb_gtk_zoom_view_win_side(pcb_gtk_view_t *v, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, int setch);

/* obsolete, do not use (kept temporarily for preview) */
void pcb_gtk_zoom_view_win(pcb_gtk_view_t *v, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2);

/* Updates width and heigth using the new zoom level; call after a call
   to pcb_gtk_zoom_*() */
void pcb_gtk_zoom_post(pcb_gtk_view_t *v);

pcb_bool pcb_gtk_coords_pcb2event(const pcb_gtk_view_t *v, pcb_coord_t pcb_x, pcb_coord_t pcb_y, int *event_x, int *event_y);
pcb_bool pcb_gtk_coords_event2pcb(const pcb_gtk_view_t *v, int event_x, int event_y, pcb_coord_t * pcb_x, pcb_coord_t * pcb_y);

void pcb_gtk_get_coords(pcb_gtk_t *ctx, pcb_gtk_view_t *vw, const char *msg, pcb_coord_t *x, pcb_coord_t *y, int force);

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
