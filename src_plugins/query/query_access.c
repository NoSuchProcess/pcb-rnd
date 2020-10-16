/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2020 Tibor 'Igor2' Palinkas
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

/* Query language - access to / extract core data */

#include "config.h"
#include <librnd/core/math_helper.h>
#include <librnd/core/compat_misc.h>
#include "board.h"
#include "data.h"
#include "query_access.h"
#include "query_exec.h"
#include "layer.h"
#include "netlist.h"
#include "fields_sphash.h"
#include "obj_pstk_inlines.h"
#include "obj_subc_parent.h"
#include "net_int.h"

#define APPEND(_lst_, _obj_) vtp0_append((vtp0_t *)_lst_, _obj_)

static int list_layer_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, int enter)
{
	if (enter)
		APPEND(ctx, layer);
	return 0;
}

static void list_line_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_line_t *line)
{
	APPEND(ctx, line);
}

static void list_arc_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_arc_t *arc)
{
	APPEND(ctx, arc);
}

static void list_text_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_text_t *text)
{
	APPEND(ctx, text);
}

static void list_poly_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_poly_t *poly)
{
	APPEND(ctx, poly);
}

static void list_pstk_cb(void *ctx, pcb_board_t *pcb, pcb_pstk_t *ps)
{
	APPEND(ctx, ps);
}

static void list_gfx_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_gfx_t *gfx)
{
	APPEND(ctx, gfx);
}

static int list_subc_cb(void *ctx, pcb_board_t *pcb, pcb_subc_t *subc, int enter);

static int list_data(void *ctx, pcb_board_t *pcb, pcb_data_t *data, int enter, pcb_objtype_t mask)
{
	if (mask & PCB_OBJ_SUBC) {
		PCB_SUBC_LOOP(data);
		{
			list_subc_cb(ctx, pcb, subc, 1);
		}
		PCB_END_LOOP;
	}

	if (mask & PCB_OBJ_PSTK) {
		PCB_PADSTACK_LOOP(data);
		{
			list_pstk_cb(ctx, pcb, padstack);
		}
		PCB_END_LOOP;
	}

	if (mask & PCB_OBJ_ARC) {
		PCB_ARC_ALL_LOOP(data);
		{
			list_arc_cb(ctx, pcb, layer, arc);
		}
		PCB_ENDALL_LOOP;
	}

	if (mask & PCB_OBJ_GFX) {
		PCB_GFX_ALL_LOOP(data);
		{
			list_gfx_cb(ctx, pcb, layer, gfx);
		}
		PCB_ENDALL_LOOP;
	}

	if (mask & PCB_OBJ_LINE) {
		PCB_LINE_ALL_LOOP(data);
		{
			list_line_cb(ctx, pcb, layer, line);
		}
		PCB_ENDALL_LOOP;
	}

	if (mask & PCB_OBJ_TEXT) {
		PCB_TEXT_ALL_LOOP(data);
		{
			list_text_cb(ctx, pcb, layer, text);
		}
		PCB_ENDALL_LOOP;
	}

	if (mask & PCB_OBJ_POLY) {
		PCB_POLY_ALL_LOOP(data);
		{
			list_poly_cb(ctx, pcb, layer, polygon);
		}
		PCB_ENDALL_LOOP;
	}

	return 0;
}

static int list_subc_cb(void *ctx, pcb_board_t *pcb, pcb_subc_t *subc, int enter)
{
	if (enter) {
		APPEND(ctx, subc);
		list_data(ctx, pcb, subc->data, enter, PCB_OBJ_ANY);
	}
	return 0;
}

void pcb_qry_list_all_pcb(pcb_qry_val_t *lst, pcb_objtype_t mask)
{
	assert(lst->type == PCBQ_VT_LST);
TODO(": rather do rtree search here to avoid recursion")
	pcb_loop_all(PCB, &lst->data.lst,
		(mask & PCB_OBJ_LAYER) ? list_layer_cb : NULL,
		(mask & PCB_OBJ_LINE) ? list_line_cb : NULL,
		(mask & PCB_OBJ_ARC) ? list_arc_cb : NULL,
		(mask & PCB_OBJ_TEXT) ? list_text_cb : NULL,
		(mask & PCB_OBJ_POLY) ? list_poly_cb : NULL,
		(mask & PCB_OBJ_GFX) ? list_gfx_cb : NULL,
		(mask & PCB_OBJ_SUBC) ? list_subc_cb : NULL,
		(mask & PCB_OBJ_PSTK) ? list_pstk_cb : NULL
	);
}

void pcb_qry_list_all_data(pcb_qry_val_t *lst, pcb_data_t *data, pcb_objtype_t mask)
{
	list_data(&lst->data.lst, PCB, data, 1, mask);
}

void pcb_qry_list_all_subc(pcb_qry_val_t *lst, pcb_subc_t *sc, pcb_objtype_t mask)
{
	list_data(&lst->data.lst, PCB, sc->data, 1, mask);
}


void pcb_qry_list_free(pcb_qry_val_t *lst_)
{
	vtp0_uninit(&lst_->data.lst);
}

int pcb_qry_list_cmp(pcb_qry_val_t *lst1, pcb_qry_val_t *lst2)
{
	abort();
}

/***********************/

/* set dst to point to the idxth field assuming field 0 is fld. Returns -1
   if there are not enough fields */
#define fld_nth_req(dst, fld, idx) \
do { \
	int __i__ = idx; \
	pcb_qry_node_t *__f__; \
	for(__f__ = (fld); __i__ > 0; __i__--, __f__ = __f__->next) { \
		if (__f__ == NULL) \
			return -1; \
	} \
	(dst) = __f__; \
} while(0)

#define fld_nth_opt(dst, fld, idx) \
do { \
	int __i__ = idx; \
	pcb_qry_node_t *__f__; \
	for(__f__ = (fld); (__i__ > 0) && (__f__ != NULL); __i__--, __f__ = __f__->next) ; \
	(dst) = __f__; \
} while(0)

/* sets const char *s to point to the string data of the idxth field. Returns
   -1 if tere are not enooung fields */
#define fld2str_req(s, fld, idx) \
do { \
	pcb_qry_node_t *_f_; \
	fld_nth_req(_f_, fld, idx); \
	if ((_f_ == NULL) || (_f_->type != PCBQ_FIELD)) \
		return -1; \
	(s) = _f_->data.str; \
} while(0)

/* Same, but doesn't return -1 on missing field but sets s to NULL */
#define fld2str_opt(s, fld, idx) \
do { \
	pcb_qry_node_t *_f_; \
	const char *__res__; \
	fld_nth_opt(_f_, fld, idx); \
	if (_f_ != NULL) { \
		if (_f_->type != PCBQ_FIELD) \
			return -1; \
		__res__ = _f_->data.str; \
	} \
	else \
		__res__ = NULL; \
	(s) = __res__; \
} while(0)

/* sets query_fields_keys_t h to point to the precomp field of the idxth field.
   Returns -1 if tere are not enooung fields */
#define fld2hash_req(h, fld, idx) \
do { \
	pcb_qry_node_t *_f_; \
	fld_nth_req(_f_, fld, idx); \
	if ((_f_ == NULL) || (_f_->type != PCBQ_FIELD)) \
		return -1; \
	(h) = _f_->precomp.fld; \
} while(0)

/* Same, but doesn't return -1 on missing field but sets s to query_fields_SPHASH_INVALID */
#define fld2hash_opt(h, fld, idx) \
do { \
	pcb_qry_node_t *_f_; \
	query_fields_keys_t *__res__; \
	fld_nth_opt(_f_, fld, idx); \
	if (_f_ != NULL) { \
		if (_f_->type != PCBQ_FIELD) \
			return -1; \
		__res__ = _f_->precomp.fld; \
	} \
	else \
		__res__ = query_fields_SPHASH_INVALID; \
	(s) = __res__; \
} while(0)

/* convert a layer type description and cache the result (overwriting the string in the field!) */
#define fld2lyt_req(ec, lyt, lyc, fld, idx) \
do { \
	pcb_qry_node_t *_f_; \
	fld_nth_req(_f_, fld, idx); \
	if (_f_->type != PCBQ_DATA_LYTC) { \
		const char *_s2_; \
		fld2str_req(_s2_, fld, 1); \
		_f_->data.lytc = field_pstk_lyt(ec, p, _s2_); \
		_f_->type = PCBQ_DATA_LYTC; \
	} \
	lyt = _f_->data.lytc.lyt; \
	lyc = _f_->data.lytc.lyc; \
} while(0)

#define COMMON_FIELDS(ec, obj, fh1, fld, res) \
do { \
	if (fh1 == query_fields_a) { \
		const char *s2; \
		fld2str_req(s2, fld, 1); \
		if (!PCB_OBJ_IS_CLASS(obj->type, PCB_OBJ_CLASS_OBJ)) \
			PCB_QRY_RET_INV(res); \
		PCB_QRY_RET_STR(res, pcb_attribute_get(&obj->Attributes, s2)); \
	} \
 \
	if (fh1 == query_fields_ID) { \
		if (!PCB_OBJ_IS_CLASS(obj->type, PCB_OBJ_CLASS_OBJ)) \
			PCB_QRY_RET_INV(res); \
		PCB_QRY_RET_INT(res, obj->ID); \
	} \
	if (fh1 == query_fields_IID) { \
		if (!PCB_OBJ_IS_CLASS(obj->type, PCB_OBJ_CLASS_OBJ)) \
			PCB_QRY_RET_INV(res); \
		PCB_QRY_RET_INT(res, pcb_obj_iid(obj)); \
	} \
 \
	if ((fh1 == query_fields_bbox) || (fh1 == query_fields_bbox_naked)) { \
		rnd_box_t *bx = (fh1 == query_fields_bbox) ? &(obj)->BoundingBox : &(obj)->bbox_naked; \
		query_fields_keys_t fh2; \
 \
		if (!PCB_OBJ_IS_CLASS(obj->type, PCB_OBJ_CLASS_OBJ)) \
			PCB_QRY_RET_INV(res); \
 \
		fld2hash_req(fh2, fld, 1); \
		switch(fh2) { \
			case query_fields_x1:     PCB_QRY_RET_COORD(res, bx->X1); \
			case query_fields_y1:     PCB_QRY_RET_COORD(res, bx->Y1); \
			case query_fields_x2:     PCB_QRY_RET_COORD(res, bx->X2); \
			case query_fields_y2:     PCB_QRY_RET_COORD(res, bx->Y2); \
			case query_fields_width:  PCB_QRY_RET_COORD(res, bx->X2 - bx->X1); \
			case query_fields_height: PCB_QRY_RET_COORD(res, bx->Y2 - bx->Y1); \
			default:; \
		} \
		PCB_QRY_RET_INV(res); \
	} \
 \
	if (fh1 == query_fields_type) \
		PCB_QRY_RET_INT(res, obj->type); \
 \
	if (fh1 == query_fields_subc) \
	 return field_subc_obj(ec, obj, fld->next, res); \
 \
} while(0)

static int field_net(pcb_qry_exec_t *ec, pcb_any_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res);

#define NETNAME_FIELDS(ec) \
do { \
	if (fh1 == query_fields_netname) { \
		if (ec != NULL) { \
			pcb_any_obj_t *term = pcb_qry_parent_net_term(ec, obj); \
			pcb_net_t *net; \
			if ((term == NULL) || (term->type != PCB_OBJ_NET_TERM)) \
				PCB_QRY_RET_INV(res); \
			net = term->parent.net; \
			if ((net == NULL) || (net->type != PCB_OBJ_NET)) \
				PCB_QRY_RET_INV(res); \
			PCB_QRY_RET_STR(res, net->name); \
		} \
		else \
			PCB_QRY_RET_INV(res); \
	} \
	if (fh1 == query_fields_net) { \
		if (ec != NULL) { \
			pcb_any_obj_t *term = pcb_qry_parent_net_term(ec, obj); \
			pcb_net_t *net; \
			if ((term == NULL) || (term->type != PCB_OBJ_NET_TERM)) \
				PCB_QRY_RET_INV(res); \
			net = term->parent.net; \
			if ((net == NULL) || (net->type != PCB_OBJ_NET)) \
				PCB_QRY_RET_INV(res); \
			if (fld->next == NULL) \
				PCB_QRY_RET_OBJ(res, (pcb_any_obj_t *)net); \
			else { \
				pcb_qry_node_t *__fn__ = (fld)->next; \
				return field_net(ec, (pcb_any_obj_t *)net, __fn__, res); \
			} \
			PCB_QRY_RET_INV(res); \
		} \
		else \
			PCB_QRY_RET_INV(res); \
	} \
	if (fh1 == query_fields_netseg) { \
		if (ec != NULL) { \
			pcb_any_obj_t *term = pcb_qry_parent_net_term(ec, obj); \
			if (term == NULL) \
				PCB_QRY_RET_INV(res); \
			PCB_QRY_RET_OBJ(res, term); \
		} \
		else \
			PCB_QRY_RET_INV(res); \
	} \
} while(0)

static int field_layer(pcb_qry_exec_t *ec, pcb_any_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_layer_t *rl, *l = (pcb_layer_t *)obj;
	query_fields_keys_t fh1;
	const char *prp;

	if (l->is_bound) {
		rl = pcb_layer_get_real(l);
		if (rl != NULL)
			l = rl;
	}


	fld2hash_req(fh1, fld, 0);

	/* in some cases .type will do the wrong thing at the caller; the escape is .layer.type; drop the redundant .layer */
	if (fh1 == query_fields_layer) {
		fld = fld->next;
		fld2hash_req(fh1, fld, 0);
	}

	if (fh1 == query_fields_a) {
		const char *s2;
		fld2str_req(s2, fld, 1);
		PCB_QRY_RET_STR(res, pcb_attribute_get(&l->Attributes, s2));
	}

	if (fld->next != NULL)
		PCB_QRY_RET_INV(res);

	switch(fh1) {
		case query_fields_IID:      PCB_QRY_RET_INT(res, pcb_obj_iid((pcb_any_obj_t *)l));
		case query_fields_name:     PCB_QRY_RET_STR(res, l->name);
		case query_fields_visible:  PCB_QRY_RET_INT(res, l->meta.real.vis);
		case query_fields_position: PCB_QRY_RET_INT(res, pcb_layer_flags_(l) & PCB_LYT_ANYWHERE);
		case query_fields_type:     PCB_QRY_RET_INT(res, pcb_layer_flags_(l) & PCB_LYT_ANYTHING);
		case query_fields_purpose:
			pcb_layer_purpose_(l, &prp);
			PCB_QRY_RET_STR(res, prp);
		default:;
	}

	PCB_QRY_RET_INV(res);
}

static int field_layergrp(pcb_qry_exec_t *ec, pcb_any_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_layergrp_t *g = (pcb_layergrp_t *)obj;
	query_fields_keys_t fh1;

	fld2hash_req(fh1, fld, 0);

	/* in some cases .type will do the wrong thing at the caller; the escape is .layer.type; drop the redundant .layer */
	if (fh1 == query_fields_layergroup) {
		fld = fld->next;
		fld2hash_req(fh1, fld, 0);
	}

	if (fh1 == query_fields_a) {
		const char *s2;
		fld2str_req(s2, fld, 1);
		PCB_QRY_RET_STR(res, pcb_attribute_get(&g->Attributes, s2));
	}

	if (fld->next != NULL)
		PCB_QRY_RET_INV(res);

	switch(fh1) {
		case query_fields_IID:      PCB_QRY_RET_INT(res, pcb_obj_iid((pcb_any_obj_t *)g));
		case query_fields_name:     PCB_QRY_RET_STR(res, g->name);
		case query_fields_visible:  PCB_QRY_RET_INT(res, g - ec->pcb->LayerGroups.grp);
		case query_fields_position: PCB_QRY_RET_INT(res, g->ltype & PCB_LYT_ANYWHERE);
		case query_fields_type:     PCB_QRY_RET_INT(res, g->ltype & PCB_LYT_ANYTHING);
		case query_fields_purpose:  PCB_QRY_RET_STR(res, g->purpose);
		default:;
	}

	PCB_QRY_RET_INV(res);
}

static int field_layer_from_ptr(pcb_qry_exec_t *ec, pcb_layer_t *l, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	return field_layer(ec, (pcb_any_obj_t *)l, fld, res);
}

static int field_layergrp_from_ptr(pcb_qry_exec_t *ec, pcb_layergrp_t *l, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	return field_layergrp(ec, (pcb_any_obj_t *)l, fld, res);
}

static int field_layergrp_from_layer_ptr(pcb_qry_exec_t *ec, pcb_layer_t *l, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_layergrp_t *grp;

	l = pcb_layer_get_real(l);
	if (l == NULL)
		PCB_QRY_RET_INV(res);

	grp = pcb_get_layergrp(ec->pcb, l->meta.real.grp);
	if (grp == NULL)
		PCB_QRY_RET_INV(res);

	return field_layergrp(ec, (pcb_any_obj_t *)grp, fld, res);
}

/* process from .layer */
static int layer_of_obj(pcb_qry_exec_t *ec, pcb_qry_node_t *fld, pcb_qry_val_t *res, pcb_layer_type_t mask)
{
	rnd_layer_id_t id;
	const char *s1;

	if (pcb_layer_list(PCB, mask, &id, 1) != 1)
		PCB_QRY_RET_INV(res);

	fld2str_req(s1, fld, 0);

	if (s1 == NULL) {
		res->source = NULL;
		res->type = PCBQ_VT_OBJ;
		res->data.obj = (pcb_any_obj_t *)&(PCB->Data->Layer[id]);
		return 0;
	}

	return field_layer_from_ptr(ec, PCB->Data->Layer+id, fld, res);
}

/* process from .layergroup */
static int layergrp_of_obj(pcb_qry_exec_t *ec, pcb_qry_node_t *fld, pcb_qry_val_t *res, pcb_layer_type_t mask)
{
	rnd_layergrp_id_t id;
	const char *s1;

	if (pcb_layergrp_list(PCB, mask, &id, 1) != 1)
		PCB_QRY_RET_INV(res);

	fld2str_req(s1, fld, 0);

	if (s1 == NULL) {
		res->source = NULL;
		res->type = PCBQ_VT_OBJ;
		res->data.obj = (pcb_any_obj_t *)&(PCB->LayerGroups.grp[id]);
		return 0;
	}

	return field_layergrp_from_ptr(ec, PCB->LayerGroups.grp+id, fld, res);
}

static double pcb_line_len2(pcb_line_t *l)
{
	double x = l->Point1.X - l->Point2.X;
	double y = l->Point1.Y - l->Point2.Y;
	return x*x + y*y;
}

static int field_line(pcb_qry_exec_t *ec, pcb_any_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_line_t *l = (pcb_line_t *)obj;
	query_fields_keys_t fh1;

	fld2hash_req(fh1, fld, 0);

	if (fh1 == query_fields_a) {
		const char *s2;
		fld2str_req(s2, fld, 1);
		PCB_QRY_RET_STR(res, pcb_attribute_get(&l->Attributes, s2));
	}

	if (fh1 == query_fields_layer) {
		if (obj->parent_type == PCB_PARENT_LAYER)
			return field_layer_from_ptr(ec, obj->parent.layer, fld->next, res);
		else
			PCB_QRY_RET_INV(res);
	}
	else if (fh1 == query_fields_layergroup) {
		if (obj->parent_type == PCB_PARENT_LAYER)
			return field_layergrp_from_layer_ptr(ec, obj->parent.layer, fld->next, res);
		else
			PCB_QRY_RET_INV(res);
	}

	NETNAME_FIELDS(ec);

	if (fld->next != NULL)
		PCB_QRY_RET_INV(res);

	switch(fh1) {
		case query_fields_x1:         PCB_QRY_RET_COORD(res, l->Point1.X);
		case query_fields_y1:         PCB_QRY_RET_COORD(res, l->Point1.Y);
		case query_fields_x2:         PCB_QRY_RET_COORD(res, l->Point2.X);
		case query_fields_y2:         PCB_QRY_RET_COORD(res, l->Point2.Y);
		case query_fields_thickness:  PCB_QRY_RET_COORD(res, l->Thickness);
		case query_fields_clearance:  PCB_QRY_RET_COORD(res, rnd_round((double)l->Clearance/2.0));
		case query_fields_length:
			PCB_QRY_RET_DBL(res, ((rnd_coord_t)rnd_round(sqrt(pcb_line_len2(l)))));
			break;
		case query_fields_length2:
			PCB_QRY_RET_DBL(res, pcb_line_len2(l));
			break;
		case query_fields_area:
			{
				double th = l->Thickness;
				double len = rnd_round(sqrt(pcb_line_len2(l)));
				if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, l))
					PCB_QRY_RET_DBL(res, (len+th) * th);
				else
					PCB_QRY_RET_DBL(res, len * th + th*th/4*M_PI);
				break;
			}
		default:;
	}

	PCB_QRY_RET_INV(res);
}

static double pcb_arc_len(pcb_arc_t *a)
{
TODO(": this breaks for elliptics; see http://tutorial.math.lamar.edu/Classes/CalcII/ArcLength.aspx")
	double r = (a->Width + a->Height)/2;
	return r * M_PI / 180.0 * a->Delta;
}

static int field_arc(pcb_qry_exec_t *ec, pcb_any_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_arc_t *a = (pcb_arc_t *)obj;
	query_fields_keys_t fh1, fh2;

	fld2hash_req(fh1, fld, 0);

	if (fh1 == query_fields_a) {
		const char *s2;
		fld2str_req(s2, fld, 1);
		PCB_QRY_RET_STR(res, pcb_attribute_get(&a->Attributes, s2));
	}

	if (fh1 == query_fields_angle) {
		fld2hash_req(fh2, fld, 1);
		switch(fh2) {
			case query_fields_start: PCB_QRY_RET_DBL(res, a->StartAngle);
			case query_fields_delta: PCB_QRY_RET_DBL(res, a->Delta);
			default:;
		}
		PCB_QRY_RET_INV(res);
	}

	if (fh1 == query_fields_layer) {
		if (obj->parent_type == PCB_PARENT_LAYER)
			return field_layer_from_ptr(ec, obj->parent.layer, fld->next, res);
		else
			PCB_QRY_RET_INV(res);
	}
	else if (fh1 == query_fields_layergroup) {
		if (obj->parent_type == PCB_PARENT_LAYER)
			return field_layergrp_from_layer_ptr(ec, obj->parent.layer, fld->next, res);
		else
			PCB_QRY_RET_INV(res);
	}

	NETNAME_FIELDS(ec);

	if (fld->next != NULL)
		PCB_QRY_RET_INV(res);

	switch(fh1) {
		case query_fields_cx:
		case query_fields_x:         PCB_QRY_RET_COORD(res, a->X);
		case query_fields_cy:
		case query_fields_y:         PCB_QRY_RET_COORD(res, a->Y);
		case query_fields_width:     PCB_QRY_RET_COORD(res, a->Width);
		case query_fields_height:    PCB_QRY_RET_COORD(res, a->Height);
		case query_fields_thickness: PCB_QRY_RET_COORD(res, a->Thickness);
		case query_fields_clearance: PCB_QRY_RET_COORD(res, (rnd_coord_t)rnd_round((double)a->Clearance/2.0));
		case query_fields_length:
			PCB_QRY_RET_COORD(res, ((rnd_coord_t)rnd_round(pcb_arc_len(a))));
			break;
		case query_fields_length2:
			{
				double l = pcb_arc_len(a);
				PCB_QRY_RET_DBL(res, l*l);
			}
			break;
		case query_fields_area:
			{
				double th = a->Thickness;
				double len = pcb_arc_len(a);
				PCB_QRY_RET_DBL(res, len * th + th*th/4*M_PI); /* approx */
			}
			break;
		default:;
	}

	PCB_QRY_RET_INV(res);
}

static int field_gfx(pcb_qry_exec_t *ec, pcb_any_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_gfx_t *g = (pcb_gfx_t *)obj;
	query_fields_keys_t fh1, fh2;

	fld2hash_req(fh1, fld, 0);

	if (fh1 == query_fields_a) {
		const char *s2;
		fld2str_req(s2, fld, 1);
		PCB_QRY_RET_STR(res, pcb_attribute_get(&g->Attributes, s2));
	}

	if (fh1 == query_fields_layer) {
		if (obj->parent_type == PCB_PARENT_LAYER)
			return field_layer_from_ptr(ec, obj->parent.layer, fld->next, res);
		else
			PCB_QRY_RET_INV(res);
	}
	else if (fh1 == query_fields_layergroup) {
		if (obj->parent_type == PCB_PARENT_LAYER)
			return field_layergrp_from_layer_ptr(ec, obj->parent.layer, fld->next, res);
		else
			PCB_QRY_RET_INV(res);
	}

	if (fld->next != NULL)
		PCB_QRY_RET_INV(res);

	switch(fh1) {
		case query_fields_area: PCB_QRY_RET_DBL(res, (double)g->sx * (double)g->sy); break;
		case query_fields_sx: PCB_QRY_RET_COORD(res, g->sx); break;
		case query_fields_sy: PCB_QRY_RET_COORD(res, g->sy); break;
		case query_fields_cx: PCB_QRY_RET_COORD(res, g->cx); break;
		case query_fields_cy: PCB_QRY_RET_COORD(res, g->cy); break;
		case query_fields_rot: PCB_QRY_RET_DBL(res, g->rot); break;
		default:;
	}
	PCB_QRY_RET_INV(res);
}

static int field_text(pcb_qry_exec_t *ec, pcb_any_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_text_t *t = (pcb_text_t *)obj;
	query_fields_keys_t fh1;

	fld2hash_req(fh1, fld, 0);

	if (fh1 == query_fields_a) {
		const char *s2;
		fld2str_req(s2, fld, 1);
		PCB_QRY_RET_STR(res, pcb_attribute_get(&t->Attributes, s2));
	}

	if (fh1 == query_fields_layer) {
		if (obj->parent_type == PCB_PARENT_LAYER)
			return field_layer_from_ptr(ec, obj->parent.layer, fld->next, res);
		else
			PCB_QRY_RET_INV(res);
	}
	else if (fh1 == query_fields_layergroup) {
		if (obj->parent_type == PCB_PARENT_LAYER)
			return field_layergrp_from_layer_ptr(ec, obj->parent.layer, fld->next, res);
		else
			PCB_QRY_RET_INV(res);
	}

	NETNAME_FIELDS(ec);

	if (fld->next != NULL)
		PCB_QRY_RET_INV(res);

	switch(fh1) {
		case query_fields_x:        PCB_QRY_RET_COORD(res, t->X);
		case query_fields_y:        PCB_QRY_RET_COORD(res, t->Y);
		case query_fields_scale:    PCB_QRY_RET_INT(res, t->Scale);
		case query_fields_scale_x:  PCB_QRY_RET_DBL(res, t->scale_x);
		case query_fields_scale_y:  PCB_QRY_RET_DBL(res, t->scale_y);
		case query_fields_rotation:
		case query_fields_rot:      PCB_QRY_RET_DBL(res, t->rot);
		case query_fields_thickness:PCB_QRY_RET_COORD(res, t->thickness);
		case query_fields_string:   PCB_QRY_RET_STR(res, t->TextString);
		case query_fields_area:     PCB_QRY_RET_DBL(res, (double)(t->BoundingBox.Y2 - t->BoundingBox.Y1) * (double)(t->BoundingBox.X2 - t->BoundingBox.X1));
		default:;
	}

	PCB_QRY_RET_INV(res);
}

static int field_polygon(pcb_qry_exec_t *ec, pcb_any_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_poly_t *p = (pcb_poly_t *)obj;
	query_fields_keys_t fh1;

	fld2hash_req(fh1, fld, 0);

	if (fh1 == query_fields_a) {
		const char *s2;
		fld2str_req(s2, fld, 1);
		PCB_QRY_RET_STR(res, pcb_attribute_get(&p->Attributes, s2));
	}

	if (fh1 == query_fields_layer) {
		if (obj->parent_type == PCB_PARENT_LAYER)
			return field_layer_from_ptr(ec, obj->parent.layer, fld->next, res);
		else
			PCB_QRY_RET_INV(res);
	}
	else if (fh1 == query_fields_layergroup) {
		if (obj->parent_type == PCB_PARENT_LAYER)
			return field_layergrp_from_layer_ptr(ec, obj->parent.layer, fld->next, res);
		else
			PCB_QRY_RET_INV(res);
	}

	NETNAME_FIELDS(ec);

	if (fld->next != NULL)
		PCB_QRY_RET_INV(res);

	switch(fh1) {
		case query_fields_clearance:
			if (PCB_FLAG_TEST(PCB_FLAG_CLEARPOLYPOLY, p))
				PCB_QRY_RET_COORD(res, (rnd_coord_t)rnd_round((double)p->Clearance/2.0));
			else
				PCB_QRY_RET_INV(res);
			break;
		case query_fields_points: PCB_QRY_RET_INT(res, p->PointN);
		case query_fields_area:
			PCB_QRY_RET_DBL(res, p->Clipped->contours->area);
		default:;
	}

	PCB_QRY_RET_INV(res);
}

static int field_rat(pcb_qry_exec_t *ec, pcb_any_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
/*	const char *s1, *s2;

	fld2str_req(s1, fld, 0);
	fld2str_opt(s2, fld, 1);*/
TODO("TODO")
	PCB_QRY_RET_INV(res);
}

static struct pcb_qry_lytc_s field_pstk_lyt(pcb_qry_exec_t *ec, pcb_pstk_t *ps, const char *where)
{
	struct pcb_qry_lytc_s lytc;
	pcb_layer_typecomb_str2bits(where, &lytc.lyt, &lytc.lyc, 1);
	return lytc;
}

static int field_pstk(pcb_qry_exec_t *ec, pcb_any_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_pstk_t *p = (pcb_pstk_t *)obj;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(p);
	query_fields_keys_t fh1;

	fld2hash_req(fh1, fld, 0);

	if (fh1 == query_fields_a) {
		const char *s2;
		fld2str_req(s2, fld, 1);
		PCB_QRY_RET_STR(res, pcb_attribute_get(&p->Attributes, s2));
	}

	if (fh1 == query_fields_shape) {
		pcb_layer_type_t lyt;
		pcb_layer_combining_t lyc = 0;
		pcb_pstk_shape_t *shp;

		fld2lyt_req(ec, lyt, lyc, fld, 1);
		if (lyt == 0)
			PCB_QRY_RET_INV(res);

		shp = pcb_pstk_shape(p, lyt, lyc);
		if (shp != NULL) {
			switch(shp->shape) {
				case PCB_PSSH_POLY: PCB_QRY_RET_STR(res, "polygon");
				case PCB_PSSH_LINE: PCB_QRY_RET_STR(res, "line");
				case PCB_PSSH_CIRC: PCB_QRY_RET_STR(res, "circle");
				case PCB_PSSH_HSHADOW: PCB_QRY_RET_STR(res, "hshadow");
			}
		}
		PCB_QRY_RET_STR(res, "");
	}

	NETNAME_FIELDS(ec);

	if (fld->next != NULL)
		PCB_QRY_RET_INV(res);

	switch(fh1) {
		case query_fields_x:         PCB_QRY_RET_COORD(res, p->x);
		case query_fields_y:         PCB_QRY_RET_COORD(res, p->y);
		case query_fields_clearance: PCB_QRY_RET_COORD(res, p->Clearance);
		case query_fields_rotation:  PCB_QRY_RET_DBL(res, p->rot);
		case query_fields_xmirror:   PCB_QRY_RET_INT(res, p->xmirror);
		case query_fields_smirror:   PCB_QRY_RET_INT(res, p->smirror);
		case query_fields_plated:    PCB_QRY_RET_INT(res, PCB_PSTK_PROTO_PLATES(proto));
		case query_fields_hole:      PCB_QRY_RET_COORD(res, proto->hdia);
		case query_fields_area:      PCB_QRY_RET_DBL(res, (double)(p->BoundingBox.Y2 - p->BoundingBox.Y1) * (double)(p->BoundingBox.X2 - p->BoundingBox.X1));
		case query_fields_proto:     PCB_QRY_RET_INT(res, p->proto);
		default:;
	}

	PCB_QRY_RET_INV(res);
}

static int field_net(pcb_qry_exec_t *ec, pcb_any_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_net_t *net = (pcb_net_t *)obj;
	query_fields_keys_t fh1;

	fld2hash_req(fh1, fld, 0);

	if (fh1 == query_fields_a) {
		const char *s2;
		fld2str_req(s2, fld, 1);
		PCB_QRY_RET_STR(res, pcb_attribute_get(&net->Attributes, s2));
	}

	if (fld->next != NULL)
		PCB_QRY_RET_INV(res);

	switch(fh1) {
		case query_fields_name:     PCB_QRY_RET_STR(res, net->name);
		default:;
	}

	PCB_QRY_RET_INV(res);

}

static int field_subc(pcb_qry_exec_t *ec, pcb_any_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_subc_t *p = (pcb_subc_t *)obj;
	query_fields_keys_t fh1;
	rnd_coord_t x, y;
	double rot;
	int on_bottom;

	fld2hash_req(fh1, fld, 0);
	if (fh1 == query_fields_a) {
		const char *s2;
		fld2str_req(s2, fld, 1);
		PCB_QRY_RET_STR(res, pcb_attribute_get(&p->Attributes, s2));
	}

	if (fh1 == query_fields_layer)
		return layer_of_obj(ec, fld->next, res, PCB_LYT_SILK | (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, p) ? PCB_LYT_BOTTOM : PCB_LYT_TOP));

	if (fh1 == query_fields_layergroup)
		return layergrp_of_obj(ec, fld->next, res, PCB_LYT_SILK | (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, p) ? PCB_LYT_BOTTOM : PCB_LYT_TOP));

	if (fld->next != NULL)
		PCB_QRY_RET_INV(res);

	switch(fh1) {
		case query_fields_x:            pcb_subc_get_origin(p, &x, &y); PCB_QRY_RET_COORD(res, x);
		case query_fields_y:            pcb_subc_get_origin(p, &x, &y); PCB_QRY_RET_COORD(res, y);
		case query_fields_rotation:     pcb_subc_get_rotation(p, &rot); PCB_QRY_RET_DBL(res, rot);
		case query_fields_side:         pcb_subc_get_side(p, &on_bottom); PCB_QRY_RET_SIDE(res, on_bottom);
		case query_fields_refdes:       PCB_QRY_RET_STR(res, p->refdes);
		case query_fields_area:         PCB_QRY_RET_DBL(res, (double)(p->BoundingBox.Y2 - p->BoundingBox.Y1) * (double)(p->BoundingBox.X2 - p->BoundingBox.X1));
		default:;
	}
	PCB_QRY_RET_INV(res);
}

static int field_subc_obj(pcb_qry_exec_t *ec, pcb_any_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res);

static int field_subc_from_ptr(pcb_qry_exec_t *ec, pcb_subc_t *s, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_any_obj_t *obj = (pcb_any_obj_t *)s;
	query_fields_keys_t fh1;

	fld2hash_req(fh1, fld, 0);
	COMMON_FIELDS(ec, obj, fh1, fld, res);

	return field_subc(ec, obj, fld, res);
}

static int field_subc_obj(pcb_qry_exec_t *ec, pcb_any_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	const char *s1;
	pcb_subc_t *parent = pcb_obj_parent_subc(obj);

	/* if parent is not a subc (or not available) evaluate to invalid */
	if (parent == NULL)
		PCB_QRY_RET_INV(res);

	/* check subfield, if there's none, return the subcircuit object */
	fld2str_opt(s1, fld, 0);
	if (s1 == NULL) {
		res->source = NULL;
		res->type = PCBQ_VT_OBJ;
		res->data.obj = (pcb_any_obj_t *)parent;
		return 0;
	}

	/* return subfields of the subcircuit */
	return field_subc_from_ptr(ec, parent, fld, res);
}

/***/

static int pcb_qry_obj_flag(pcb_any_obj_t *obj, pcb_qry_node_t *nflg, pcb_qry_val_t *res)
{
	if (obj == NULL)
		return -1;

	if ((nflg->precomp.flg->object_types & obj->type) != obj->type) {
		/* flag not applicable on object type */
		PCB_QRY_RET_INV(res);
	}
	PCB_QRY_RET_INT(res, PCB_FLAG_TEST(nflg->precomp.flg->mask, obj));
}

int pcb_qry_obj_field(pcb_qry_exec_t *ec, pcb_qry_val_t *objval, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_any_obj_t *obj;
	query_fields_keys_t fh1;

	if (objval->type != PCBQ_VT_OBJ)
		return -1;
	obj = objval->data.obj;

	res->source = NULL;

	if (fld->type == PCBQ_FLAG)
		return pcb_qry_obj_flag(obj, fld, res);

	fld2hash_req(fh1, fld, 0);

	COMMON_FIELDS(ec, obj, fh1, fld, res);

	switch(obj->type) {
/*		case PCB_OBJ_POINT:    return field_point(ec, obj, fld, res);*/
		case PCB_OBJ_LINE:     return field_line(ec, obj, fld, res);
		case PCB_OBJ_TEXT:     return field_text(ec, obj, fld, res);
		case PCB_OBJ_POLY:     return field_polygon(ec, obj, fld, res);
		case PCB_OBJ_ARC:      return field_arc(ec, obj, fld, res);
		case PCB_OBJ_GFX:      return field_gfx(ec, obj, fld, res);
		case PCB_OBJ_RAT:      return field_rat(ec, obj, fld, res);
		case PCB_OBJ_PSTK:     return field_pstk(ec, obj, fld, res);
		case PCB_OBJ_SUBC:     return field_subc(ec, obj, fld, res);

		case PCB_OBJ_NET:      return field_net(ec, obj, fld, res);
		case PCB_OBJ_LAYER:    return field_layer(ec, obj, fld, res);
		case PCB_OBJ_LAYERGRP: return field_layergrp(ec, obj, fld, res);
		default:;
	}

	return -1;
}

