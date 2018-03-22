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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */
#include "config.h"

#include <stddef.h>
#include <ctype.h>
#include "conf_core.h"
#include "flag_str.h"
#include "global_typedefs.h"
#include "const.h"
#include "error.h"
#include "obj_common.h"
#include "obj_arc_ui.h"
#include "obj_pstk.h"

const char *pcb_obj_type_name(pcb_objtype_t type)
{
	switch(type) {
		case PCB_OBJ_VOID:    return "void";
		case PCB_OBJ_POINT:   return "point";
		case PCB_OBJ_LINE:    return "line";
		case PCB_OBJ_TEXT:    return "text";
		case PCB_OBJ_POLY:    return "polygon";
		case PCB_OBJ_ARC:     return "arc";
		case PCB_OBJ_RAT:     return "ratline";
		case PCB_OBJ_PAD:     return "pad";
		case PCB_OBJ_PIN:     return "pin";
		case PCB_OBJ_VIA:     return "via";
		case PCB_OBJ_PSTK:    return "padstack";
		case PCB_OBJ_ELEMENT: return "element";
		case PCB_OBJ_SUBC:    return "subcircuit";
		case PCB_OBJ_NET:     return "net";
		case PCB_OBJ_LAYER:   return "layer";
		case PCB_OBJ_ELINE:
		case PCB_OBJ_EARC:
		case PCB_OBJ_ETEXT:
			break;
	}
	return "<unknown/composite>";
}

/* returns a pointer to an objects bounding box;
 * data is valid until the routine is called again
 */
int pcb_obj_get_bbox(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_box_t *res)
{
	switch (Type) {
	case PCB_TYPE_LINE:
	case PCB_TYPE_ARC:
	case PCB_TYPE_TEXT:
	case PCB_TYPE_POLY:
	case PCB_TYPE_PAD:
	case PCB_TYPE_PIN:
	case PCB_TYPE_PSTK:
	case PCB_TYPE_ELEMENT_NAME:
		*res = *(pcb_box_t *)Ptr2;
		return 0;
	case PCB_TYPE_VIA:
	case PCB_TYPE_ELEMENT:
	case PCB_TYPE_SUBC:
		*res = *(pcb_box_t *)Ptr1;
		return 0;
	case PCB_TYPE_POLY_POINT:
	case PCB_TYPE_LINE_POINT:
		*res = *(pcb_box_t *)Ptr3;
		return 0;
	case PCB_TYPE_ARC_POINT:
		return pcb_obj_ui_arc_point_bbox(Type, Ptr1, Ptr2, Ptr3, res);
	default:
		pcb_message(PCB_MSG_ERROR, "Request for bounding box of unsupported type %d\n", Type);
		*res = *(pcb_box_t *)Ptr2;
		return -1;
	}
}



/* current object ID; incremented after each creation of an object */
long int ID = 1;

pcb_bool pcb_create_being_lenient = pcb_false;

/* ---------------------------------------------------------------------------
 *  Set the lenience mode.
 */
void pcb_create_be_lenient(pcb_bool v)
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

void pcb_obj_add_attribs(void *obj, const pcb_attribute_list_t *src)
{
	pcb_any_obj_t *o = obj;
	if (src == NULL)
		return;
	pcb_attribute_copy_all(&o->Attributes, src);
}

void pcb_obj_center(const pcb_any_obj_t *obj, pcb_coord_t *x, pcb_coord_t *y)
{
	switch (obj->type) {
		case PCB_OBJ_PIN:
			*x = ((const pcb_pin_t *)(obj))->X;
			*y = ((const pcb_pin_t *)(obj))->Y;
			break;
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

void pcb_obj_attrib_post_change(pcb_attribute_list_t *list, const char *name, const char *value)
{
	pcb_any_obj_t *obj = (pcb_any_obj_t *)(((char *)list) - offsetof(pcb_any_obj_t, Attributes));
	if (strcmp(name, "term") == 0) {
		const char *inv;
		obj->term = value;
		inv = pcb_obj_id_invalid(obj->term);
		if (inv != NULL)
			pcb_message(PCB_MSG_ERROR, "Invalid character '%c' in terminal name (term attribute) '%s'\n", *inv, obj->term);
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
}

const char *pcb_obj_id_invalid(const char *id)
{
	const char *s;
	if (id != NULL)
		for(s = id; *s != '\0'; s++) {
			if (isalnum(*s))
				continue;
			switch(*s) {
				case '_': case '.': case '$': case ':': continue;
			}
			return s;
		}
	return NULL;
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

pcb_coord_t pcb_obj_clearance_at(pcb_board_t *pcb, const pcb_any_obj_t *o, pcb_layer_t *at)
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
