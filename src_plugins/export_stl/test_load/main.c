#include "safe_fs.h"
#include "error.h"
#define STL_TESTER
#include "../stl_models.c"

stl_facet_t *stl_solid_load(rnd_hidlib_t *hl, const char *fn)
{
	FILE *f;
	stl_facet_t *res;

	f = rnd_fopen(hl, fn, "r");
	if (f == NULL)
		return NULL;

	res = stl_solid_fload(hl, fn);

	fclose(f);

	return res;
}

int main()
{
	FILE *f;
	stl_facet_t *solid = stl_solid_load(NULL, "1206.stl");

	f = fopen("A.stl", "w");
	fprintf(f, "solid t1\n");
	stl_solid_print_facets(f, solid,    0, 0, M_PI/6,    12, 0, 0);
	fprintf(f, "endsolid\n");
	fclose(f);

	stl_solid_free(solid);
}
