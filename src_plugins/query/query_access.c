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

#include "global.h"
#include "data.h"
#include "query_access.h"
#include "query_exec.h"
#include "misc.h"
#include "layer.h"

#define APPEND(_ctx_, _type_, _obj_, _parenttype_, _parent_) \
do { \
	pcb_objlist_t *lst = (pcb_objlist_t *)_ctx_; \
	pcb_obj_t *o = malloc(sizeof(pcb_obj_t)); \
	o->type = _type_; \
	o->data.any = _obj_; \
	o->parent_type = _parenttype_; \
	o->parent.any = _parent_; \
	pcb_objlist_append(lst, o); \
} while(0)

static int list_layer_cb(void *ctx, PCBType *pcb, LayerType *layer, int enter)
{
	if (enter)
		APPEND(ctx, PCB_OBJ_LAYER, layer, PCB_PARENT_DATA, pcb->Data);
	return 0;
}

static void list_line_cb(void *ctx, PCBType *pcb, LayerType *layer, LineType *line)
{
	APPEND(ctx, PCB_OBJ_LINE, line, PCB_PARENT_LAYER, layer);
}

static void list_arc_cb(void *ctx, PCBType *pcb, LayerType *layer, ArcType *arc)
{
	APPEND(ctx, PCB_OBJ_ARC, arc, PCB_PARENT_LAYER, layer);
}

static void list_text_cb(void *ctx, PCBType *pcb, LayerType *layer, TextType *text)
{
	APPEND(ctx, PCB_OBJ_TEXT, text, PCB_PARENT_LAYER, layer);
}

static void list_poly_cb(void *ctx, PCBType *pcb, LayerType *layer, PolygonType *poly)
{
	APPEND(ctx, PCB_OBJ_POLYGON, poly, PCB_PARENT_LAYER, layer);
}

static int list_element_cb(void *ctx, PCBType *pcb, ElementType *element, int enter)
{
	if (enter)
		APPEND(ctx, PCB_OBJ_ELEMENT, element, PCB_PARENT_DATA, pcb->Data);
	return 0;
}

static void list_eline_cb(void *ctx, PCBType *pcb, ElementType *element, LineType *line)
{
	APPEND(ctx, PCB_OBJ_ELINE, line, PCB_PARENT_ELEMENT, element);
}

static void list_earc_cb(void *ctx, PCBType *pcb, ElementType *element, ArcType *arc)
{
	APPEND(ctx, PCB_OBJ_EARC, arc, PCB_PARENT_ELEMENT, element);
}

static void list_etext_cb(void *ctx, PCBType *pcb, ElementType *element, TextType *text)
{
	APPEND(ctx, PCB_OBJ_ETEXT, text, PCB_PARENT_ELEMENT, element);
}

static void list_epin_cb(void *ctx, PCBType *pcb, ElementType *element, PinType *pin)
{
	APPEND(ctx, PCB_OBJ_PIN, pin, PCB_PARENT_ELEMENT, element);
}

static void list_epad_cb(void *ctx, PCBType *pcb, ElementType *element, PadType *pad)
{
	APPEND(ctx, PCB_OBJ_PAD, pad, PCB_PARENT_ELEMENT, element);
}

static void list_via_cb(void *ctx, PCBType *pcb, PinType *via)
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
	if (_f_->type != PCBQ_FIELD) \
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

static int field_layer(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	LayerType *l = obj->data.layer;
	const char *s1, *s2;

	fld2str_req(s1, fld, 0);
	fld2str_opt(s2, fld, 1);

	if ((s1[0] == 'a') && (s1[1] == '\0'))
		PCB_QRY_RET_STR(res, AttributeGetFromList(&l->Attributes, s2));

	if (s2 != NULL)
		PCB_QRY_RET_INV(res);

	if (strcmp(s1, "name") == 0)
		PCB_QRY_RET_STR(res, l->Name);
	if (strcmp(s1, "visible") == 0)
		PCB_QRY_RET_INT(res, l->On);

	if (strcmp(s1, "position") == 0)
		PCB_QRY_RET_INT(res, pcb_layer_flags(GetLayerNumber(PCB->Data, l)) & PCB_LYT_ANYWHERE);

	if (strcmp(s1, "type") == 0)
		PCB_QRY_RET_INT(res, pcb_layer_flags(GetLayerNumber(PCB->Data, l)) & PCB_LYT_ANYTHING);

	PCB_QRY_RET_INV(res);
}

static int field_layer_from_ptr(LayerType *l, pcb_qry_node_t *fld, pcb_qry_val_t *res)
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
	LineType *l = obj->data.line;
	const char *s1, *s2;

	fld2str_req(s1, fld, 0);
	fld2str_opt(s2, fld, 1);

	if ((s1[0] == 'a') && (s1[1] == '\0'))
		PCB_QRY_RET_STR(res, AttributeGetFromList(&l->Attributes, s2));

	if ((s1[0] == 'p') && (s1[1] == '\0')) {
		if (strcmp(s2, "x1") == 0)  PCB_QRY_RET_INT(res, l->Point1.X);
		if (strcmp(s2, "y1") == 0)  PCB_QRY_RET_INT(res, l->Point1.Y);
		if (strcmp(s2, "x2") == 0)  PCB_QRY_RET_INT(res, l->Point1.X);
		if (strcmp(s2, "y2") == 0)  PCB_QRY_RET_INT(res, l->Point1.Y);
		if (strcmp(s2, "thickness") == 0)  PCB_QRY_RET_INT(res, l->Thickness);
		if (strcmp(s2, "clearance") == 0)  PCB_QRY_RET_INT(res, l->Clearance);
	}

	if (strcmp(s1, "layer") == 0) {
		if (obj->parent_type == PCB_PARENT_LAYER)
			return field_layer_from_ptr(obj->parent.layer, fld->next, res);
		else
			PCB_QRY_RET_INV(res);
	}

	PCB_QRY_RET_INV(res);
}

static int field_arc(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	ArcType *a = obj->data.arc;
	const char *s1, *s2;

	fld2str_req(s1, fld, 0);
	fld2str_opt(s2, fld, 1);

	if ((s1[0] == 'p') && (s1[1] == '\0')) {
		if (strcmp(s2, "x") == 0)  PCB_QRY_RET_INT(res, a->X);
		if (strcmp(s2, "y") == 0)  PCB_QRY_RET_INT(res, a->Y);
		if (strcmp(s2, "thickness") == 0)  PCB_QRY_RET_INT(res, a->Thickness);
		if (strcmp(s2, "clearance") == 0)  PCB_QRY_RET_INT(res, a->Clearance);
#warning TODO: this is really angle.start and angle.delta
		if (strcmp(s2, "start") == 0)  PCB_QRY_RET_INT(res, a->StartAngle);
		if (strcmp(s2, "delta") == 0)  PCB_QRY_RET_INT(res, a->Delta);
	}

	if (strcmp(s1, "layer") == 0) {
		if (obj->parent_type == PCB_PARENT_LAYER)
			return field_layer_from_ptr(obj->parent.layer, fld->next, res);
		else
			PCB_QRY_RET_INV(res);
	}

	PCB_QRY_RET_INV(res);
}

static int field_text(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	TextType *t = obj->data.text;
	const char *s1, *s2;

	fld2str_req(s1, fld, 0);
	fld2str_opt(s2, fld, 1);

	if ((s1[0] == 'a') && (s1[1] == '\0'))
		PCB_QRY_RET_STR(res, AttributeGetFromList(&t->Attributes, s2));

	if ((s1[0] == 'p') && (s1[1] == '\0')) {
		if (strcmp(s2, "x") == 0)  PCB_QRY_RET_INT(res, t->X);
		if (strcmp(s2, "y") == 0)  PCB_QRY_RET_INT(res, t->Y);
		if (strcmp(s2, "scale") == 0)  PCB_QRY_RET_INT(res, t->Scale);
		if (strcmp(s2, "rotation") == 0)  PCB_QRY_RET_INT(res, t->Direction);
		if (strcmp(s2, "string") == 0)  PCB_QRY_RET_STR(res, t->TextString);
	}

	if (strcmp(s1, "layer") == 0) {
		if (obj->parent_type == PCB_PARENT_LAYER)
			return field_layer_from_ptr(obj->parent.layer, fld->next, res);
		else
			PCB_QRY_RET_INV(res);
	}

	PCB_QRY_RET_INV(res);
}

static int field_polygon(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	PolygonType *p = obj->data.polygon;
	const char *s1, *s2;

	fld2str_req(s1, fld, 0);
	fld2str_opt(s2, fld, 1);

	if ((s1[0] == 'a') && (s1[1] == '\0'))
		PCB_QRY_RET_STR(res, AttributeGetFromList(&p->Attributes, s2));

	if ((s1[0] == 'p') && (s1[1] == '\0')) {
		if (strcmp(s2, "points") == 0)  PCB_QRY_RET_INT(res, p->PointN);
	}

	if (strcmp(s1, "layer") == 0) {
		if (obj->parent_type == PCB_PARENT_LAYER)
			return field_layer_from_ptr(obj->parent.layer, fld->next, res);
		else
			PCB_QRY_RET_INV(res);
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

static int field_pad(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	PadType *p = obj->data.pad;
	const char *s1, *s2;

	fld2str_req(s1, fld, 0);
	fld2str_opt(s2, fld, 1);

	if ((s1[0] == 'a') && (s1[1] == '\0'))
		PCB_QRY_RET_STR(res, AttributeGetFromList(&p->Attributes, s2));

	if ((s1[0] == 'p') && (s1[1] == '\0')) {
		if (strcmp(s2, "x1") == 0)  PCB_QRY_RET_INT(res, p->Point1.X);
		if (strcmp(s2, "y1") == 0)  PCB_QRY_RET_INT(res, p->Point1.Y);
		if (strcmp(s2, "x2") == 0)  PCB_QRY_RET_INT(res, p->Point1.X);
		if (strcmp(s2, "y2") == 0)  PCB_QRY_RET_INT(res, p->Point1.Y);
		if (strcmp(s2, "thickness") == 0)  PCB_QRY_RET_INT(res, p->Thickness);
		if (strcmp(s2, "clearance") == 0)  PCB_QRY_RET_INT(res, p->Clearance);
		if (strcmp(s2, "mask") == 0)  PCB_QRY_RET_INT(res, p->Mask);
		if (strcmp(s2, "name") == 0)  PCB_QRY_RET_STR(res, p->Name);
		if (strcmp(s2, "number") == 0)  PCB_QRY_RET_STR(res, p->Number);
	}

	if (strcmp(s1, "layer") == 0)
		return layer_of_obj(fld->next, res, PCB_LYT_COPPER | (TEST_FLAG(PCB_FLAG_ONSOLDER, p) ? PCB_LYT_BOTTOM : PCB_LYT_TOP));

	if (s2 != NULL)
		PCB_QRY_RET_INV(res);

	if (strcmp(s1, "element") != 0) {
#warning TODO: call the element
	}

	PCB_QRY_RET_INV(res);
}

static int field_via(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	PinType *p = obj->data.via;
	const char *s1, *s2;

	fld2str_req(s1, fld, 0);
	fld2str_opt(s2, fld, 1);

	if ((s1[0] == 'a') && (s1[1] == '\0'))
		PCB_QRY_RET_STR(res, AttributeGetFromList(&p->Attributes, s2));

	if ((s1[0] == 'p') && (s1[1] == '\0')) {
		if (strcmp(s2, "x") == 0)  PCB_QRY_RET_INT(res, p->X);
		if (strcmp(s2, "y") == 0)  PCB_QRY_RET_INT(res, p->Y);
		if (strcmp(s2, "thickness") == 0)  PCB_QRY_RET_INT(res, p->Thickness);
		if (strcmp(s2, "clearance") == 0)  PCB_QRY_RET_INT(res, p->Clearance);
		if (strcmp(s2, "hole") == 0)  PCB_QRY_RET_INT(res, p->DrillingHole);
		if (strcmp(s2, "mask") == 0)  PCB_QRY_RET_INT(res, p->Mask);
		if (strcmp(s2, "name") == 0)  PCB_QRY_RET_STR(res, p->Name);
		if (strcmp(s2, "number") == 0)  PCB_QRY_RET_STR(res, p->Number);
	}
	PCB_QRY_RET_INV(res);
}

static int field_pin(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	const char *s1, *s2;

	fld2str_req(s1, fld, 0);
	fld2str_opt(s2, fld, 1);

	if (strcmp(s1, "element") == 0) {
		if (s2 != NULL)
			PCB_QRY_RET_INV(res);
#warning TODO: call the element
		PCB_QRY_RET_INV(res);
	}
	return field_via(obj, fld, res);
}


static int field_element(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	ElementType *p = obj->data.element;
	const char *s1, *s2;

	fld2str_req(s1, fld, 0);
	fld2str_opt(s2, fld, 1);

	if ((s1[0] == 'a') && (s1[1] == '\0'))
		PCB_QRY_RET_STR(res, AttributeGetFromList(&p->Attributes, s2));

	if ((s1[0] == 'p') && (s1[1] == '\0')) {
		if (strcmp(s2, "x") == 0)  PCB_QRY_RET_INT(res, p->MarkX);
		if (strcmp(s2, "y") == 0)  PCB_QRY_RET_INT(res, p->MarkY);
		if (strcmp(s2, "name") == 0)  PCB_QRY_RET_STR(res, p->Name[NAMEONPCB_INDEX].TextString);
		if (strcmp(s2, "description") == 0)  PCB_QRY_RET_STR(res, p->Name[DESCRIPTION_INDEX].TextString);
		if (strcmp(s2, "value") == 0)  PCB_QRY_RET_STR(res, p->Name[VALUE_INDEX].TextString);
	}
	PCB_QRY_RET_INV(res);
}

static int field_element_from_ptr(ElementType *e, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	pcb_obj_t tmp;
	tmp.type = PCB_OBJ_ELEMENT;
	tmp.data.layer = e;
	tmp.parent_type = PCB_PARENT_DATA;
	tmp.parent.data = PCB->Data;
	return field_element(&tmp, fld, res);
}


static int field_net(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
/*	const char *s1, *s2;

	fld2str_req(s1, fld, 0);
	fld2str_opt(s2, fld, 1);*/

	PCB_QRY_RET_INV(res);
}


static int field_eline(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	LineType *l = obj->data.line;
	const char *s1;

	fld2str_req(s1, fld, 0);

	if (strcmp(s1, "layer") == 0)
		return layer_of_obj(fld->next, res, PCB_LYT_SILK | (TEST_FLAG(PCB_FLAG_ONSOLDER, l) ? PCB_LYT_BOTTOM : PCB_LYT_TOP));

	if (strcmp(s1, "element") == 0) {
		const char *s2;
		fld2str_req(s2, fld, 1);
		if (s2 != NULL) {
			if (obj->parent_type == PCB_PARENT_ELEMENT)
				return field_element_from_ptr(obj->parent.element, fld->next, res);
			else
				PCB_QRY_RET_INV(res);
		}
		else {
#warning TODO: return the element
			PCB_QRY_RET_INV(res);
		}
	}

	return field_line(obj, fld, res);
}

static int field_earc(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	ArcType *a = obj->data.arc;
	const char *s1;

	fld2str_req(s1, fld, 0);

	if (strcmp(s1, "layer") == 0)
		return layer_of_obj(fld->next, res, PCB_LYT_SILK | (TEST_FLAG(PCB_FLAG_ONSOLDER, a) ? PCB_LYT_BOTTOM : PCB_LYT_TOP));
	return field_arc(obj, fld, res);
}

static int field_etext(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	TextType *t = obj->data.text;
	const char *s1;

	fld2str_req(s1, fld, 0);

	if (strcmp(s1, "layer") == 0)
		return layer_of_obj(fld->next, res, PCB_LYT_SILK | (TEST_FLAG(PCB_FLAG_ONSOLDER, t) ? PCB_LYT_BOTTOM : PCB_LYT_TOP));
	return field_text(obj, fld, res);
}

/***/

int pcb_qry_obj_field(pcb_qry_val_t *objval, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	const char *s1, *s2;
	pcb_obj_t *obj;

	if (objval->type != PCBQ_VT_OBJ)
		return -1;
	obj = &objval->data.obj;

	fld2str_req(s1, fld, 0);
	fld2str_opt(s2, fld, 1);

	if ((s1[0] == 'a') && (s1[1] == '\0')) {
		if (s2 == NULL)
			return -1;
		if (!PCB_OBJ_IS_CLASS(obj->type, PCB_OBJ_CLASS_OBJ))
			PCB_QRY_RET_INV(res);
		PCB_QRY_RET_STR(res, AttributeGetFromList(&obj->data.anyobj->Attributes, s2));
	}

	if ((s1[0] == 'I') && (s1[1] == 'D') && (s1[2] == '\0')) {
		if (!PCB_OBJ_IS_CLASS(obj->type, PCB_OBJ_CLASS_OBJ))
			PCB_QRY_RET_INV(res);
		PCB_QRY_RET_INT(res, obj->data.anyobj->ID);
	}

	if (strcmp(s1, "bbox") == 0) {
		if (!PCB_OBJ_IS_CLASS(obj->type, PCB_OBJ_CLASS_OBJ))
			PCB_QRY_RET_INV(res);
		if (s2 == NULL)
			return -1;
		if (s2[0] == 'x') {
			if (s2[1] == '1')
				PCB_QRY_RET_INT(res, obj->data.anyobj->BoundingBox.X1);
			if (s2[1] == '2')
				PCB_QRY_RET_INT(res, obj->data.anyobj->BoundingBox.X2);
			return -1;
		}
		if (s2[0] == 'y') {
			if (s2[1] == '1')
				PCB_QRY_RET_INT(res, obj->data.anyobj->BoundingBox.Y1);
			if (s2[1] == '2')
				PCB_QRY_RET_INT(res, obj->data.anyobj->BoundingBox.Y2);
			return -1;
		}
		if (s2[0] == 'w') /* width */
			PCB_QRY_RET_INT(res, obj->data.anyobj->BoundingBox.X2 - obj->data.anyobj->BoundingBox.X1);
		if (s2[0] == 'h') /* height */
			PCB_QRY_RET_INT(res, obj->data.anyobj->BoundingBox.Y2 - obj->data.anyobj->BoundingBox.Y1);
		if (s2[0] == 'a') /* area */
			PCB_QRY_RET_INT(res, (obj->data.anyobj->BoundingBox.Y2 - obj->data.anyobj->BoundingBox.Y1) * (obj->data.anyobj->BoundingBox.X2 - obj->data.anyobj->BoundingBox.X1));
	}

	if (strcmp(s1, "type") == 0)
		PCB_QRY_RET_INT(res, obj->type);

	switch(obj->type) {
/*		case PCB_OBJ_POINT:    return field_point(obj, fld, res, s1, s2);*/
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


