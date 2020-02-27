/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2020 Tibor 'Igor2' Palinkas
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

#ifndef PCB_EVENT_H
#define PCB_EVENT_H

#include <librnd/core/event.h>

enum {
	PCB_EVENT_NEW_PSTK = RND_EVENT_app,   /* called when a new padstack is created */

	PCB_EVENT_ROUTE_STYLES_CHANGED,       /* called after any route style change (used to be the RouteStylesChanged action) */
	PCB_EVENT_NETLIST_CHANGED,            /* called after any netlist change (used to be the NetlistChanged action) */
	PCB_EVENT_LAYERS_CHANGED,             /* called after layers or layer groups change (used to be the LayersChanged action) */
	PCB_EVENT_LAYER_CHANGED_GRP,          /* called after a layer changed its group; argument: layer pointer */
	PCB_EVENT_LAYERVIS_CHANGED,           /* called after the visibility of layers has changed */
	PCB_EVENT_LIBRARY_CHANGED,            /* called after a change in the footprint lib (used to be the LibraryChanged action) */
	PCB_EVENT_FONT_CHANGED,               /* called when a font has changed; argument is the font ID */

	PCB_EVENT_UNDO_POST,                  /* called after an undo/redo operation; argument is an integer pcb_undo_ev_t */

	PCB_EVENT_RUBBER_RESET,               /* rubber band: reset attached */
	PCB_EVENT_RUBBER_MOVE,                /* rubber band: object moved */
	PCB_EVENT_RUBBER_MOVE_DRAW,           /* rubber band: draw crosshair-attached rubber band objects after a move or copy */
	PCB_EVENT_RUBBER_ROTATE90,            /* rubber band: crosshair object rotated by 90 degrees */
	PCB_EVENT_RUBBER_ROTATE,              /* rubber band: crosshair object rotated by arbitrary angle */
	PCB_EVENT_RUBBER_LOOKUP_LINES,        /* rubber band: attach rubber banded line objects to crosshair */
	PCB_EVENT_RUBBER_LOOKUP_RATS,         /* rubber band: attach rubber banded rat lines objects to crosshair */
	PCB_EVENT_RUBBER_CONSTRAIN_MAIN_LINE, /* rubber band: adapt main line to keep rubberband lines direction */

	PCB_EVENT_DRAW_CROSSHAIR_CHATT,       /* called from crosshair code upon attached object recalculation; event handlers can use this hook to enforce various geometric restrictions */

	PCB_EVENT_DRC_RUN,                    /* called from core to run all configured DRCs (implemented in plugins) */

	PCB_EVENT_NET_INDICATE_SHORT,         /* called by core to get a shortcircuit indicated (e.g. by mincut). Args: (pcb_net_t *net, pcb_any_obj_t *offending_term, pcb_net_t *offending_net, int *handled, int *cancel) - if *handled is non-zero, the short is already indicated; if *cancel is non-zero the whole process is cancelled, no more advanced short checking should take place in this session */

	PCB_EVENT_LAYER_KEY_CHANGE,           /* called by core if a pcb-rnd::key::* attribute on a layer changes */

	PCB_EVENT_last                        /* not a real event */
};

void pcb_event_init_app(void);

#endif
