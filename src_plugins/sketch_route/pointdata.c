#include <assert.h>
#include "pointdata.h"
#include "wire.h"

void pointdata_create(point_t *p, pcb_any_obj_t *obj)
{
	pointdata_t *pd;
	int i;

	assert(p->data == NULL);
	p->data = calloc(1, sizeof(pointdata_t));
	pd = p->data;
	pd->obj = obj;

	for (i = 0; i < 4; i++) {
		/* TODO:
		spoke_bbox(&pd->spokes[i].bbox, i);
		pcb_r_insert_entry(spoke_tree, &pd->spokes[i]);
		*/
		vtp0_init(&pd->spokes[i].slots);
		pd->spokes[i].p = p;
	}
}

void pointdata_free(point_t *p)
{
	pointdata_t *pd = p->data;
	int i;

	if (pd != NULL) {
		wirelist_free(pd->terminal_wires);
		wirelist_free(pd->uturn_wires);
		wirelist_free(pd->attached_wires[0]);
		wirelist_free(pd->attached_wires[1]);
		for (i = 0; i < 4; i++) {
			/* TODO:
			pcb_r_delete_entry(spoke_tree, &pd->spokes[i]);
			*/
			vtp0_uninit(&pd->spokes[i].slots);
		}
		free(pd);
	}
}
