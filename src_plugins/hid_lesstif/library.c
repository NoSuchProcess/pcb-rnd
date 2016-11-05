#include "xincludes.h"

#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "compat_misc.h"
#include "data.h"
#include "set.h"
#include "buffer.h"
#include "vtptr.h"
#include "plug_footprint.h"

#include "hid.h"
#include "lesstif.h"
#include "stdarg.h"

static Widget library_dialog = 0;
static Widget library_list, libnode_list;

static XmString *library_strings = 0;
static XmString *libnode_strings = 0;
static int last_pick = -1;

vtptr_t picks;      /* of library_t * */
vtptr_t pick_names; /* of char * */

static void pick_net(int pick)
{
	library_t *menu = (library_t *)picks.array[pick];
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
	library_t *e = picks.array[last_pick];
	e = &e->data.dir.children.array[cbs->item_position - 1];
	if (LoadElementToBuffer(PASTEBUFFER, e->data.fp.loc_info))
		SetMode(PCB_MODE_PASTE_BUFFER);
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

static void lib_dfs(library_t *parent, int level)
{
	library_t *l;
	char *s;
	int n, len;

	if (parent->type != LIB_DIR)
		return;

	if (parent->name != NULL) {
		vtptr_append(&picks, parent);
		len = strlen(parent->name);
		s = malloc(len+level+1);
		for(n = 0; n < level-1; n++) s[n] = ' ';
		strcpy(s+level-1, parent->name);
		vtptr_append(&pick_names, s);
	}

	for(l = parent->data.dir.children.array, n = 0; n < parent->data.dir.children.used; n++,l++)
		lib_dfs(l, level+1);
}

static int LibraryChanged(int argc, const char **argv, Coord x, Coord y)
{
	int i;
	if (library.data.dir.children.used == 0)
		return 0;
	if (build_library_dialog())
		return 0;
	last_pick = -1;

	for (i = 0; i < pick_names.used; i++)
		free(pick_names.array[i]);

	vtptr_truncate(&picks, 0);
	vtptr_truncate(&pick_names, 0);

	lib_dfs(&library, 0);


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
	return 0;
}

static const char libraryshow_syntax[] = "LibraryShow()";

static const char libraryshow_help[] = "Displays the library window.";

/* %start-doc actions LibraryShow

%end-doc */

static int LibraryShow(int argc, const char **argv, Coord x, Coord y)
{
	if (build_library_dialog())
		return 0;
	return 0;
}

void lesstif_show_library()
{
	if (mainwind) {
		if (!library_dialog)
			LibraryChanged(0, 0, 0, 0);
		XtManageChild(library_dialog);
	}
}

HID_Action lesstif_library_action_list[] = {
	{"LibraryChanged", 0, LibraryChanged,
	 librarychanged_help, librarychanged_syntax}
	,
	{"LibraryShow", 0, LibraryShow,
	 libraryshow_help, libraryshow_syntax}
	,
};

REGISTER_ACTIONS(lesstif_library_action_list, lesstif_cookie)
