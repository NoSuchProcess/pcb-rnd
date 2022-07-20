/*
 *                            COPYRIGHT
 *
 *  librnd, modular 2D CAD framework - 2d matrix transformations
 *  librnd Copyright (C) 2022 Tibor 'Igor2' Palinkas
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
 *    Project page: http://repo.hu/projects/librnd
 *    lead developer: http://repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include <librnd/core/config.h>
#include <librnd/core/math_helper.h>
#include <math.h>
#include <string.h>

#include "xform_mx.h"

static void mmult(rnd_xform_mx_t dst, const rnd_xform_mx_t src)
{
	rnd_xform_mx_t tmp;

	tmp[0] = dst[0] * src[0] + dst[1] * src[3] + dst[2] * src[6];
	tmp[1] = dst[0] * src[1] + dst[1] * src[4] + dst[2] * src[7];
	tmp[2] = dst[0] * src[2] + dst[1] * src[5] + dst[2] * src[8];

	tmp[3] = dst[3] * src[0] + dst[4] * src[3] + dst[5] * src[6];
	tmp[4] = dst[3] * src[1] + dst[4] * src[4] + dst[5] * src[7];
	tmp[5] = dst[3] * src[2] + dst[4] * src[5] + dst[5] * src[8];

	tmp[6] = dst[6] * src[0] + dst[7] * src[3] + dst[8] * src[6];
	tmp[7] = dst[6] * src[1] + dst[7] * src[4] + dst[8] * src[7];
	tmp[8] = dst[6] * src[2] + dst[7] * src[5] + dst[8] * src[8];

	memcpy(dst, tmp, sizeof(tmp));
}

void rnd_xform_mx_rotate(rnd_xform_mx_t mx, double deg)
{
	rnd_xform_mx_t tr;

	deg /= RND_RAD_TO_DEG;

	tr[0] = cos(deg);
	tr[1] = sin(deg);
	tr[2] = 0;

	tr[3] = -sin(deg);
	tr[4] = cos(deg);
	tr[5] = 0;

	tr[6] = 0;
	tr[7] = 0;
	tr[8] = 1;

	mmult(mx, tr);
}

void rnd_xform_mx_translate(rnd_xform_mx_t mx, double xt, double yt)
{
	rnd_xform_mx_t tr;

	tr[0] = 1;
	tr[1] = 0;
	tr[2] = xt;

	tr[3] = 0;
	tr[4] = 1;
	tr[5] = yt;

	tr[6] = 0;
	tr[7] = 0;
	tr[8] = 1;

	mmult(mx, tr);
}

void rnd_xform_mx_scale(rnd_xform_mx_t mx, double sx, double sy)
{
	rnd_xform_mx_t tr;

	tr[0] = sx;
	tr[1] = 0;
	tr[2] = 0;

	tr[3] = 0;
	tr[4] = sy;
	tr[5] = 0;

	tr[6] = 0;
	tr[7] = 0;
	tr[8] = 1;

	mmult(mx, tr);
}

void rnd_xform_mx_shear(rnd_xform_mx_t mx, double sx, double sy)
{
	rnd_xform_mx_t tr;

	tr[0] = 1;
	tr[1] = sx;
	tr[2] = 0;

	tr[3] = sy;
	tr[4] = 1;
	tr[5] = 0;

	tr[6] = 0;
	tr[7] = 0;
	tr[8] = 1;

	mmult(mx, tr);
}

void rnd_xform_mx_mirrorx(rnd_xform_mx_t mx)
{
	rnd_xform_mx_scale(mx, 0, -1);
}
