#include "../stl_models.c"

int main()
{
	FILE *f;
	stl_facet_t *solid = stl_solid_load(NULL, "1206.stl");

	f = fopen("A.stl", "w");
	fprintf(f, "solid t1\n");
	stl_solid_print_facets(f, solid,    0, 0, M_PI/2,    12, 0, 0);
	fprintf(f, "endsolid\n");
	fclose(f);

	stl_solid_free(solid);
}
