#include <stdlib.h>
#include <assert.h>
#include "layout.h"
#include "src/change.h"
#include "src/const.h"
#include "src/board.h"
#include "src/move.h"
#include "src/conf_core.h"
#include "src/layer.h"

int layout_obj_coord(layout_object_t *obj, layout_object_coord_t coord)
{
	pcb_any_obj_t *o;

	if (obj == NULL)
		return -1;

	o = (pcb_any_obj_t *)obj->obj.l;

	/* bounding box is the same for any type */
	switch (coord) {
		case OC_BX1: return o->BoundingBox.X1;
		case OC_BX2: return o->BoundingBox.X2;
		case OC_BY1: return o->BoundingBox.Y1;
		case OC_BY2: return o->BoundingBox.Y2;
		case OC_OBJ: return -1;
		default: /* avoids warnings for unhandled requests we handle later, per object type */
			;
	}

	switch(obj->type) {
		case OM_LINE:
			switch (coord) {
				case OC_P1X: return obj->obj.l->Point1.X;
				case OC_P2X: return obj->obj.l->Point2.X;
				case OC_P1Y: return obj->obj.l->Point1.Y;
				case OC_P2Y: return obj->obj.l->Point2.Y;
				default: /* avoids warnings for unhandled requests we handled above */
				;
			}
			break;
		case OM_TEXT:
			switch (coord) {
				case OC_P1X:
				case OC_P2X: return obj->obj.t->X;
				case OC_P1Y:
				case OC_P2Y: return obj->obj.t->Y;
				default: /* avoids warnings for unhandled requests we handled above */
				;
			}
			break;
		case OM_VIA:
			switch (coord) {
				case OC_P1X:
				case OC_P2X: return obj->obj.v->X;
				case OC_P1Y:
				case OC_P2Y: return obj->obj.v->Y;
				default: /* avoids warnings for unhandled requests we handled above */
				;
			}
			break;
		case OM_PIN:
			switch (coord) {
				case OC_P1X:
				case OC_P2X: return obj->obj.pin->X;
				case OC_P1Y:
				case OC_P2Y: return obj->obj.pin->Y;
				default: /* avoids warnings for unhandled requests we handled above */
				;
			}
			break;
	}

	return -1;
}

layout_object_mask_t layout_obj_type(layout_object_t *obj)
{
	if (obj == NULL)
		return 0;
	return obj->type;
}

int layout_obj_move(layout_object_t *obj, layout_object_coord_t coord, int dx, int dy)
{
	void *what = NULL;;
	pcb_layer_t *ly;

	if (obj == NULL)
		return -1;

	ly = pcb_get_layer(PCB->Data, obj->layer);
	if (ly == NULL)
		return -1;

	switch(obj->type) {
		case OM_LINE:
			switch(coord) {
				case OC_OBJ:
					pcb_move_obj(PCB_TYPE_LINE_POINT, ly, obj->obj.l, &(obj->obj.l->Point2), dx, dy);
					/* intended falltrough */
				case OC_P1X:
				case OC_P1Y: what = &(obj->obj.l->Point1); break;
				case OC_P2X:
				case OC_P2Y: what = &(obj->obj.l->Point2); break;
				default: /* we do not handle anything else for now */
					;
			}
			pcb_move_obj(PCB_TYPE_LINE_POINT, ly, obj->obj.l, what, dx, dy);
			return 0;
		case OM_TEXT:
			pcb_move_obj(PCB_TYPE_TEXT, ly, obj->obj.t, obj->obj.t, dx, dy);
			return 0;
		case OM_VIA:
			pcb_move_obj(PCB_TYPE_VIA, obj->obj.v, obj->obj.v, obj->obj.v, dx, dy);
			return 0;
		case OM_PIN:
			pcb_move_obj(PCB_TYPE_PIN, obj->obj.pin, obj->obj.pin, obj->obj.pin, dx, dy);
			return 0;
		case OM_ARC:
			switch(coord) {
				case OC_OBJ:
					pcb_move_obj(PCB_TYPE_ARC, ly, obj->obj.a, obj->obj.a, dx, dy);
					return 0;
				default: /* we do not handle anything else for now */
					;
			}
			/*TODO: move endpoints! */
			break;
		case OM_POLYGON:
			if (obj->layer != -1) {
				pcb_move_obj(PCB_TYPE_POLYGON, PCB->Data->Layer + obj->layer, obj->obj.p, obj->obj.p, dx, dy);
				return 0;
			}
	}
	return -1;
}

int layout_arc_angles(layout_object_t *obj, int relative, int start, int delta)
{
	pcb_layer_t *ly;

	ly = pcb_get_layer(PCB->Data, obj->layer);
	if (ly == NULL)
		return -1;

	if (obj == NULL)
		return -1;
	if (obj->type != OM_ARC)
		return 1;
	if (relative) {
		start += obj->obj.a->StartAngle;
		delta += obj->obj.a->Delta;
	}
	pcb_arc_set_angles(ly, obj->obj.a, start, delta);
	return 0;
}
