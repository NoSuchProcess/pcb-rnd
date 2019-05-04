#include "common.h"
#include "event.h"
#include "gui.h"
#include "tool.h"
#include "../src_plugins/lib_gtk_common/win_place.h"
#include "../src_plugins/lib_gtk_common/hid_gtk_conf.h"
#include "../src_plugins/lib_gtk_common/lib_gtk_config.h"

static void ghid_gui_sync(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	/* Sync gui status display with pcb state */
	pcb_tool_adjust_attached_objects();
	if (gport->drawing_area != NULL)
		ghid_invalidate_all(hidlib);

	/* Sync menu checkboxes */
	ghid_update_toggle_flags(&ghidgui->topwin, NULL);
}

static void ghid_Busy(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	if (gport != NULL)
		ghid_watch_cursor(&gport->mouse);
}

void glue_event_init(const char *cookie)
{
	pcb_event_bind(PCB_EVENT_LAYERS_CHANGED, ghid_LayersChanged, NULL, cookie);
	pcb_event_bind(PCB_EVENT_LAYERVIS_CHANGED, ghid_LayervisChanged, NULL, cookie);
	pcb_event_bind(PCB_EVENT_BUSY, ghid_Busy, NULL, cookie);
	pcb_event_bind(PCB_EVENT_GUI_SYNC, ghid_gui_sync, NULL, cookie);
}
