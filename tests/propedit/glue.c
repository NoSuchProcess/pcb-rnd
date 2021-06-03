#include <stdlib.h>
#include <puplug/libs.h>
#include <librnd/core/hidlib.h>

/*** hidlib glue ***/

const char *rnd_hidlib_default_embedded_menu = "";
const char *rnd_conf_internal = "";
const char *rnd_menu_file_paths[] = { "./", NULL };
const char *rnd_menu_name_fmt = "menu.lht";

const char *rnd_conf_userdir_path = "./";
const char *rnd_conf_user_path = "./conf.lht";

/* hack for running from ./ without internal version of the conf */
const char *rnd_conf_sysdir_path = "./";
const char *rnd_conf_sys_path = "./conf.lht";

const char *rnd_app_package = "librnd_test";
const char *rnd_app_version = "0.0.0";
const char *rnd_app_url = "n/a";


typedef struct design_s {
	rnd_hidlib_t hidlib; /* shall be the first */
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

void rnd_hidlib_adjust_attached_objects(rnd_hidlib_t *hl)
{
}

void *rnd_hidlib_crosshair_suspend(rnd_hidlib_t *hl)
{
	return NULL;
}

void rnd_hidlib_crosshair_restore(rnd_hidlib_t *hl, void *susp_data)
{
}

void rnd_hidlib_crosshair_move_to(rnd_hidlib_t *hl, rnd_coord_t abs_x, rnd_coord_t abs_y, int mouse_mot)
{
}

void rnd_draw_marks(rnd_hidlib_t *hidlib, rnd_bool inhibit_drawing_mode)
{
}

void rnd_draw_attached(rnd_hidlib_t *hidlib, rnd_bool inhibit_drawing_mode)
{
}

void rnd_expose_main(rnd_hid_t *hid, const rnd_hid_expose_ctx_t *region, rnd_xform_t *xform_caller)
{
}

void rnd_expose_preview(rnd_hid_t *hid, const rnd_hid_expose_ctx_t *e)
{
}
