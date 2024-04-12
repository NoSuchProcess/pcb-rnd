
static void coll_report_arc_cb(grbs_t *grbs, grbs_2net_t *tn, grbs_2net_t *coll_tn, grbs_arc_t *coll_arc)
{
	rnd_trace("coll arc report\n");
}

static void coll_report_pt_cb(grbs_t *grbs, grbs_2net_t *tn, grbs_point_t *coll_pt)
{
	rbsr_stretch_t *rbss = grbs->user_data;
	rnd_trace("coll pt report at %f;%f\n", coll_pt->x, coll_pt->y);
	rbss->coll_pt = coll_pt;
}


int rbsr_stretch_line_begin(rbsr_stretch_t *rbss, pcb_board_t *pcb, pcb_line_t *line)
{
	grbs_line_t *gl;
	grbs_2net_t *orig_tn;
	rnd_layer_id_t lid = pcb_layer_id(pcb->Data, line->parent.layer);

	rbsr_map_pcb(&rbss->map, pcb, lid);
	rbsr_map_debug_draw(&rbss->map, "rbss1.svg");
	rbsr_map_debug_dump(&rbss->map, "rbss1.dump");

	rbss->map.grbs.user_data = rbss;
	rbss->map.grbs.coll_report_arc_cb = coll_report_arc_cb;
	rbss->map.grbs.coll_report_pt_cb = coll_report_pt_cb;

	gl = htpp_get(&rbss->map.robj2grbs, line);
	if (gl == NULL) {
		rnd_message(RND_MSG_ERROR, "rbsr_stretch_line_begin(): can't stretch this line (not in the grbs map)\n");
		return -1;
	}

	if (gl->a1->r == 0) {
		rbss->from.type = ADDR_ARC_END | ADDR_POINT;
		rbss->from.obj.pt = gl->a1->parent_pt;
	}
	else {
		rbss->from.type = ADDR_ARC_END | ADDR_ARC_CONVEX;
		rbss->from.obj.arc = gl->a1;
	}
	rbss->from.last_real = NULL;
	gl->a1->new_r = gl->a1->r;
	gl->a1->new_sa = gl->a1->sa;
	gl->a1->new_adir = (gl->a1->da >= 0) ? +1 : -1;
	gl->a1->new_da = 0;
	gl->a1->new_in_use = 1;



	if (gl->a2->r == 0) {
		rbss->to.type = ADDR_ARC_END | ADDR_POINT;
		rbss->to.obj.pt = gl->a2->parent_pt;
	}
	else {
		rbss->to.type = ADDR_ARC_START | ADDR_ARC_CONVEX;
		rbss->to.obj.arc = gl->a2;
	}
	rbss->to.last_real = NULL;
	gl->a2->new_r = gl->a2->r;
	gl->a2->new_sa = gl->a2->sa;
	gl->a2->new_adir = (gl->a2->da >= 0) ? +1 : -1;
	gl->a2->new_da = 0;
	gl->a2->new_in_use = 1;

	orig_tn = grbs_arc_parent_2net(gl->a1);
	rbss->tn = grbs_2net_new(&rbss->map.grbs, orig_tn->copper, orig_tn->clearance);

	htpp_pop(&rbss->map.robj2grbs, line);
	grbs_path_remove_line(&rbss->map.grbs, gl);

	rbsr_map_debug_draw(&rbss->map, "rbss2.svg");
	rbsr_map_debug_dump(&rbss->map, "rbss2.dump");

	return 0;
}

void rbsr_stretch_line_end(rbsr_stretch_t *rbss)
{
	TODO("implement me");
	/* No need to free rbss->via separately: it's part of the grbs map */
}

int rbsr_stretch_line_to_coords(rbsr_stretch_t *rbss, rnd_coord_t tx, rnd_coord_t ty)
{
	grbs_addr_t *a1, *a2, *a3 = NULL;
	if (rbss->via != NULL) {
		int seg;

		grbs_point_unreg(&rbss->map.grbs, rbss->via);
		rbss->via->x = RBSR_R2G(tx);
		rbss->via->y = RBSR_R2G(ty);
		grbs_point_reg(&rbss->map.grbs, rbss->via);

		/* remove the previous version of the arc */
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
	}
	else
		rbss->via = grbs_point_new(&rbss->map.grbs, RBSR_R2G(tx), RBSR_R2G(ty), RBSR_R2G(RND_MM_TO_COORD(2.1)), RBSR_R2G(RND_MM_TO_COORD(0.1)));

	a1 = grbs_path_next(&rbss->map.grbs, rbss->tn, &rbss->from, rbss->via, GRBS_ADIR_CONVEX_CW);
	if (a1 == NULL) {
		rnd_message(RND_MSG_ERROR, "rbsr_stretch_line_to_coord(): failed to route to a2\n");
		return -1;
	}

	rbss->to.obj.arc->new_in_use = 0; /* avoid spiral detection */
	a2 = path_path_next_to_addr(&rbss->map.grbs, rbss->tn, a1, &rbss->to);
	if (a2 == NULL) {
		if (rbss->coll_pt != NULL) {
			rnd_trace("route around coll pt\n");
			a2 = grbs_path_next(&rbss->map.grbs, rbss->tn, a1, rbss->coll_pt, 0);
			a3 = path_path_next_to_addr(&rbss->map.grbs, rbss->tn, a2, &rbss->to);
			assert(a3 != NULL);
		}
		else {
			rnd_message(RND_MSG_ERROR, "rbsr_stretch_line_to_coord(): failed to route to a2\n");
			return -1;
		}
	}

	rbsr_map_debug_draw(&rbss->map, "rbss3.svg");
	rbsr_map_debug_dump(&rbss->map, "rbss3.dump");


	/* realize backward */
	if (a3 != NULL)
		grbs_path_realize(&rbss->map.grbs,rbss->tn, a3, 0);
	grbs_path_realize(&rbss->map.grbs,rbss->tn, a2, 0);
	grbs_path_realize(&rbss->map.grbs, rbss->tn, a1, 0);
	grbs_path_realize(&rbss->map.grbs, rbss->tn, &rbss->from, 0);


	rbsr_map_debug_draw(&rbss->map, "rbss4.svg");
	rbsr_map_debug_dump(&rbss->map, "rbss4.dump");

	return 0;
}
