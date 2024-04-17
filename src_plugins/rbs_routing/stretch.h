#include <libgrbs/snapshot.h>

typedef struct rbsr_stretch_s {
	rbsr_map_t map;
	grbs_addr_t from, to;
	grbs_2net_t *tn;

	grbs_point_t *via;

	grbs_snapshot_t *snap;

	htpp_t removes;        /* key: pcb_any_obj_t *; value: NULL; list of pcb-rnd objects to remove when the grbs route is copied back */

} rbsr_stretch_t;


/* Start stretching a routing line; returns 0 on success */
int rbsr_stretch_line_begin(rbsr_stretch_t *rbss, pcb_board_t *pcb, pcb_line_t *line, rnd_coord_t tx, rnd_coord_t ty);
void rbsr_stretch_line_end(rbsr_stretch_t *rbss);

/* Stretch the current line so it goes around tx;ty */
int rbsr_stretch_line_to_coords(rbsr_stretch_t *rbss, rnd_coord_t tx, rnd_coord_t ty);
