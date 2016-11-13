/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Contact addresses for paper mail and Email:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */
#include "config.h"
#include "conf.h"
#include "data.h"
#include "action_helper.h"
#include "change.h"
#include "error.h"
#include "undo.h"
#include "plugins.h"
#include "hid_actions.h"
#include "plug_footprint.h"


static void conf_toggle(conf_role_t role, const char *path)
{
	conf_native_t *n = conf_get_field(path);
	if (n == NULL) {
		pcb_message(PCB_MSG_DEFAULT, "Error: can't find config node %s to toggle\n", path);
		return;
	}
	if (n->type != CFN_BOOLEAN) {
		pcb_message(PCB_MSG_DEFAULT, "Error: config node %s is not a boolean, can't toggle\n", path);
		return;
	}

	conf_set(role, path, -1, n->val.boolean[0] ? "0" : "1", POL_OVERWRITE);
}

/* -------------------------------------------------------------------------- */

static const char dumplibrary_syntax[] = "DumpLibrary()";

static const char dumplibrary_help[] = "Display the entire contents of the libraries.";

/* %start-doc actions DumpLibrary


%end-doc */
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
	pcb_cardinal_t n;

	ind(level);
	printf("%s/\n", l->name);
	for(n = 0; n < vtlib_len(&l->data.dir.children); n++)
		dump_lib_any(level+1, l->data.dir.children.array+n);
}

static void dump_lib_fp(int level, pcb_fplibrary_t *l)
{
	ind(level);
	printf("%s", l->name);
	switch(l->data.fp.type) {
		case PCB_FP_INVALID:      printf(" type(??)"); break;
		case PCB_FP_DIR:          printf(" type(DIR)"); break;
		case PCB_FP_FILE:         printf(" type(file)"); break;
		case PCB_FP_PARAMETRIC:   printf(" type(parametric)"); break;
	}
	printf(" loc_info(%s)\n", l->data.fp.loc_info);
}

static void dump_lib_any(int level, pcb_fplibrary_t *l)
{
	switch(l->type) {
		case LIB_INVALID:     printf("??\n"); break;
		case LIB_DIR:         dump_lib_dir(level, l); break;
		case LIB_FOOTPRINT:   dump_lib_fp(level, l); break;
	}
}


static int ActionDumpLibrary(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	dump_lib_any(0, &library);

	return 0;
}

/* ---------------------------------------------------------------------------
 * no operation, just for testing purposes
 * syntax: Bell(volume)
 */
static const char bell_syntax[] = "Bell()";

static const char bell_help[] = "Attempt to produce audible notification (e.g. beep the speaker).";

static int ActionBell(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	gui->beep();
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char debug_syntax[] = "Debug(...)";

static const char debug_help[] = "Debug action.";

/* %start-doc actions Debug

This action exists to help debug scripts; it simply prints all its
arguments to stdout.

%end-doc */

static const char debugxy_syntax[] = "DebugXY(...)";

static const char debugxy_help[] = "Debug action, with coordinates";

/* %start-doc actions DebugXY

Like @code{Debug}, but requires a coordinate.  If the user hasn't yet
indicated a location on the board, the user will be prompted to click
on one.

%end-doc */

static int Debug(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	int i;
	printf("Debug:");
	for (i = 0; i < argc; i++)
		printf(" [%d] `%s'", i, argv[i]);
	pcb_printf(" x,y %$mD\n", x, y);
	return 0;
}

static const char return_syntax[] = "Return(0|1)";

static const char return_help[] = "Simulate a passing or failing action.";

/* %start-doc actions Return

This is for testing.  If passed a 0, does nothing and succeeds.  If
passed a 1, does nothing but pretends to fail.

%end-doc */

static int Return(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return atoi(argv[0]);
}


static const char djopt_sao_syntax[] = "OptAutoOnly()";

static const char djopt_sao_help[] = "Toggles the optimize-only-autorouted flag.";

/* %start-doc actions OptAutoOnly

The original purpose of the trace optimizer was to clean up the traces
created by the various autorouters that have been used with PCB.  When
a board has a mix of autorouted and carefully hand-routed traces, you
don't normally want the optimizer to move your hand-routed traces.
But, sometimes you do.  By default, the optimizer only optimizes
autorouted traces.  This action toggles that setting, so that you can
optimize hand-routed traces also.

%end-doc */



int djopt_set_auto_only(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	conf_toggle(CFR_DESIGN, "plugins/djopt/auto_only");
	return 0;
}

/* ************************************************************ */

static const char toggle_vendor_syntax[] = "ToggleVendor()";

static const char toggle_vendor_help[] = "Toggles the state of automatic drill size mapping.";

/* %start-doc actions ToggleVendor

@cindex vendor map
@cindex vendor drill table
@findex ToggleVendor()

When drill mapping is enabled, new instances of pins and vias will
have their drill holes mapped to one of the allowed drill sizes
specified in the currently loaded vendor drill table.  To enable drill
mapping, a vendor lihata file containing a drill table must be
loaded first.

%end-doc */

int ActionToggleVendor(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	conf_toggle(CFR_DESIGN, "plugins/vendor/enable");
	return 0;
}

/* ************************************************************ */

static const char enable_vendor_syntax[] = "EnableVendor()";

static const char enable_vendor_help[] = "Enables automatic drill size mapping.";

/* %start-doc actions EnableVendor

@cindex vendor map
@cindex vendor drill table
@findex EnableVendor()

When drill mapping is enabled, new instances of pins and vias will
have their drill holes mapped to one of the allowed drill sizes
specified in the currently loaded vendor drill table.  To enable drill
mapping, a vendor lihata file containing a drill table must be
loaded first.

%end-doc */

int ActionEnableVendor(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	conf_set(CFR_DESIGN, "plugins/vendor/enable", -1, "1", POL_OVERWRITE);
	return 0;
}

/* ************************************************************ */

static const char disable_vendor_syntax[] = "DisableVendor()";

static const char disable_vendor_help[] = "Disables automatic drill size mapping.";

/* %start-doc actions DisableVendor

@cindex vendor map
@cindex vendor drill table
@findex DisableVendor()

When drill mapping is enabled, new instances of pins and vias will
have their drill holes mapped to one of the allowed drill sizes
specified in the currently loaded vendor drill table.

%end-doc */

int ActionDisableVendor(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	conf_set(CFR_DESIGN, "plugins/vendor/enable", -1, "0", POL_OVERWRITE);
	return 0;
}


pcb_hid_action_t oldactions_action_list[] = {
	{"DumpLibrary", 0, ActionDumpLibrary,
	 dumplibrary_help, dumplibrary_syntax},
	{"Bell", 0, ActionBell,
	 bell_help, bell_syntax},
	{"Debug", 0, Debug,
	 debug_help, debug_syntax},
	{"DebugXY", "Click X,Y for Debug", Debug,
	 debugxy_help, debugxy_syntax},
	{"Return", 0, Return,
	 return_help, return_syntax},
	{"OptAutoOnly", 0, djopt_set_auto_only,
	 djopt_sao_help, djopt_sao_syntax},
	{"ToggleVendor", 0, ActionToggleVendor,
	 toggle_vendor_help, toggle_vendor_syntax},
	{"EnableVendor", 0, ActionEnableVendor,
	 enable_vendor_help, enable_vendor_syntax},
	{"DisableVendor", 0, ActionDisableVendor,
	 disable_vendor_help, disable_vendor_syntax}
};

static const char *oldactions_cookie = "oldactions plugin";

PCB_REGISTER_ACTIONS(oldactions_action_list, oldactions_cookie)

static void hid_oldactions_uninit(void)
{
	hid_remove_actions_by_cookie(oldactions_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_oldactions_init(void)
{
	PCB_REGISTER_ACTIONS(oldactions_action_list, oldactions_cookie)
	return hid_oldactions_uninit;
}
