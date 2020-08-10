#include "../stl_models.c"

int main()
{
	stl_facet_t *solid = stl_solid_load(NULL, "1206.stl");

	stl_solid_free(solid);
}
