#include "unit.h"
#include "hid_init.h"
#include "hid.h"
#include "conf.h"
#include "buildin.hidlib.h"
#include "compat_misc.h"
#include "plugins.h"
#include "hidlib.h"
#include "polyarea.h"

/*** hidlib glue ***/

const char *pcb_hidlib_default_embedded_menu = "";
const char *pcb_conf_internal = "";
const char *pcbhl_menu_file_paths[] = { "./", NULL };
const char *pcbhl_menu_name_fmt = "menu.lht";

const char *pcbhl_conf_userdir_path = "./";
const char *pcphl_conf_user_path = "./conf.lht";

/* hack for running from ./ without internal version of the conf */
const char *pcbhl_conf_sysdir_path = "./";
const char *pcbhl_conf_sys_path = "./conf.lht";

const char *pcbhl_app_package = "librnd_test";
const char *pcbhl_app_version = "0.0.0";
const char *pcbhl_app_url = "n/a";


typedef struct design_s {
	pcb_hidlib_t hidlib; /* shall be the first */
} design_t;

design_t CTX;

const pup_buildin_t local_buildins[] = {
	{NULL, NULL, NULL, NULL, 0, NULL}
};

static const char *action_args[] = {
	NULL, NULL, NULL, NULL, NULL /* terminator */
};

void conf_core_init()
{
}

void pcb_hidlib_adjust_attached_objects(void)
{
}

void *pcb_hidlib_crosshair_suspend(void)
{
	return NULL;
}

void pcb_hidlib_crosshair_restore(void *susp_data)
{
}

void pcb_hidlib_crosshair_move_to(pcb_coord_t abs_x, pcb_coord_t abs_y, int mouse_mot)
{
}

void pcbhl_draw_marks(pcb_hidlib_t *hidlib, pcb_bool inhibit_drawing_mode)
{
}

void pcbhl_draw_attached(pcb_hidlib_t *hidlib, pcb_bool inhibit_drawing_mode)
{
}

void pcbhl_expose_main(pcb_hid_t *hid, const pcb_hid_expose_ctx_t *region, pcb_xform_t *xform_caller)
{
}

void pcbhl_expose_preview(pcb_hid_t *hid, const pcb_hid_expose_ctx_t *e)
{
}

void pcb_tool_gui_init(void)
{
}

/*** init test ***/

static void poly_test()
{
	pcb_polyarea_t pa;
	pcb_polyarea_init(&pa);
	pcb_poly_valid(&pa);
}

int main(int argc, char *argv[])
{
	int n;
	pcbhl_main_args_t ga;

	pcb_fix_locale();

	pcbhl_main_args_init(&ga, argc, action_args);

	pcb_hidlib_init1(conf_core_init);
	for(n = 1; n < argc; n++)
		n += pcbhl_main_args_add(&ga, argv[n], argv[n+1]);
	pcb_hidlib_init2(pup_buildins, local_buildins);

	pcb_conf_set(CFR_CLI, "editor/view/flip_y", 0, "1", POL_OVERWRITE);

	if (pcbhl_main_args_setup1(&ga) != 0) {
		fprintf(stderr, "setup1 fail\n");
		pcbhl_main_args_uninit(&ga);
		exit(1);
	}

	if (pcbhl_main_args_setup2(&ga, &n) != 0) {
		fprintf(stderr, "setup2 fail\n");
		pcbhl_main_args_uninit(&ga);
		exit(n);
	}

	if (pcbhl_main_exported(&ga, &CTX.hidlib, 0)) {
		fprintf(stderr, "main_exported fail\n");
		pcbhl_main_args_uninit(&ga);
		exit(1);
	}

	poly_test();

	pcbhl_main_args_uninit(&ga);
	exit(0);
}
