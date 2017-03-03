#include "config.h"

#include <stdlib.h>

#include "action_helper.h"
#include "error.h"
#include "gui.h"
#include "pcb-printf.h"
#include "plugins.h"
#include "hid_init.h"
#include "conf_core.h"

#include "gtkhid-main.h"

#include "../src_plugins/lib_gtk_common/glue.h"
#include "../src_plugins/lib_gtk_common/dlg_drc.h"
#include "../src_plugins/lib_gtk_config/lib_gtk_config.h"
#include "../src_plugins/lib_gtk_config/hid_gtk_conf.h"

#include "glue_common.h"
#include "glue_hid.h"
#include "glue_conf.h"
#include "glue_event.h"
#include "common.h"
#include "render.h"

const char *ghid_cookie = "gtk hid";
const char *ghid_menu_cookie = "gtk hid menu";

GhidGui _ghidgui, *ghidgui = &_ghidgui;
GHidPort ghid_port, *gport;

static void ghid_drc_window_append_violation_glue(pcb_drc_violation_t *violation)
{
	ghid_drc_window_append_violation(&ghidgui->common, violation);
}

static int ghid_drc_window_throw_dialog_glue()
{
	return ghid_drc_window_throw_dialog(&ghidgui->common);
}

pcb_hid_drc_gui_t ghid_drc_gui = {
	1,  /* log_drc_overview */
	0,  /* log_drc_details */
	ghid_drc_window_reset_message,
	ghid_drc_window_append_violation_glue,
	ghid_drc_window_throw_dialog_glue,
};

void ghid_draw_area_update(GHidPort *port, GdkRectangle *rect)
{
	gdk_window_invalidate_rect(gtk_widget_get_window(port->drawing_area), rect, FALSE);
}

/*
 * We will need these for finding the windows installation
 * directory.  Without that we can't find our fonts and
 * footprint libraries.
 */
#ifdef WIN32
#include <windows.h>
#include <winreg.h>
#endif



static void init_conf_watch(conf_hid_callbacks_t *cbs, const char *path, void (*func) (conf_native_t *))
{
	conf_native_t *n = conf_get_field(path);
	if (n != NULL) {
		memset(cbs, 0, sizeof(conf_hid_callbacks_t));
		cbs->val_change_post = func;
		conf_hid_set_cb(n, ghid_conf_id, cbs);
	}
}

static void ghid_conf_regs()
{
	static conf_hid_callbacks_t cbs_refraction, cbs_direction, cbs_fullscreen, cbs_show_sside;

	init_conf_watch(&cbs_direction, "editor/all_direction_lines", ghid_confchg_all_direction_lines);
	init_conf_watch(&cbs_refraction, "editor/line_refraction", ghid_confchg_line_refraction);
	init_conf_watch(&cbs_fullscreen, "editor/fullscreen", ghid_confchg_fullscreen);
	init_conf_watch(&cbs_show_sside, "editor/show_solder_side", ghid_confchg_flip);
}

void hid_hid_gtk_uninit()
{
	pcb_event_unbind_allcookie(ghid_cookie);
	conf_hid_unreg(ghid_cookie);
	conf_hid_unreg(ghid_menu_cookie);
}

pcb_hid_t ghid_hid;

pcb_uninit_t hid_hid_gtk_init()
{
#ifdef WIN32
	char *tmps;
	char *share_dir;
	char *loader_cache;
	FILE *loader_file;
#endif

#ifdef WIN32
	tmps = g_win32_get_package_installation_directory(PACKAGE "-" VERSION, NULL);
#define REST_OF_PATH G_DIR_SEPARATOR_S "share" G_DIR_SEPARATOR_S PACKAGE
#define REST_OF_CACHE G_DIR_SEPARATOR_S "loaders.cache"
	share_dir = (char *) malloc(strlen(tmps) + strlen(REST_OF_PATH) + 1);
	sprintf(share_dir, "%s%s", tmps, REST_OF_PATH);

	/* Point to our gdk-pixbuf loader cache.  */
	loader_cache = (char *) malloc(strlen("bindir_todo12") + strlen(REST_OF_CACHE) + 1);
	sprintf(loader_cache, "%s%s", "bindir_todo12", REST_OF_CACHE);
	loader_file = fopen(loader_cache, "r");
	if (loader_file) {
		fclose(loader_file);
		g_setenv("GDK_PIXBUF_MODULE_FILE", loader_cache, TRUE);
	}

	free(tmps);
#undef REST_OF_PATH
	printf("\"Share\" installation path is \"%s\"\n", "share_dir_todo12");
#endif

	ghid_glue_common_init();
	ghid_glue_hid_init(&ghid_hid);

	ghid_hid.name = "gtk";
	ghid_hid.description = "Gtk - The Gimp Toolkit, with GDK software pixmap rendering";
	ghid_hid.drc_gui = &ghid_drc_gui;

	pcb_gtk_conf_init();
	ghidgui->topwin.menu.ghid_menuconf_id = conf_hid_reg(ghid_menu_cookie, NULL);
	ghidgui->topwin.menu.confchg_checkbox = ghid_confchg_checkbox;
	ghid_conf_regs();

	pcb_hid_register_hid(&ghid_hid);

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(conf_hid_gtk, field,isarray,type_name,cpath,cname,desc,flags);
#include "../src_plugins/lib_gtk_config/hid_gtk_conf_fields.h"

	glue_event_init();

	return hid_hid_gtk_uninit;
}
