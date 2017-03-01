/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 */

#include "config.h"

#include "../src_plugins/lib_gtk_common/dlg_route_style.h"
#include "../src_plugins/lib_gtk_common/dlg_fontsel.h"
#include "search.h"
#include "change.h"


int SelectLayer(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return pcb_gtk_SelectLayer(ghidgui->topwin.layer_selector, argc, argv, x, y);
}

int ToggleView(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return pcb_gtk_ToggleView(ghidgui->topwin.layer_selector, argc, argv, x, y);
}

/* ------------------------------------------------------------ */

static const char adjuststyle_syntax[] = "AdjustStyle()\n";

static const char adjuststyle_help[] = "Open the window which allows editing of the route styles.";

/* %start-doc actions AdjustStyle

Opens the window which allows editing of the route styles.

%end-doc */

static int AdjustStyle(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	if (argc > 1)
		PCB_AFAIL(adjuststyle);

  pcb_gtk_route_style_edit_dialog(&ghidgui->common, GHID_ROUTE_STYLE(ghidgui->topwin.route_style_selector));
	return 0;
}

/* ------------------------------------------------------------ */

static const char editlayergroups_syntax[] = "EditLayerGroups()\n";

static const char editlayergroups_help[] = "Open the preferences window which allows editing of the layer groups.";

/* %start-doc actions EditLayerGroups

Opens the preferences window which is where the layer groups
are edited.  This action is primarily provides to provide menu
lht compatibility with the lesstif HID.

%end-doc */

static int EditLayerGroups(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{

	if (argc != 0)
		PCB_AFAIL(editlayergroups);

#warning TODO: extend the DoWindows action so it opens the right preferences tab

	pcb_hid_actionl("DoWindows", "Preferences", NULL);

	return 0;
}


static const char pcb_acts_fontsel[] = "FontSel()\n";
static const char pcb_acth_fontsel[] = "Select the font to draw new text with.";
static int pcb_act_fontsel(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	if (argc > 1)
		PCB_ACT_FAIL(fontsel);

	if (argc > 0) {
		if (pcb_strcasecmp(argv[0], "Object") == 0) {
			int type;
			void *ptr1, *ptr2, *ptr3;
			pcb_gui->get_coords(_("Select an Object"), &x, &y);
			if ((type = pcb_search_screen(x, y, PCB_CHANGENAME_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_TYPE_NONE) {
/*				pcb_undo_save_serial();*/
				pcb_gtk_dlg_fontsel(&ghidgui->common, ptr1, ptr2, type, 1);
			}
		}
		else
			PCB_ACT_FAIL(fontsel);
	}
	else
		pcb_gtk_dlg_fontsel(&ghidgui->common, NULL, NULL, 0, 0);
	return 0;
}

/* ------------------------------------------------------------ */

pcb_hid_action_t ghid_menu_action_list[] = {
	{"SelectLayer", 0, SelectLayer,
	 selectlayer_help, selectlayer_syntax}
	,
	{"ToggleView", 0, ToggleView,
	 toggleview_help, toggleview_syntax}
	,
	{"AdjustStyle", 0, AdjustStyle, adjuststyle_help, adjuststyle_syntax}
	,
	{"EditLayerGroups", 0, EditLayerGroups, editlayergroups_help, editlayergroups_syntax}
	,
	{"fontsel", 0, pcb_act_fontsel, pcb_acth_fontsel, pcb_acts_fontsel}
};

PCB_REGISTER_ACTIONS(ghid_menu_action_list, ghid_cookie)
