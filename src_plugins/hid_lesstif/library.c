#include "xincludes.h"

#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "compat_misc.h"
#include "data.h"
#include "actions.h"
#include "buffer.h"
#include <genvector/vtp0.h>
#include "plug_footprint.h"

#include "hid.h"
#include "lesstif.h"
#include "stdarg.h"
#include "event.h"
#include "tool.h"

static Widget library_dialog = 0;
static Widget library_list, libnode_list;

static XmString *library_strings = 0;
static XmString *libnode_strings = 0;
static int last_pick = -1;

vtp0_t picks;      /* of pcb_fplibrary_t * */
vtp0_t pick_names; /* of char * */

static void pick_net(int pick)
{
	pcb_fplibrary_t *menu = (pcb_fplibrary_t *)picks.array[pick];
	int i, found;

	if (pick == last_pick)
		return;
	last_pick = pick;

	if (libnode_strings)
		free(libnode_strings);

	libnode_strings = (XmString *) malloc(menu->data.dir.children.used * sizeof(XmString));
	for (found = 0, i = 0; i < menu->data.dir.children.used; i++) {
		if (menu->data.dir.children.array[i].type == LIB_FOOTPRINT) {
			libnode_strings[i] = XmStringCreatePCB(menu->data.dir.children.array[i].name);
			found++;
		}
	}

	stdarg_n = 0;
	stdarg(XmNitems, libnode_strings);
	stdarg(XmNitemCount, found);
	XtSetValues(libnode_list, stdarg_args, stdarg_n);
}

static void library_browse(Widget w, void *v, XmListCallbackStruct * cbs)
{
	pick_net(cbs->item_position - 1);
}

static void libnode_select(Widget w, void *v, XmListCallbackStruct * cbs)
{
	pcb_fplibrary_t *e = picks.array[last_pick];
	e = &e->data.dir.children.array[cbs->item_position - 1];
	if (pcb_buffer_load_footprint(PCB_PASTEBUFFER, e->data.fp.loc_info, NULL))
		pcb_tool_select_by_id(PCB_MODE_PASTE_BUFFER);
}

static int build_library_dialog()
{
	if (!mainwind)
		return 1;
	if (library_dialog)
		return 0;

	stdarg_n = 0;
	stdarg(XmNresizePolicy, XmRESIZE_GROW);
	stdarg(XmNtitle, "Element Library");
	library_dialog = XmCreateFormDialog(mainwind, XmStrCast("library"), stdarg_args, stdarg_n);

	stdarg_n = 0;
	stdarg(XmNtopAttachment, XmATTACH_FORM);
	stdarg(XmNbottomAttachment, XmATTACH_FORM);
	stdarg(XmNleftAttachment, XmATTACH_FORM);
	stdarg(XmNvisibleItemCount, 10);
	library_list = XmCreateScrolledList(library_dialog, XmStrCast("nets"), stdarg_args, stdarg_n);
	XtManageChild(library_list);
	XtAddCallback(library_list, XmNbrowseSelectionCallback, (XtCallbackProc) library_browse, 0);

	stdarg_n = 0;
	stdarg(XmNtopAttachment, XmATTACH_FORM);
	stdarg(XmNbottomAttachment, XmATTACH_FORM);
	stdarg(XmNrightAttachment, XmATTACH_FORM);
	stdarg(XmNleftAttachment, XmATTACH_WIDGET);
	stdarg(XmNleftWidget, library_list);
	libnode_list = XmCreateScrolledList(library_dialog, XmStrCast("nodes"), stdarg_args, stdarg_n);
	XtManageChild(libnode_list);
	XtAddCallback(libnode_list, XmNbrowseSelectionCallback, (XtCallbackProc) libnode_select, 0);

	return 0;
}

static void lib_dfs(pcb_fplibrary_t *parent, int level)
{
	pcb_fplibrary_t *l;
	char *s;
	int n, len;

	if (parent->type != LIB_DIR)
		return;

	if (parent->name != NULL) {
		vtp0_append(&picks, parent);
		len = strlen(parent->name);
		s = malloc(len+level+1);
		for(n = 0; n < level-1; n++) s[n] = ' ';
		strcpy(s+level-1, parent->name);
		vtp0_append(&pick_names, s);
	}

	for(l = parent->data.dir.children.array, n = 0; n < parent->data.dir.children.used; n++,l++)
		lib_dfs(l, level+1);
}

void LesstifLibraryChanged(void *user_data, int argc, pcb_event_arg_t argv[])
{
	int i;
	if (pcb_library.data.dir.children.used == 0)
		return;
	if (build_library_dialog())
		return;
	last_pick = -1;

	for (i = 0; i < pick_names.used; i++)
		free(pick_names.array[i]);

	vtp0_truncate(&picks, 0);
	vtp0_truncate(&pick_names, 0);

	lib_dfs(&pcb_library, 0);


	if (library_strings)
		free(library_strings);
	library_strings = (XmString *) malloc(picks.used * sizeof(XmString));
	for (i = 0; i < picks.used; i++)
		library_strings[i] = XmStringCreatePCB(pick_names.array[i]);

	stdarg_n = 0;
	stdarg(XmNitems, library_strings);
	stdarg(XmNitemCount, picks.used);
	XtSetValues(library_list, stdarg_args, stdarg_n);

	pick_net(0);
	return;
}

static const char pcb_acts_LibraryShow[] = "LibraryShow()";
static const char pcb_acth_LibraryShow[] = "[DEPRECATED] Displays the library window.";
static fgw_error_t pcb_act_LibraryShow(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	PCB_ACT_IRES(build_library_dialog());
	return 0;
}

void lesstif_show_library()
{
	if (mainwind) {
		if (!library_dialog)
			LesstifLibraryChanged(0, 0, 0);
		XtManageChild(library_dialog);
		pcb_ltf_winplace(display, XtWindow(XtParent(library_dialog)), "library", 300, 300);
		XtAddEventHandler(XtParent(library_dialog), StructureNotifyMask, False, pcb_ltf_wplc_config_cb, "library");
	}
}

pcb_action_t lesstif_library_action_list[] = {
	{"LibraryShow", pcb_act_LibraryShow, pcb_acth_LibraryShow, pcb_acts_LibraryShow}
};

PCB_REGISTER_ACTIONS(lesstif_library_action_list, lesstif_cookie)
