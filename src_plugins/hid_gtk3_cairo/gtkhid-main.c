#include "config.h"

#include <stdlib.h>

#include "plugins.h"
#include "hid_init.h"

#include "../src_plugins/lib_gtk_common/glue_common.h"
#include "../src_plugins/lib_gtk_common/glue_hid.h"

const char *ghid_cairo_cookie = "gtk3 hid, cairo";

pcb_hid_t gtk3_cairo_hid;

extern void ghid_cairo_install(pcb_gtk_impl_t *impl, pcb_hid_t *hid);

int gtk3_cairo_parse_arguments(pcb_hid_t *hid, int *argc, char ***argv)
{
	ghid_glue_common_init(ghid_cookie);
	ghid_cairo_install(&ghidgui->impl, hid);
	return gtkhid_parse_arguments(hid, argc, argv);
}

int pplg_check_ver_hid_gtk3_cairo(int ver_needed) { return 0; }

void pplg_uninit_hid_gtk3_cairo(void)
{
	ghid_glue_common_uninit(ghid_cookie);
}

int pplg_init_hid_gtk3_cairo(void)
{
	PCB_API_CHK_VER;

	ghid_glue_hid_init(&gtk3_cairo_hid);

	gtk3_cairo_hid.parse_arguments = gtk3_cairo_parse_arguments;
	ghid_cairo_install(NULL, &gtk3_cairo_hid);

	gtk3_cairo_hid.name = "gtk3_cairo";
	gtk3_cairo_hid.description = "Gtk3 - The Gimp Toolkit, with cairo rendering";

	pcb_hid_register_hid(&gtk3_cairo_hid);

	return 0;
}
