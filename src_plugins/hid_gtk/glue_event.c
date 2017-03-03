#include "common.h"
#include "event.h"
#include "gui.h"
#include "action_helper.h"
#include "../src_plugins/lib_gtk_common/dlg_netlist.h"
#include "../src_plugins/lib_gtk_common/win_place.h"
#include "../src_plugins/lib_gtk_config/hid_gtk_conf.h"

#warning TODO: remove
#include "gtkhid-main.h"

static void RouteStylesChanged(void *user_data, int argc, pcb_event_arg_t argv[])
{
	if (!ghidgui || !ghidgui->topwin.route_style_selector)
		return;

	pcb_gtk_route_style_sync
		(GHID_ROUTE_STYLE(ghidgui->topwin.route_style_selector),
		 conf_core.design.line_thickness, conf_core.design.via_drilling_hole, conf_core.design.via_thickness,
		 conf_core.design.clearance);

	return;
}

static void ev_pcb_changed(void *user_data, int argc, pcb_event_arg_t argv[])
{
	if ((!ghidgui) || (!gtkhid_active))
		return;

	if (PCB != NULL)
		ghidgui->common.window_set_name_label(PCB->Name);

	if (!gport->pixmap)
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


static void GhidNetlistChanged(void *user_data, int argc, pcb_event_arg_t argv[])
{
	if (!gtkhid_active)
		return;

	pcb_gtk_netlist_changed(&ghidgui->common, user_data, argc, argv);
}

static void ghid_gui_sync(void *user_data, int argc, pcb_event_arg_t argv[])
{
	/* Sync gui widgets with pcb state */
	ghid_mode_buttons_update();

	/* Sync gui status display with pcb state */
	pcb_adjust_attached_objects();
	ghid_invalidate_all();
	ghidgui->common.window_set_name_label(PCB->Name);
	ghidgui->common.set_status_line_label();
}

static void ghid_Busy(void *user_data, int argc, pcb_event_arg_t argv[])
{
	ghid_watch_cursor(&gport->mouse);
}

void glue_event_init(void)
{
	pcb_event_bind(PCB_EVENT_SAVE_PRE, ghid_conf_save_pre_wgeo, NULL, ghid_cookie);
	pcb_event_bind(PCB_EVENT_LOAD_POST, ghid_conf_load_post_wgeo, NULL, ghid_cookie);
	pcb_event_bind(PCB_EVENT_BOARD_CHANGED, ev_pcb_changed, NULL, ghid_cookie);
	pcb_event_bind(PCB_EVENT_NETLIST_CHANGED, GhidNetlistChanged, NULL, ghid_cookie);
	pcb_event_bind(PCB_EVENT_ROUTE_STYLES_CHANGED, RouteStylesChanged, NULL, ghid_cookie);
	pcb_event_bind(PCB_EVENT_LAYERS_CHANGED, ghid_LayersChanged, NULL, ghid_cookie);
	pcb_event_bind(PCB_EVENT_BUSY, ghid_Busy, NULL, ghid_cookie);
	pcb_event_bind(PCB_EVENT_GUI_SYNC, ghid_gui_sync, NULL, ghid_cookie);
}
