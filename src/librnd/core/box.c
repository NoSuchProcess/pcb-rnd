/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include <librnd/config.h>

#include <string.h>
#include <librnd/core/rotate.h>
#include <librnd/core/box.h>

void rnd_box_rotate90(rnd_rnd_box_t *Box, rnd_coord_t X, rnd_coord_t Y, unsigned Number)
{
	rnd_coord_t x1 = Box->X1, y1 = Box->Y1, x2 = Box->X2, y2 = Box->Y2;

	RND_COORD_ROTATE90(x1, y1, X, Y, Number);
	RND_COORD_ROTATE90(x2, y2, X, Y, Number);
	Box->X1 = MIN(x1, x2);
	Box->Y1 = MIN(y1, y2);
	Box->X2 = MAX(x1, x2);
	Box->Y2 = MAX(y1, y2);
}

void rnd_box_enlarge(rnd_rnd_box_t *box, double xfactor, double yfactor)
{
	double w = (double)(box->X2 - box->X1) * xfactor / 2.0;
	double h = (double)(box->Y2 - box->Y1) * yfactor / 2.0;

	box->X1 = rnd_round(box->X1 - w);
	box->Y1 = rnd_round(box->Y1 - h);
	box->X2 = rnd_round(box->X2 + w);
	box->Y2 = rnd_round(box->Y2 + h);
}
