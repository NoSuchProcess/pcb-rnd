#include "config.h"

#include <stdlib.h>

#include "action_helper.h"
#include "error.h"
#include "gui.h"
#include "pcb-printf.h"
#include "plugins.h"
#include "hid_init.h"
#include "conf_core.h"

#include "../src_plugins/lib_gtk_common/glue.h"
#include "../src_plugins/lib_gtk_config/lib_gtk_config.h"
#include "../src_plugins/lib_gtk_config/hid_gtk_conf.h"

#include "glue_common.h"
#include "glue_hid.h"
#include "glue_conf.h"
#include "glue_event.h"
#include "glue_win32.h"
#include "common.h"
#include "render.h"

const char *ghid_cookie = "gtk hid";
const char *ghid_menu_cookie = "gtk hid menu";

GhidGui _ghidgui, *ghidgui = &_ghidgui;
GHidPort ghid_port, *gport;

#warning TODO: move to render common
void ghid_draw_area_update(GHidPort *port, GdkRectangle *rect)
{
	gdk_window_invalidate_rect(gtk_widget_get_window(port->drawing_area), rect, FALSE);
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
	ghid_win32_init();

	ghid_glue_common_init();
	ghid_glue_hid_init(&ghid_hid);

	ghid_hid.name = "gtk";
	ghid_hid.description = "Gtk - The Gimp Toolkit, with GDK software pixmap rendering";

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
