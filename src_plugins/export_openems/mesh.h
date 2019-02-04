#include "layer.h"
#include "vtc0.h"
#include "vtr0.h"
#include <libfungw/fungw.h>

typedef struct {
	vtc0_t user_line; /* input: lines forced by the user */
	vtr0_t user_dens; /* input: density forced by the user */
	vtc0_t edge;      /* input: around object edge - lines that must be in the mesh */
	vtr0_t dens;      /* input: density ranges; data[0].c is the target density */
	vtc0_t result;    /* resulting line coordinates */
} pcb_mesh_lines_t;

typedef enum {
	PCB_MESH_HORIZONTAL, /* variable y coord (horizontal lines) */
	PCB_MESH_VERTICAL,   /* variable x coord (vertical lines) */
	PCB_MESH_Z,
	PCB_MESH_max
} pcb_mesh_dir_t;

typedef struct {
	pcb_layer_t *layer;                    /* input layer (objects are picked up from this layer) */
	pcb_layer_t *ui_layer_xy, *ui_layer_z; /* optional UI layers to draw the mesh on */
	char *ui_name_xy;                      /* name of the UI layer */
	pcb_coord_t dens_obj, dens_gap;        /* target density: distance between mesh lines above objects and above gaps */
	pcb_coord_t min_space;                 /* make sure there's always at least this much space between two mesh lines */
	pcb_coord_t def_subs_thick;            /* default substrate thickness */
	pcb_coord_t def_copper_thick;          /* default copper thickness */
	pcb_mesh_lines_t line[PCB_MESH_max];   /* actual lines of the mesh */
	const char *bnd[6];                    /* temporary: boundary conditions */
	pcb_coord_t z_bottom_copper;           /* z coordinate of the bottom copper layer, along the z-mesh (0 is the top copper) */
	int pml;                               /* add pml cells around the exterior of the existing mesh of "perfectly matched" impedance */
	int subslines;                         /* number of mesh lines in substrate (z) */
	pcb_coord_t dens_air;                  /* mesh line density (spacing) in air */
	pcb_coord_t max_air;                   /* how far out to mesh in air */
	unsigned hor:1;                        /* enable adding horizontal mesh lines */
	unsigned ver:1;                        /* enable adding vertical mesh lines */
	unsigned smooth:1;                     /* if set, avoid jumps in the meshing by gradually changing meshing distance: x and y direction */
	unsigned smoothz:1;                    /* if set, avoid jumps in the meshing by gradually changing meshing distance: z direction */
	unsigned air_top:1;                    /* add mesh lines in air above the top of the board */
	unsigned air_bot:1;                    /* add mesh lines in air below the top of the board */
	unsigned noimpl:1;                     /* when set, do not add extra implicit mesh lines, keep the explicit ones only */
} pcb_mesh_t;

extern const char pcb_acts_mesh[];
extern const char pcb_acth_mesh[];
fgw_error_t pcb_act_mesh(fgw_arg_t *res, int oargc, fgw_arg_t *oargv);

/* Get one of the configured meshes */
pcb_mesh_t *pcb_mesg_get(const char *name);
