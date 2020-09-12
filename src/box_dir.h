/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1998,1999,2000,2001 harry eaton
 *
 *  this file, box.h, was written and is
 *  Copyright (c) 2001 C. Scott Ananian.
 *
 *  Copyright (C) 2017,2020 Tibor 'Igor2' Palinkas
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
 *
 *  Old contact info:
 *  harry eaton, 6697 Buttonhole Ct, Columbia, MD 21044 USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */


#ifndef RND_BOX_DIR_H
#define RND_BOX_DIR_H


typedef enum {
	RND_NORTH = 0, RND_EAST = 1, RND_SOUTH = 2, RND_WEST = 3,
	RND_NE = 4, RND_SE = 5, RND_SW = 6, RND_NW = 7, RND_ANY_DIR = 8
} rnd_direction_t;

#define RND_BOX_ROTATE_TO_NORTH(box, dir) \
do { rnd_coord_t t;\
	switch(dir) {\
	case RND_EAST: \
		t = (box).X1; (box).X1 = (box).Y1; (box).Y1 = -(box).X2;\
		(box).X2 = (box).Y2; (box).Y2 = -t; break;\
	case RND_SOUTH: \
		t = (box).X1; (box).X1 = -(box).X2; (box).X2 = -t;\
		t = (box).Y1; (box).Y1 = -(box).Y2; (box).Y2 = -t; break;\
	case RND_WEST: \
		t = (box).X1; (box).X1 = -(box).Y2; (box).Y2 = (box).X2;\
		(box).X2 = -(box).Y1; (box).Y1 = t; break;\
	case RND_NORTH: break;\
	default: assert(0);\
	}\
} while (0)

#define RND_BOX_ROTATE_FROM_NORTH(box, dir) \
do { rnd_coord_t t;\
	switch(dir) {\
	case RND_WEST: \
		t = (box).X1; (box).X1 = (box).Y1; (box).Y1 = -(box).X2;\
		(box).X2 = (box).Y2; (box).Y2 = -t; break;\
	case RND_SOUTH: \
		t = (box).X1; (box).X1 = -(box).X2; (box).X2 = -t;\
		t = (box).Y1; (box).Y1 = -(box).Y2; (box).Y2 = -t; break;\
	case RND_EAST: \
		t = (box).X1; (box).X1 = -(box).Y2; (box).Y2 = (box).X2;\
		(box).X2 = -(box).Y1; (box).Y1 = t; break;\
	case RND_NORTH: break;\
	default: assert(0);\
	}\
} while (0)

#endif
