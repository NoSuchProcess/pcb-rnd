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
	pcb_conf_reg_field_(NULL, 1, CFN_COORD, "should_never_match", "dummy", 0);
}

void pcb_hidlib_adjust_attached_objects(rnd_hidlib_t *hl)
{
}

void *pcb_hidlib_crosshair_suspend(rnd_hidlib_t *hl)
{
	return NULL;
}

void pcb_hidlib_crosshair_restore(rnd_hidlib_t *hl, void *susp_data)
{
}

void pcb_hidlib_crosshair_move_to(rnd_hidlib_t *hl, rnd_coord_t abs_x, rnd_coord_t abs_y, int mouse_mot)
{
}

void pcbhl_draw_marks(rnd_hidlib_t *hidlib, rnd_bool inhibit_drawing_mode)
{
}

void pcbhl_draw_attached(rnd_hidlib_t *hidlib, rnd_bool inhibit_drawing_mode)
{
}

void pcbhl_expose_main(pcb_hid_t *hid, const pcb_hid_expose_ctx_t *region, pcb_xform_t *xform_caller)
{
}

void pcbhl_expose_preview(pcb_hid_t *hid, const pcb_hid_expose_ctx_t *e)
{
}
