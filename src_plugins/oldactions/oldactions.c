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

static const char pcb_acts_DumpLibrary[] = "DumpLibrary()";

static const char pcb_acth_DumpLibrary[] = "Display the entire contents of the libraries.";

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


static int pcb_act_DumpLibrary(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	dump_lib_any(0, &pcb_library);

	return 0;
}

/* ---------------------------------------------------------------------------
 * no operation, just for testing purposes
 * syntax: Bell(volume)
 */
static const char pcb_acts_Bell[] = "Bell()";

static const char pcb_acth_Bell[] = "Attempt to produce audible notification (e.g. beep the speaker).";

static int pcb_act_Bell(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_gui->beep();
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_acts_debug[] = "Debug(...)";

static const char pcb_acth_debug[] = "Debug action.";

/* %start-doc actions Debug

This action exists to help debug scripts; it simply prints all its
arguments to stdout.

%end-doc */

static const char pcb_acts_debugxy[] = "DebugXY(...)";

static const char pcb_acth_debugxy[] = "Debug action, with coordinates";

/* %start-doc actions DebugXY

Like @code{Debug}, but requires a coordinate.  If the user hasn't yet
indicated a location on the board, the user will be prompted to click
on one.

%end-doc */

static int pcb_act_Debug(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	int i;
	printf("Debug:");
	for (i = 0; i < argc; i++)
		printf(" [%d] `%s'", i, argv[i]);
	pcb_printf(" x,y %$mD\n", x, y);
	return 0;
}

static const char pcb_acts_return[] = "Return(0|1)";

static const char pcb_acth_return[] = "Simulate a passing or failing action.";

/* %start-doc actions Return

This is for testing.  If passed a 0, does nothing and succeeds.  If
passed a 1, does nothing but pretends to fail.

%end-doc */

static int pcb_act_Return(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return atoi(argv[0]);
}


static const char pcb_acts_djopt_sao[] = "OptAutoOnly()";

static const char pcb_acth_djopt_sao[] = "Toggles the optimize-only-autorouted flag.";

/* %start-doc actions OptAutoOnly

The original purpose of the trace optimizer was to clean up the traces
created by the various autorouters that have been used with PCB.  When
a board has a mix of autorouted and carefully hand-routed traces, you
don't normally want the optimizer to move your hand-routed traces.
But, sometimes you do.  By default, the optimizer only optimizes
autorouted traces.  This action toggles that setting, so that you can
optimize hand-routed traces also.

%end-doc */



int pcb_act_djopt_set_auto_only(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	conf_toggle(CFR_DESIGN, "plugins/djopt/auto_only");
	return 0;
}

/* ************************************************************ */

static const char pcb_acts_toggle_vendor[] = "ToggleVendor()";

static const char pcb_acth_toggle_vendor[] = "Toggles the state of automatic drill size mapping.";

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

int pcb_act_ToggleVendor(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	conf_toggle(CFR_DESIGN, "plugins/vendor/enable");
	return 0;
}

/* ************************************************************ */

static const char pcb_acts_enable_vendor[] = "EnableVendor()";

static const char pcb_acth_enable_vendor[] = "Enables automatic drill size mapping.";

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

int pcb_act_EnableVendor(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	conf_set(CFR_DESIGN, "plugins/vendor/enable", -1, "1", POL_OVERWRITE);
	return 0;
}

/* ************************************************************ */

static const char pcb_acts_disable_vendor[] = "DisableVendor()";

static const char pcb_acth_disable_vendor[] = "Disables automatic drill size mapping.";

/* %start-doc actions DisableVendor

@cindex vendor map
@cindex vendor drill table
@findex DisableVendor()

When drill mapping is enabled, new instances of pins and vias will
have their drill holes mapped to one of the allowed drill sizes
specified in the currently loaded vendor drill table.

%end-doc */

int pcb_act_DisableVendor(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	conf_set(CFR_DESIGN, "plugins/vendor/enable", -1, "0", POL_OVERWRITE);
	return 0;
}


pcb_hid_action_t oldactions_action_list[] = {
	{"DumpLibrary", 0, pcb_act_DumpLibrary,
	 pcb_acth_DumpLibrary, pcb_acts_DumpLibrary},
	{"Bell", 0, pcb_act_Bell,
	 pcb_acth_Bell, pcb_acts_Bell},
	{"Debug", 0, pcb_act_Debug,
	 pcb_acth_debug, pcb_acts_debug},
	{"DebugXY", "Click X,Y for Debug", pcb_act_Debug,
	 pcb_acth_debugxy, pcb_acts_debugxy},
	{"Return", 0, pcb_act_Return,
	 pcb_acth_return, pcb_acts_return},
	{"OptAutoOnly", 0, pcb_act_djopt_set_auto_only,
	 pcb_acth_djopt_sao, pcb_acts_djopt_sao},
	{"ToggleVendor", 0, pcb_act_ToggleVendor,
	 pcb_acth_toggle_vendor, pcb_acts_toggle_vendor},
	{"EnableVendor", 0, pcb_act_EnableVendor,
	 pcb_acth_enable_vendor, pcb_acts_enable_vendor},
	{"DisableVendor", 0, pcb_act_DisableVendor,
	 pcb_acth_disable_vendor, pcb_acts_disable_vendor}
};

static const char *oldactions_cookie = "oldactions plugin";

PCB_REGISTER_ACTIONS(oldactions_action_list, oldactions_cookie)

static void hid_oldactions_uninit(void)
{
	pcb_hid_remove_actions_by_cookie(oldactions_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_oldactions_init(void)
{
	PCB_REGISTER_ACTIONS(oldactions_action_list, oldactions_cookie)
	return hid_oldactions_uninit;
}
