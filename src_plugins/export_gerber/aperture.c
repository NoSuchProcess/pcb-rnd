
#include "config.h"

#include <stdlib.h>

#include "pcb-printf.h"
#include "math_helper.h"
#include "aperture.h"

void init_aperture_list(aperture_list_t *list)
{
	list->data = NULL;
	list->count = 0;
}

void uninit_aperture_list(aperture_list_t *list)
{
	aperture_t *search = list->data;
	aperture_t *next;
	while (search) {
		next = search->next;
		free(search);
		search = next;
	}
	init_aperture_list(list);
}

aperture_t *add_aperture(aperture_list_t *list, pcb_coord_t width, aperture_shape_t shape)
{
	static int aperture_count;

	aperture_t *app = (aperture_t *) malloc(sizeof *app);
	if (app == NULL)
		return NULL;

	app->width = width;
	app->shape = shape;
	app->dCode = DCODE_BASE + aperture_count++;
	app->next = list->data;

	list->data = app;
	++list->count;

	return app;
}

aperture_t *find_aperture(aperture_list_t *list, pcb_coord_t width, aperture_shape_t shape)
{
	aperture_t *search;

	/* we never draw zero-width lines */
	if (width == 0)
		return NULL;

	/* Search for an appropriate aperture. */
	for (search = list->data; search; search = search->next)
		if (search->width == width && search->shape == shape)
			return search;

	/* Failing that, create a new one */
	return add_aperture(list, width, shape);
}

void fprint_aperture(FILE *f, aperture_t *aptr)
{
	switch (aptr->shape) {
	case ROUND:
		pcb_fprintf(f, "%%ADD%dC,%.4mi*%%\r\n", aptr->dCode, aptr->width);
		break;
	case SQUARE:
		pcb_fprintf(f, "%%ADD%dR,%.4miX%.4mi*%%\r\n", aptr->dCode, aptr->width, aptr->width);
		break;
	case OCTAGON:
		pcb_fprintf(f, "%%AMOCT%d*5,0,8,0,0,%.4mi,22.5*%%\r\n"
								"%%ADD%dOCT%d*%%\r\n", aptr->dCode, (pcb_coord_t) ((double) aptr->width / PCB_COS_22_5_DEGREE), aptr->dCode, aptr->dCode);
		break;
	}
}
