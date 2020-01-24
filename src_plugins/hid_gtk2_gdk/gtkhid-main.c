#include "config.h"

#include <stdlib.h>

#include <librnd/core/plugins.h>
#include <librnd/core/hid_init.h>

#include "../src_plugins/lib_gtk_common/glue_common.h"
#include "../src_plugins/lib_gtk_common/glue_hid.h"

const char *ghid_cookie = "gtk2 hid, gdk";

pcb_hid_t gtk2_gdk_hid;

extern void ghid_gdk_install(pcb_gtk_impl_t *impl, pcb_hid_t *hid);

int gtk2_gdk_parse_arguments(pcb_hid_t *hid, int *argc, char ***argv)
{
	ghid_glue_common_init(ghid_cookie);
	ghid_gdk_install(&ghidgui->impl, hid);
	return gtkhid_parse_arguments(hid, argc, argv);
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

	gtk2_gdk_hid.parse_arguments = gtk2_gdk_parse_arguments;

	gtk2_gdk_hid.name = "gtk2_gdk";
	gtk2_gdk_hid.description = "Gtk2 - The Gimp Toolkit, with GDK software pixmap rendering";

	pcb_hid_register_hid(&gtk2_gdk_hid);

	return 0;
}
