#include "layer.h"
#include "vtc0.h"
#include "vtr0.h"

typedef struct {
	vtc0_t fixed;    /* input: fixed lines that must be in the mesh */
	vtr0_t ranges;   /* input: density ranges */
	vtc0_t result;   /* resulting line coordinates */
} pcb_mesh_lines_t;

typedef struct {
	pcb_layer_t *layer;
	pcb_coord_t dens_obj, dens_air; /* target density: distance between mesh lines above objects and above air */
	pcb_mesh_lines_t hor, ver; /* horizontal (variable y) and vertical (variable x) lines of the mesh */
} pcb_mesh_t;

extern const char pcb_acts_mesh[];
extern const char pcb_acth_mesh[];
int pcb_act_mesh(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y);
