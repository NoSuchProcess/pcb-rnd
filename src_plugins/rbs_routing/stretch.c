
int rbsr_stretch_line_begin(rbsr_stretch_t *rbss, pcb_board_t *pcb, rnd_layer_id_t lid, pcb_line_t *line)
{
	grbs_line_t *gl;

	rbsr_map_pcb(&rbss->map, pcb, lid);
	rbsr_map_debug_draw(&rbss->map, "rbss1.svg");
	rbsr_map_debug_dump(&rbss->map, "rbss1.dump");

	gl = htpp_get(&rbss->map.robj2grbs, line);
	if (gl == NULL) {
		rnd_message(RND_MSG_ERROR, "rbsr_stretch_line_begin(): can't stretch this line (not in the grbs map)\n");
		return -1;
	}

	return 0;
}

void rbsr_stretch_line_end(rbsr_stretch_t *rbss)
{

}

