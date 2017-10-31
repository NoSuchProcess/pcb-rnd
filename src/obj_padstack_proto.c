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

#include <assert.h>

#include "board.h"
#include "buffer.h"
#include "conf_core.h"
#include "data.h"
#include "data_list.h"
#include "obj_padstack.h"
#include "obj_padstack_inlines.h"
#include "undo.h"

static const char core_proto_cookie[] = "padstack prototypes";

unsigned long pcb_padstack_alloc_group(pcb_data_t *data)
{
	pcb_cardinal_t n;
	unsigned long mg;

	if (data->ps_next_grp > 0)     /* cheap way: answer in cache */
		return data->ps_next_grp++;

	for(n = 0, mg = 0; n < pcb_vtpadstack_proto_len(&data->ps_protos); n++)
		if (data->ps_protos.array[n].group > mg)
			mg = data->ps_protos.array[n].group;

	data->ps_next_grp = mg+1;

	return data->ps_next_grp++;
}


void pcb_padstack_proto_free_fields(pcb_padstack_proto_t *dst)
{
#warning TODO: do a full field free here
	free(dst->shape);
}

void pcb_padstack_shape_alloc_poly(pcb_padstack_poly_t *poly, int len)
{
	poly->x = malloc(sizeof(poly->x[0]) * len * 2);
	poly->y = poly->x + len;
	poly->len = len;
}

void pcb_padstack_shape_copy_poly(pcb_padstack_poly_t *dst, const pcb_padstack_poly_t *src)
{
	memcpy(dst->x, src->x, sizeof(src->x[0]) * src->len * 2);
}


static int pcb_padstack_proto_conv(pcb_data_t *data, pcb_padstack_proto_t *dst, int quiet, vtp0_t *objs, pcb_coord_t ox, pcb_coord_t oy)
{
	int ret = -1, n, m, i;
	pcb_any_obj_t **o;

	dst->in_use = 1;
	dst->shape = NULL;
	dst->hdia = 0;
	dst->htop = dst->hbottom = 0;

	if (vtp0_len(objs) > data->LayerN) {
		if (!quiet)
			pcb_message(PCB_MSG_ERROR, "Padstack conversion: too many objects selected\n");
		goto quit;
	}

	/* allocate shapes */
	dst->len = 0;
	for(n = 0, o = (pcb_any_obj_t **)objs->array; n < vtp0_len(objs); n++,o++) {
		switch((*o)->type) {
			case PCB_OBJ_LINE:
			case PCB_OBJ_POLYGON:
				dst->len++;
				break;
			case PCB_OBJ_VIA:
				if (dst->hdia != 0) {
					if (!quiet)
						pcb_message(PCB_MSG_ERROR, "Padstack conversion: multiple vias\n");
					goto quit;
				}
				dst->hdia = (*(pcb_pin_t **)o)->DrillingHole;
				dst->hplated = !PCB_FLAG_TEST(PCB_FLAG_HOLE, *o);
				if ((ox != (*(pcb_pin_t **)o)->X) || (oy != (*(pcb_pin_t **)o)->Y)) {
					pcb_message(PCB_MSG_INFO, "Padstack conversion: adjusting origin to via hole\n");
					ox = (*(pcb_pin_t **)o)->X;
					oy = (*(pcb_pin_t **)o)->Y;
				}
				break;
			default:;
				if (!quiet)
					pcb_message(PCB_MSG_ERROR, "Padstack conversion: invalid object type (%x) selected; must be via, line or polygon\n", (*o)->type);
				goto quit;
		}
	}
	dst->shape = malloc(dst->len * sizeof(pcb_padstack_shape_t));

	/* convert local (line/poly) objects */
	for(i = 0, n = -1, o = (pcb_any_obj_t **)objs->array; i < vtp0_len(objs); o++,i++) {
		pcb_layer_t *ly;
		switch((*o)->type) {
			case PCB_OBJ_LINE:
				n++;
				dst->shape[n].shape = PCB_PSSH_LINE;
				dst->shape[n].data.line.x1 = (*(pcb_line_t **)o)->Point1.X - ox;
				dst->shape[n].data.line.y1 = (*(pcb_line_t **)o)->Point1.Y - oy;
				dst->shape[n].data.line.x2 = (*(pcb_line_t **)o)->Point2.X - ox;
				dst->shape[n].data.line.y2 = (*(pcb_line_t **)o)->Point2.Y - oy;
				dst->shape[n].data.line.thickness = (*(pcb_line_t **)o)->Thickness;
				dst->shape[n].data.line.square = 0;
				dst->shape[n].clearance = (*(pcb_line_t **)o)->Clearance;
				break;
			case PCB_OBJ_POLYGON:
				{
					pcb_poly_it_t it;
					unsigned long int len;
					int p;
					pcb_coord_t x, y;
					int go;

					n++;
					pcb_poly_island_first((*(pcb_poly_t **)o), &it);
					pcb_poly_contour(&it);
					for(go = pcb_poly_vect_first(&it, &x, &y), len = 0; go; go = pcb_poly_vect_next(&it, &x, &y))
						len++;
					if (len >= (1L << (sizeof(int)-1))) {
						if (!quiet)
							pcb_message(PCB_MSG_ERROR, "Padstack conversion: polygon has too many points\n");
						goto quit;
					}
					pcb_padstack_shape_alloc_poly(&dst->shape[n].data.poly, len);
					for(go = pcb_poly_vect_first(&it, &x, &y), p = 0; go; go = pcb_poly_vect_next(&it, &x, &y), p++) {
						dst->shape[n].data.poly.x[p] = x - ox;
						dst->shape[n].data.poly.y[p] = y - oy;
					}

					dst->shape[n].shape = PCB_PSSH_POLY;
					dst->shape[n].clearance = (*(pcb_poly_t **)o)->Clearance;
				}
				break;
			default: continue;
		}
		assert((*o)->parent_type == PCB_PARENT_LAYER);
		ly = (*o)->parent.layer;
		dst->shape[n].layer_mask = pcb_layer_flags_(ly);
		dst->shape[n].comb = ly->comb;
		for(m = 0; m < n; m++) {
			if ((dst->shape[n].layer_mask == dst->shape[m].layer_mask) && (dst->shape[n].comb == dst->shape[m].comb)) {
				if (!quiet)
					pcb_message(PCB_MSG_ERROR, "Padstack conversion: multiple objects on the same layer\n");
				goto quit;
			}
		}
	}

	/* all went fine */
	dst->hash = pcb_padstack_hash(dst);
	ret = 0;

	quit:;
	if (ret == 0)
		dst->group = pcb_padstack_alloc_group(data);
	else
		pcb_padstack_proto_free_fields(dst);
	return ret;
}

int pcb_padstack_proto_conv_selection(pcb_board_t *pcb, pcb_padstack_proto_t *dst, int quiet, pcb_coord_t ox, pcb_coord_t oy)
{
	int ret;
	vtp0_t objs;

	vtp0_init(&objs);
	pcb_data_list_by_flag(pcb->Data, &objs, PCB_OBJ_CLASS_REAL, PCB_FLAG_SELECTED);
	ret = pcb_padstack_proto_conv(pcb->Data, dst, quiet, &objs, ox, oy);
	vtp0_uninit(&objs);

	return ret;
}


int pcb_padstack_proto_conv_buffer(pcb_padstack_proto_t *dst, int quiet)
{
	int ret;
	vtp0_t objs;
	pcb_coord_t ox, oy;
	pcb_box_t bb;

	pcb_data_bbox(&bb, PCB_PASTEBUFFER->Data, 0);

	ox = (bb.X1 + bb.X2) / 2;
	oy = (bb.Y1 + bb.Y2) / 2;

	vtp0_init(&objs);
	pcb_data_list_by_flag(PCB_PASTEBUFFER->Data, &objs, PCB_OBJ_CLASS_REAL, PCB_FLAGS);
	ret = pcb_padstack_proto_conv(PCB_PASTEBUFFER->Data, dst, quiet, &objs, ox, oy);
	vtp0_uninit(&objs);

	return ret;
}

void pcb_padstack_proto_copy(pcb_padstack_proto_t *dst, const pcb_padstack_proto_t *src)
{
	int n;
	memcpy(dst, src, sizeof(pcb_padstack_proto_t));
	dst->shape = malloc(sizeof(pcb_padstack_shape_t) * src->len);
	memcpy(dst->shape, src->shape, sizeof(pcb_padstack_shape_t) * src->len);
	for(n = 0; n < src->len; n++) {
		switch(src->shape[n].shape) {
			case PCB_PSSH_LINE:
			case PCB_PSSH_CIRC:
				break; /* do nothing, all fields are copied already by the memcpy */
			case PCB_PSSH_POLY:
				pcb_padstack_shape_alloc_poly(&dst->shape[n].data.poly, src->shape[n].data.poly.len);
				pcb_padstack_shape_copy_poly(&dst->shape[n].data.poly, &src->shape[n].data.poly);
				break;
		}
	}
	dst->in_use = 1;
}


/* Matches proto against all protos in data's cache; returns
   PCB_PADSTACK_INVALID (and loads first_free_out) if not found */
static pcb_cardinal_t pcb_padstack_proto_insert_try(pcb_data_t *data, const pcb_padstack_proto_t *proto, pcb_cardinal_t *first_free_out)
{
	pcb_cardinal_t n, first_free = PCB_PADSTACK_INVALID;

	/* look for the first existing padstack that matches */
	for(n = 0; n < pcb_vtpadstack_proto_len(&data->ps_protos); n++) {
		if (!(data->ps_protos.array[n].in_use)) {
			if (first_free == PCB_PADSTACK_INVALID)
				first_free = n;
		}
		else if (data->ps_protos.array[n].hash == proto->hash) {
			if (pcb_padstack_eq(&data->ps_protos.array[n], proto))
				return n;
		}
	}
	*first_free_out = first_free;
	return PCB_PADSTACK_INVALID;
}

pcb_cardinal_t pcb_padstack_proto_insert_or_free(pcb_data_t *data, pcb_padstack_proto_t *proto, int quiet)
{
	pcb_cardinal_t n, first_free;

	n = pcb_padstack_proto_insert_try(data, proto, &first_free);
	if (n != PCB_PADSTACK_INVALID) {
		pcb_padstack_proto_free_fields(proto);
		return n; /* already in cache */
	}

	/* no match, have to register a new one */
	if (first_free == PCB_PADSTACK_INVALID) {
		n = pcb_vtpadstack_proto_len(&data->ps_protos);
		pcb_vtpadstack_proto_append(&data->ps_protos, *proto);
	}
	else {
		memcpy(data->ps_protos.array+first_free, proto, sizeof(pcb_padstack_proto_t));
		data->ps_protos.array[first_free].in_use = 1;
	}
	memset(proto, 0, sizeof(pcb_padstack_proto_t)); /* make sure a subsequent free() won't do any harm */
	return n;
}

pcb_cardinal_t pcb_padstack_proto_insert_dup(pcb_data_t *data, const pcb_padstack_proto_t *proto, int quiet)
{
	pcb_cardinal_t n, first_free;

	n = pcb_padstack_proto_insert_try(data, proto, &first_free);
	if (n != PCB_PADSTACK_INVALID)
		return n; /* already in cache */

	/* no match, have to register a new one, which is a dup of the original */
	if (first_free == PCB_PADSTACK_INVALID) {
		pcb_padstack_proto_t *nproto;
		n = pcb_vtpadstack_proto_len(&data->ps_protos);
		nproto = pcb_vtpadstack_proto_alloc_append(&data->ps_protos, 1);
		pcb_padstack_proto_copy(nproto, proto);
		nproto->parent = data;
	}
	else {
		pcb_padstack_proto_copy(data->ps_protos.array+first_free, proto);
		data->ps_protos.array[first_free].in_use = 1;
		data->ps_protos.array[first_free].parent = data;
	}
	return n;
}

pcb_cardinal_t pcb_padstack_conv_selection(pcb_board_t *pcb, int quiet, pcb_coord_t ox, pcb_coord_t oy)
{
	pcb_padstack_proto_t proto;

	if (pcb_padstack_proto_conv_selection(pcb, &proto, quiet, ox, oy) != 0)
		return -1;

	return pcb_padstack_proto_insert_or_free(pcb->Data, &proto, quiet);
}

pcb_cardinal_t pcb_padstack_conv_buffer(int quiet)
{
	pcb_padstack_proto_t proto;

	if (pcb_padstack_proto_conv_buffer(&proto, quiet) != 0)
		return -1;

	return pcb_padstack_proto_insert_or_free(PCB_PASTEBUFFER->Data, &proto, quiet);
}


void pcb_padstack_shape_update_pa(pcb_padstack_poly_t *poly)
{
	int n;
	pcb_vector_t v;
	pcb_pline_t *pl;

	v[0] = poly->x[0]; v[1] = poly->y[0];
	pl = pcb_poly_contour_new(v);
	for(n = 1; n < poly->len; n++) {
		v[0] = poly->x[n]; v[1] = poly->y[n];
		pcb_poly_vertex_include(pl->head.prev, pcb_poly_node_create(v));
	}
	pcb_poly_contour_pre(pl, 1);

	poly->pa = pcb_polyarea_create();
	pcb_polyarea_contour_include(poly->pa, pl);
}

/*** Undoable hole change ***/

typedef struct {
	long int parent_ID; /* -1 for pcb, positive for a subc */
	pcb_cardinal_t proto;

	int hplated;
	pcb_coord_t hdia;
	int htop, hbottom;
} padstack_proto_change_hole_t;

#define swap(a,b,type) \
	do { \
		type tmp = a; \
		a = b; \
		b = tmp; \
	} while(0)

static int undo_change_hole_swap(void *udata)
{
	padstack_proto_change_hole_t *u = udata;
	pcb_data_t *data;
	pcb_padstack_proto_t *proto;

	if (u->parent_ID != -1) {
		pcb_subc_t *subc = pcb_subc_by_id(PCB->Data, u->parent_ID);
		if (subc == NULL) {
			pcb_message(PCB_MSG_ERROR, "Can't undo padstack prototype hole change: parent subc #%ld is not found\n", u->parent_ID);
			return -1;
		}
		data = subc->data;
	}
	else
		data = PCB->Data;

	proto = pcb_padstack_get_proto_(data, u->proto);
	if (proto == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't undo padstack prototype hole change: proto ID #%ld is not available\n", u->parent_ID);
		return -1;
	}

	swap(proto->hplated, u->hplated, int);
	swap(proto->hdia,    u->hdia,    pcb_coord_t);
	swap(proto->htop,    u->htop,    int);
	swap(proto->hbottom, u->hbottom, int);
	return 0;
}

static void undo_change_hole_print(void *udata, char *dst, size_t dst_len)
{
	padstack_proto_change_hole_t *u = udata;
	pcb_snprintf(dst, dst_len, "padstack proto hole change: plated=%d dia=%$mm top=%d bottom=%d\n", u->hplated, u->hdia, u->htop, u->hbottom);
}

static const uundo_oper_t undo_padstack_proto_change_hole = {
	core_proto_cookie,
	NULL, /* free */
	undo_change_hole_swap,
	undo_change_hole_swap,
	undo_change_hole_print
};

int pcb_padstack_proto_change_hole(pcb_padstack_proto_t *proto, const int *hplated, const pcb_coord_t *hdia, const int *htop, const int *hbottom)
{
	padstack_proto_change_hole_t *u;
	long int parent_ID;

	switch(proto->parent->parent_type) {
		case PCB_PARENT_BOARD: parent_ID = -1; break;
		case PCB_PARENT_SUBC: parent_ID = proto->parent->parent.subc->ID; break;
		default: return -1;
	}

	u = pcb_undo_alloc(PCB, &undo_padstack_proto_change_hole, sizeof(padstack_proto_change_hole_t));
	u->parent_ID = parent_ID;
	u->proto = pcb_padstack_get_proto_id(proto);
	u->hplated = hplated ? *hplated : proto->hplated;
	u->hdia = hdia ? *hdia : proto->hdia;
	u->htop = htop ? *htop : proto->htop;
	u->hbottom = hbottom ? *hbottom : proto->hbottom;
	undo_change_hole_swap(u);

	pcb_undo_inc_serial();
	return 0;
}

/*** hash ***/
static unsigned int pcb_padstack_shape_hash(const pcb_padstack_shape_t *sh)
{
	unsigned int n, ret = murmurhash32(sh->layer_mask) ^ murmurhash32(sh->comb) ^ pcb_hash_coord(sh->clearance);

	switch(sh->shape) {
		case PCB_PSSH_POLY:
			for(n = 0; n < sh->data.poly.len; n++)
				ret ^= pcb_hash_coord(sh->data.poly.x[n]) ^ pcb_hash_coord(sh->data.poly.y[n]);
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
	unsigned int n, ret = pcb_hash_coord(p->hdia) ^ pcb_hash_coord(p->htop) ^ pcb_hash_coord(p->hbottom) ^ pcb_hash_coord(p->hplated) ^ pcb_hash_coord(p->len);
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
				if (sh1->data.poly.x[n] != sh2->data.poly.x[n]) return 0;
				if (sh1->data.poly.y[n] != sh2->data.poly.y[n]) return 0;
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
	if (p1->hplated != p2->hplated) return 0;
	if (p1->len != p2->len) return 0;

	for(n1 = 0; n1 < p1->len; n1++) {
		for(n2 = 0; n2 < p2->len; n2++)
			if (pcb_padstack_shape_eq(p1->shape + n1, p2->shape + n2))
				goto found;
		return 0;
		found:;
	}

	return 1;
}

