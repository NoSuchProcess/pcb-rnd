/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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

#ifndef RND_GLOBALCONST_H
#define RND_GLOBALCONST_H

#include <librnd/config.h>

#define RND_LARGE_VALUE      (RND_COORD_MAX / 2 - 1) /* maximum extent of board and elements */

#define RND_MAX_COORD        ((rnd_coord_t)RND_LARGE_VALUE) /* coordinate limits */
#define RND_MIN_SIZE         0

#ifndef RND_PATH_MAX   /* maximum path length */
#ifdef PATH_MAX
#define RND_PATH_MAX PATH_MAX
#else
#define RND_PATH_MAX 2048
#endif
#endif

/* number of dynamic flag bits that can be allocated at once; should be n*64 for
   memory efficiency */
#define RND_DYNFLAG_BLEN 64

#define RND_MAX_LINE_POINT_DISTANCE     0   /* maximum distance when searching line points; same for arc point */
#define RND_MAX_POLYGON_POINT_DISTANCE  0   /* maximum distance when searching polygon points */
#define RND_MAX_NETLIST_LINE_LENGTH     255 /* maximum line length for netlist files */
#define RND_MIN_GRID_DISTANCE           4   /* minimum distance between point to enable grid drawing */


#endif
