#include <genht/htpp.h>

#include "layer.h"

#include "../src_plugins/lib_netmap/map_2nets.h"

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

	htpp_t term4incident;  /* key: pcb_any_obj_t *; value: grbs_point_t *; grbs point to use for incident lines string/ending at each terminal */
	htpp_t robj2grbs;      /* key: pcb_any_obj_t *; value: grbs_arc_t * or grbs_line_t *;  pcb-rnd routing object (line/arc) to grbs object */

	pcb_2netmap_t twonets;

	pcb_layer_t *ui_layer;

} rbsr_map_t;

/* conversion between rnd coords and grbs coords forth and back; use micrometers for grbs */
#define RBSR_R2G(v)  ((double)(v)/1000.0)
#define RBSR_G2R(v)  ((rnd_coord_t)rnd_round((v)*1000.0))

#define RBSR_WIREFRAME_FLAG user_flg1

int rbsr_map_pcb(rbsr_map_t *dst, pcb_board_t *pcb, rnd_layer_id_t lid);
void rbsr_map_uninit(rbsr_map_t *dst);


void rbsr_map_debug_draw(rbsr_map_t *rbs, const char *fn);
void rbsr_map_debug_dump(rbsr_map_t *rbs, const char *fn);

/*** utility ***/

/* Return the point whose center point is close enough to cx;cy or NULL if
   nothing is close */
grbs_point_t *rbsr_find_point_by_center(rbsr_map_t *rbs, rnd_coord_t cx, rnd_coord_t cy);

/* Return the point that is close enough to cx;cy or NULL if nothing is close */
grbs_point_t *rbsr_find_point(rbsr_map_t *rbs, rnd_coord_t cx, rnd_coord_t cy);

/* Same as rbsr_find_point but searches with a bigger box (of width 2*delta) */
grbs_point_t *rbsr_find_point_thick(rbsr_map_t *rbs, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t delta);

/* whether two coords are matching within 10 nm */
#define   RCRDEQ_DELTA   10
#define   GCRDEQ_DELTA   RBSR_R2G(RCRDEQ_DELTA)
RND_INLINE int gcrdeq(double c1, double c2)
{
	double d = c1-c2;
	return ((d > -GCRDEQ_DELTA) && (d < +GCRDEQ_DELTA));
}

RND_INLINE int rcrdeq(rnd_coord_t c1, rnd_coord_t c2)
{
	rnd_coord_t d = c1-c2;
	return ((d > -RCRDEQ_DELTA) && (d < +RCRDEQ_DELTA));
}
