#include <librnd/core/unit.h>
#include <librnd/core/hid_init.h>
#include <librnd/core/hid.h>
#include <librnd/core/conf.h>
#include <librnd/core/buildin.hidlib.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/plugins.h>
#include <librnd/core/hidlib.h>
#include <librnd/poly/polyarea.h>

#include "glue.c"


/*
static void print_pline(rnd_pline_t *pl)
{
	rnd_vnode_t *n = pl->head;
	printf("poliline:\n");
	do {
		printf(" %ld %ld\n", n->point[0], n->point[1]);
		n = n->next;
	} while(n != pl->head);
	
}
*/

/*** poly contour vs. point test ***/

static void poly_test()
{
	static rnd_pline_t pl;
	int res;

	rnd_vector_t v;
	rnd_polyarea_t pa;
	rnd_polyarea_init(&pa);


	rnd_poly_contour_init(&pl);

	v[0] = 10; v[1] = 0;
	rnd_poly_vertex_include(pl.head->prev, rnd_poly_node_create(v));

	v[0] = 0; v[1] = 0;
	rnd_poly_vertex_include(pl.head->prev, rnd_poly_node_create(v));

	v[0] = 0; v[1] = 15;
	rnd_poly_vertex_include(pl.head->prev, rnd_poly_node_create(v));

	v[0] = 5; v[1] = 12;
	rnd_poly_vertex_include(pl.head->prev, rnd_poly_node_create(v));

	v[0] = 8; v[1] = 12;
	rnd_poly_vertex_include(pl.head->prev, rnd_poly_node_create(v));

	v[0] = 10; v[1] = 15;
	rnd_poly_vertex_include(pl.head->prev, rnd_poly_node_create(v));

	v[0] = 10; v[1] = 0;
	rnd_poly_vertex_include(pl.head->prev, rnd_poly_node_create(v));

/*	print_pline(&pl);*/

	rnd_poly_contour_pre(&pl, 1);

/*	print_pline(&pl);*/

	v[0] = 2; v[1] = 12;
	res = rnd_poly_contour_inside(&pl, v);
	assert(res == -1);

	v[0] = -2; v[1] = 12;
	res = rnd_poly_contour_inside(&pl, v);
	assert(res == 0);



	rnd_polyarea_contour_include(&pa, &pl);

	rnd_poly_valid(&pa);

}

int main(int argc, char *argv[])
{
	int n;
	rnd_main_args_t ga;

	rnd_fix_locale_and_env();

	rnd_main_args_init(&ga, argc, action_args);

	rnd_hidlib_init1(conf_core_init);
	for(n = 1; n < argc; n++)
		n += rnd_main_args_add(&ga, argv[n], argv[n+1]);
	rnd_hidlib_init2(pup_buildins, local_buildins);

	rnd_conf_set(RND_CFR_CLI, "editor/view/flip_y", 0, "1", RND_POL_OVERWRITE);

	if (rnd_main_args_setup1(&ga) != 0) {
		fprintf(stderr, "setup1 fail\n");
		rnd_main_args_uninit(&ga);
		exit(1);
	}

	if (rnd_main_args_setup2(&ga, &n) != 0) {
		fprintf(stderr, "setup2 fail\n");
		rnd_main_args_uninit(&ga);
		exit(n);
	}

	if (rnd_main_exported(&ga, &CTX.hidlib, 0)) {
		fprintf(stderr, "main_exported fail\n");
		rnd_main_args_uninit(&ga);
		exit(1);
	}

	poly_test();

	rnd_main_args_uninit(&ga);
	exit(0);
}
