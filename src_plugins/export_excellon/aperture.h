#ifndef PCB_APERTURE_H
#define PCB_APERTURE_H

/*** generic aperture ***/

enum aperture_shape_e {
	ROUND,												/* Shaped like a circle */
	OCTAGON,											/* octagonal shape */
	SQUARE												/* Shaped like a square */
};
typedef enum aperture_shape_e aperture_shape_t;

/* This is added to the global aperture array indexes to get gerber
   dcode and macro numbers.  */
#define DCODE_BASE 11

typedef struct aperture {
	int dCode;										/* The RS-274X D code */
	pcb_coord_t width;									/* Size in pcb units */
	aperture_shape_t shape;					/* ROUND/SQUARE etc */
	struct aperture *next;
} aperture_t;

typedef struct {
	aperture_t *data;
	int count;
} aperture_list_t;

void init_aperture_list(aperture_list_t *list);
void uninit_aperture_list(aperture_list_t *list);

/* Create and add a new aperture to the list */
aperture_t *add_aperture(aperture_list_t *list, pcb_coord_t width, aperture_shape_t shape);

/* Fetch an aperture from the list with the specified
 *  width/shape, creating a new one if none exists */
aperture_t *find_aperture(aperture_list_t *list, pcb_coord_t width, aperture_shape_t shape);

/*** drill ***/

typedef struct {
	pcb_coord_t diam;
	pcb_coord_t x;
	pcb_coord_t y;

	/* for slots */
	int is_slot;
	pcb_coord_t x2;
	pcb_coord_t y2;
} pcb_pending_drill_t;

#define GVT(x) vtpdr_ ## x
#define GVT_ELEM_TYPE pcb_pending_drill_t
#define GVT_SIZE_TYPE size_t
#define GVT_DOUBLING_THRS 2048
#define GVT_START_SIZE 32
#define GVT_FUNC
/*#define GVT_SET_NEW_BYTES_TO 0*/

#include <genvector/genvector_impl.h>
#define GVT_REALLOC(vect, ptr, size)  realloc(ptr, size)
#define GVT_FREE(vect, ptr)           free(ptr)
#include <genvector/genvector_undef.h>

typedef struct pcb_drill_ctx_s {
	vtpdr_t obj;
	aperture_list_t apr;
} pcb_drill_ctx_t;

void pcb_drill_init(pcb_drill_ctx_t *ctx);
void pcb_drill_uninit(pcb_drill_ctx_t *ctx);
pcb_pending_drill_t *pcb_drill_new_pending(pcb_drill_ctx_t *ctx, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_coord_t diam);
void pcb_drill_sort(pcb_drill_ctx_t *ctx);

#endif
