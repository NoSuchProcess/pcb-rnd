#include "layer.h"

/* undo leftover from librnd's own rtree before including grbs' rtree */
#undef RTR
#undef RTRU
#undef RTREE_NO_TREE_TYPEDEFS

#include <libgrbs/grbs.h>

typedef struct rbsr_map_s {

	/* the map is created for a single layer into grbs */
	pcb_board_t *pcb;
	rnd_layer_id_t lid;
	grbs_t grbs;

} rbsr_map_t;

/* conversion between rnd coords and grbs coords forth and back; use micrometers for grbs */
#define RBSR_R2G(v)  ((double)(v)/1000.0)
#define RBSR_G2R(v)  ((rnd_coord_t)rnd_round((v)*1000.0))

int rbsr_map_pcb(rbsr_map_t *dst, pcb_board_t *pcb, rnd_layer_id_t lid);

