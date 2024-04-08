typedef struct rbsr_stretch_s {
	rbsr_map_t map;
	grbs_addr_t *from, *to;

	grbs_point_t *via;
} rbsr_stretch_t;


/* Start stretching a routing line; returns 0 on success */
int rbsr_stretch_line_begin(rbsr_stretch_t *rbss, pcb_board_t *pcb, rnd_layer_id_t lid, pcb_line_t *line);
void rbsr_stretch_line_end(rbsr_stretch_t *rbss);
