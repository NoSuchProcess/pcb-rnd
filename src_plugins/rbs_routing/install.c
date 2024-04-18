static int rbsr_install_arc(rbsr_stretch_t *rbss, grbs_2net_t *tn, grbs_arc_t *arc)
{
	if (arc->user_data == NULL) { /* create new */
		pcb_arc_t *pa;
		pcb_layer_t *ly;
		double cx, cy, sa, da;

		if (arc->r == 0)
			return 0; /* do not create dummy arcs used for incident lines */

		ly = pcb_get_layer(rbss->map.pcb->Data, rbss->map.lid);
		sa = 180.0 - (arc->sa * RND_RAD_TO_DEG);
		da = - (arc->da * RND_RAD_TO_DEG);
		cx = arc->parent_pt->x;
		cy = arc->parent_pt->y;

		pa = pcb_arc_new(ly, RBSR_G2R(cx), RBSR_G2R(cy),
			RBSR_G2R(arc->r), RBSR_G2R(arc->r), sa, da,
			RBSR_G2R(tn->copper*2), RBSR_G2R(tn->clearance*2),
			pcb_flag_make(PCB_FLAG_CLEARLINE), 1);
	}
	else {
		TODO("verify existing");
	}
	return 0;
}

static int rbsr_install_line(rbsr_stretch_t *rbss, grbs_2net_t *tn, grbs_line_t *line)
{
	if (line->user_data == NULL) { /* create new */
		pcb_line_t *pl;
		pcb_layer_t *ly;

		if ((line->x1 == line->x2) && (line->y1 == line->y2))
			return 0;

		ly = pcb_get_layer(rbss->map.pcb->Data, rbss->map.lid);
		pl = pcb_line_new(ly,
			RBSR_G2R(line->x1), RBSR_G2R(line->y1),
			RBSR_G2R(line->x2), RBSR_G2R(line->y2),
			RBSR_G2R(tn->copper*2), RBSR_G2R(tn->clearance*2),
			pcb_flag_make(PCB_FLAG_CLEARLINE));
	}
	else {
		TODO("verify existing");
	}
	return 0;
}

static int rbsr_install_pt_arcs(rbsr_stretch_t *rbss, grbs_2net_t *tn, grbs_point_t *pt)
{
	int seg, res = 0;

	for(seg = 0; seg < GRBS_MAX_SEG; seg++) {
		grbs_arc_t *a = gdl_first(&pt->arcs[seg]);
		for(a = gdl_next(&pt->arcs[seg], a) ; a != NULL; a = gdl_next(&pt->arcs[seg], a))
			res |= rbsr_install_arc(rbss, tn, a);
	}

	return res;
}

int rbsr_install_2net(rbsr_stretch_t *rbss, grbs_2net_t *tn)
{
	grbs_arc_t *a, *prev = NULL;
	int res = 0;

	for(a = gdl_first(&tn->arcs); a != NULL; prev = a, a = gdl_next(&tn->arcs, a)) {
		if (prev != NULL)
			res |= rbsr_install_line(rbss, tn, a->sline);

		/* verify all arcs,their radius and angles may have changed due to new arc
		   inserted */
		res |= rbsr_install_pt_arcs(rbss, tn, a->parent_pt);
	}

	return res;
}
