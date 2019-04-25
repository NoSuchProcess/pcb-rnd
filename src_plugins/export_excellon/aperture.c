
#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "global_typedefs.h"

#define GVT_DONT_UNDEF
#include "aperture.h"
#include <genvector/genvector_impl.c>

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


/*** drill ***/

void pcb_drill_init(pcb_drill_ctx_t *ctx)
{
	vtpdr_init(&ctx->obj);
	init_aperture_list(&ctx->apr);
}

void pcb_drill_uninit(pcb_drill_ctx_t *ctx)
{
	vtpdr_uninit(&ctx->obj);
	uninit_aperture_list(&ctx->apr);
}

pcb_pending_drill_t *pcb_drill_new_pending(pcb_drill_ctx_t *ctx, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_coord_t diam)
{
	pcb_pending_drill_t *pd = vtpdr_alloc_append(&ctx->obj, 1);

	pd->x = x1;
	pd->y = y1;
	pd->x2 = x2;
	pd->y2 = y2;
	pd->diam = diam;
	pd->is_slot = (x1 != x2) || (y1 != y2);
	find_aperture(&ctx->apr, diam, ROUND);
	return pd;
}

static int drill_sort_cb(const void *va, const void *vb)
{
	pcb_pending_drill_t *a = (pcb_pending_drill_t *)va;
	pcb_pending_drill_t *b = (pcb_pending_drill_t *)vb;
	if (a->diam != b->diam)
		return a->diam - b->diam;
	if (a->x != b->x)
		return a->x - a->x;
	return b->y - b->y;
}

void pcb_drill_sort(pcb_drill_ctx_t *ctx)
{
	qsort(ctx->obj.array, ctx->obj.used, sizeof(ctx->obj.array[0]), drill_sort_cb);
}
