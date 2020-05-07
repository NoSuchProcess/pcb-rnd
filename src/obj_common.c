/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
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
 *
 */
#include "config.h"

#include <stddef.h>
#include <ctype.h>
#include "conf_core.h"
#include "flag_str.h"
#include <librnd/core/global_typedefs.h>
#include <librnd/core/error.h>
#include "obj_common.h"
#include "obj_arc_ui.h"
#include "obj_pstk.h"
#include "obj_subc.h"
#include "obj_subc_parent.h"
#include "obj_term.h"
#include "extobj.h"

const char *pcb_obj_type_name(pcb_objtype_t type)
{
	switch(type) {
		case PCB_OBJ_VOID:        return "void";
		case PCB_OBJ_LINE:        return "line";
		case PCB_OBJ_TEXT:        return "text";
		case PCB_OBJ_POLY:        return "polygon";
		case PCB_OBJ_ARC:         return "arc";
		case PCB_OBJ_RAT:         return "ratline";
		case PCB_OBJ_GFX:         return "gfx";
		case PCB_OBJ_PSTK:        return "padstack";
		case PCB_OBJ_SUBC:        return "subcircuit";
		case PCB_OBJ_NET:         return "net";
		case PCB_OBJ_NET_TERM:    return "net_term";
		case PCB_OBJ_LAYER:       return "layer";
		case PCB_OBJ_LAYERGRP:    return "layer_group";
	}
	switch((pcb_vobjtype_t)type) {
		case PCB_OBJ_LINE_POINT:  return "line_point";
		case PCB_OBJ_POLY_POINT:  return "poly_point";
		case PCB_OBJ_ARC_POINT:   return "arc_point";
		case PCB_OBJ_SUBC_PART:   return "subc_part";
		case PCB_OBJ_LOCKED:      return "locked";
		case PCB_OBJ_FLOATER:     return "floater";
	}
	return "<unknown/composite>";
}

/* returns a pointer to an objects bounding box;
 * data is valid until the routine is called again
 */
int pcb_obj_get_bbox_naked(int Type, void *Ptr1, void *Ptr2, void *Ptr3, rnd_rnd_box_t *res)
{
	switch (Type) {
	case PCB_OBJ_LINE:
	case PCB_OBJ_ARC:
	case PCB_OBJ_TEXT:
	case PCB_OBJ_POLY:
	case PCB_OBJ_PSTK:
	case PCB_OBJ_GFX:
		*res = ((pcb_any_obj_t *)Ptr2)->bbox_naked;
		return 0;
	case PCB_OBJ_SUBC:
		*res = ((pcb_any_obj_t *)Ptr1)->bbox_naked;
		return 0;
	case PCB_OBJ_POLY_POINT:
	case PCB_OBJ_LINE_POINT:
		*res = *(rnd_rnd_box_t *)Ptr3;
		return 0;
	case PCB_OBJ_ARC_POINT:
		return pcb_obj_ui_arc_point_bbox(Type, Ptr1, Ptr2, Ptr3, res);
	default:
		rnd_message(RND_MSG_ERROR, "Request for bounding box of unsupported type %d\n", Type);
		*res = *(rnd_rnd_box_t *)Ptr2;
		return -1;
	}
}



/* current object ID; incremented after each creation of an object */
long int ID = 1;

rnd_bool pcb_create_being_lenient = rnd_false;

void pcb_create_be_lenient(rnd_bool v)
{
	pcb_create_being_lenient = v;
}

void pcb_create_ID_bump(int min_id)
{
	if (ID < min_id)
		ID = min_id;
}

void pcb_create_ID_reset(void)
{
	ID = 1;
}

long int pcb_create_ID_get(void)
{
	return ID++;
}

static void pcb_attribute_copy_all_smart(rnd_attribute_list_t *dest, const rnd_attribute_list_t *src, pcb_any_obj_t *dstobj, pcb_any_obj_t *copy_from_any)
{
	int i;
	for (i = 0; i < src->Number; i++)
		rnd_attribute_put(dest, src->List[i].name, src->List[i].value);
}


void pcb_obj_add_attribs(pcb_any_obj_t *o, const rnd_attribute_list_t *src, pcb_any_obj_t *copy_from)
{
	if (src == NULL)
		return;
	if (copy_from)
		pcb_attribute_copy_all_smart(&o->Attributes, src, o, copy_from);
	else
		rnd_attribute_copy_all(&o->Attributes, src);
}

void pcb_obj_center(const pcb_any_obj_t *obj, rnd_coord_t *x, rnd_coord_t *y)
{
	switch (obj->type) {
		case PCB_OBJ_PSTK:
			*x = ((const pcb_pstk_t *)(obj))->x;
			*y = ((const pcb_pstk_t *)(obj))->y;
			break;
		case PCB_OBJ_ARC:
			pcb_arc_middle((const pcb_arc_t *)obj, x, y);
			break;
		default:
			*x = (obj->BoundingBox.X1 + obj->BoundingBox.X2) / 2;
			*y = (obj->BoundingBox.Y1 + obj->BoundingBox.Y2) / 2;
	}
}

void pcb_obj_attrib_post_change(rnd_attribute_list_t *list, const char *name, const char *value)
{
	pcb_any_obj_t *obj = (pcb_any_obj_t *)(((char *)list) - offsetof(pcb_any_obj_t, Attributes));
	if (strcmp(name, "term") == 0) {
		const char *inv;
		pcb_subc_t *subc = pcb_obj_parent_subc(obj);

		if ((subc != NULL) && (obj->term != NULL)) /* remove the old term */
			pcb_term_del(&subc->terminals, obj->term, obj);
		obj->term = value;
		if ((subc != NULL) && (obj->term != NULL)) /* add the new term */
			pcb_term_add(&subc->terminals, obj);
		inv = pcb_obj_id_invalid(obj->term);
		if (inv != NULL)
			rnd_message(RND_MSG_ERROR, "Invalid character '%c' in terminal name (term attribute) '%s'\n", *inv, obj->term);
	}
	else if (strcmp(name, "intconn") == 0) {
		long cid = 0;
		if (value != NULL) {
			char *end;
			cid = strtol(value, &end, 10);
			if (*end != '\0')
				cid = 0;
		}
		obj->intconn = cid;
	}
	else if (strcmp(name, "intnoconn") == 0) {
		long cid = 0;
		if (value != NULL) {
			char *end;
			cid = strtol(value, &end, 10);
			if (*end != '\0')
				cid = 0;
		}
		obj->intnoconn = cid;
	}
	else if (strncmp(name, "noexport", 8) == 0) {
		obj->noexport = 1;
		obj->noexport_named = (name[8] == ':');
	}
	else if (strncmp(name, "extobj::", 8) == 0) {
		const char *arg = name+8;
		if (strcmp(arg, "role") == 0)
			obj->extobj_role = value;
	}
	else if ((obj->type == PCB_OBJ_TEXT) && (strcmp(name, "tight_clearance") == 0)) {
		pcb_text_t *t = (pcb_text_t *)obj;
		t->tight_clearance = rnd_istrue(value);
	}
}

const char *pcb_obj_id_invalid(const char *id)
{
	const char *s;
	if (id != NULL)
		for(s = id; *s != '\0'; s++) {
			if (isalnum(*s))
				continue;
			switch(*s) {
				case '_': case '.': case '$': case '*': case ':': continue;
			}
			return s;
		}
	return NULL;
}

char *pcb_obj_id_fix(char *id)
{
	char *s;
	if (id == NULL)
		return NULL;

	for(s = id; *s != '\0'; s++) {
		if (isalnum(*s))
			continue;
		switch(*s) {
			case '_': case '.': case '$': case '*': case ':': continue;
			default: *s = '_';
		}
	}

	return id;
}


pcb_flag_values_t pcb_obj_valid_flags(unsigned long int objtype)
{
	pcb_flag_values_t res = 0;
	int n;

	for(n = 0; n < pcb_object_flagbits_len; n++)
		if (pcb_object_flagbits[n].object_types & objtype)
			res |= pcb_object_flagbits[n].mask;

	return res;
}

rnd_coord_t pcb_obj_clearance_at(pcb_board_t *pcb, const pcb_any_obj_t *o, pcb_layer_t *at)
{
	switch(o->type) {
		case PCB_OBJ_POLY:
			if (!(PCB_POLY_HAS_CLEARANCE((pcb_poly_t *)o)))
				return 0;
			return ((pcb_poly_t *)o)->Clearance;
		case PCB_OBJ_LINE:
			if (!(PCB_NONPOLY_HAS_CLEARANCE((pcb_line_t *)o)))
				return 0;
			return ((pcb_line_t *)o)->Clearance;
		case PCB_OBJ_TEXT:
			return 0;
		case PCB_OBJ_ARC:
			if (!(PCB_NONPOLY_HAS_CLEARANCE((pcb_arc_t *)o)))
				return 0;
			return ((pcb_arc_t *)o)->Clearance;
		case PCB_OBJ_PSTK:
			return obj_pstk_get_clearance(pcb, (pcb_pstk_t *)o, at);
		default:
			return 0;
	}
}

static void mmult(pcb_xform_mx_t dst, const pcb_xform_mx_t src)
{
	pcb_xform_mx_t tmp;

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

void pcb_xform_mx_rotate(pcb_xform_mx_t mx, double deg)
{
	pcb_xform_mx_t tr;

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

void pcb_xform_mx_translate(pcb_xform_mx_t mx, double xt, double yt)
{
	pcb_xform_mx_t tr;

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

void pcb_xform_mx_scale(pcb_xform_mx_t mx, double sx, double sy)
{
	pcb_xform_mx_t tr;

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

void pcb_xform_mx_shear(pcb_xform_mx_t mx, double sx, double sy)
{
	pcb_xform_mx_t tr;

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

void pcb_xform_mx_mirrorx(pcb_xform_mx_t mx)
{
	pcb_xform_mx_scale(mx, 0, -1);
}

void pcb_obj_pre(pcb_any_obj_t *o)
{
	switch(o->type) {
		case PCB_OBJ_LINE: pcb_line_pre((pcb_line_t *)o); break;
		case PCB_OBJ_TEXT: pcb_text_pre((pcb_text_t *)o); break;
		case PCB_OBJ_POLY: pcb_poly_pre((pcb_poly_t *)o); break;
		case PCB_OBJ_ARC:  pcb_arc_pre((pcb_arc_t *)o); break;
		case PCB_OBJ_GFX:  pcb_gfx_pre((pcb_gfx_t *)o); break;
		case PCB_OBJ_PSTK: pcb_pstk_pre((pcb_pstk_t *)o); break;
		default: break;
	}
}

void pcb_obj_post(pcb_any_obj_t *o)
{
	switch(o->type) {
		case PCB_OBJ_LINE: pcb_line_post((pcb_line_t *)o); break;
		case PCB_OBJ_TEXT: pcb_text_post((pcb_text_t *)o); break;
		case PCB_OBJ_POLY: pcb_poly_post((pcb_poly_t *)o); break;
		case PCB_OBJ_ARC:  pcb_arc_post((pcb_arc_t *)o); break;
		case PCB_OBJ_GFX:  pcb_gfx_post((pcb_gfx_t *)o); break;
		case PCB_OBJ_PSTK: pcb_pstk_post((pcb_pstk_t *)o); break;
		default: break;
	}
}

void pcb_obj_change_id(pcb_any_obj_t *obj, long int new_id)
{
	pcb_data_t *data;

	if (obj->parent_type == PCB_PARENT_DATA)
		data = obj->parent.data;
	else if (obj->parent_type == PCB_PARENT_LAYER)
		data = obj->parent.layer->parent.data;
	else
		return;

	pcb_obj_id_del(data, obj);
	obj->ID = new_id;
	pcb_obj_id_reg(data, obj);
}


void pcb_set_point_bounding_box(rnd_point_t *Pnt)
{
	Pnt->X2 = Pnt->X + 1;
	Pnt->Y2 = Pnt->Y + 1;
}

void pcb_obj_common_free(pcb_any_obj_t *o)
{
	if (o->override_color != NULL) {
		free(o->override_color);
		o->override_color = NULL;
	}
}

unsigned char *pcb_obj_common_get_thermal(pcb_any_obj_t *obj, unsigned long lid, rnd_bool_t alloc)
{
	if (obj->type == PCB_OBJ_PSTK)
		return pcb_pstk_get_thermal((pcb_pstk_t *)obj, lid, alloc);

	return &obj->thermal;
}

