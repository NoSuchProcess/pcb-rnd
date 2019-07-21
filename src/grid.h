/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#ifndef PCB_GRID_H
#define PCB_GRID_H

#include <genvector/gds_char.h>
#include "pcb_bool.h"
#include "global_typedefs.h"
#include "unit.h"

/* String packed syntax (bracket means optional):

     [name:]size[@offs][!unit]

  "!" means to switch the UI to the unit specified. */
typedef struct {
	char *name;
	pcb_coord_t size;
	pcb_coord_t ox, oy;
	const pcb_unit_t *unit; /* force switching to unit if not NULL */
} pcb_grid_t;

/* Returns the nearest grid-point to the given coord x */
pcb_coord_t pcb_grid_fit(pcb_coord_t x, pcb_coord_t grid_spacing, pcb_coord_t grid_offset);

/* Parse packed string format src into dst; allocat dst->name on success */
pcb_bool_t pcb_grid_parse(pcb_grid_t *dst, const char *src);

/* Free all allocated fields of a grid struct */
void pcb_grid_free(pcb_grid_t *dst);

/* Convert src into packed string format */
pcb_bool_t pcb_grid_append_print(gds_t *dst, const pcb_grid_t *src);
char *pcb_grid_print(const pcb_grid_t *src);

/* Apply grid settings from src to the pcb */
void pcb_grid_set(pcb_hidlib_t *hidlib, const pcb_grid_t *src);

/* Jump to grid index dst (clamped); absolute set */
pcb_bool_t pcb_grid_list_jump(pcb_hidlib_t *hidlib, int dst);

/* Step stp steps (can be 0) on the grids list and set the resulting grid; relative set */
pcb_bool_t pcb_grid_list_step(pcb_hidlib_t *hidlib, int stp);

/* invalidate the grid index; call this when changing the grid settings */
void pcb_grid_inval(void);

/* Reinstall grid menus */
void pcb_grid_install_menu(void);

#endif
