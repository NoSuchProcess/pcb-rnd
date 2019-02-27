#ifndef PCB_APERTURE_H
#define PCB_APERTURE_H

#include <stdio.h>

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
aperture_t *find_aperture(const aperture_list_t *list, pcb_coord_t width, aperture_shape_t shape);

/* Output aperture data to the file */
void fprint_aperture(FILE * f, aperture_t *aptr);

#endif
