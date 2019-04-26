/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#ifndef PCB_HIDLIB_H
#define PCB_HIDLIB_H

#include "config.h"

struct pcb_hidlib_s {
	pcb_coord_t grid;                  /* grid resolution */
	pcb_coord_t grid_ox, grid_oy;      /* grid offset */
	pcb_coord_t size_x, size_y;        /* drawing area extents (or board dimensions) */
	char *name;                        /* name of the design */
	char *filename;                    /* name of the file (from load) */
};

/* optional: if non-NULL, called back to determine the file name or project
   name of the current design */
const char *(*pcb_hidlib_get_filename)(void);
const char *(*pcb_hidlib_get_name)(void);

#endif
