#include "config.h"
#include "conf_core.h"

#include "spoke.h"
#include "pointdata.h"
#include "wire.h"


void spoke_init(spoke_t *sp, spoke_dir_t dir, point_t *p)
{
	vtp0_init(&sp->slots);
	sp->dir = dir;
	sp->p = p;
	/* TODO:
	spoke_bbox(sp);
	rnd_r_insert_entry(spoke_tree, sp);
	*/
}

void spoke_uninit(spoke_t *sp)
{
	/* TODO:
	rnd_r_delete_entry(spoke_tree, &pd->spokes[i]);
	*/
	vtp0_uninit(&sp->slots);
}

static void spoke_pos(spoke_t *sp, rnd_coord_t spacing, rnd_coord_t *x, rnd_coord_t *y)
{
	rnd_rnd_box_t *obj_bbox = &((pointdata_t *) sp->p->data)->obj->bbox_naked;
	rnd_coord_t half_obj_w, half_obj_h;

	half_obj_w = (obj_bbox->X2 - obj_bbox->X1 + 1) / 2;
	half_obj_h = (obj_bbox->Y2 - obj_bbox->Y1 + 1) / 2;

	switch(sp->dir) {
	case SPOKE_DIR_1PI4:
		*x = sp->p->pos.x + half_obj_w + spacing;
		*y = (-sp->p->pos.y) - half_obj_h - spacing;
		break;
	case SPOKE_DIR_3PI4:
		*x = sp->p->pos.x - half_obj_w - spacing;
		*y = (-sp->p->pos.y) - half_obj_h - spacing;
		break;
	case SPOKE_DIR_5PI4:
		*x = sp->p->pos.x - half_obj_w - spacing;
		*y = (-sp->p->pos.y) + half_obj_h + spacing;
		break;
	case SPOKE_DIR_7PI4:
		*x = sp->p->pos.x + half_obj_w + spacing;
		*y = (-sp->p->pos.y) + half_obj_h + spacing;
		break;
	}
}

void spoke_pos_at_wire_point(spoke_t *sp, wire_point_t *wp, rnd_coord_t *x, rnd_coord_t *y)
{
	pointdata_t *pd = wp->p->data;
	wirelist_node_t *w_node = wp->wire_node;
	rnd_coord_t spacing;

	assert(w_node != NULL);

	spacing = (w_node->item->thickness + 1)/2;
	spacing += conf_core.design.bloat;
	w_node = w_node->next;
	if (wirelist_get_index(pd->uturn_wires, w_node) != -1) {
		WIRELIST_FOREACH(w, wp->wire_node)
			spacing += w->thickness;
			spacing += conf_core.design.bloat;
		WIRELIST_FOREACH_END();
	}
	else {
		WIRELIST_FOREACH(w, w_node)
			spacing += w->thickness;
			spacing += conf_core.design.bloat;
		WIRELIST_FOREACH_END();
		WIRELIST_FOREACH(w, pd->uturn_wires)
			spacing += w->thickness;
			spacing += conf_core.design.bloat;
		WIRELIST_FOREACH_END();
	}

	spoke_pos(sp, spacing, x, y);
}

void spoke_pos_at_slot(spoke_t *sp, int slot, rnd_coord_t *x, rnd_coord_t *y)
{
	rnd_coord_t spacing = 0;
	int i;

	assert(slot < vtp0_len(&sp->slots));

	for (i = 0; i < slot; i++) {
		spacing += conf_core.design.bloat;
		spacing += ((ewire_t *) (sp->slots.array[i]))->wire->thickness;
	}
	spacing += conf_core.design.bloat;
	spacing += (((ewire_t *) (sp->slots.array[slot]))->wire->thickness + 1)/2;

	spoke_pos(sp, spacing, x, y);
}

void spoke_pos_at_end(spoke_t *sp, rnd_coord_t bloat, rnd_coord_t *x, rnd_coord_t *y)
{
	rnd_coord_t spacing = 0;
	int i;

	if (vtp0_len(&sp->slots) > 0) {
		for (i = 0; i < vtp0_len(&sp->slots) - 1; i++) {
			spacing += conf_core.design.bloat;
			spacing += ((ewire_t *) (sp->slots.array[i]))->wire->thickness;
		}
		if (bloat < 0)
			spacing += (((ewire_t *) (sp->slots.array[vtp0_len(&sp->slots) - 1]))->wire->thickness + 1)/2;
		else
			spacing += ((ewire_t *) (sp->slots.array[vtp0_len(&sp->slots) - 1]))->wire->thickness;
	}

	if (bloat >= 0) {
		spacing += conf_core.design.bloat;
		spacing += bloat;
	}

	spoke_pos(sp, spacing, x, y);
}

void spoke_insert_wire_at_slot(spoke_t *sp, int slot_num, ewire_t *ew)
{
	if (vtp0_in_bound(&sp->slots, slot_num) && sp->slots.array[slot_num] != NULL) {
		int i;
		/* move all slots, starting from the slot_num, one step outside */
		for (i = slot_num; i < vtp0_len(&sp->slots); i++) {
			ewire_point_t *ewp = ewire_get_point_at_slot(sp->slots.array[i], sp, i);
			assert(ewp != NULL);
			ewp->sp_slot = i + 1;
		}
		vtp0_alloc_insert(&sp->slots, slot_num, 1);
	}
	vtp0_set(&sp->slots, slot_num, ew);
}
