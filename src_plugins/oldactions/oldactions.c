/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
 *
 *
 *  Old contact info:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */
#include "config.h"
#include <librnd/core/conf.h>
#include "data.h"
#include "event.h"
#include "change.h"
#include <librnd/core/error.h>
#include "undo.h"
#include <librnd/core/plugins.h>
#include <librnd/core/actions.h>
#include "plug_footprint.h"
#include "obj_subc.h"
#include <librnd/core/compat_misc.h>
#include "netlist.h"
#include "funchash_core.h"
#include "search.h"

static void conf_toggle(rnd_conf_role_t role, const char *path)
{
	rnd_conf_native_t *n = rnd_conf_get_field(path);
	if (n == NULL) {
		rnd_message(RND_MSG_ERROR, "Error: can't find config node %s to toggle\n", path);
		return;
	}
	if (n->type != RND_CFN_BOOLEAN) {
		rnd_message(RND_MSG_ERROR, "Error: config node %s is not a boolean, can't toggle\n", path);
		return;
	}

	rnd_conf_set(role, path, -1, n->val.boolean[0] ? "0" : "1", RND_POL_OVERWRITE);
}

static const char pcb_acts_DumpLibrary[] = "DumpLibrary()";
static const char pcb_acth_DumpLibrary[] = "Display the entire contents of the libraries.";
static void ind(int level)
{
	static char inds[] = "                                                                               ";

	if (level > sizeof(inds)-1)
		return;
	inds[level] = '\0';
	printf("%s", inds);
	inds[level] = ' ';
}

static void dump_lib_any(int level, pcb_fplibrary_t *l);

static void dump_lib_dir(int level, pcb_fplibrary_t *l)
{
	rnd_cardinal_t n;

	ind(level);
	printf("%s/\n", l->name);
	for(n = 0; n < vtp0_len(&l->data.dir.children); n++)
		dump_lib_any(level+1, l->data.dir.children.array[n]);
}

static void dump_lib_fp(int level, pcb_fplibrary_t *l)
{
	ind(level);
	printf("%s", l->name);
	switch(l->data.fp.type) {
		case PCB_FP_INVALID:      printf(" type(INVALID)"); break;
		case PCB_FP_DIR:          printf(" type(DIR)"); break;
		case PCB_FP_FILEDIR:      printf(" type(FILEDIR)"); break;
		case PCB_FP_FILE:         printf(" type(file)"); break;
		case PCB_FP_PARAMETRIC:   printf(" type(parametric)"); break;
	}
	printf(" loc_info(%s)\n", l->data.fp.loc_info);
}

static void dump_lib_any(int level, pcb_fplibrary_t *l)
{
	switch(l->type) {
		case PCB_LIB_INVALID:     printf("??\n"); break;
		case PCB_LIB_DIR:         dump_lib_dir(level, l); break;
		case PCB_LIB_FOOTPRINT:   dump_lib_fp(level, l); break;
	}
}


static fgw_error_t pcb_act_DumpLibrary(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	dump_lib_any(0, &pcb_library);

	RND_ACT_IRES(0);
	return 0;
}

/* ---------------------------------------------------------------------------
 * no operation, just for testing purposes
 * syntax: Bell(volume)
 */
static const char pcb_acts_Bell[] = "Bell()";

static const char pcb_acth_Bell[] = "Attempt to produce audible notification (e.g. beep the speaker).";

static fgw_error_t pcb_act_Bell(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_gui->beep(rnd_gui);
	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_debug[] = "Debug(...)";
static const char pcb_acth_debug[] = "Debug action.";
/* This action exists to help debug scripts; it simply prints all its arguments to stdout. */

static const char pcb_acts_debugxy[] = "DebugXY(...)";
static const char pcb_acth_debugxy[] = "Debug action, with coordinates";
/* Like @code{Debug}, but requires a coordinate.  If the user hasn't yet indicated a location on the board, the user will be prompted to click on one. */

static fgw_error_t pcb_act_Debug(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_coord_t x, y;
	int i;
	printf("Debug:");
	for (i = 1; i < argc; i++) {
		const char *s;
		RND_ACT_CONVARG(i, FGW_STR, debugxy, s = argv[i].val.str);
		printf(" [%d] `%s'", i, s);
	}
	rnd_hid_get_coords("Click X,Y for Debug", &x, &y, 0);
	rnd_printf(" x,y %$mD\n", x, y);
	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_djopt_sao[] = "OptAutoOnly()";
static const char pcb_acth_djopt_sao[] = "Toggles the optimize-only-autorouted flag.";
/*
The original purpose of the trace optimizer was to clean up the traces
created by the various autorouters that have been used with PCB.  When
a board has a mix of autorouted and carefully hand-routed traces, you
don't normally want the optimizer to move your hand-routed traces.
But, sometimes you do.  By default, the optimizer only optimizes
autorouted traces.  This action toggles that setting, so that you can
optimize hand-routed traces also.
*/
fgw_error_t pcb_act_djopt_set_auto_only(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	conf_toggle(RND_CFR_DESIGN, "plugins/djopt/auto_only");
	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_toggle_vendor[] = "ToggleVendor()";
static const char pcb_acth_toggle_vendor[] = "Toggles the state of automatic drill size mapping.";
/*
@cindex vendor map
@cindex vendor drill table
@findex ToggleVendor()

When drill mapping is enabled, new instances of pins and vias will
have their drill holes mapped to one of the allowed drill sizes
specified in the currently loaded vendor drill table.  To enable drill
mapping, a vendor lihata file containing a drill table must be
loaded first.
*/
fgw_error_t pcb_act_ToggleVendor(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	conf_toggle(RND_CFR_DESIGN, "plugins/vendor/enable");
	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_enable_vendor[] = "EnableVendor()";
static const char pcb_acth_enable_vendor[] = "Enables automatic drill size mapping.";
/*
@cindex vendor map
@cindex vendor drill table
@findex EnableVendor()
When drill mapping is enabled, new instances of pins and vias will
have their drill holes mapped to one of the allowed drill sizes
specified in the currently loaded vendor drill table.  To enable drill
mapping, a vendor lihata file containing a drill table must be
loaded first.
*/
fgw_error_t pcb_act_EnableVendor(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_conf_set(RND_CFR_DESIGN, "plugins/vendor/enable", -1, "1", RND_POL_OVERWRITE);
	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_disable_vendor[] = "DisableVendor()";
static const char pcb_acth_disable_vendor[] = "Disables automatic drill size mapping.";
/*
@cindex vendor map
@cindex vendor drill table
@findex DisableVendor()
When drill mapping is enabled, new instances of pins and vias will
have their drill holes mapped to one of the allowed drill sizes
specified in the currently loaded vendor drill table.
*/
fgw_error_t pcb_act_DisableVendor(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_conf_set(RND_CFR_DESIGN, "plugins/vendor/enable", -1, "0", RND_POL_OVERWRITE);
	RND_ACT_IRES(0);
	return 0;
}

fgw_error_t pcb_act_ListRotations(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	PCB_SUBC_LOOP(PCB->Data);
	{
		double rot;
		const char *refdes = RND_UNKNOWN(subc->refdes);
		if (pcb_subc_get_rotation(subc, &rot) == 0)
			rnd_message(RND_MSG_INFO, "%f %s\n", rot, refdes);
		else
			rnd_message(RND_MSG_INFO, "<unknown> %s\n", refdes);
	}
	PCB_END_LOOP;
	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_PCBChanged[] = "PCBChanged([revert])";
static const char pcb_acth_PCBChanged[] =
	"Tells the GUI that the whole PCB has changed. The optional \"revert\""
	"parameter can be used as a hint to the GUI that the same design is being"
	"reloaded, and that it might keep some viewport settings";
static fgw_error_t pcb_act_PCBChanged(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *rv = NULL;
	RND_ACT_IRES(0);
	RND_ACT_MAY_CONVARG(1, FGW_STR, PCBChanged, rv = argv[1].val.str);
	pcb_board_changed((rv != NULL) && (rnd_strcasecmp(rv, "revert") == 0));
	return 0;
}

static const char pcb_acts_NetlistChanged[] = "NetlistChanged()";
static const char pcb_acth_NetlistChanged[] = "Tells the GUI that the netlist has changed.";
static fgw_error_t pcb_act_NetlistChanged(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_netlist_changed(0);
	return 0;
}


static const char pcb_acts_RouteStylesChanged[] = "RouteStylesChanged()";
static const char pcb_acth_RouteStylesChanged[] = "Tells the GUI that the routing styles have changed.";
static fgw_error_t pcb_act_RouteStylesChanged(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_event(&PCB->hidlib, PCB_EVENT_ROUTE_STYLES_CHANGED, NULL);
	return 0;
}

static const char pcb_acts_LibraryChanged[] = "LibraryChanged()";
static const char pcb_acth_LibraryChanged[] = "Tells the GUI that the libraries have changed.";
static fgw_error_t pcb_act_LibraryChanged(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_event(&PCB->hidlib, PCB_EVENT_LIBRARY_CHANGED, NULL);
	return 0;
}

static const char pcb_acts_ImportGUI[] = "ImportGUI()";
static const char pcb_acth_ImportGUI[] = "Asks user which schematics to import into PCB.\n";
/* DOC: importgui.html */
static fgw_error_t pcb_act_ImportGUI(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_message(RND_MSG_ERROR, "The ImportGUI() action is deprecated. Using ImportSch() instead.\nFor details see: http://repo.hu/projects/pcb-rnd/help/err0002.html\n");
	RND_ACT_IRES(rnd_actionva(RND_ACT_HIDLIB, "ImportSch", NULL));
	return 0;
}

static const char pcb_acts_Import[] =
	"Import()\n"
	"Import([gnetlist|make[,source,source,...]])\n"
	"Import(setnewpoint[,(mark|center|X,Y)])\n"
	"Import(setdisperse,D,units)\n";
static const char pcb_acth_Import[] = "Import schematics.";
/* DOC: import.html */
static fgw_error_t pcb_act_Import(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_message(RND_MSG_ERROR, "Import() is the old, deprecated import netlist/schematics action that got removed\nPlease switch over to using the new action, ImportSch().\nFor details see: http://repo.hu/projects/pcb-rnd/help/err0002.html\n");
	RND_ACT_IRES(1);
	return 0;
}

/*** deprecated ***/

static fgw_error_t pcb_act_ToggleHideName(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_message(RND_MSG_ERROR, "ToggleHideName: deprecated feature removed with subcircuits; just delete\nthe text object if it should not be on the silk of the final board.\n");
	return 1;
}

static fgw_error_t pcb_act_MinMaskGap(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_message(RND_MSG_ERROR, "MinMaskGap: deprecated feature; use padstackedit() instead\n");
	return 1;
}

static fgw_error_t pcb_act_ChangeHole(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_message(RND_MSG_ERROR, "ChangeHole: deprecated feature; use padstackedit() instead\n");
	return 1;
}

static fgw_error_t pcb_act_ChangePaste(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_message(RND_MSG_ERROR, "ChangePaste: deprecated feature; use padstackedit() instead\n");
	return 1;
}

static fgw_error_t pcb_act_ChangeSquare(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_message(RND_MSG_ERROR, "ChangeSquare: deprecated feature; use padstackedit() instead\n");
	return 1;
}

static fgw_error_t pcb_act_SetSquare(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_message(RND_MSG_ERROR, "SetSquare: deprecated feature; use padstackedit() instead\n");
	return 1;
}

static fgw_error_t pcb_act_ClearSquare(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_message(RND_MSG_ERROR, "ClearSquare: deprecated feature; use padstackedit() instead\n");
	return 1;
}

static fgw_error_t pcb_act_ChangeOctagon(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_message(RND_MSG_ERROR, "ChangeOctagon: deprecated feature; use padstackedit() instead\n");
	return 1;
}

static fgw_error_t pcb_act_SetOctagon(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_message(RND_MSG_ERROR, "SetOctagon: deprecated feature; use padstackedit() instead\n");
	return 1;
}

static fgw_error_t pcb_act_ClearOctagon(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	rnd_message(RND_MSG_ERROR, "ClearOctagon: deprecated feature; use padstackedit() instead\n");
	return 1;
}

static const char pcb_acts_MinClearGap[] = "MinClearGap(delta)\n" "MinClearGap(Selected, delta)";
static const char pcb_acth_MinClearGap[] = "Ensures that polygons are a minimum distance from objects.";
static void minclr(pcb_data_t *data, rnd_coord_t value, int flags)
{
	PCB_SUBC_LOOP(data);
	{
		if (!PCB_FLAGS_TEST(flags, subc))
			continue;
		minclr(subc->data, value, 0);
	}
	PCB_END_LOOP;

	PCB_PADSTACK_LOOP(data);
	{
		if (!PCB_FLAGS_TEST(flags, padstack))
			continue;
		if (padstack->Clearance < value) {
			pcb_chg_obj_clear_size(PCB_OBJ_PSTK, padstack, 0, 0, value, 1);
			pcb_undo_restore_serial();
		}
	}
	PCB_END_LOOP;

	PCB_LINE_ALL_LOOP(data);
	{
		if (!PCB_FLAGS_TEST(flags, line))
			continue;
		if ((line->Clearance != 0) && (line->Clearance < value)) {
			pcb_chg_obj_clear_size(PCB_OBJ_LINE, layer, line, 0, value, 1);
			pcb_undo_restore_serial();
		}
	}
	PCB_ENDALL_LOOP;
	PCB_ARC_ALL_LOOP(data);
	{
		if (!PCB_FLAGS_TEST(flags, arc))
			continue;
		if ((arc->Clearance != 0) && (arc->Clearance < value)) {
			pcb_chg_obj_clear_size(PCB_OBJ_ARC, layer, arc, 0, value, 1);
			pcb_undo_restore_serial();
		}
	}
	PCB_ENDALL_LOOP;
	PCB_POLY_ALL_LOOP(data);
	{
		if (!PCB_FLAGS_TEST(flags, polygon))
			continue;
		if ((polygon->Clearance != 0) && (polygon->Clearance < value)) {
			pcb_chg_obj_clear_size(PCB_OBJ_POLY, layer, polygon, 0, value, 1);
			pcb_undo_restore_serial();
		}
	}
	PCB_ENDALL_LOOP;
}

static fgw_error_t pcb_act_MinClearGap(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	const char *function;
	const char *delta = NULL;
	const char *units = NULL;
	rnd_bool absolute;
	rnd_coord_t value;
	int flags;

	RND_ACT_CONVARG(1, FGW_STR, MinClearGap, function = argv[1].val.str);
	RND_ACT_MAY_CONVARG(2, FGW_STR, MinClearGap, delta = argv[2].val.str);
	RND_ACT_MAY_CONVARG(3, FGW_STR, MinClearGap, delta = argv[3].val.str);

	if (rnd_strcasecmp(function, "Selected") == 0)
		flags = PCB_FLAG_SELECTED;
	else {
		units = delta;
		delta = function;
		flags = 0;
	}
	value = 2 * rnd_get_value(delta, units, &absolute, NULL);

	pcb_undo_save_serial();
	minclr(pcb->Data, value, flags);
	pcb_undo_restore_serial();
	pcb_undo_inc_serial();
	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_Attributes[] = "Attributes(Layout|Layer|Element|Subc)\n" "Attributes(Layer,layername)";
static const char pcb_acth_Attributes[] =
	"Let the user edit the attributes of the layout, current or given\n"
	"layer, or selected subcircuit.";
/* DOC: attributes.html */
static fgw_error_t pcb_act_Attributes(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	int id;
	const char *layername = NULL;

	RND_ACT_CONVARG(1, FGW_KEYWORD, Attributes, id = fgw_keyword(&argv[1]));
	RND_ACT_MAY_CONVARG(2, FGW_STR, Attributes, layername = argv[2].val.str);
	RND_ACT_IRES(0);

	switch(id) {
	case F_Layout:
		rnd_actionl("propedit", "board", NULL);
		break;

	case F_Layer:
		if (layername != NULL) {
			char *tmp = rnd_concat("layer:", layername, NULL);
			rnd_actionl("propedit", tmp, NULL);
			free(tmp);
		}
		else
			rnd_actionl("propedit", "layer", NULL);
		break;

	case F_Element:
	case F_Subc:
		{
			int n_found = 0;
			pcb_subc_t *s = NULL;
			PCB_SUBC_LOOP(pcb->Data);
			{
				if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, subc)) {
					s = subc;
					n_found++;
				}
			}
			PCB_END_LOOP;
			if (n_found > 1) {
				rnd_message(RND_MSG_ERROR, "Too many subcircuits selected\n");
				return 1;
			}
			if (n_found == 0) {
				rnd_coord_t x, y;
				void *ptrtmp;
				rnd_hid_get_coords("Click on a subcircuit", &x, &y, 0);
				if ((pcb_search_screen(x, y, PCB_OBJ_SUBC, &ptrtmp, &ptrtmp, &ptrtmp)) != PCB_OBJ_VOID)
					s = (pcb_subc_t *)ptrtmp;
				else {
					rnd_message(RND_MSG_ERROR, "No subcircuit found there\n");
					RND_ACT_IRES(-1);
					return 0;
				}
			}

			{
				char *tmp = rnd_strdup_printf("object:%ld", s->ID);
				rnd_actionl("propedit", tmp, NULL);
				free(tmp);
			}

			break;
		}

	default:
		RND_ACT_FAIL(Attributes);
	}

	return 0;
}



rnd_action_t oldactions_action_list[] = {
	{"DumpLibrary", pcb_act_DumpLibrary, pcb_acth_DumpLibrary, pcb_acts_DumpLibrary},
	{"Bell", pcb_act_Bell, pcb_acth_Bell, pcb_acts_Bell},
	{"Debug", pcb_act_Debug, pcb_acth_debug, pcb_acts_debug},
	{"DebugXY", pcb_act_Debug, pcb_acth_debugxy, pcb_acts_debugxy},
	{"OptAutoOnly", pcb_act_djopt_set_auto_only, pcb_acth_djopt_sao, pcb_acts_djopt_sao},
	{"ToggleVendor", pcb_act_ToggleVendor, pcb_acth_toggle_vendor, pcb_acts_toggle_vendor},
	{"EnableVendor", pcb_act_EnableVendor, pcb_acth_enable_vendor, pcb_acts_enable_vendor},
	{"DisableVendor", pcb_act_DisableVendor, pcb_acth_disable_vendor, pcb_acts_disable_vendor},
	{"ListRotations", pcb_act_ListRotations, 0, 0},
	{"PCBChanged", pcb_act_PCBChanged, pcb_acth_PCBChanged, pcb_acts_PCBChanged},
	{"NetlistChanged", pcb_act_NetlistChanged, pcb_acth_NetlistChanged, pcb_acts_NetlistChanged},
	{"RouteStylesChanged", pcb_act_RouteStylesChanged, pcb_acth_RouteStylesChanged, pcb_acts_RouteStylesChanged},
	{"LibraryChanged", pcb_act_LibraryChanged, pcb_acth_LibraryChanged, pcb_acts_LibraryChanged},
	{"ImportGUI", pcb_act_ImportGUI, pcb_acth_ImportGUI, pcb_acts_ImportGUI},
	{"Import", pcb_act_Import, pcb_acth_Import, pcb_acts_Import},
	{"Attributes", pcb_act_Attributes, pcb_acth_Attributes, pcb_acts_Attributes},

	/* deprecated actions */
	{"ToggleHideName", pcb_act_ToggleHideName, 0, 0},
	{"MinMaskGap", pcb_act_MinMaskGap, 0, 0},
	{"MinClearGap", pcb_act_MinClearGap, pcb_acth_MinClearGap, pcb_acts_MinClearGap},
	{"ChangeHole", pcb_act_ChangeHole, 0, 0},
	{"ChangePaste", pcb_act_ChangePaste, 0, 0},
	{"ChangeSquare", pcb_act_ChangeSquare, 0, 0},
	{"ChangeOctagon", pcb_act_ChangeOctagon, 0, 0},
	{"ClearSquare", pcb_act_ClearSquare, 0, 0},
	{"ClearOctagon", pcb_act_ClearOctagon, 0, 0},
	{"SetSquare", pcb_act_SetSquare, 0, 0},
	{"SetOctagon", pcb_act_SetOctagon, 0, 0}
};

static const char *oldactions_cookie = "oldactions plugin";

int pplg_check_ver_oldactions(int ver_needed) { return 0; }

void pplg_uninit_oldactions(void)
{
	rnd_remove_actions_by_cookie(oldactions_cookie);
}

int pplg_init_oldactions(void)
{
	RND_API_CHK_VER;
	RND_REGISTER_ACTIONS(oldactions_action_list, oldactions_cookie)
	return 0;
}
