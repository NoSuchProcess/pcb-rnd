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

const char *ghid_cookie = "gtk2 hid, gdk";
const char *ghid_menu_cookie = "gtk2 hid menu, gdk";

pcb_hid_t gtk2_gdk_hid;

void gtk2_gdk_parse_arguments(int *argc, char ***argv)
{
	ghid_gdk_install(&ghidgui->common, NULL);
	gtkhid_parse_arguments(argc, argv);
}

int pplg_check_ver_hid_gtk2_gdk(int ver_needed) { return 0; }

void pplg_uninit_hid_gtk2_gdk(void)
{
	pcb_event_unbind_allcookie(ghid_cookie);
	conf_hid_unreg(ghid_cookie);
	conf_hid_unreg(ghid_menu_cookie);
}

int pplg_init_hid_gtk2_gdk(void)
{
	ghid_win32_init();

	ghid_glue_hid_init(&gtk2_gdk_hid);
	gtk2_gdk_hid.enable_fake_composite = 1;
	ghid_glue_common_init();

	gtk2_gdk_hid.parse_arguments = gtk2_gdk_parse_arguments;
	ghid_gdk_install(NULL, &gtk2_gdk_hid);

	gtk2_gdk_hid.name = "gtk2_gdk";
	gtk2_gdk_hid.description = "Gtk2 - The Gimp Toolkit, with GDK software pixmap rendering";

	ghidgui->topwin.menu.ghid_menuconf_id = conf_hid_reg(ghid_menu_cookie, NULL);
	ghidgui->topwin.menu.confchg_checkbox = ghid_confchg_checkbox;
	ghid_conf_regs(ghid_cookie);

	pcb_hid_register_hid(&gtk2_gdk_hid);

	glue_event_init(ghid_cookie);

	return 0;
}
