#include "config.h"
#include "conf_core.h"

#include "spoke.h"
#include "pointdata.h"
#include "wire.h"

void spoke_pos_at_wire_node(spoke_t *sp, wirelist_node_t *w_node, pcb_coord_t *x, pcb_coord_t *y)
{
	pcb_box_t *obj_bbox = &((pointdata_t *) sp->p->data)->obj->BoundingBox;
	pcb_coord_t half_obj_w, half_obj_h;
	pcb_coord_t delta;

	assert(w_node != NULL);

	delta = (w_node->item->thickness + 1)/2;
	delta += conf_core.design.bloat;
	w_node = w_node->next;
	WIRELIST_FOREACH(w, w_node)
		delta += w->thickness;
		delta += conf_core.design.bloat;
	WIRELIST_FOREACH_END();

	half_obj_w = (obj_bbox->X2 - obj_bbox->X1 + 1) / 2;
	half_obj_h = (obj_bbox->Y2 - obj_bbox->Y1 + 1) / 2;

	switch(sp->dir) {
	case SPOKE_DIR_1PI4:
		*x = sp->p->pos.x + half_obj_w + delta;
		*y = (-sp->p->pos.y) - half_obj_h - delta;
		break;
	case SPOKE_DIR_3PI4:
		*x = sp->p->pos.x - half_obj_w - delta;
		*y = (-sp->p->pos.y) - half_obj_h - delta;
		break;
	case SPOKE_DIR_5PI4:
		*x = sp->p->pos.x - half_obj_w - delta;
		*y = (-sp->p->pos.y) + half_obj_h + delta;
		break;
	case SPOKE_DIR_7PI4:
		*x = sp->p->pos.x + half_obj_w + delta;
		*y = (-sp->p->pos.y) + half_obj_h + delta;
		break;
	}
}
