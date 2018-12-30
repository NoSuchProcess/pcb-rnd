#include "common.h"
#include "event.h"
#include "gui.h"
#include "conf_core.h"
#include "tool.h"
#include "../src_plugins/lib_gtk_common/dlg_netlist.h"
#include "../src_plugins/lib_gtk_common/win_place.h"
#include "../src_plugins/lib_gtk_common/hid_gtk_conf.h"
#include "../src_plugins/lib_gtk_common/lib_gtk_config.h"

void pcb_ghid_rst_chg(void)
{
	if (!ghidgui || !ghidgui->topwin.route_style_selector)
		return;

	pcb_gtk_route_style_sync (GHID_ROUTE_STYLE(ghidgui->topwin.route_style_selector), conf_core.design.line_thickness, conf_core.design.via_drilling_hole, conf_core.design.via_thickness, conf_core.design.clearance);

	return;
}

static void RouteStylesChanged(void *user_data, int argc, pcb_event_arg_t argv[])
{
	pcb_ghid_rst_chg();
}


static void ev_pcb_changed(void *user_data, int argc, pcb_event_arg_t argv[])
{
	if ((!ghidgui) || (!ghidgui->hid_active))
		return;

	if (PCB != NULL)
		ghidgui->common.window_set_name_label(PCB->Name);

	if (!gport->drawing_allowed)
		return;

	if (ghidgui->topwin.route_style_selector) {
		pcb_gtk_route_style_empty(GHID_ROUTE_STYLE(ghidgui->topwin.route_style_selector));
		make_route_style_buttons(GHID_ROUTE_STYLE(ghidgui->topwin.route_style_selector));
	}
	RouteStylesChanged(0, 0, NULL);

	pcb_gtk_tw_ranges_scale(&ghidgui->topwin);
	pcb_gtk_zoom_view_fit(&gport->view);
	ghid_sync_with_new_layout(&ghidgui->topwin);
}

static void ev_pcb_meta_changed(void *user_data, int argc, pcb_event_arg_t argv[])
{
	if ((!ghidgui) || (!ghidgui->hid_active))
		return;

	if (PCB != NULL)
		ghidgui->common.window_set_name_label(PCB->Name);
}


static void GhidNetlistChanged(void *user_data, int argc, pcb_event_arg_t argv[])
{
	if (!ghidgui->hid_active)
		return;

	pcb_gtk_netlist_changed(&ghidgui->common, user_data, argc, argv);
}

static void ghid_gui_sync(void *user_data, int argc, pcb_event_arg_t argv[])
{
	/* Sync gui widgets with pcb state */
	ghid_mode_buttons_update();

	/* Sync gui status display with pcb state */
	pcb_tool_adjust_attached_objects();
	ghid_invalidate_all();
	ghidgui->common.window_set_name_label(PCB->Name);
	ghidgui->common.set_status_line_label();
}

static void ghid_gui_sync_status(void *user_data, int argc, pcb_event_arg_t argv[])
{
	ghidgui->common.window_set_name_label(PCB->Name);
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
	pcb_event_bind(PCB_EVENT_NETLIST_CHANGED, GhidNetlistChanged, NULL, cookie);
	pcb_event_bind(PCB_EVENT_ROUTE_STYLES_CHANGED, RouteStylesChanged, NULL, cookie);
	pcb_event_bind(PCB_EVENT_LAYERS_CHANGED, ghid_LayersChanged, NULL, cookie);
	pcb_event_bind(PCB_EVENT_LAYERVIS_CHANGED, ghid_LayervisChanged, NULL, cookie);
	pcb_event_bind(PCB_EVENT_BUSY, ghid_Busy, NULL, cookie);
	pcb_event_bind(PCB_EVENT_GUI_SYNC, ghid_gui_sync, NULL, cookie);
	pcb_event_bind(PCB_EVENT_GUI_SYNC_STATUS, ghid_gui_sync_status, NULL, cookie);
}
