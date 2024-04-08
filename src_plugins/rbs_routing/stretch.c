
int rbsr_stretch_line_begin(rbsr_stretch_t *rbss, pcb_board_t *pcb, pcb_line_t *line)
{
	grbs_line_t *gl;
	rnd_layer_id_t lid = pcb_layer_id(pcb->Data, line->parent.layer);

	rbsr_map_pcb(&rbss->map, pcb, lid);
	rbsr_map_debug_draw(&rbss->map, "rbss1.svg");
	rbsr_map_debug_dump(&rbss->map, "rbss1.dump");

	gl = htpp_get(&rbss->map.robj2grbs, line);
	if (gl == NULL) {
		rnd_message(RND_MSG_ERROR, "rbsr_stretch_line_begin(): can't stretch this line (not in the grbs map)\n");
		return -1;
	}

	rbss->from.type = ADDR_ARC_END;
	rbss->from.obj.arc = gl->a1;
	rbss->from.last_real = NULL;

	rbss->to.type = ADDR_ARC_START;
	rbss->to.obj.arc = gl->a2;
	rbss->to.last_real = NULL;

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
	if (rbss->via != NULL) {
		grbs_point_unreg(&rbss->map.grbs, rbss->via);
		rbss->via->x = RBSR_R2G(tx);
		rbss->via->y = RBSR_R2G(ty);
		grbs_point_reg(&rbss->map.grbs, rbss->via);
	}
	else
		rbss->via = grbs_point_new(&rbss->map.grbs, RBSR_R2G(tx), RBSR_R2G(ty), RBSR_R2G(RND_MM_TO_COORD(0.1)), RBSR_R2G(RND_MM_TO_COORD(0.1)));

}
