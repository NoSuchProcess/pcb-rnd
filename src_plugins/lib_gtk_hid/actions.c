/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include "gui.h"

#include "board.h"
#include "actions.h"
#include "data.h"
#include "search.h"
#include "change.h"
#include "hid_attrib.h"
#include "../src/actions.h"
#include "compat_misc.h"

#include "../src_plugins/lib_gtk_common/wt_layersel.h"
#include "../src_plugins/lib_gtk_common/dlg_route_style.h"
#include "../src_plugins/lib_gtk_common/dlg_library.h"
#include "../src_plugins/lib_gtk_common/dlg_log.h"
#include "../src_plugins/lib_gtk_common/dlg_search.h"
#include "../src_plugins/lib_gtk_common/lib_gtk_config.h"

#include "actions.h"

static const char *ghid_act_cookie = "gtk HID actions";

static const char pcb_acts_AdjustStyle[] = "AdjustStyle()\n";
static const char pcb_acth_AdjustStyle[] = "Open the window which allows editing of the route styles.";
/* DOC: adjuststyle.html */
static fgw_error_t pcb_act_AdjustStyle(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	if (argc > 2)
		PCB_ACT_FAIL(AdjustStyle);

	pcb_gtk_route_style_edit_dialog(&ghidgui->common, GHID_ROUTE_STYLE(ghidgui->topwin.route_style_selector));
	PCB_ACT_IRES(0);
	return 0;
}

/* DOC: command.html */
static fgw_error_t pcb_act_Command(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	ghid_handle_user_command(&ghidgui->topwin.cmd, TRUE);
	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_DoWindows[] =
	"DoWindows(1|2|3|4|5|6|7 [,false])\n"
	"DoWindows(Layout|Library|Log|Search [,false])";
static const char pcb_acth_DoWindows[] = "Open various GUI windows. With false, do not raise the window (no focus stealing).";
/* DOC: dowindows.html */
static fgw_error_t pcb_act_DoWindows(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *a = "", *b = NULL;
	gboolean raise = TRUE;

	PCB_ACT_MAY_CONVARG(1, FGW_STR, DoWindows, a = argv[1].val.str);
	PCB_ACT_MAY_CONVARG(2, FGW_STR, DoWindows, b = argv[2].val.str);

	if (b != NULL) {
		char c = tolower(b[0]);
		if ((c == 'n') || (c == 'f') || (c == '0'))
			raise = FALSE;
	}

	if (strcmp(a, "1") == 0 || pcb_strcasecmp(a, "Layout") == 0) {
	}
	else if (strcmp(a, "2") == 0 || pcb_strcasecmp(a, "Library") == 0) {
		pcb_gtk_library_show(&ghidgui->common, raise);
	}
	else if (strcmp(a, "3") == 0 || pcb_strcasecmp(a, "Log") == 0) {
		pcb_gtk_dlg_log_show(raise);
	}
	else if (strcmp(a, "4") == 0 || pcb_strcasecmp(a, "Netlist") == 0) {
		pcb_actionl("NetlistDialog", NULL);
	}
	else if (strcmp(a, "5") == 0 || pcb_strcasecmp(a, "Preferences") == 0) {
		pcb_message(PCB_MSG_ERROR, "Please use the new drc preferences() action instead\n");
	}
	else if (strcmp(a, "6") == 0 || pcb_strcasecmp(a, "DRC") == 0) {
		pcb_message(PCB_MSG_ERROR, "Please use the new drc action instead\n");
	}
	else if (strcmp(a, "7") == 0 || pcb_strcasecmp(a, "Search") == 0) {
		ghid_search_window_show(gport->top_window, raise);
	}
	else {
		PCB_ACT_FAIL(DoWindows);
	}

	PCB_ACT_IRES(0);
	return 0;
}

/* ------------------------------------------------------------ */
static const char pcb_acts_Popup[] = "Popup(MenuName, [Button])";

static const char pcb_acth_Popup[] =
	"Bring up the popup menu specified by @code{MenuName}.\n"
	"If called by a mouse event then the mouse button number\n" "must be specified as the optional second argument.";
/* DOC: popup.html */
static fgw_error_t pcb_act_Popup(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	GtkWidget *menu = NULL;
	char name[256];
	const char *tn = NULL, *a0, *a1 = NULL;
	enum {
		CTX_NONE,
		CTX_OBJ_TYPE
	} ctx_sens = CTX_NONE;

	if (argc != 2 && argc != 3)
		PCB_ACT_FAIL(Popup);

	PCB_ACT_CONVARG(1, FGW_STR, Popup, a0 = argv[1].val.str);
	PCB_ACT_MAY_CONVARG(2, FGW_STR, Popup, a1 = argv[2].val.str);

	if (argc == 3) {
		if (strcmp(a1, "obj-type") == 0) ctx_sens = CTX_OBJ_TYPE;
	}

	if (strlen(a0) < sizeof(name) - 32) {
		lht_node_t *menu_node;
		switch(ctx_sens) {
			case CTX_OBJ_TYPE:
				{
					pcb_coord_t x, y;;
					pcb_objtype_t type;
					void *o1, *o2, *o3;
					pcb_hid_get_coords("context sensitive popup: select object", &x, &y, 0);
					type = pcb_search_screen(x, y, PCB_OBJ_PSTK | PCB_OBJ_SUBC_PART, &o1, &o2, &o3);
					if (type == 0)
						type = pcb_search_screen(x, y, PCB_OBJ_CLASS_REAL, &o1, &o2, &o3);

					if (type == 0)
						tn = "none";
					else
						tn = pcb_obj_type_name(type);

					sprintf(name, "/popups/%s-%s", a0, tn);
					menu_node = pcb_hid_cfg_get_menu(ghidgui->topwin.ghid_cfg, name);

					if (menu_node == NULL) {
						sprintf(name, "/popups/%s-misc", a0);
						menu_node = pcb_hid_cfg_get_menu(ghidgui->topwin.ghid_cfg, name);
					}
				}
				break;
			case CTX_NONE:
				sprintf(name, "/popups/%s", a0);
				menu_node = pcb_hid_cfg_get_menu(ghidgui->topwin.ghid_cfg, name);
				break;
				
		}
		if (menu_node != NULL)
			menu = pcb_gtk_menu_widget(menu_node);
	}

	if (!GTK_IS_MENU(menu)) {
		pcb_message(PCB_MSG_ERROR, "The specified popup menu \"%s\" (context: '%s') has not been defined.\n", a0, (tn == NULL ? "" : tn));
		PCB_ACT_IRES(1);
		return 0;
	}
	else {
		ghidgui->topwin.in_popup = TRUE;
		gtk_widget_grab_focus(ghid_port.drawing_area);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
	}

	PCB_ACT_IRES(0);
	return 0;
}

/* ------------------------------------------------------------ */
static const char pcb_acts_SaveWinGeo[] = "SaveWindowGeometry()";
static const char pcb_acth_SaveWinGeo[] = "Saves window geometry in the config.\n";

static fgw_error_t pcb_act_SaveWinGeo(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	PCB_ACT_IRES(0);
	return 0;
}


/* ------------------------------------------------------------ */
static fgw_error_t pcb_act_Zoom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	PCB_ACT_IRES(pcb_gtk_act_zoom(&gport->view, res, argc, argv));
	return 0;
}

static fgw_error_t pcb_act_Center(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	fgw_error_t e;
	pcb_coord_t x, y;
	int offset_x, offset_y, pointer_x, pointer_y;
	GdkDisplay *display = gdk_display_get_default();
	GdkScreen *screen = gdk_display_get_default_screen(display);

	pcb_hid_get_coords("Click to center", &x, &y, 0);

	gdk_window_get_origin(gtk_widget_get_window(gport->drawing_area), &offset_x, &offset_y);
	e = pcb_gtk_act_center(&gport->view, res, argc, argv, x, y, offset_x, offset_y, &pointer_x, &pointer_y);
	gdk_display_warp_pointer(display, screen, pointer_x, pointer_y);
	return e;
}

static fgw_error_t pcb_act_SwapSides(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int rv, oa;

	/* ugly workaround: turn off gui refreshes until the swap finishes to avoid triggering more events that may lead to infinite loop */
	oa = ghidgui->hid_active;
	ghidgui->hid_active = 0;

	rv = pcb_gtk_swap_sides(&gport->view, res, argc, argv);

	ghidgui->hid_active = oa;
	return rv;
}

static fgw_error_t pcb_act_Scroll(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	if (ghidgui == NULL)
		return 0;

	return pcb_gtk_act_scroll(&gport->view, res, argc, argv);
}

static fgw_error_t pcb_act_Pan(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	if (ghidgui == NULL)
		return 0;

	return pcb_gtk_act_pan(&gport->view, res, argc, argv);
}

/* ------------------------------------------------------------
 *
 * Actions specific to the GTK HID follow from here
 *
 */

pcb_action_t ghid_menu_action_list[] = {
	{"AdjustStyle", pcb_act_AdjustStyle, pcb_acth_AdjustStyle, pcb_acts_AdjustStyle},
};

PCB_REGISTER_ACTIONS(ghid_menu_action_list, ghid_act_cookie)

pcb_action_t ghid_main_action_list[] = {
	{"Center", pcb_act_Center, pcb_acth_center, pcb_acts_center},
	{"Command", pcb_act_Command},
	{"DoWindows", pcb_act_DoWindows, pcb_acth_DoWindows, pcb_acts_DoWindows},
	{"LogShowOnAppend", pcb_gtk_act_logshowonappend, pcb_gtk_acth_logshowonappend, pcb_gtk_acts_logshowonappend},
	{"Pan", pcb_act_Pan, pcb_acth_pan, pcb_acts_pan},
	{"Popup", pcb_act_Popup, pcb_acth_Popup, pcb_acts_Popup},
	{"SaveWindowGeometry", pcb_act_SaveWinGeo, pcb_acth_SaveWinGeo, pcb_acts_SaveWinGeo},
	{"Scroll", pcb_act_Scroll, pcb_acth_scroll, pcb_acts_scroll},
	{"SwapSides", pcb_act_SwapSides, pcb_acth_swapsides, pcb_acts_swapsides},
	{"Zoom", pcb_act_Zoom, pcb_acth_zoom, pcb_acts_zoom},
	{"ZoomTo", pcb_act_Zoom, pcb_acth_zoom, pcb_acts_zoom}
};

PCB_REGISTER_ACTIONS(ghid_main_action_list, ghid_act_cookie)
#include "dolists.h"

void pcb_gtk_action_reg(void)
{
	PCB_REGISTER_ACTIONS(ghid_main_action_list, ghid_act_cookie)
	PCB_REGISTER_ACTIONS(ghid_menu_action_list, ghid_act_cookie)
}

void pcb_gtk_action_unreg(void)
{
	pcb_remove_actions_by_cookie(ghid_act_cookie);
	pcb_hid_remove_attributes_by_cookie(ghid_act_cookie);
}
