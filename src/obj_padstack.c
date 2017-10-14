/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"

#include "obj_padstack.h"


/*** hash ***/
static unsigned int pcb_padstack_shape_hash(const pcb_padstack_shape_t *sh)
{
	unsigned int n, ret = murmurhash32(sh->layer_mask) ^ murmurhash32(sh->comb) ^ pcb_hash_coord(sh->clearance);

	switch(sh->shape) {
		case PCB_PSSH_POLY:
			for(n = 0; n < sh->data.poly.len; n++)
				ret ^= pcb_hash_coord(sh->data.poly.pt[n].X) ^ pcb_hash_coord(sh->data.poly.pt[n].Y);
			break;
		case PCB_PSSH_LINE:
			ret ^= pcb_hash_coord(sh->data.line.x1) ^ pcb_hash_coord(sh->data.line.x2) ^ pcb_hash_coord(sh->data.line.y1) ^ pcb_hash_coord(sh->data.line.y2);
			ret ^= pcb_hash_coord(sh->data.line.thickness);
			ret ^= sh->data.line.square;
			break;
		case PCB_PSSH_CIRC:
			ret ^= pcb_hash_coord(sh->data.circ.x) ^ pcb_hash_coord(sh->data.circ.y);
			ret ^= pcb_hash_coord(sh->data.circ.dia);
			break;
	}
	return ret;
}

unsigned int pcb_padstack_hash(const pcb_padstack_proto_t *p)
{
	unsigned int n, ret = pcb_hash_coord(p->hdia) ^ pcb_hash_coord(p->htop) ^ pcb_hash_coord(p->hbottom) ^ pcb_hash_coord(p->len);
	for(n = 0; n < p->len; n++)
		ret ^= pcb_padstack_shape_hash(p->shape + n);
	return ret;
}

static int pcb_padstack_shape_eq(const pcb_padstack_shape_t *sh1, const pcb_padstack_shape_t *sh2)
{
	int n;

	if (sh1->layer_mask != sh2->layer_mask) return 0;
	if (sh1->comb != sh2->comb) return 0;
	if (sh1->clearance != sh2->clearance) return 0;
	if (sh1->shape != sh2->shape) return 0;

	switch(sh1->shape) {
		case PCB_PSSH_POLY:
			if (sh1->data.poly.len != sh2->data.poly.len) return 0;
			for(n = 0; n < sh1->data.poly.len; n++) {
				if (sh1->data.poly.pt[n].X != sh2->data.poly.pt[n].X) return 0;
				if (sh1->data.poly.pt[n].Y != sh2->data.poly.pt[n].Y) return 0;
			}
			break;
		case PCB_PSSH_LINE:
			if (sh1->data.line.x1 != sh2->data.line.x1) return 0;
			if (sh1->data.line.x2 != sh2->data.line.x2) return 0;
			if (sh1->data.line.y1 != sh2->data.line.y1) return 0;
			if (sh1->data.line.y2 != sh2->data.line.y2) return 0;
			if (sh1->data.line.thickness != sh2->data.line.thickness) return 0;
			if (sh1->data.line.square != sh2->data.line.square) return 0;
			break;
		case PCB_PSSH_CIRC:
			if (sh1->data.circ.x != sh2->data.circ.x) return 0;
			if (sh1->data.circ.y != sh2->data.circ.y) return 0;
			if (sh1->data.circ.dia != sh2->data.circ.dia) return 0;
			break;
	}

	return 1;
}

int pcb_padstack_eq(const pcb_padstack_proto_t *p1, const pcb_padstack_proto_t *p2)
{
	int n1, n2;

	if (p1->hdia != p2->hdia) return 0;
	if (p1->htop != p2->htop) return 0;
	if (p1->hbottom != p2->hbottom) return 0;
	if (p1->len != p2->len) return 0;

	for(n1 = 0; n1 < p1->len; n1++) {
		for(n2 = 0; n1 < p2->len; n2++)
			if (pcb_padstack_shape_eq(p1->shape + n1, p2->shape + n2))
				goto found;
		return 0;
		found:;
	}

	return 1;
}

