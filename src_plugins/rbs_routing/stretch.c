
int rbsr_stretch_line_begin(rbsr_stretch_t *rbss, pcb_board_t *pcb, rnd_layer_id_t lid, pcb_line_t *line)
{
	grbs_line_t *gl;

	rbsr_map_pcb(&rbss->map, pcb, lid);
	rbsr_map_debug_draw(&rbss->map, "rbss1.svg");
	rbsr_map_debug_dump(&rbss->map, "rbss1.dump");

	gl = htpp_get(&rbss->map.robj2grbs, line);
rnd_trace("line=%p -> %p\n", line, gl);
	if (gl == NULL) {
		rnd_message(RND_MSG_ERROR, "rbsr_stretch_line_begin(): can't stretch this line (not in the grbs map)\n");
		return -1;
	}

	htpp_pop(&rbss->map.robj2grbs, line);
	grbs_path_remove_line(&rbss->map.grbs, gl);

	rbsr_map_debug_draw(&rbss->map, "rbss2.svg");
	rbsr_map_debug_dump(&rbss->map, "rbss2.dump");

	return 0;
}

void rbsr_stretch_line_end(rbsr_stretch_t *rbss)
{

}

