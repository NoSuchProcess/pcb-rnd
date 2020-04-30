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

/*** init test ***/

static void poly_test()
{
	rnd_polyarea_t pa;
	pcb_polyarea_init(&pa);
	pcb_poly_valid(&pa);
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
