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

const char *ghid_cairo_cookie = "gtk3 hid, cairo";
const char *ghid_cairo_menu_cookie = "gtk3 hid menu, cairo";

pcb_hid_t gtk3_cairo_hid;

void gtk3_cairo_parse_arguments(int *argc, char ***argv)
{
	ghid_gdk_install(&ghidgui->common, NULL);
	gtkhid_parse_arguments(argc, argv);
}

static void hid_hid_gtk3_cairo_uninit()
{
	pcb_event_unbind_allcookie(ghid_cairo_cookie);
	conf_hid_unreg(ghid_cairo_cookie);
	conf_hid_unreg(ghid_cairo_menu_cookie);
}

pcb_uninit_t hid_hid_gtk3_cairo_init()
{
	ghid_win32_init();

	ghid_glue_hid_init(&gtk3_cairo_hid);
	ghid_glue_common_init();

	gtk3_cairo_hid.parse_arguments = gtk3_cairo_parse_arguments;
	ghid_gdk_install(NULL, &gtk3_cairo_hid);

	gtk3_cairo_hid.name = "gtk3_cairo";
	gtk3_cairo_hid.description = "Gtk3 - The Gimp Toolkit, with cairo rendering";

	ghidgui->topwin.menu.ghid_menuconf_id = conf_hid_reg(ghid_cairo_menu_cookie, NULL);
	ghidgui->topwin.menu.confchg_checkbox = ghid_confchg_checkbox;
	ghid_conf_regs(ghid_cairo_cookie);

	pcb_hid_register_hid(&gtk3_cairo_hid);

	glue_event_init(ghid_cairo_cookie);

	return hid_hid_gtk3_cairo_uninit;
}
