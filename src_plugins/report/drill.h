/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *
 *  This module, drill.h, was written and is Copyright (C) 1997 harry eaton
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
 *
 */
#ifndef PCB_DRILL_H
#define PCB_DRILL_H

typedef struct {								/* holds drill information */
	rnd_coord_t DrillSize;							/* this drill's diameter */
	pcb_cardinal_t ElementN,						/* the number of elements using this drill size */
	  ElementMax,									/* max number of elements from malloc() */
	  PinCount,										/* number of pins drilled this size */
	  ViaCount,										/* number of vias drilled this size */
	  UnplatedCount,							/* number of these holes that are unplated */
	  PinN,												/* number of drill coordinates in the list */
	  PinMax;											/* max number of coordinates from malloc() */
	pcb_any_obj_t **hole;					/* the objects that had the drill */
	pcb_any_obj_t **parent;				/* all parents referenced by any hole above */
} pcb_drill_t;

typedef struct {								/* holds a range of Drill Infos */
	pcb_cardinal_t DrillN,							/* number of drill sizes */
	  DrillMax;										/* max number from malloc() */
	pcb_drill_t *Drill;						/* plated holes */
} pcb_drill_info_t;

pcb_drill_info_t *drill_get_info(pcb_data_t *);
void drill_info_free(pcb_drill_info_t *);
void drill_round_info(pcb_drill_info_t *, int);
pcb_drill_t *drill_info_alloc(pcb_drill_info_t *);
pcb_any_obj_t **drill_pin_alloc(pcb_drill_t *);
pcb_any_obj_t **drill_element_alloc(pcb_drill_t *);

#define DRILL_LOOP(top) do             {                           \
        pcb_cardinal_t        n;                                   \
        pcb_drill_t   *drill;                                     \
        for (n = 0; (top)->DrillN > 0 && n < (top)->DrillN; n++)   \
        {                                                          \
                drill = &(top)->Drill[n]

#endif
