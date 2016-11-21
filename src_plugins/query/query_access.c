/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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

/* Query language - access to / extract core data */

#include "config.h"
#include "math_helper.h"
#include "board.h"
#include "data.h"
#include "query_access.h"
#include "query_exec.h"
#include "layer.h"
#include "fields_sphash.h"

#define APPEND(_ctx_, _type_, _obj_, _parenttype_, _parent_) \
do { \
	pcb_objlist_t *lst = (pcb_objlist_t *)_ctx_; \
	pcb_obj_t *o = calloc(sizeof(pcb_obj_t), 1); \
	o->type = _type_; \
	o->data.any = _obj_; \
	o->parent_type = _parenttype_; \
	o->parent.any = _parent_; \
	pcb_objlist_append(lst, o); \
} while(0)

static int list_layer_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, int enter)
{
	if (enter)
		APPEND(ctx, PCB_OBJ_LAYER, layer, PCB_PARENT_DATA, pcb->Data);
	return 0;
}

static void list_line_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_line_t *line)
{
	APPEND(ctx, PCB_OBJ_LINE, line, PCB_PARENT_LAYER, layer);
}

static void list_arc_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_arc_t *arc)
{
	APPEND(ctx, PCB_OBJ_ARC, arc, PCB_PARENT_LAYER, layer);
}

static void list_text_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_text_t *text)
{
	APPEND(ctx, PCB_OBJ_TEXT, text, PCB_PARENT_LAYER, layer);
}

static void list_poly_cb(void *ctx, pcb_board_t *pcb, pcb_layer_t *layer, pcb_polygon_t *poly)
{
	APPEND(ctx, PCB_OBJ_POLYGON, poly, PCB_PARENT_LAYER, layer);
}

static int list_element_cb(void *ctx, pcb_board_t *pcb, pcb_element_t *element, int enter)
{
	if (enter)
		APPEND(ctx, PCB_OBJ_ELEMENT, element, PCB_PARENT_DATA, pcb->Data);
	return 0;
}

static void list_eline_cb(void *ctx, pcb_board_t *pcb, pcb_element_t *element, pcb_line_t *line)
{
	APPEND(ctx, PCB_OBJ_ELINE, line, PCB_PARENT_ELEMENT, element);
}

static void list_earc_cb(void *ctx, pcb_board_t *pcb, pcb_element_t *element, pcb_arc_t *arc)
{
	APPEND(ctx, PCB_OBJ_EARC, arc, PCB_PARENT_ELEMENT, element);
}

static void list_etext_cb(void *ctx, pcb_board_t *pcb, pcb_element_t *element, pcb_text_t *text)
{
	APPEND(ctx, PCB_OBJ_ETEXT, text, PCB_PARENT_ELEMENT, element);
}

static void list_epin_cb(void *ctx, pcb_board_t *pcb, pcb_element_t *element, pcb_pin_t *pin)
{
	APPEND(ctx, PCB_OBJ_PIN, pin, PCB_PARENT_ELEMENT, element);
}

static void list_epad_cb(void *ctx, pcb_board_t *pcb, pcb_element_t *element, pcb_pad_t *pad)
{
	APPEND(ctx, PCB_OBJ_PAD, pad, PCB_PARENT_ELEMENT, element);
}

static void list_via_cb(void *ctx, pcb_board_t *pcb, pcb_pin_t *via)
{
	APPEND(ctx, PCB_OBJ_VIA, via, PCB_PARENT_DATA, pcb->Data);
}

void pcb_qry_list_all(pcb_qry_val_t *lst, pcb_objtype_t mask)
{
	assert(lst->type == PCBQ_VT_LST);
	pcb_loop_all(&lst->data.lst,
		(mask & PCB_OBJ_LAYER) ? list_layer_cb : NULL,
		(mask & PCB_OBJ_LINE) ? list_line_cb : NULL,
		(mask & PCB_OBJ_ARC) ? list_arc_cb : NULL,
		(mask & PCB_OBJ_TEXT) ? list_text_cb : NULL,
		(mask & PCB_OBJ_POLYGON) ? list_poly_cb : NULL,
		(mask & PCB_OBJ_ELEMENT) ? list_element_cb : NULL,
		(mask & PCB_OBJ_ELINE) ? list_eline_cb : NULL,
		(mask & PCB_OBJ_EARC) ? list_earc_cb : NULL,
		(mask & PCB_OBJ_ETEXT) ? list_etext_cb : NULL,
		(mask & PCB_OBJ_PIN) ? list_epin_cb : NULL,
		(mask & PCB_OBJ_PAD) ? list_epad_cb : NULL,
		(mask & PCB_OBJ_VIA) ? list_via_cb : NULL
	);
}

void pcb_qry_list_free(pcb_qry_val_t *lst_)
{
	pcb_objlist_t *lst = &lst_->data.lst;

	assert(lst_->type == PCBQ_VT_LST);

	for(;;) {
		pcb_obj_t *n = pcb_objlist_first(lst);
		if (n == NULL)
			break;
		pcb_objlist_remove(n);
		free(n);
	}
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

static int field_layer(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_layer_t *l = obj->data.layer;
	query_fields_keys_t fh1;

	fld2hash_req(fh1, fld, 0);

	if (fh1 == query_fields_a) {
		const char *s2;
		fld2str_req(s2, fld, 1);
		PCB_QRY_RET_STR(res, pcb_attribute_get(&l->Attributes, s2));
	}

	if (fld->next != NULL)
		PCB_QRY_RET_INV(res);

	switch(fh1) {
		case query_fields_name:     PCB_QRY_RET_STR(res, l->Name);
		case query_fields_visible:  PCB_QRY_RET_INT(res, l->On);
		case query_fields_position: PCB_QRY_RET_INT(res, pcb_layer_flags(GetLayerNumber(PCB->Data, l)) & PCB_LYT_ANYWHERE);
		case query_fields_type:     PCB_QRY_RET_INT(res, pcb_layer_flags(GetLayerNumber(PCB->Data, l)) & PCB_LYT_ANYTHING);
		default:;
	}

	PCB_QRY_RET_INV(res);
}

static int field_layer_from_ptr(pcb_layer_t *l, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_obj_t tmp;
	tmp.type = PCB_OBJ_LAYER;
	tmp.data.layer = l;
	tmp.parent_type = PCB_PARENT_DATA;
	tmp.parent.data = PCB->Data;
	return field_layer(&tmp, fld, res);
}

/* process from .layer */
static int layer_of_obj(pcb_qry_node_t *fld, pcb_qry_val_t *res, pcb_layer_type_t mask)
{
	int id;
	const char *s1;

	if (pcb_layer_list(mask, &id, 1) != 1)
		PCB_QRY_RET_INV(res);

	fld2str_req(s1, fld, 0);

	if (s1 == NULL) {
		res->type = PCBQ_VT_OBJ;
		res->data.obj.type = PCB_OBJ_LAYER;
		res->data.obj.data.layer = PCB->Data->Layer+id;
		return 0;
	}

	return field_layer_from_ptr(PCB->Data->Layer+id, fld, res);
}


static int field_line(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_line_t *l = obj->data.line;
	query_fields_keys_t fh1;

	fld2hash_req(fh1, fld, 0);

	if (fh1 == query_fields_a) {
		const char *s2;
		fld2str_req(s2, fld, 1);
		PCB_QRY_RET_STR(res, pcb_attribute_get(&l->Attributes, s2));
	}

	if (fh1 == query_fields_layer) {
		if (obj->parent_type == PCB_PARENT_LAYER)
			return field_layer_from_ptr(obj->parent.layer, fld->next, res);
		else
			PCB_QRY_RET_INV(res);
	}

	if (fld->next != NULL)
		PCB_QRY_RET_INV(res);

	switch(fh1) {
		case query_fields_x1:         PCB_QRY_RET_INT(res, l->Point1.X);
		case query_fields_y1:         PCB_QRY_RET_INT(res, l->Point1.Y);
		case query_fields_x2:         PCB_QRY_RET_INT(res, l->Point2.X);
		case query_fields_y2:         PCB_QRY_RET_INT(res, l->Point2.Y);
		case query_fields_thickness:  PCB_QRY_RET_INT(res, l->Thickness);
		case query_fields_clearance:  PCB_QRY_RET_INT(res, l->Clearance);
		case query_fields_length:
			{
				double x = l->Point1.X - l->Point2.X;
				double y = l->Point1.Y - l->Point2.Y;
				double len = sqrt(x*x + y*y);
				PCB_QRY_RET_INT(res, ((pcb_coord_t)len));
			}
			break;
		default:;
	}

	PCB_QRY_RET_INV(res);
}

static int field_arc(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_arc_t *a = obj->data.arc;
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
			case query_fields_start: PCB_QRY_RET_INT(res, a->StartAngle);
			case query_fields_delta: PCB_QRY_RET_INT(res, a->Delta);
			default:;
		}
		PCB_QRY_RET_INV(res);
	}

	if (fh1 == query_fields_layer) {
		if (obj->parent_type == PCB_PARENT_LAYER)
			return field_layer_from_ptr(obj->parent.layer, fld->next, res);
		else
			PCB_QRY_RET_INV(res);
	}

	if (fld->next != NULL)
		PCB_QRY_RET_INV(res);

	switch(fh1) {
		case query_fields_cx:
		case query_fields_x:         PCB_QRY_RET_INT(res, a->X);
		case query_fields_cy:
		case query_fields_y:         PCB_QRY_RET_INT(res, a->Y);
		case query_fields_thickness: PCB_QRY_RET_INT(res, a->Thickness);
		case query_fields_clearance: PCB_QRY_RET_INT(res, a->Clearance);
		case query_fields_length:
			{
#warning TODO: this breaks for elliptics; see http://tutorial.math.lamar.edu/Classes/CalcII/ArcLength.aspx
				double r = (a->Width + a->Height)/2;
				double len = r * M_PI / 180.0 * a->Delta;
				PCB_QRY_RET_INT(res, ((pcb_coord_t)len));
			}
			break;
		default:;
	}

	PCB_QRY_RET_INV(res);
}

static int field_text(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_text_t *t = obj->data.text;
	query_fields_keys_t fh1;

	fld2hash_req(fh1, fld, 0);

	if (fh1 == query_fields_a) {
		const char *s2;
		fld2str_req(s2, fld, 1);
		PCB_QRY_RET_STR(res, pcb_attribute_get(&t->Attributes, s2));
	}

	if (fh1 == query_fields_layer) {
		if (obj->parent_type == PCB_PARENT_LAYER)
			return field_layer_from_ptr(obj->parent.layer, fld->next, res);
		else
			PCB_QRY_RET_INV(res);
	}

	if (fld->next != NULL)
		PCB_QRY_RET_INV(res);

	switch(fh1) {
		case query_fields_x:        PCB_QRY_RET_INT(res, t->X);
		case query_fields_y:        PCB_QRY_RET_INT(res, t->Y);
		case query_fields_scale:    PCB_QRY_RET_INT(res, t->Scale);
		case query_fields_rotation: PCB_QRY_RET_INT(res, t->Direction);
		case query_fields_string:   PCB_QRY_RET_STR(res, t->TextString);
		default:;
	}

	PCB_QRY_RET_INV(res);
}

static int field_polygon(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_polygon_t *p = obj->data.polygon;
	query_fields_keys_t fh1;

	fld2hash_req(fh1, fld, 0);

	if (fh1 == query_fields_a) {
		const char *s2;
		fld2str_req(s2, fld, 1);
		PCB_QRY_RET_STR(res, pcb_attribute_get(&p->Attributes, s2));
	}

	if (fh1 == query_fields_layer) {
		if (obj->parent_type == PCB_PARENT_LAYER)
			return field_layer_from_ptr(obj->parent.layer, fld->next, res);
		else
			PCB_QRY_RET_INV(res);
	}

	if (fld->next != NULL)
		PCB_QRY_RET_INV(res);

	switch(fh1) {
		case query_fields_points: PCB_QRY_RET_INT(res, p->PointN);
		default:;
	}

	PCB_QRY_RET_INV(res);
}

static int field_rat(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
/*	const char *s1, *s2;

	fld2str_req(s1, fld, 0);
	fld2str_opt(s2, fld, 1);*/
#warning TODO
	PCB_QRY_RET_INV(res);
}

static int field_via(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_pin_t *p = obj->data.via;
	query_fields_keys_t fh1;

	fld2hash_req(fh1, fld, 0);

	if (fh1 == query_fields_a) {
		const char *s2;
		fld2str_req(s2, fld, 1);
		PCB_QRY_RET_STR(res, pcb_attribute_get(&p->Attributes, s2));
	}

	if (fld->next != NULL)
		PCB_QRY_RET_INV(res);

	switch(fh1) {
		case query_fields_x:         PCB_QRY_RET_INT(res, p->X);
		case query_fields_y:         PCB_QRY_RET_INT(res, p->Y);
		case query_fields_thickness: PCB_QRY_RET_INT(res, p->Thickness);
		case query_fields_clearance: PCB_QRY_RET_INT(res, p->Clearance);
		case query_fields_hole:      PCB_QRY_RET_INT(res, p->DrillingHole);
		case query_fields_mask:      PCB_QRY_RET_INT(res, p->Mask);
		case query_fields_name:      PCB_QRY_RET_STR(res, p->Name);
		case query_fields_number:    PCB_QRY_RET_STR(res, p->Number);
		default:;
	}

	PCB_QRY_RET_INV(res);
}

static int field_element(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_element_t *p = obj->data.element;
	query_fields_keys_t fh1;

	fld2hash_req(fh1, fld, 0);
	if (fh1 == query_fields_a) {
		const char *s2;
		fld2str_req(s2, fld, 1);
		PCB_QRY_RET_STR(res, pcb_attribute_get(&p->Attributes, s2));
	}

	if (fh1 == query_fields_layer)
		return layer_of_obj(fld->next, res, PCB_LYT_SILK | (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, p) ? PCB_LYT_BOTTOM : PCB_LYT_TOP));

	if (fld->next != NULL)
		PCB_QRY_RET_INV(res);

	switch(fh1) {
		case query_fields_x:            PCB_QRY_RET_INT(res, p->MarkX);
		case query_fields_y:            PCB_QRY_RET_INT(res, p->MarkY);
		case query_fields_refdes:       /* alias of: */
		case query_fields_name:         PCB_QRY_RET_STR(res, p->Name[PCB_ELEMNAME_IDX_REFDES].TextString);
		case query_fields_description:  PCB_QRY_RET_STR(res, p->Name[PCB_ELEMNAME_IDX_DESCRIPTION].TextString);
		case query_fields_value:        PCB_QRY_RET_STR(res, p->Name[PCB_ELEMNAME_IDX_VALUE].TextString);

		default:;
	}
	PCB_QRY_RET_INV(res);
}

static int field_element_from_ptr(pcb_element_t *e, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_obj_t tmp;
	tmp.type = PCB_OBJ_ELEMENT;
	tmp.data.element = e;
	tmp.parent_type = PCB_PARENT_DATA;
	tmp.parent.data = PCB->Data;
	return field_element(&tmp, fld, res);
}

static int field_element_obj(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	const char *s1;

	/* if parent is not an element (or not available) evaluate to invalid */
	if (obj->parent_type != PCB_PARENT_ELEMENT)
		PCB_QRY_RET_INV(res);

	/* check subfield, if there's none, return the element object */
	fld2str_opt(s1, fld, 0);
	if (s1 == NULL) {
		res->type = PCBQ_VT_OBJ;
		res->data.obj.data.element = obj->parent.element;
		res->data.obj.parent_type = PCB_PARENT_DATA;
		res->data.obj.parent.data = PCB->Data;
		return 0;
	}

	/* return subfields of the element */
	return field_element_from_ptr(obj->parent.element, fld, res);
}

static int field_net(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
/*	const char *s1, *s2;

	fld2str_req(s1, fld, 0);
	fld2str_opt(s2, fld, 1);*/
#warning TODO
	PCB_QRY_RET_INV(res);
}


static int field_eline(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_line_t *l = obj->data.line;
	query_fields_keys_t fh1;

	fld2hash_req(fh1, fld, 0);

	switch(fh1) {
		case query_fields_layer:   return layer_of_obj(fld->next, res, PCB_LYT_SILK | (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, l) ? PCB_LYT_BOTTOM : PCB_LYT_TOP));
		case query_fields_element: return field_element_obj(obj, fld->next, res);
		default:;
	}

	return field_line(obj, fld, res);
}

static int field_earc(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_arc_t *a = obj->data.arc;
	query_fields_keys_t fh1;

	fld2hash_req(fh1, fld, 0);

	switch(fh1) {
		case query_fields_layer:   return layer_of_obj(fld->next, res, PCB_LYT_SILK | (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, a) ? PCB_LYT_BOTTOM : PCB_LYT_TOP));
		case query_fields_element: return field_element_obj(obj, fld->next, res);
		default:;
	}

	return field_arc(obj, fld, res);
}

static int field_etext(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_text_t *t = obj->data.text;
	query_fields_keys_t fh1;

	fld2hash_req(fh1, fld, 0);

	switch(fh1) {
		case query_fields_layer:   return layer_of_obj(fld->next, res, PCB_LYT_SILK | (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, t) ? PCB_LYT_BOTTOM : PCB_LYT_TOP));
		case query_fields_element: return field_element_obj(obj, fld->next, res);
		default:;
	}

	return field_text(obj, fld, res);
}

static int field_pin(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	query_fields_keys_t fh1;

	fld2hash_req(fh1, fld, 0);

	switch(fh1) {
		case query_fields_element: return field_element_obj(obj, fld->next, res);
		default:;
	}

	return field_via(obj, fld, res);
}

static int field_pad(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_pad_t *p = obj->data.pad;
	query_fields_keys_t fh1;

	fld2hash_req(fh1, fld, 0);

	switch(fh1) {
		case query_fields_a:
			{
				const char *s2;
				fld2str_req(s2, fld, 1);
				PCB_QRY_RET_STR(res, pcb_attribute_get(&p->Attributes, s2));
			}
		case query_fields_layer:   return layer_of_obj(fld->next, res, PCB_LYT_COPPER | (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, p) ? PCB_LYT_BOTTOM : PCB_LYT_TOP));
		case query_fields_element: return field_element_obj(obj, fld->next, res);
		default:;
	}


	if (fld->next != NULL)
		PCB_QRY_RET_INV(res);

	switch(fh1) {
		case query_fields_x1:        PCB_QRY_RET_INT(res, p->Point1.X);
		case query_fields_y1:        PCB_QRY_RET_INT(res, p->Point1.Y);
		case query_fields_x2:        PCB_QRY_RET_INT(res, p->Point2.X);
		case query_fields_y2:        PCB_QRY_RET_INT(res, p->Point2.Y);
		case query_fields_thickness: PCB_QRY_RET_INT(res, p->Thickness);
		case query_fields_clearance: PCB_QRY_RET_INT(res, p->Clearance);
		case query_fields_mask:      PCB_QRY_RET_INT(res, p->Mask);
		case query_fields_name:      PCB_QRY_RET_STR(res, p->Name);
		case query_fields_number:    PCB_QRY_RET_STR(res, p->Number);
		default:;
	}

	PCB_QRY_RET_INV(res);
}


/***/

int pcb_qry_obj_field(pcb_qry_val_t *objval, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_obj_t *obj;
	query_fields_keys_t fh1;

	if (objval->type != PCBQ_VT_OBJ)
		return -1;
	obj = &objval->data.obj;

	fld2hash_req(fh1, fld, 0);

	if (fh1 == query_fields_a) {
		const char *s2;
		fld2str_req(s2, fld, 1);
		if (!PCB_OBJ_IS_CLASS(obj->type, PCB_OBJ_CLASS_OBJ))
			PCB_QRY_RET_INV(res);
		PCB_QRY_RET_STR(res, pcb_attribute_get(&obj->data.anyobj->Attributes, s2));
	}

	if (fh1 == query_fields_ID) {
		if (!PCB_OBJ_IS_CLASS(obj->type, PCB_OBJ_CLASS_OBJ))
			PCB_QRY_RET_INV(res);
		PCB_QRY_RET_INT(res, obj->data.anyobj->ID);
	}

	if (fh1 == query_fields_bbox) {
		query_fields_keys_t fh2;

		if (!PCB_OBJ_IS_CLASS(obj->type, PCB_OBJ_CLASS_OBJ))
			PCB_QRY_RET_INV(res);

		fld2hash_req(fh2, fld, 1);
		switch(fh2) {
			case query_fields_x1:     PCB_QRY_RET_INT(res, obj->data.anyobj->BoundingBox.X1);
			case query_fields_y1:     PCB_QRY_RET_INT(res, obj->data.anyobj->BoundingBox.Y1);
			case query_fields_x2:     PCB_QRY_RET_INT(res, obj->data.anyobj->BoundingBox.X2);
			case query_fields_y2:     PCB_QRY_RET_INT(res, obj->data.anyobj->BoundingBox.Y2);
			case query_fields_width:  PCB_QRY_RET_INT(res, obj->data.anyobj->BoundingBox.X2 - obj->data.anyobj->BoundingBox.X1);
			case query_fields_height: PCB_QRY_RET_INT(res, obj->data.anyobj->BoundingBox.Y2 - obj->data.anyobj->BoundingBox.Y1);
			case query_fields_area:   PCB_QRY_RET_DBL(res, (double)(obj->data.anyobj->BoundingBox.Y2 - obj->data.anyobj->BoundingBox.Y1) * (double)(obj->data.anyobj->BoundingBox.X2 - obj->data.anyobj->BoundingBox.X1));
			default:;
		}
		PCB_QRY_RET_INV(res);
	}

	if (fh1 == query_fields_type)
		PCB_QRY_RET_INT(res, obj->type);

	switch(obj->type) {
/*		case PCB_OBJ_POINT:    return field_point(obj, fld, res);*/
		case PCB_OBJ_LINE:     return field_line(obj, fld, res);
		case PCB_OBJ_TEXT:     return field_text(obj, fld, res);
		case PCB_OBJ_POLYGON:  return field_polygon(obj, fld, res);
		case PCB_OBJ_ARC:      return field_arc(obj, fld, res);
		case PCB_OBJ_RAT:      return field_rat(obj, fld, res);
		case PCB_OBJ_PAD:      return field_pad(obj, fld, res);
		case PCB_OBJ_PIN:      return field_pin(obj, fld, res);
		case PCB_OBJ_VIA:      return field_via(obj, fld, res);
		case PCB_OBJ_ELEMENT:  return field_element(obj, fld, res);

		case PCB_OBJ_NET:      return field_net(obj, fld, res);
		case PCB_OBJ_LAYER:    return field_layer(obj, fld, res);

		case PCB_OBJ_ELINE:    return field_eline(obj, fld, res);
		case PCB_OBJ_EARC:     return field_earc(obj, fld, res);
		case PCB_OBJ_ETEXT:    return field_etext(obj, fld, res);
		default:;
	}

	return -1;
}

