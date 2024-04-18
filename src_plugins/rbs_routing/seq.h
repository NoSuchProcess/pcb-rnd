/* undo leftover from librnd's own rtree before including grbs' rtree */
#undef RTR
#undef RTRU
#undef RTREE_NO_TREE_TYPEDEFS

#include <libgrbs/grbs.h>
#include <libgrbs/snapshot.h>

#define RBSR_SEQ_MAX 256
#define RBS_ADIR_invalid -42

typedef struct rbsr_seq_addr_s {
	grbs_point_t *pt;
	grbs_arc_dir_t dir;
} rbsr_seq_addr_t;

typedef struct rbsr_seq_s {
	rbsr_map_t map;
	grbs_2net_t *tn;

	rbsr_seq_addr_t consider; /* next point to route to, before click; valid if .adir != RBS_ADIR_invalid */
	rbsr_seq_addr_t path[RBSR_SEQ_MAX];
	long used; /* number of path items already accepted */

	rnd_coord_t last_x, last_y; /* last point coords on the path for the tool code */

	rnd_coord_t rlast_x, rlast_y; /* last points realized in the last redraw */
	grbs_snapshot_t *snap;
} rbsr_seq_t;


/* Start seqing a routing from tx;ty; returns 0 on success */
int rbsr_seq_begin_at(rbsr_seq_t *rbss, pcb_board_t *pcb, rnd_layer_id_t lid, rnd_coord_t tx, rnd_coord_t ty, rnd_coord_t copper, rnd_coord_t clearance);
void rbsr_seq_end(rbsr_seq_t *rbss);

int rbsr_seq_consider(rbsr_seq_t *rbss, rnd_coord_t tx, rnd_coord_t ty, int *need_redraw_out);
int rbsr_seq_accept(rbsr_seq_t *rbss);

