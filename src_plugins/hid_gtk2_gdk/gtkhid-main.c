#include "config.h"

#include <stdlib.h>

#include "plugins.h"
#include "hid_init.h"

#include "../src_plugins/lib_gtk_common/glue_common.h"
#include "../src_plugins/lib_gtk_common/glue_hid.h"
#include "../src_plugins/lib_gtk_common/common.h"
#include "../src_plugins/lib_gtk_common/render.h"

const char *ghid_cookie = "gtk2 hid, gdk";

pcb_hid_t gtk2_gdk_hid;

extern void ghid_gdk_install(pcb_gtk_common_t *common, pcb_hid_t *hid);

int gtk2_gdk_parse_arguments(int *argc, char ***argv)
{
	ghid_gdk_install(&ghidgui->common, NULL);
	return gtkhid_parse_arguments(argc, argv);
}

int pplg_check_ver_hid_gtk2_gdk(int ver_needed) { return 0; }

void pplg_uninit_hid_gtk2_gdk(void)
{
	ghid_glue_common_uninit(ghid_cookie);
}

int pplg_init_hid_gtk2_gdk(void)
{
	PCB_API_CHK_VER;

	ghid_glue_hid_init(&gtk2_gdk_hid);
	ghid_glue_common_init(ghid_cookie);

	gtk2_gdk_hid.parse_arguments = gtk2_gdk_parse_arguments;
	ghid_gdk_install(NULL, &gtk2_gdk_hid);

	gtk2_gdk_hid.name = "gtk2_gdk";
	gtk2_gdk_hid.description = "Gtk2 - The Gimp Toolkit, with GDK software pixmap rendering";

	pcb_hid_register_hid(&gtk2_gdk_hid);

	return 0;
}
