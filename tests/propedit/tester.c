#include <stdio.h>
#include <assert.h>
#include "props.h"

void print_all(htsp_t *props)
{
	htsp_entry_t *pe;
	for (pe = htsp_first(props); pe; pe = htsp_next(props, pe)) {
		htprop_entry_t *e;
		pcb_props_t *p = pe->value;
		printf("%s [%s]\n", pe->key, pcb_props_type_name(p->type));
		for (e = htprop_first(&p->values); e; e = htprop_next(&p->values, e)) {
			switch(p->type) {
				case PCB_PROPT_STRING: printf(" %s", e->key.string); break;
				case PCB_PROPT_COORD:  printf(" %d", e->key.coord); break;
				case PCB_PROPT_ANGLE:  printf(" %f", e->key.angle); break;
				case PCB_PROPT_INT:    printf(" %d", e->key.i); break;
				default: printf(" ???");
			}
			printf(" (%lu)\n", e->value);
		}
	}
}

int main()
{
	htsp_t *props = pcb_props_init();
	assert(props != NULL);
	pcb_propval_t v;

	/* --- add a few items properly - should work --- */
	
	/* coord */
	v.coord = 42; assert(pcb_props_add(props, "crd", PCB_PROPT_COORD, v) != NULL);
	v.coord = 10; assert(pcb_props_add(props, "crd", PCB_PROPT_COORD, v) != NULL);
	v.coord = 42; assert(pcb_props_add(props, "crd", PCB_PROPT_COORD, v) != NULL);
	v.coord = 42; assert(pcb_props_add(props, "crd", PCB_PROPT_COORD, v) != NULL);

	/* int */
	v.i = 42; assert(pcb_props_add(props, "num", PCB_PROPT_INT, v) != NULL);
	v.i = 10; assert(pcb_props_add(props, "num", PCB_PROPT_INT, v) != NULL);
	v.i = 42; assert(pcb_props_add(props, "num", PCB_PROPT_INT, v) != NULL);
	v.i = 42; assert(pcb_props_add(props, "num", PCB_PROPT_INT, v) != NULL);

	/* angle */
	v.angle = 42.0; assert(pcb_props_add(props, "ang", PCB_PROPT_ANGLE, v) != NULL);
	v.angle = 10.5; assert(pcb_props_add(props, "ang", PCB_PROPT_ANGLE, v) != NULL);
	v.angle = 42.0; assert(pcb_props_add(props, "ang", PCB_PROPT_ANGLE, v) != NULL);
	v.angle = 42.0; assert(pcb_props_add(props, "ang", PCB_PROPT_ANGLE, v) != NULL);

	/* string */
	v.string = "foo"; assert(pcb_props_add(props, "str", PCB_PROPT_STRING, v) != NULL);
	v.string = "bar"; assert(pcb_props_add(props, "str", PCB_PROPT_STRING, v) != NULL);
	v.string = "BAZ"; assert(pcb_props_add(props, "str", PCB_PROPT_STRING, v) != NULL);
	v.string = "foo"; assert(pcb_props_add(props, "str", PCB_PROPT_STRING, v) != NULL);
	v.string = "foo"; assert(pcb_props_add(props, "str", PCB_PROPT_STRING, v) != NULL);

	/* --- add a items with the wrong type - should fail --- */
	v.i = 42; assert(pcb_props_add(props, "crd", PCB_PROPT_INT, v) == NULL);
	v.i = 42; assert(pcb_props_add(props, "crd", 1234, v) == NULL);
	v.i = 42; assert(pcb_props_add(props, "ang", 1234, v) == NULL);


	print_all(props);
}
