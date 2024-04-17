int rbsr_seq_begin_at(rbsr_seq_t *rbsq, pcb_board_t *pcb, rnd_layer_id_t lid, rnd_coord_t tx, rnd_coord_t ty, rnd_coord_t copper, rnd_coord_t clearance)
{
	grbs_point_t *start;

	rbsr_map_pcb(&rbsq->map, pcb, lid);
	rbsr_map_debug_draw(&rbsq->map, "rbsq1.svg");
	rbsr_map_debug_dump(&rbsq->map, "rbsq1.dump");

	rbsq->map.grbs.user_data = rbsq;
#if 0
	rbsq->map.grbs.coll_ingore_tn_line = coll_ingore_tn_line;
	rbsq->map.grbs.coll_ingore_tn_arc = coll_ingore_tn_arc;
	rbsq->map.grbs.coll_ingore_tn_point = coll_ingore_tn_point;
#endif

	start = rbsr_find_point_by_center(&rbsq->map, tx, ty);
	if (start == NULL) {
		rnd_message(RND_MSG_ERROR, "No suitable starting point\n");
		return -1;
	}

	rbsq->tn = grbs_2net_new(&rbsq->map.grbs, RBSR_R2G(copper), RBSR_R2G(clearance));

	rbsq->snap = grbs_snapshot_save(&rbsq->map.grbs);

	return 0;
}


int rbsr_seq_consider(rbsr_seq_t *rbsq, rnd_coord_t tx, rnd_coord_t ty)
{
	grbs_point_t *end;

	end = rbsr_find_point_by_center(&rbsq->map, tx, ty);
	if (end != NULL) {
	
	}

	return -1;
}
