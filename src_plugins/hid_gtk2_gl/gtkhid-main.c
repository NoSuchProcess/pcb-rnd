#include "config.h"

#include <stdlib.h>

#include "plugins.h"
#include "hid_init.h"
#include "hidlib_conf.h"

#include "../src_plugins/lib_hid_gl/draw_gl.h"

#include "../src_plugins/lib_gtk_common/glue_common.h"
#include "../src_plugins/lib_gtk_common/glue_hid.h"
#include "../src_plugins/lib_gtk_common/glue_win32.h"
#include "../src_plugins/lib_gtk_common/common.h"
#include "../src_plugins/lib_gtk_common/render.h"

const char *ghid_gl_cookie = "gtk2 hid, gl";

pcb_hid_t gtk2_gl_hid;

int gtk2_gl_parse_arguments(int *argc, char ***argv)
{
	ghid_glue_common_uninit(ghid_cookie);
}

int pplg_check_ver_hid_gtk2_gl(int ver_needed) { return 0; }

void pplg_uninit_hid_gtk2_gl(void)
{
	pcb_event_unbind_allcookie(ghid_gl_cookie);
	conf_hid_unreg(ghid_gl_cookie);
	drawgl_uninit();
}

int pplg_init_hid_gtk2_gl(void)
{
	PCB_API_CHK_VER;
	ghid_win32_init();

	ghid_glue_hid_init(&gtk2_gl_hid);
	ghid_glue_common_init(ghid_gl_cookie);

	gtk2_gl_hid.parse_arguments = gtk2_gl_parse_arguments;
	ghid_gl_install(NULL, &gtk2_gl_hid);

	gtk2_gl_hid.name = "gtk2_gl";
	gtk2_gl_hid.description = "Gtk2 - The Gimp Toolkit, with opengl rendering";

	pcb_hid_register_hid(&gtk2_gl_hid);

	return 0;
}
