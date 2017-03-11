#include "config.h"

#include <stdlib.h>

#include "action_helper.h"
#include "error.h"
#include "pcb-printf.h"
#include "plugins.h"
#include "hid_init.h"
#include "conf_core.h"

#include "../src_plugins/lib_gtk_common/glue.h"
#include "../src_plugins/lib_gtk_config/lib_gtk_config.h"
#include "../src_plugins/lib_gtk_config/hid_gtk_conf.h"

#include "../src_plugins/lib_gtk_hid/gui.h"
#include "../src_plugins/lib_gtk_hid/glue_common.h"
#include "../src_plugins/lib_gtk_hid/glue_hid.h"
#include "../src_plugins/lib_gtk_hid/glue_conf.h"
#include "../src_plugins/lib_gtk_hid/glue_event.h"
#include "../src_plugins/lib_gtk_hid/glue_win32.h"
#include "../src_plugins/lib_gtk_hid/common.h"
#include "../src_plugins/lib_gtk_hid/render.h"

const char *ghid_gl_cookie = "gtk2 hid, gl";
const char *ghid_gl_menu_cookie = "gtk2 hid menu, gl";

pcb_hid_t gtk2_gl_hid;

void gtk2_gl_parse_arguments(int *argc, char ***argv)
{
	ghid_gl_install(&ghidgui->common, NULL);
	gtkhid_parse_arguments(argc, argv);
}

static void hid_hid_gtk2_gl_uninit()
{
	pcb_event_unbind_allcookie(ghid_gl_cookie);
	conf_hid_unreg(ghid_gl_cookie);
	conf_hid_unreg(ghid_gl_menu_cookie);
}

pcb_uninit_t hid_hid_gtk2_gl_init()
{
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

	glue_event_init(ghid_gl_cookie);

	return hid_hid_gtk2_gl_uninit;
}
