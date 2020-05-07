/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017, 2018 Tibor 'Igor2' Palinkas
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

#include <genht/hash.h>
#include <genlist/gendlist.h>
#include <librnd/core/compat_misc.h>

/* compare two strings and return 0 if they are equal. NULL == NULL means equal. */
RND_INLINE int pcb_neqs(const char *s1, const char *s2)
{
	if ((s1 == NULL) && (s2 == NULL)) return 0;
	if ((s1 == NULL) || (s2 == NULL)) return 1;
	return strcmp(s1, s2) != 0;
}

RND_INLINE unsigned pcb_hash_coord(rnd_coord_t c)
{
	return murmurhash(&(c), sizeof(rnd_coord_t));
}

/* cheat: round with a tolerance of a few nanometers to overcome the usual
   +-1 nanometer rounding error */
RND_INLINE rnd_coord_t pcb_round_tol(double v, int tol)
{
	return rnd_round(v/(double)tol)*tol;
}

RND_INLINE void pcb_hash_tr_coords(const pcb_host_trans_t *tr, rnd_coord_t *dstx, rnd_coord_t *dsty, rnd_coord_t srcx, rnd_coord_t srcy)
{
	rnd_coord_t px, py;

	px = srcx + tr->ox;
	py = srcy + tr->oy;

	if ((tr->rot == 0.0) && (!tr->on_bottom)) {
		*dstx = px;
		*dsty = py;
	}
	else {
		double x, y;
		x = (double)px * tr->cosa + (double)py * tr->sina;
		y = (double)py * tr->cosa - (double)px * tr->sina;
		if (tr->on_bottom) y = -(y);

		*dstx = pcb_round_tol(x, 4);
		*dsty = pcb_round_tol(y, 4);
	}
}


RND_INLINE unsigned pcb_hash_angle(const pcb_host_trans_t *tr, rnd_angle_t ang)
{
	long l;
	ang = fmod(ang + tr->rot, 360.0);
	ang *= 10000;
	l = floor(ang);
	return murmurhash(&l, sizeof(l));
}

RND_INLINE unsigned pcb_hash_scale(rnd_angle_t ang)
{
	long l;
	ang *= 100000.0;
	l = floor(ang);
	return murmurhash(&l, sizeof(l));
}

#define pcb_hash_str(s) ((s) == NULL ? 0 : strhash(s))

/* compare two fields and return 0 if they are equal */
#define pcb_field_neq(s1, s2, f) ((s1)->f != (s2)->f)

/* retruns if two sets of tr;x;y mismatches */
RND_INLINE rnd_bool pcb_neq_tr_coords(const pcb_host_trans_t *tr1, rnd_coord_t x1, rnd_coord_t y1, const pcb_host_trans_t *tr2, rnd_coord_t x2, rnd_coord_t y2)
{
	pcb_hash_tr_coords(tr1, &x1, &y1, x1, y1);
	pcb_hash_tr_coords(tr2, &x2, &y2, x2, y2);
	if (x1 != x2) return rnd_true;
	if (y1 != y2) return rnd_true;
	return rnd_false;
}

