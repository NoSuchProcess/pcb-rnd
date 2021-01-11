/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  delayed creation of objects
 *  pcb-rnd Copyright (C) 2021 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 PET Fund in 2021)
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

#include <genht/hash.h>
#include <librnd/core/error.h>
#include <librnd/core/compat_misc.h>

#include "obj_subc.h"
#include "obj_text.h"
#include "obj_line.h"
#include "obj_arc.h"
#include "obj_pstk.h"

#include "delay_create.h"

#define DEBUG_LOC_ATTR 1

#define CRDX(x) (x)
#define CRDX_BRD(x) (x)
#define CRDY(y) (dlcr->flip_y ? ((subc == NULL) ? (dlcr->board_bbox.Y2 - (y)) : (subc->bbox_naked.Y2 - (y))) : (y))
#define CRDY_BRD(y) (dlcr->flip_y ? ((dlcr->board_bbox.Y2 - (y))) : (y))


void pcb_dlcr_init(pcb_dlcr_t *dlcr)
{
	memset(dlcr, 0, sizeof(pcb_dlcr_t));
	htsp_init(&dlcr->name2layer, strhash, strkeyeq);
	htsp_init(&dlcr->name2subc, strhash, strkeyeq);
}

void pcb_dlcr_uninit(pcb_dlcr_t *dlcr)
{
	TODO("free everything");
}

void pcb_dlcr_layer_reg(pcb_dlcr_t *dlcr, pcb_dlcr_layer_t *layer)
{
	pcb_dlcr_layer_t **p;

	if (layer->id < 0)
		layer->id = dlcr->id2layer.used;

	htsp_set(&dlcr->name2layer, layer->name, layer);
	p = (pcb_dlcr_layer_t **)vtp0_get(&dlcr->id2layer, layer->id, 1);
	*p = layer;
}


void pcb_dlcr_layer_free(pcb_dlcr_layer_t *layer)
{
	free(layer->name);
	free(layer);
}

static void pcb_dlcr_create_layer(pcb_board_t *pcb, pcb_dlcr_t *dlcr, pcb_dlcr_layer_t *l)
{
	rnd_layer_id_t lid;
	pcb_layergrp_t *g = pcb_get_grp_new_raw(pcb, 0);
	g->ltype = l->lyt;
	g->name = l->name; l->name = NULL;
	lid = pcb_layer_create(pcb, g - pcb->LayerGroups.grp, g->name, 0);
	l->ly = pcb_get_layer(pcb->Data, lid);
}

static void pcb_dlcr_create_lyt_layer(pcb_board_t *pcb, pcb_dlcr_t *dlcr, pcb_layer_type_t lyt)
{
	rnd_layergrp_id_t n;
	for(n = 0; n < dlcr->id2layer.used; n++) {
		pcb_dlcr_layer_t *l = dlcr->id2layer.array[n];
		if ((l != NULL) && (l->lyt == lyt))
			pcb_dlcr_create_layer(pcb, dlcr, l);
	}
}


static void pcb_dlcr_create_layers(pcb_board_t *pcb, pcb_dlcr_t *dlcr)
{
	rnd_layergrp_id_t n;

	pcb_dlcr_create_lyt_layer(pcb, dlcr, PCB_LYT_TOP | PCB_LYT_PASTE);
	pcb_dlcr_create_lyt_layer(pcb, dlcr, PCB_LYT_TOP | PCB_LYT_SILK);
	pcb_dlcr_create_lyt_layer(pcb, dlcr, PCB_LYT_TOP | PCB_LYT_MASK);

	/* create copper layers, assuming they are in order */
	for(n = 0; n < dlcr->id2layer.used; n++) {
		pcb_dlcr_layer_t *l = dlcr->id2layer.array[n];
		if ((l != NULL) && (l->lyt & PCB_LYT_COPPER))
			pcb_dlcr_create_layer(pcb, dlcr, l);
	}

	pcb_dlcr_create_lyt_layer(pcb, dlcr, PCB_LYT_BOTTOM | PCB_LYT_MASK);
	pcb_dlcr_create_lyt_layer(pcb, dlcr, PCB_LYT_BOTTOM | PCB_LYT_SILK);
	pcb_dlcr_create_lyt_layer(pcb, dlcr, PCB_LYT_BOTTOM | PCB_LYT_PASTE);

	/* create doc/mech layers */
	for(n = 0; n < dlcr->id2layer.used; n++) {
		pcb_dlcr_layer_t *l = dlcr->id2layer.array[n];
		if ((l != NULL) && ((l->lyt & PCB_LYT_DOC) || (l->lyt & PCB_LYT_MECH) || (l->lyt & PCB_LYT_BOUNDARY)))
			pcb_dlcr_create_layer(pcb, dlcr, l);
	}
}

pcb_subc_t *pcb_dlcr_subc_new_in_lib(pcb_dlcr_t *dlcr, const char *name)
{
	pcb_subc_t *subc = htsp_get(&dlcr->name2subc, name);
	if (subc != NULL) {
		rnd_message(RND_MSG_ERROR, "pcb_dlcr_subc_new(): '%s' is already in use\n", name);
		return NULL;
	}

	subc = pcb_subc_new();
	htsp_set(&dlcr->name2subc, rnd_strdup(name), subc);
	return subc;
}

static pcb_dlcr_draw_t *dlcr_new(pcb_dlcr_t *dlcr, pcb_dlcr_type_t type)
{
	pcb_dlcr_draw_t *obj = calloc(sizeof(pcb_dlcr_draw_t), 1);
	obj->type = type;
	switch(type) {
		case DLCR_OBJ: obj->val.obj.layer_id = PCB_DLCR_INVALID_LAYER_ID; break;
		default:;
	}
	gdl_append(&dlcr->drawing, obj, link);
	return obj;
}

pcb_dlcr_draw_t *pcb_dlcr_line_new(pcb_dlcr_t *dlcr, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t width, rnd_coord_t clearance)
{
	pcb_dlcr_draw_t *obj = dlcr_new(dlcr, DLCR_OBJ);
	pcb_line_t *l = &obj->val.obj.obj.line;

	l->type = PCB_OBJ_LINE;
	l->Thickness = width;
	l->Clearance = clearance;
	l->Point1.X = x1;
	l->Point1.Y = y1;
	l->Point2.X = x2;
	l->Point2.Y = y2;
	pcb_line_bbox(l);
	if (dlcr->subc_begin != NULL)
		rnd_box_bump_box(&dlcr->subc_begin->val.subc_begin.subc->bbox_naked, &l->bbox_naked);
	else
		rnd_box_bump_box(&dlcr->board_bbox, &l->bbox_naked);
	return obj;
}

pcb_dlcr_draw_t *pcb_dlcr_arc_new(pcb_dlcr_t *dlcr, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t r, double start_deg, double delta_deg, rnd_coord_t width, rnd_coord_t clearance)
{
	pcb_dlcr_draw_t *obj = dlcr_new(dlcr, DLCR_OBJ);
	pcb_arc_t *a = &obj->val.obj.obj.arc;

	a->type = PCB_OBJ_ARC;
	a->Thickness = width;
	a->Clearance = clearance;
	a->Width = a->Height = r;
	a->X = cx;
	a->Y = cy;
	a->StartAngle = start_deg;
	a->Delta = delta_deg;
	pcb_arc_bbox(a);
	if (dlcr->subc_begin != NULL) {
rnd_trace("bbox subc: %p\n", dlcr->subc_begin->val.subc_begin.subc);
		rnd_box_bump_box(&dlcr->subc_begin->val.subc_begin.subc->bbox_naked, &a->bbox_naked);
	}
	else
		rnd_box_bump_box(&dlcr->board_bbox, &a->bbox_naked);
	return obj;
}

pcb_dlcr_draw_t *pcb_dlcr_text_new(pcb_dlcr_t *dlcr, rnd_coord_t x, rnd_coord_t y, double rot, int scale, rnd_coord_t thickness, const char *str)
{
	pcb_dlcr_draw_t *obj = dlcr_new(dlcr, DLCR_OBJ);
	pcb_text_t *t = &obj->val.obj.obj.text;

	t->type = PCB_OBJ_TEXT;
	t->thickness = thickness;
	t->X = x;
	t->Y = y;
	t->rot = rot;
	t->Scale = scale;
	t->TextString = rnd_strdup(str);
	pcb_text_bbox(pcb_font(PCB, 0, 1), t);
	if (dlcr->subc_begin != NULL)
		rnd_box_bump_box(&dlcr->subc_begin->val.subc_begin.subc->bbox_naked, &t->bbox_naked);
	else
		rnd_box_bump_box(&dlcr->board_bbox, &t->bbox_naked);
	return obj;
}

pcb_dlcr_draw_t *pcb_dlcr_via_new(pcb_dlcr_t *dlcr, rnd_coord_t x, rnd_coord_t y, rnd_coord_t clearance, long id, const char *name)
{
	pcb_dlcr_draw_t *obj;
	pcb_pstk_t *p;
	rnd_cardinal_t i, pid = -1;
	pcb_data_t *data = (dlcr->subc_begin != NULL) ? dlcr->subc_begin->val.subc_begin.subc->data : &dlcr->pstks;

	if (id >= 0) {
		if (id < data->ps_protos.used)
			pid = id;
	}
	else if (name != NULL) {
		for(i = 0; i < data->ps_protos.used; i++) {
			const char *pname = data->ps_protos.array[i].name;
			if ((pname != NULL) && (strcmp(pname, name) == 0)) {
				pid = i;
				break;
			}
		}
	}

	if (pid == -1) {
		rnd_message(RND_MSG_ERROR, "pcb_dlcr_via_new(): padstack prototype not found: '%s'/%ld\n", name, id);
		return NULL; /* found by neither id nor name */
	}

	obj = dlcr_new(dlcr, DLCR_OBJ);
	p = &obj->val.obj.obj.pstk;
	p->type = PCB_OBJ_PSTK;
	p->x = x;
	p->y = y;
	p->proto = pid;
	p->Clearance = clearance;

TODO("why does this fail?");
#if 0
	pcb_pstk_bbox(p);
	if (dlcr->subc_begin != NULL)
		rnd_box_bump_box(&dlcr->subc_begin->val.subc_begin.bbox, &p->bbox_naked);
	else
		rnd_box_bump_box(&dlcr->board_bbox, &p->bbox_naked);
#endif
	return obj;
}

pcb_dlcr_draw_t *pcb_dlcr_subc_new_from_lib(pcb_dlcr_t *dlcr, rnd_coord_t x, rnd_coord_t y, double rot, int on_bottom, const char *names, long names_len)
{
	pcb_dlcr_draw_t *obj = dlcr_new(dlcr, DLCR_SUBC_FROM_LIB);

	obj->val.subc_from_lib.x = x;
	obj->val.subc_from_lib.y = y;
	obj->val.subc_from_lib.rot = rot;
	obj->val.subc_from_lib.on_bottom = on_bottom;
	obj->val.subc_from_lib.names = malloc(names_len+1);
	memcpy(obj->val.subc_from_lib.names, names, names_len);
	obj->val.subc_from_lib.names[names_len] = '\0'; /* add an extra \0, just in case; this makes the call easier with a single name which can be passed as a simple \0 terminated string */

	/* can not update the bbox as we may not yet have the subc in lib */

	return obj;
}

pcb_pstk_proto_t *pcb_dlcr_pstk_proto_new(pcb_dlcr_t *dlcr, long int *pid_out)
{
	pcb_pstk_proto_t *proto;
	pcb_data_t *data = (dlcr->subc_begin != NULL) ? dlcr->subc_begin->val.subc_begin.subc->data : &dlcr->pstks;
	if (pid_out != NULL)
		*pid_out = data->ps_protos.used;
	proto = pcb_vtpadstack_proto_alloc_append(&data->ps_protos, 1);
	proto->in_use = 1;
	return proto;
}

void pcb_dlcr_subc_begin(pcb_dlcr_t *dlcr, pcb_subc_t *subc)
{
	assert(dlcr->subc_begin == NULL);
	dlcr->subc_begin = dlcr_new(dlcr, DLCR_SUBC_BEGIN);
	dlcr->subc_begin->val.subc_begin.subc = subc;
rnd_trace("subc begin: %p\n", subc);
}

void pcb_dlcr_subc_end(pcb_dlcr_t *dlcr)
{
	assert(dlcr->subc_begin != NULL);
	dlcr_new(dlcr, DLCR_SUBC_END);
	dlcr->subc_begin = NULL;
}

/* Look up a board layer using object's layer id or layer name */
static pcb_layer_t *pcb_dlcr_lookup_board_layer(pcb_board_t *pcb, pcb_dlcr_t *dlcr, pcb_dlcr_draw_t *obj, int *specd)
{
	pcb_layer_t *ly = NULL;

	*specd = 0;
	if (obj->val.obj.layer_id != PCB_DLCR_INVALID_LAYER_ID) {
		pcb_dlcr_layer_t **dlp = (pcb_dlcr_layer_t **)vtp0_get(&dlcr->id2layer, obj->val.obj.layer_id, 0);
		*specd = 1;
		if ((dlp == NULL) || (*dlp == NULL)) {
			pcb_dlcr_layer_t *dl;
			rnd_message(RND_MSG_ERROR, "delay create: invalid layer id %ld (loc: %ld); creating a dummy doc layer\n", obj->val.obj.layer_id, obj->loc_line);
			dl = calloc(sizeof(pcb_dlcr_layer_t), 1);
			dl->id = obj->val.obj.layer_id;
			dl->user_data = &dl->local_user_data;
			dl->name = rnd_strdup("UNASSIGNED");
			dl->lyt = PCB_LYT_DOC;
			dl->purpose = "unassigned";
			pcb_dlcr_layer_reg(dlcr, dl);
rnd_trace("Layer create: unassigned for %ld\n", obj->val.obj.layer_id);
			pcb_dlcr_create_layer(pcb, dlcr, dl);
		}
		else
			ly = (*dlp)->ly;
	}
	else if (obj->val.obj.lyt != 0) {
		long n;
		for(n = 0; n < dlcr->id2layer.used; n++) {
			pcb_dlcr_layer_t *dl = dlcr->id2layer.array[n];
			if ((dl != NULL) && (dl->lyt == obj->val.obj.lyt)) {
				ly = dl->ly;
				break;
			}
		}
		if (ly == NULL) {
			rnd_message(RND_MSG_ERROR, "delay create: invalid layer type '%x' (loc: %ld)\n", obj->val.obj.lyt, obj->loc_line);
			return NULL;
		}
	}
	if ((ly == NULL) && (obj->val.obj.layer_name != NULL)) {
		pcb_dlcr_layer_t *dl = htsp_get(&dlcr->name2layer, obj->val.obj.layer_name);
		*specd = 1;
		if (dl != NULL)
			ly = dl->ly;
		if (ly == NULL)
			rnd_message(RND_MSG_ERROR, "delay create: invalid layer name '%s' (loc: %ld)\n", obj->val.obj.layer_name, obj->loc_line);
	}

	return ly;
}

static void pcb_dlcr_draw_free_obj(pcb_board_t *pcb, pcb_subc_t *subc, pcb_dlcr_t *dlcr, pcb_dlcr_draw_t *obj)
{
	pcb_data_t *data = (subc != NULL) ? subc->data : pcb->Data;
	pcb_any_obj_t *r;
	pcb_line_t *l = &obj->val.obj.obj.line;
	pcb_arc_t *a = &obj->val.obj.obj.arc;
	pcb_text_t *t = &obj->val.obj.obj.text;
	pcb_pstk_t *p = &obj->val.obj.obj.pstk;
	pcb_layer_t *ly;
	int specd;

	/* retrieve target layer for layer objects */
	if (obj->val.obj.obj.any.type != PCB_OBJ_PSTK) {
		if (subc != NULL) {
			TODO("create objects on a subc layer");
			return;
		}
		else
			ly = pcb_dlcr_lookup_board_layer(pcb, dlcr, obj, &specd);

		if (ly == NULL) {
			if (!specd)
				rnd_message(RND_MSG_ERROR, "delay create: layer not specified (loc: %ld)\n", obj->loc_line);
			return;
		}
	}

	switch(obj->val.obj.obj.any.type) {
		case PCB_OBJ_LINE:
			r = (pcb_any_obj_t *)pcb_line_new(ly, CRDX(l->Point1.X), CRDY(l->Point1.Y), CRDX(l->Point2.X), CRDY(l->Point2.Y), l->Thickness, l->Clearance, pcb_flag_make(PCB_FLAG_CLEARLINE));
			break;
		case PCB_OBJ_ARC:
			r = (pcb_any_obj_t *)pcb_arc_new(ly, CRDX(a->X), CRDY(a->Y), a->Width, a->Height, a->StartAngle, a->Delta, a->Thickness, a->Clearance, pcb_flag_make(PCB_FLAG_CLEARLINE), 0);
			break;
		case PCB_OBJ_TEXT:
			r = (pcb_any_obj_t *)pcb_text_new(ly, pcb_font(pcb, 0, 1), CRDX(t->X), CRDY(t->Y), t->rot, t->Scale, t->thickness, t->TextString, pcb_flag_make(PCB_FLAG_CLEARLINE));
			free(t->TextString);
			break;
		case PCB_OBJ_PSTK:
			r = (pcb_any_obj_t *)pcb_pstk_new(data, 0, p->proto, CRDX(p->x), CRDY(p->y), p->Clearance, pcb_flag_make(PCB_FLAG_CLEARLINE));
			break;
		default:
			break;
	}

#ifdef DEBUG_LOC_ATTR
	{
		char tmp[32];
		sprintf(tmp, "%ld", obj->loc_line);
		pcb_attribute_set(pcb, &r->Attributes, "io_pads_loc_line", tmp, 0);
	}
#endif
	(void)r;

	free(obj->val.obj.layer_name);
	free(obj);
}

static void pcb_dlcr_fixup_pstk_proto_lyt(pcb_board_t *pcb, pcb_dlcr_t *dlcr, pcb_data_t *dst);

static void pcb_dlcr_draw_subc_from_lib(pcb_board_t *pcb, pcb_dlcr_t *dlcr, pcb_dlcr_draw_t *obj)
{
	rnd_coord_t dx, dy, ox = 0, oy = 0;
	pcb_subc_t *subc, *nsc;
	char *name;

	rnd_trace("*** new from lib:\n");
	for(name = obj->val.subc_from_lib.names; *name != '\0'; name = name + strlen(name) + 1) {
		subc = htsp_get(&dlcr->name2subc, name);
		if (subc != NULL)
			break;
	}

	if (subc == NULL) {
		rnd_message(RND_MSG_ERROR, "delay create: invalid subc-from-lib: not found by name (loc: %ld); tried:\n", obj->loc_line);
		for(name = obj->val.subc_from_lib.names; *name != '\0'; name = name + strlen(name) + 1)
			rnd_message(RND_MSG_ERROR, " '%s'\n", name);
		return;
	}


	pcb_subc_get_origin(subc, &ox, &oy);
	dx = CRDX_BRD(obj->val.subc_from_lib.x - ox);
	dy = CRDY_BRD(obj->val.subc_from_lib.y - oy);
	nsc = pcb_subc_dup_at(pcb, pcb->Data, subc, dx, dy, 0, 0);
	pcb_dlcr_fixup_pstk_proto_lyt(pcb, dlcr, nsc->data);

	rnd_trace("  using name '%s' %p -> %p\n", name, subc, nsc);

	if (nsc == NULL)
		rnd_message(RND_MSG_ERROR, "pcb_dlcr_draw_subc_from_lib(): failed to dup subc '%s' from lib\n", name);
}

TODO("this is pads-specific, figure how to handle this; see also: TODO#71");
static void proto_layer_lookup(pcb_dlcr_t *dlcr, pcb_pstk_shape_t *shp)
{
	int level = shp->layer_mask;
	switch(level) { /* set layer type */
		case -2: shp->layer_mask = PCB_LYT_TOP | PCB_LYT_COPPER; break;
		case -1: shp->layer_mask = PCB_LYT_INTERN | PCB_LYT_COPPER; break;
		case  0: shp->layer_mask = PCB_LYT_BOTTOM | PCB_LYT_COPPER; break;
		default:
			{
				pcb_dlcr_layer_t *ly, **lyp = vtp0_get(&dlcr->id2layer, level, 0);
				if ((lyp != NULL) && (*lyp != NULL)) {
					ly = *lyp;
					if (!(ly->lyt & PCB_LYT_COPPER)) {
						shp->layer_mask = ly->lyt;
						if (ly->lyt & PCB_LYT_MASK)
							shp->comb = PCB_LYC_SUB;
					}
					else {
						rnd_message(RND_MSG_ERROR, "Padstack prototype can not have a different shape on copper layer %d\n", level);
					}
				}
				else {
					rnd_message(RND_MSG_ERROR, "Padstack prototype references non-existing layer %d\n", level);
				}
			}
		break;
	}
}

static void pcb_dlcr_create_pstk_protos(pcb_board_t *pcb, pcb_dlcr_t *dlcr, pcb_data_t *dst, const pcb_data_t *src)
{
	rnd_cardinal_t n, m;
	for(n = 0; n < src->ps_protos.used; n++) {
		pcb_pstk_proto_t *proto = &src->ps_protos.array[n];
		pcb_pstk_tshape_t *ts = pcb_vtpadstack_tshape_get(&proto->tr, 0, 0);
		int i;
		for(i = 0; i < ts->len; i++) {
			pcb_pstk_shape_t *shp = ts->shape + i;
			proto_layer_lookup(dlcr, shp);
		}
		m = pcb_pstk_proto_insert_forcedup(dst, proto, 0, 0);
		if (n != m) {
			rnd_message(RND_MSG_ERROR, "pcb_dlcr_create_pstk_protos: failed to create padstack prototype\n");
			break;
		}
	}
}

static void pcb_dlcr_fixup_pstk_proto_lyt(pcb_board_t *pcb, pcb_dlcr_t *dlcr, pcb_data_t *dst)
{
	rnd_cardinal_t n, m;
	for(n = 0; n < dst->ps_protos.used; n++) {
		pcb_pstk_proto_t *proto = &dst->ps_protos.array[n];
		pcb_pstk_tshape_t *ts = pcb_vtpadstack_tshape_get(&proto->tr, 0, 0);
		int i;
		for(i = 0; i < ts->len; i++) {
			pcb_pstk_shape_t *shp = ts->shape + i;
			proto_layer_lookup(dlcr, shp);
		}
	}
}


static void pcb_dlcr_create_drawings(pcb_board_t *pcb, pcb_dlcr_t *dlcr)
{
	pcb_dlcr_draw_t *obj;
	pcb_subc_t *subc = NULL;
	while((obj = gdl_first(&dlcr->drawing)) != NULL) {
		gdl_remove(&dlcr->drawing, obj, link);
		switch(obj->type) {
			case DLCR_OBJ: pcb_dlcr_draw_free_obj(pcb, subc, dlcr, obj); break;
			case DLCR_SUBC_BEGIN: subc = obj->val.subc_begin.subc; break;
			case DLCR_SUBC_END: subc = NULL; break;
			case DLCR_SUBC_FROM_LIB: pcb_dlcr_draw_subc_from_lib(pcb, dlcr, obj); break;
		}
	}
}

void pcb_dlcr_create(pcb_board_t *pcb, pcb_dlcr_t *dlcr)
{
	pcb_dlcr_create_layers(pcb, dlcr);
	pcb_dlcr_create_pstk_protos(pcb, dlcr, pcb->Data, &dlcr->pstks);
	pcb_dlcr_create_drawings(pcb, dlcr);
	pcb->hidlib.size_x = dlcr->board_bbox.X2;
	pcb->hidlib.size_y = dlcr->board_bbox.Y2;
}
