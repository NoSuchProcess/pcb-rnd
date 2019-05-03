#include "common.h"
#include "event.h"
#include "gui.h"
#include "tool.h"
#include "../src_plugins/lib_gtk_common/win_place.h"
#include "../src_plugins/lib_gtk_common/hid_gtk_conf.h"
#include "../src_plugins/lib_gtk_common/lib_gtk_config.h"

static void ev_pcb_meta_changed(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	if ((!ghidgui) || (!ghidgui->hid_active))
		return;

	if (hidlib != NULL)
		ghidgui->common.window_set_name_label(ghidgui->common.hidlib->name);
}

static void ghid_gui_sync(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	/* Sync gui status display with pcb state */
	pcb_tool_adjust_attached_objects();
	ghid_invalidate_all();
	ghidgui->common.window_set_name_label(ghidgui->common.hidlib->name);

	/* Sync menu checkboxes */
	ghid_update_toggle_flags(&ghidgui->topwin, NULL);
}

static void ghid_gui_sync_status(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	ghidgui->common.window_set_name_label(ghidgui->common.hidlib->name);
}

static void ghid_Busy(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	if (gport != NULL)
		ghid_watch_cursor(&gport->mouse);
}

void glue_event_init(const char *cookie)
{
	pcb_event_bind(PCB_EVENT_BOARD_META_CHANGED, ev_pcb_meta_changed, NULL, cookie);
	pcb_event_bind(PCB_EVENT_LAYERS_CHANGED, ghid_LayersChanged, NULL, cookie);
	pcb_event_bind(PCB_EVENT_LAYERVIS_CHANGED, ghid_LayervisChanged, NULL, cookie);
	pcb_event_bind(PCB_EVENT_BUSY, ghid_Busy, NULL, cookie);
	pcb_event_bind(PCB_EVENT_GUI_SYNC, ghid_gui_sync, NULL, cookie);
	pcb_event_bind(PCB_EVENT_GUI_SYNC_STATUS, ghid_gui_sync_status, NULL, cookie);
}
