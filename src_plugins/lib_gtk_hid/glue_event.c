#include "common.h"
#include "event.h"
#include "gui.h"
#include "tool.h"
#include "../src_plugins/lib_gtk_common/win_place.h"
#include "../src_plugins/lib_gtk_common/hid_gtk_conf.h"
#include "../src_plugins/lib_gtk_common/lib_gtk_config.h"

static void ghid_Busy(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	if (gport != NULL)
		ghid_watch_cursor(&gport->mouse);
}

void glue_event_init(const char *cookie)
{
	pcb_event_bind(PCB_EVENT_BUSY, ghid_Busy, NULL, cookie);
}
