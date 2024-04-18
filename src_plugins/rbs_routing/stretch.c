#include "remove.h"
#include "install.c"

static int rbsr_stretch_install_2net(rbsr_stretch_t *rbss, grbs_2net_t *tn)
{
	pcb_layer_t *ly = pcb_get_layer(rbss->map.pcb->Data, rbss->map.lid);
	return rbsr_install_2net(ly, tn);
}


static int coll_ingore_tn_line(grbs_t *grbs, grbs_2net_t *tn, grbs_line_t *l)
{
	rnd_trace("ign coll line\n");
	return 1;
}

static int coll_ingore_tn_arc(grbs_t *grbs, grbs_2net_t *tn, grbs_arc_t *a)
{
	rnd_trace("ign coll arc\n");
	return 1;
}

static int coll_ingore_tn_point(grbs_t *grbs, grbs_2net_t *tn, grbs_point_t *p)
{
	rnd_trace("ign coll pt\n");
	return 1;
}

RND_INLINE void schedule_remove(rbsr_stretch_t *rbss, void *user_data)
{
	pcb_2netmap_obj_t *obj = user_data;

	if (obj->orig != NULL)
		htpp_set(&rbss->removes, obj->orig, NULL);
}

RND_INLINE void stretch_get_addr(rbsr_stretch_t *rbss, grbs_addr_t *dst, grbs_arc_t *arc, grbs_arc_t **rem)
{
	grbs_arc_t *target;

	if (arc->r == 0) {
		/* incident */
		dst->type = ADDR_POINT;
		dst->obj.pt = arc->parent_pt;
	}
	else {
		rbss->from.type = ADDR_ARC_END | ADDR_ARC_CONVEX;
		target = arc->link_point.prev;
		arc->in_use = 0;
		gdl_remove(arc->link_2net.parent, arc, link_2net);
		dst->obj.arc = target;
		target->new_r = arc->r;
		target->new_sa = arc->sa;
		target->new_adir = (arc->da >= 0) ? +1 : -1;
		target->new_da = 0;
		target->new_in_use = 1;
		*rem = arc;
	}
	dst->last_real = NULL;
}

int rbsr_stretch_line_begin(rbsr_stretch_t *rbss, pcb_board_t *pcb, pcb_line_t *line, rnd_coord_t tx, rnd_coord_t ty)
{
	grbs_line_t *gl;
	grbs_2net_t *orig_tn;
	rnd_layer_id_t lid = pcb_layer_id(pcb->Data, line->parent.layer);
	grbs_arc_t *rem1 = NULL, *rem2 = NULL;

	htpp_init(&rbss->removes, ptrhash, ptrkeyeq);

	rbsr_map_pcb(&rbss->map, pcb, lid);
	rbsr_map_debug_draw(&rbss->map, "rbss1.svg");
	rbsr_map_debug_dump(&rbss->map, "rbss1.dump");

	rbss->map.grbs.user_data = rbss;
	rbss->map.grbs.coll_ingore_tn_line = coll_ingore_tn_line;
	rbss->map.grbs.coll_ingore_tn_arc = coll_ingore_tn_arc;
	rbss->map.grbs.coll_ingore_tn_point = coll_ingore_tn_point;

	gl = htpp_get(&rbss->map.robj2grbs, line);
	if (gl == NULL) {
		rnd_message(RND_MSG_ERROR, "rbsr_stretch_line_begin(): can't stretch this line (not in the grbs map)\n");
		return -1;
	}

	orig_tn = grbs_arc_parent_2net(gl->a1);
	stretch_get_addr(rbss, &rbss->from, gl->a1, &rem1);
	stretch_get_addr(rbss, &rbss->to, gl->a2, &rem2);

	rbss->tn = grbs_2net_new(&rbss->map.grbs, orig_tn->copper, orig_tn->clearance);

	htpp_pop(&rbss->map.robj2grbs, line);
	schedule_remove(rbss, gl->user_data);
	grbs_path_remove_line(&rbss->map.grbs, gl);

	if (rem1 != NULL) {
		schedule_remove(rbss, rem1->user_data);
		grbs_path_remove_arc(&rbss->map.grbs, rem1);
	}
	if (rem2 != NULL) {
		schedule_remove(rbss, rem2->user_data);
		grbs_path_remove_arc(&rbss->map.grbs, rem2);
	}

	rbss->via = grbs_point_new(&rbss->map.grbs, RBSR_R2G(tx), RBSR_R2G(ty), RBSR_R2G(RND_MM_TO_COORD(2.1)), RBSR_R2G(RND_MM_TO_COORD(0.1)));
	rbss->snap = grbs_snapshot_save(&rbss->map.grbs);


	rbsr_map_debug_draw(&rbss->map, "rbss2.svg");
	rbsr_map_debug_dump(&rbss->map, "rbss2.dump");

	return 0;
}

void rbsr_stretch_line_end(rbsr_stretch_t *rbss)
{
	htpp_entry_t *e;

	/* apply object removals */
	for(e = htpp_first(&rbss->removes); e != NULL; e = htpp_next(&rbss->removes, e)) {
		pcb_any_obj_t *obj = e->key;
		pcb_remove_object(obj->type, obj->parent.layer, obj, obj);
	}

	/* tune existing objects and install new objects */
	rbsr_stretch_install_2net(rbss, rbss->tn);

	/* No need to free rbss->via separately: it's part of the grbs map */
	htpp_uninit(&rbss->removes);
}
int rbsr_stretch_line_to_coords(rbsr_stretch_t *rbss, rnd_coord_t tx, rnd_coord_t ty)
{
	grbs_addr_t *a1, *a2;
	int new_via = (rbss->via == NULL), seg;

	grbs_snapshot_restore(rbss->snap);

	/* move the via point */
	grbs_point_unreg(&rbss->map.grbs, rbss->via);
	rbss->via->x = RBSR_R2G(tx);
	rbss->via->y = RBSR_R2G(ty);
	grbs_point_reg(&rbss->map.grbs, rbss->via);

	/* remove the previous version of the bypass arc on the attached line */
	for(seg = 0; seg < GRBS_MAX_SEG; seg++) {
		grbs_arc_t *a;
		for(a = gdl_first(&rbss->via->arcs[seg]); a != NULL; a = gdl_next(&rbss->via->arcs[seg], a)) {
			if (grbs_arc_parent_2net(a) == rbss->tn) {
				grbs_del_arc(&rbss->map.grbs, a);
				break;
			}
		}
	}
	grbs_clean_unused_sentinel(&rbss->map.grbs, rbss->via);

	/* try routing the bypass anew */
	a1 = grbs_path_next(&rbss->map.grbs, rbss->tn, &rbss->from, rbss->via, GRBS_ADIR_CONVEX_CW);
	if (a1 == NULL) {
		rnd_message(RND_MSG_ERROR, "rbsr_stretch_line_to_coord(): failed to route to a1\n");
		return -1;
	}

	rbss->to.obj.arc->new_in_use = 0; /* avoid spiral detection */
	a2 = path_path_next_to_addr(&rbss->map.grbs, rbss->tn, a1, &rbss->to);
	if (a2 == NULL) {
		rnd_message(RND_MSG_ERROR, "rbsr_stretch_line_to_coord(): failed to route to a2\n");
		return -1;
	}

	rbsr_map_debug_draw(&rbss->map, "rbss3.svg");
	rbsr_map_debug_dump(&rbss->map, "rbss3.dump");


	/* realize backward */
	grbs_path_realize(&rbss->map.grbs,rbss->tn, a2, 0);
	grbs_path_realize(&rbss->map.grbs, rbss->tn, a1, 0);
	grbs_path_realize(&rbss->map.grbs, rbss->tn, &rbss->from, 0);


	rbsr_map_debug_draw(&rbss->map, "rbss4.svg");
	rbsr_map_debug_dump(&rbss->map, "rbss4.dump");

	return 0;
}
