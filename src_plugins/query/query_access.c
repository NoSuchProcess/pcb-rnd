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

#define APPEND(_ctx_, _type_, _obj_) \
do { \
	pcb_objlist_t *lst = (pcb_objlist_t *)_ctx_; \
	pcb_obj_t *o = malloc(sizeof(pcb_obj_t)); \
	o->type = _type_; \
	o->data.any = _obj_; \
	pcb_objlist_append(lst, o); \
} while(0)

static int list_layer_cb(void *ctx, PCBType *pcb, LayerType *layer, int enter)
{
	if (enter)
		APPEND(ctx, PCB_OBJ_LAYER, layer);
	return 0;
}

static void list_line_cb(void *ctx, PCBType *pcb, LayerType *layer, LineType *line)
{
	APPEND(ctx, PCB_OBJ_LINE, line);
}

static void list_arc_cb(void *ctx, PCBType *pcb, LayerType *layer, ArcType *arc)
{
	APPEND(ctx, PCB_OBJ_ARC, arc);
}

static void list_text_cb(void *ctx, PCBType *pcb, LayerType *layer, TextType *text)
{
	APPEND(ctx, PCB_OBJ_TEXT, text);
}

static void list_poly_cb(void *ctx, PCBType *pcb, LayerType *layer, PolygonType *poly)
{
	APPEND(ctx, PCB_OBJ_POLYGON, poly);
}

static int list_element_cb(void *ctx, PCBType *pcb, ElementType *element, int enter)
{
	if (enter)
		APPEND(ctx, PCB_OBJ_ELEMENT, element);
	return 0;
}

static void list_eline_cb(void *ctx, PCBType *pcb, ElementType *element, LineType *line)
{
	APPEND(ctx, PCB_OBJ_ELINE, line);
}

static void list_earc_cb(void *ctx, PCBType *pcb, ElementType *element, ArcType *arc)
{
	APPEND(ctx, PCB_OBJ_EARC, arc);
}

static void list_etext_cb(void *ctx, PCBType *pcb, ElementType *element, TextType *text)
{
	APPEND(ctx, PCB_OBJ_ETEXT, text);
}

static void list_epin_cb(void *ctx, PCBType *pcb, ElementType *element, PinType *pin)
{
	APPEND(ctx, PCB_OBJ_PIN, pin);
}

static void list_epad_cb(void *ctx, PCBType *pcb, ElementType *element, PadType *pad)
{
	APPEND(ctx, PCB_OBJ_PAD, pad);
}

static void list_via_cb(void *ctx, PCBType *pcb, PinType *via)
{
	APPEND(ctx, PCB_OBJ_VIA, via);
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

static int field_layer(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res, const char *s1, const char *s2)
{
	LayerType *l = obj->data.layer;

	if ((s1[0] == 'a') && (s1[1] == '\0'))
		PCB_QRY_RET_STR(res, AttributeGetFromList(&l->Attributes, s2));

	if (s2 != NULL)
		PCB_QRY_RET_INV(res);

	if (strcmp(s1, "name") == 0)
		PCB_QRY_RET_STR(res, l->Name);
	if (strcmp(s1, "visible") == 0)
		PCB_QRY_RET_INT(res, l->On);

	PCB_QRY_RET_INV(res);
}

/* process from .layer */
static int layer_of_obj(pcb_qry_node_t *fld, const char *s2, pcb_qry_val_t *res, pcb_layer_type_t mask)
{
	int id;
	pcb_obj_t tmp;
	const char *s3;

	if (pcb_layer_list(mask, &id, 1) != 1)
		PCB_QRY_RET_INV(res);

	if (s2 == NULL) {
		res->type = PCBQ_VT_OBJ;
		res->data.obj.type = PCB_OBJ_LAYER;
		res->data.obj.data.layer = PCB->Data->Layer+id;
		return 0;
	}

	tmp.type = PCB_OBJ_LAYER;
	tmp.data.layer = PCB->Data->Layer+id;

	if (fld->next != NULL) {
		if (fld->next->type != PCBQ_FIELD)
			return -1;
		s3 = fld->next->data.str;
	}
	else
		s3 = NULL;

	return field_layer(&tmp, fld, res, s2, s3);
}


static int field_line(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res, const char *s1, const char *s2)
{
	LineType *l = obj->data.line;

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

	PCB_QRY_RET_INV(res);
}

static int field_arc(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res, const char *s1, const char *s2)
{
	ArcType *a = obj->data.arc;

	if ((s1[0] == 'p') && (s1[1] == '\0')) {
		if (strcmp(s2, "x") == 0)  PCB_QRY_RET_INT(res, a->X);
		if (strcmp(s2, "y") == 0)  PCB_QRY_RET_INT(res, a->Y);
		if (strcmp(s2, "thickness") == 0)  PCB_QRY_RET_INT(res, a->Thickness);
		if (strcmp(s2, "clearance") == 0)  PCB_QRY_RET_INT(res, a->Clearance);
#warning TODO: this is really angle.start and angle.delta
		if (strcmp(s2, "start") == 0)  PCB_QRY_RET_INT(res, a->StartAngle);
		if (strcmp(s2, "delta") == 0)  PCB_QRY_RET_INT(res, a->Delta);
	}

	PCB_QRY_RET_INV(res);
}

static int field_text(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res, const char *s1, const char *s2)
{
	TextType *t = obj->data.text;

	if ((s1[0] == 'a') && (s1[1] == '\0'))
		PCB_QRY_RET_STR(res, AttributeGetFromList(&t->Attributes, s2));

	if ((s1[0] == 'p') && (s1[1] == '\0')) {
		if (strcmp(s2, "x") == 0)  PCB_QRY_RET_INT(res, t->X);
		if (strcmp(s2, "y") == 0)  PCB_QRY_RET_INT(res, t->Y);
		if (strcmp(s2, "scale") == 0)  PCB_QRY_RET_INT(res, t->Scale);
		if (strcmp(s2, "rotation") == 0)  PCB_QRY_RET_INT(res, t->Direction);
		if (strcmp(s2, "string") == 0)  PCB_QRY_RET_STR(res, t->TextString);
	}

	PCB_QRY_RET_INV(res);
}

static int field_polygon(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res, const char *s1, const char *s2)
{
	PolygonType *p = obj->data.polygon;

	if ((s1[0] == 'a') && (s1[1] == '\0'))
		PCB_QRY_RET_STR(res, AttributeGetFromList(&p->Attributes, s2));

	if ((s1[0] == 'p') && (s1[1] == '\0')) {
		if (strcmp(s2, "points") == 0)  PCB_QRY_RET_INT(res, p->PointN);
	}

	PCB_QRY_RET_INV(res);
}

static int field_rat(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res, const char *s1, const char *s2)
{
#warning TODO
	PCB_QRY_RET_INV(res);
}

static int field_pad(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res, const char *s1, const char *s2)
{
	PCB_QRY_RET_INV(res);
}

static int field_pin(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res, const char *s1, const char *s2)
{
	PCB_QRY_RET_INV(res);
}

static int field_via(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res, const char *s1, const char *s2)
{
	PCB_QRY_RET_INV(res);
}

static int field_element(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res, const char *s1, const char *s2)
{
	PCB_QRY_RET_INV(res);
}

static int field_net(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res, const char *s1, const char *s2)
{
	PCB_QRY_RET_INV(res);
}


static int field_eline(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res, const char *s1, const char *s2)
{
	LineType *l = obj->data.line;
	if (strcmp(s1, "layer") == 0)
		return layer_of_obj(fld->next, s2, res, PCB_LYT_SILK | (TEST_FLAG(PCB_FLAG_ONSOLDER, l) ? PCB_LYT_BOTTOM : PCB_LYT_TOP));

	return field_line(obj, fld, res, s1, s2);
}

static int field_earc(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res, const char *s1, const char *s2)
{
	ArcType *a = obj->data.arc;
	if (strcmp(s1, "layer") == 0)
		return layer_of_obj(fld->next, s2, res, PCB_LYT_SILK | (TEST_FLAG(PCB_FLAG_ONSOLDER, a) ? PCB_LYT_BOTTOM : PCB_LYT_TOP));
	return field_arc(obj, fld, res, s1, s2);
}

static int field_etext(pcb_obj_t *obj, pcb_qry_node_t *fld, pcb_qry_val_t *res, const char *s1, const char *s2)
{
	TextType *t = obj->data.text;
	if (strcmp(s1, "layer") == 0)
		return layer_of_obj(fld->next, s2, res, PCB_LYT_SILK | (TEST_FLAG(PCB_FLAG_ONSOLDER, t) ? PCB_LYT_BOTTOM : PCB_LYT_TOP));
	return field_text(obj, fld, res, s1, s2);
}

/***/

int pcb_qry_obj_field(pcb_qry_val_t *objval, pcb_qry_node_t *fld, pcb_qry_val_t *res)
{
	const char *s1, *s2;
	pcb_obj_t *obj;
	pcb_qry_node_t *fld2;

	if (objval->type != PCBQ_VT_OBJ)
		return -1;
	obj = &objval->data.obj;

	if (fld->type != PCBQ_FIELD)
		return -1;
	s1 = fld->data.str;
	fld2 = fld->next;
	if (fld2 != NULL) {
		if (fld2->type != PCBQ_FIELD)
			return -1;
		s2 = fld2->data.str;
	}
	else
		s2 = NULL;

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
		case PCB_OBJ_LINE:     return field_line(obj, fld, res, s1, s2);
		case PCB_OBJ_TEXT:     return field_text(obj, fld, res, s1, s2);
		case PCB_OBJ_POLYGON:  return field_polygon(obj, fld, res, s1, s2);
		case PCB_OBJ_ARC:      return field_arc(obj, fld, res, s1, s2);
		case PCB_OBJ_RAT:      return field_rat(obj, fld, res, s1, s2);
		case PCB_OBJ_PAD:      return field_pad(obj, fld, res, s1, s2);
		case PCB_OBJ_PIN:      return field_pin(obj, fld, res, s1, s2);
		case PCB_OBJ_VIA:      return field_via(obj, fld, res, s1, s2);
		case PCB_OBJ_ELEMENT:  return field_element(obj, fld, res, s1, s2);

		case PCB_OBJ_NET:      return field_net(obj, fld, res, s1, s2);
		case PCB_OBJ_LAYER:    return field_layer(obj, fld, res, s1, s2);

		case PCB_OBJ_ELINE:    return field_eline(obj, fld, res, s1, s2);
		case PCB_OBJ_EARC:     return field_earc(obj, fld, res, s1, s2);
		case PCB_OBJ_ETEXT:    return field_etext(obj, fld, res, s1, s2);
		default:;
	}

	return -1;
}


