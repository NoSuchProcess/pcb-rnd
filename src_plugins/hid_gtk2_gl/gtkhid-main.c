#include "config.h"

#include <stdlib.h>

#include "plugins.h"
#include "hid_init.h"
#include "hidlib_conf.h"

#include "../src_plugins/lib_hid_gl/draw_gl.h"

#include "../src_plugins/lib_gtk_hid/glue_common.h"
#include "../src_plugins/lib_gtk_hid/glue_hid.h"
#include "../src_plugins/lib_gtk_hid/glue_conf.h"
#include "../src_plugins/lib_gtk_hid/glue_win32.h"
#include "../src_plugins/lib_gtk_hid/common.h"
#include "../src_plugins/lib_gtk_hid/render.h"

const char *ghid_gl_cookie = "gtk2 hid, gl";
const char *ghid_gl_menu_cookie = "gtk2 hid menu, gl";

pcb_hid_t gtk2_gl_hid;

int gtk2_gl_parse_arguments(int *argc, char ***argv)
{
	ghid_gl_install(&ghidgui->common, NULL);
	return gtkhid_parse_arguments(argc, argv);
}

int pplg_check_ver_hid_gtk2_gl(int ver_needed) { return 0; }

void pplg_uninit_hid_gtk2_gl(void)
{
	pcb_event_unbind_allcookie(ghid_gl_cookie);
	conf_hid_unreg(ghid_gl_cookie);
	conf_hid_unreg(ghid_gl_menu_cookie);
	drawgl_uninit();
}

int pplg_init_hid_gtk2_gl(void)
{
	PCB_API_CHK_VER;
	ghid_win32_init();

	ghid_glue_hid_init(&gtk2_gl_hid);
	ghid_glue_common_init();

	gtk2_gl_hid.parse_arguments = gtk2_gl_parse_arguments;
	ghid_gl_install(NULL, &gtk2_gl_hid);

	gtk2_gl_hid.name = "gtk2_gl";
	gtk2_gl_hid.description = "Gtk2 - The Gimp Toolkit, with opengl rendering";

	ghidgui->topwin.menu.ghid_menuconf_id = conf_hid_reg(ghid_gl_menu_cookie, NULL);
	ghidgui->topwin.menu.confchg_checkbox = ghid_confchg_checkbox;
	ghid_conf_regs(ghid_gl_cookie);

	pcb_hid_register_hid(&gtk2_gl_hid);

	return 0;
}
