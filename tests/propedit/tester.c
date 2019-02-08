#include <stdio.h>
#include <assert.h>
#include "props.h"

void *pcb_gui = NULL;

int pcb_propsel_set(const char *prop, const char *value)
{
	return 0;
}

int pcb_propsel_del(const char *key)
{
	return 0;
}

void pcb_propsel_map_core(htsp_t *props)
{
}



static void print_val(pcb_prop_type_t type, pcb_propval_t val)
{
	switch(type) {
		case PCB_PROPT_STRING: printf("%s", val.string); break;
		case PCB_PROPT_COORD:  printf("%d", val.coord); break;
		case PCB_PROPT_ANGLE:  printf("%f", val.angle); break;
		case PCB_PROPT_INT:    printf("%d", val.i); break;
		default: printf(" ???");
	}
}

static void print_all(htsp_t *props)
{
	htsp_entry_t *pe;
	for (pe = htsp_first(props); pe; pe = htsp_next(props, pe)) {
		htprop_entry_t *e;
		pcb_props_t *p = pe->value;
		printf("%s [%s]\n", pe->key, pcb_props_type_name(p->type));
		for (e = htprop_first(&p->values); e; e = htprop_next(&p->values, e)) {
			printf(" ");
			print_val(p->type, e->key);
			printf(" (%lu)\n", e->value);
		}
	}
}

static pcb_props_t *print_stat(pcb_propedit_t *ctx, const char *name, int all)
{
	pcb_propval_t most_common, min, max, avg;
	pcb_props_t *p;
	int res;

	p = pcb_props_get(ctx, name);
	if (p == NULL)
		return NULL;

	if (all)
		res = pcb_props_stat(ctx, p, &most_common, &min, &max, &avg);
	else
		res = pcb_props_stat(ctx, p, &most_common, NULL, NULL, NULL);

	if (res != 0)
		return NULL;

	printf("Stats %s:", name);
	printf("  common: "); print_val(p->type, most_common);
	if (all) {
		printf("  min: "); print_val(p->type, min);
		printf("  max: "); print_val(p->type, max);
		printf("  avg: "); print_val(p->type, avg);
	}
	printf("\n");

	return p;
}

int main()
{
	pcb_propedit_t ctx;
	pcb_propval_t v;

	pcb_props_init(&ctx, NULL);

	/* --- add a few items properly - should work --- */
	
	/* coord */
	v.coord = 42; assert(pcb_props_add(&ctx, "crd", PCB_PROPT_COORD, v) != NULL);
	v.coord = 10; assert(pcb_props_add(&ctx, "crd", PCB_PROPT_COORD, v) != NULL);
	v.coord = 42; assert(pcb_props_add(&ctx, "crd", PCB_PROPT_COORD, v) != NULL);
	v.coord = 42; assert(pcb_props_add(&ctx, "crd", PCB_PROPT_COORD, v) != NULL);

	/* int */
	v.i = 42; assert(pcb_props_add(&ctx, "num", PCB_PROPT_INT, v) != NULL);
	v.i = 10; assert(pcb_props_add(&ctx, "num", PCB_PROPT_INT, v) != NULL);
	v.i = 42; assert(pcb_props_add(&ctx, "num", PCB_PROPT_INT, v) != NULL);
	v.i = 42; assert(pcb_props_add(&ctx, "num", PCB_PROPT_INT, v) != NULL);

	/* angle */
	v.angle = 42.0; assert(pcb_props_add(&ctx, "ang", PCB_PROPT_ANGLE, v) != NULL);
	v.angle = 10.5; assert(pcb_props_add(&ctx, "ang", PCB_PROPT_ANGLE, v) != NULL);
	v.angle = 42.0; assert(pcb_props_add(&ctx, "ang", PCB_PROPT_ANGLE, v) != NULL);
	v.angle = 42.0; assert(pcb_props_add(&ctx, "ang", PCB_PROPT_ANGLE, v) != NULL);

	/* string */
	v.string = "foo"; assert(pcb_props_add(&ctx, "str", PCB_PROPT_STRING, v) != NULL);
	v.string = "bar"; assert(pcb_props_add(&ctx, "str", PCB_PROPT_STRING, v) != NULL);
	v.string = "BAZ"; assert(pcb_props_add(&ctx, "str", PCB_PROPT_STRING, v) != NULL);
	v.string = "foo"; assert(pcb_props_add(&ctx, "str", PCB_PROPT_STRING, v) != NULL);
	v.string = "foo"; assert(pcb_props_add(&ctx, "str", PCB_PROPT_STRING, v) != NULL);

	/* --- add a items with the wrong type - should fail --- */
	v.i = 42; assert(pcb_props_add(&ctx, "crd", PCB_PROPT_INT, v) == NULL);
	v.i = 42; assert(pcb_props_add(&ctx, "crd", 1234, v) == NULL);
	v.i = 42; assert(pcb_props_add(&ctx, "ang", 1234, v) == NULL);

	print_all(&ctx.props);

	/* --- get some stats --- */

	/* these should work */
	assert(print_stat(&ctx, "crd", 1) != NULL);
	assert(print_stat(&ctx, "num", 1) != NULL);
	assert(print_stat(&ctx, "ang", 1) != NULL);
	assert(print_stat(&ctx, "str", 0) != NULL);

	/* these should fail */
	assert(print_stat(&ctx, "str", 1) == NULL);
	assert(print_stat(&ctx, "HAH", 1) == NULL);

	return 0;
}
