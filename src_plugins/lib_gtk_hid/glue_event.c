#include "common.h"
#include "event.h"
#include "gui.h"
#include "conf_core.h"
#include "tool.h"
#include "../src_plugins/lib_gtk_common/win_place.h"
#include "../src_plugins/lib_gtk_common/hid_gtk_conf.h"
#include "../src_plugins/lib_gtk_common/lib_gtk_config.h"

static void ev_pcb_changed(void *user_data, int argc, pcb_event_arg_t argv[])
{
	ghidgui->common.hidlib = &PCB->hidlib;

	if ((!ghidgui) || (!ghidgui->hid_active))
		return;

	if (PCB != NULL)
		ghidgui->common.window_set_name_label(PCB->hidlib.name);

	if (!gport->drawing_allowed)
		return;

	pcb_gtk_tw_ranges_scale(&ghidgui->topwin);
	pcb_gtk_zoom_view_fit(&gport->view);
	ghid_sync_with_new_layout(&ghidgui->topwin);
}

static void ev_pcb_meta_changed(void *user_data, int argc, pcb_event_arg_t argv[])
{
	if ((!ghidgui) || (!ghidgui->hid_active))
		return;

	if (PCB != NULL)
		ghidgui->common.window_set_name_label(ghidgui->common.hidlib->name);
}

static void ghid_gui_sync(void *user_data, int argc, pcb_event_arg_t argv[])
{
	/* Sync gui widgets with pcb state */
	ghid_mode_buttons_update();

	/* Sync gui status display with pcb state */
	pcb_tool_adjust_attached_objects();
	ghid_invalidate_all();
	ghidgui->common.window_set_name_label(ghidgui->common.hidlib->name);
	ghidgui->common.set_status_line_label();

	/* Sync menu checkboxes */
	ghid_update_toggle_flags(&ghidgui->topwin, NULL);
}

static void ghid_gui_sync_status(void *user_data, int argc, pcb_event_arg_t argv[])
{
	ghidgui->common.window_set_name_label(ghidgui->common.hidlib->name);
	ghidgui->common.set_status_line_label();
}

static void ghid_Busy(void *user_data, int argc, pcb_event_arg_t argv[])
{
	if (gport != NULL)
		ghid_watch_cursor(&gport->mouse);
}

void glue_event_init(const char *cookie)
{
	pcb_event_bind(PCB_EVENT_BOARD_CHANGED, ev_pcb_changed, NULL, cookie);
	pcb_event_bind(PCB_EVENT_BOARD_META_CHANGED, ev_pcb_meta_changed, NULL, cookie);
	pcb_event_bind(PCB_EVENT_LAYERS_CHANGED, ghid_LayersChanged, NULL, cookie);
	pcb_event_bind(PCB_EVENT_LAYERVIS_CHANGED, ghid_LayervisChanged, NULL, cookie);
	pcb_event_bind(PCB_EVENT_BUSY, ghid_Busy, NULL, cookie);
	pcb_event_bind(PCB_EVENT_GUI_SYNC, ghid_gui_sync, NULL, cookie);
	pcb_event_bind(PCB_EVENT_GUI_SYNC_STATUS, ghid_gui_sync_status, NULL, cookie);
}
