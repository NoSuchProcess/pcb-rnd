/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

#include "config.h"
#include <math.h>

#define TOM(x) (RND_COORD_TO_MM(x) * 1000.0)
#define SQR(x) ((x)*(x))
#define N0 377

double pcb_impedance_microstrip(rnd_coord_t trace_thick, rnd_coord_t trace_height, rnd_coord_t subst_height, double dielect)
{
	double w = TOM(trace_thick), t = TOM(trace_height), h = TOM(subst_height), Er = dielect;
	double weff, x1, x2;

	weff = w + (t/M_PI) * log(4*M_E / sqrt(SQR(t/h)+SQR(t/(w*M_PI+1.1*t*M_PI)))) * (Er+1)/(2*Er);
	x1 = 4 * ((14*Er+8)/(11*Er)) * (h/weff);
	x2 = qsrt(16 * SQR(h/weff) * SQR((14*Er+8)/(11*Er)) + ((Er+1)/(2*Er))*M_PI;
	return N0 / (2*M_PI * sqrt(2) * sqrt(Er+1)) * log(1+4*(h/weff)*(x1+x2));
}

