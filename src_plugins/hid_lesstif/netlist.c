#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "xincludes.h"

#include "compat_misc.h"
#include "data.h"

#include "find.h"
#include "rats.h"
#include "select.h"
#include "undo.h"
#include "remove.h"
#include "crosshair.h"
#include "draw.h"
#include "obj_all.h"

#include "hid.h"
#include "hid_actions.h"
#include "lesstif.h"
#include "stdarg.h"

static Widget netlist_dialog = 0;
static Widget netlist_list, netnode_list;

static XmString *netlist_strings = 0;
static XmString *netnode_strings = 0;
static int n_netnode_strings;
static int last_pick = -1;

static int LesstifNetlistChanged(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y);

static void pick_net(int pick)
{
	pcb_lib_menu_t *menu = PCB->NetlistLib[PCB_NETLIST_EDITED].Menu + pick;
	int i;

	if (pick == last_pick)
		return;
	last_pick = pick;

	if (netnode_strings)
		free(netnode_strings);			/* XXX leaked all XmStrings??? */
	n_netnode_strings = menu->EntryN;
	netnode_strings = (XmString *) malloc(menu->EntryN * sizeof(XmString));
	for (i = 0; i < menu->EntryN; i++)
		netnode_strings[i] = XmStringCreatePCB(menu->Entry[i].ListEntry);
	stdarg_n = 0;
	stdarg(XmNitems, netnode_strings);
	stdarg(XmNitemCount, menu->EntryN);
	XtSetValues(netnode_list, stdarg_args, stdarg_n);
}

static void netlist_select(Widget w, void *v, XmListCallbackStruct * cbs)
{
	XmString str;
	int pos = cbs->item_position;
	pcb_lib_menu_t *net = &(PCB->NetlistLib[PCB_NETLIST_EDITED].Menu[pos - 1]);
	char *name = net->Name;
	if (name[0] == ' ') {
		name[0] = '*';
		net->flag = 0;
	}
	else {
		name[0] = ' ';
		net->flag = 1;
	}

	str = XmStringCreatePCB(name);
	XmListReplaceItemsPos(netlist_list, &str, 1, pos);
	XmStringFree(str);
	XmListSelectPos(netlist_list, pos, False);
}

static void netlist_extend(Widget w, void *v, XmListCallbackStruct * cbs)
{
	if (cbs->selected_item_count == 1)
		pick_net(cbs->item_position - 1);
}

typedef void (*Std_Nbcb_Func) (pcb_lib_menu_t *, int);

static void nbcb_rat_on(pcb_lib_menu_t *net, int pos)
{
	XmString str;
	char *name = net->Name;
	name[0] = ' ';
	net->flag = 1;
	str = XmStringCreatePCB(name);
	XmListReplaceItemsPos(netlist_list, &str, 1, pos);
	XmStringFree(str);
}

static void nbcb_rat_off(pcb_lib_menu_t *net, int pos)
{
	XmString str;
	char *name = net->Name;
	name[0] = '*';
	net->flag = 0;
	str = XmStringCreatePCB(name);
	XmListReplaceItemsPos(netlist_list, &str, 1, pos);
	XmStringFree(str);
}


/* Select on the layout the current net treeview selection
 */
static void nbcb_select_common(pcb_lib_menu_t *net, int pos, int select_flag)
{
	pcb_lib_entry_t *entry;
	pcb_connection_t conn;
	int i;

	pcb_conn_lookup_init();
	pcb_reset_conns(pcb_true);

	for (i = net->EntryN, entry = net->Entry; i; i--, entry++)
		if (pcb_rat_seek_pad(entry, &conn, pcb_false))
			pcb_rat_find_hook(conn.type, conn.ptr1, conn.ptr2, conn.ptr2, pcb_true, pcb_true);

	pcb_select_connection(select_flag);
	pcb_reset_conns(pcb_false);
	pcb_conn_lookup_uninit();
	pcb_undo_inc_serial();
	pcb_draw();
}

static void nbcb_select(pcb_lib_menu_t *net, int pos)
{
	nbcb_select_common(net, pos, 1);
}

static void nbcb_deselect(pcb_lib_menu_t *net, int pos)
{
	nbcb_select_common(net, pos, 0);
}

static void nbcb_find(pcb_lib_menu_t *net, int pos)
{
	char *name = net->Name + 2;
	pcb_hid_actionl("netlist", "find", name, NULL);
}

static void nbcb_std_callback(Widget w, Std_Nbcb_Func v, XmPushButtonCallbackStruct * cbs)
{
	int *posl, posc, i;
	XmString **items, **selected;
	if (XmListGetSelectedPos(netlist_list, &posl, &posc) == False)
		return;
	if (v == nbcb_find)
		pcb_hid_actionl("connection", "reset", NULL);
	for (i = 0; i < posc; i++) {
		pcb_lib_menu_t *net = &(PCB->NetlistLib[PCB_NETLIST_EDITED].Menu[posl[i] - 1]);
		v(net, posl[i]);
	}
	stdarg_n = 0;
	stdarg(XmNitems, &items);
	XtGetValues(netlist_list, stdarg_args, stdarg_n);
	selected = (XmString **) malloc(posc * sizeof(XmString *));
	for (i = 0; i < posc; i++)
		selected[i] = items[posl[i] - 1];

	stdarg_n = 0;
	stdarg(XmNselectedItems, selected);
	XtSetValues(netlist_list, stdarg_args, stdarg_n);
}

static void nbcb_ripup(Widget w, Std_Nbcb_Func v, XmPushButtonCallbackStruct * cbs)
{
	nbcb_std_callback(w, nbcb_find, cbs);

	PCB_LINE_VISIBLE_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_FOUND, line) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, line))
			pcb_remove_object(PCB_TYPE_LINE, layer, line, line);
	}
	PCB_ENDALL_LOOP;

	PCB_ARC_VISIBLE_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_FOUND, arc) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, arc))
			pcb_remove_object(PCB_TYPE_ARC, layer, arc, arc);
	}
	PCB_ENDALL_LOOP;

	if (PCB->ViaOn)
		PCB_VIA_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_FOUND, via) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, via))
			pcb_remove_object(PCB_TYPE_VIA, via, via, via);
	}
	PCB_END_LOOP;
}

static void netnode_browse(Widget w, XtPointer v, XmListCallbackStruct * cbs)
{
	pcb_lib_menu_t *menu = PCB->NetlistLib[PCB_NETLIST_EDITED].Menu + last_pick;
	const char *name = menu->Entry[cbs->item_position - 1].ListEntry;
	char *ename, *pname;

	ename = pcb_strdup(name);
	pname = strchr(ename, '-');
	if (!pname) {
		free(ename);
		return;
	}
	*pname++ = 0;

	PCB_ELEMENT_LOOP(PCB->Data);
	{
		char *es = element->Name[PCB_ELEMNAME_IDX_REFDES].TextString;
		if (es && strcmp(es, ename) == 0) {
			PCB_PIN_LOOP(element);
			{
				if (strcmp(pin->Number, pname) == 0) {
					pcb_crosshair_move_absolute(pin->X, pin->Y);
					free(ename);
					return;
				}
			}
			PCB_END_LOOP;
			PCB_PAD_LOOP(element);
			{
				if (strcmp(pad->Number, pname) == 0) {
					int x = (pad->Point1.X + pad->Point2.X) / 2;
					int y = (pad->Point1.Y + pad->Point2.Y) / 2;
					pcb_gui->set_crosshair(x, y, HID_SC_PAN_VIEWPORT);
					free(ename);
					return;
				}
			}
			PCB_END_LOOP;
		}
	}
	PCB_END_LOOP;
	free(ename);
}

#define NLB_FORM ((Widget)(~0))
static Widget
netlist_button(Widget parent, const char *name, const char *string,
							 Widget top, Widget bottom, Widget left, Widget right, XtCallbackProc callback, void *user_data)
{
	Widget rv;
	XmString str;

#define NLB_W(w) if (w == NLB_FORM) { stdarg(XmN ## w ## Attachment, XmATTACH_FORM); } \
  else if (w) { stdarg(XmN ## w ## Attachment, XmATTACH_WIDGET); \
    stdarg (XmN ## w ## Widget, w); }

	NLB_W(top);
	NLB_W(bottom);
	NLB_W(left);
	NLB_W(right);
	str = XmStringCreatePCB(string);
	stdarg(XmNlabelString, str);
	rv = XmCreatePushButton(parent, XmStrCast(name), stdarg_args, stdarg_n);
	XtManageChild(rv);
	if (callback)
		XtAddCallback(rv, XmNactivateCallback, callback, (XtPointer) user_data);
	XmStringFree(str);
	return rv;
}

static int build_netlist_dialog()
{
	Widget b_sel, b_unsel, b_find, /*b_ripup,*/ b_rat_on, /*b_rat_off,*/ l_ops;
	XmString ops_str;

	if (!mainwind)
		return 1;
	if (netlist_dialog)
		return 0;

	stdarg_n = 0;
	stdarg(XmNresizePolicy, XmRESIZE_GROW);
	stdarg(XmNtitle, "Netlists");
	stdarg(XmNautoUnmanage, False);
	netlist_dialog = XmCreateFormDialog(mainwind, XmStrCast("netlist"), stdarg_args, stdarg_n);

	stdarg_n = 0;
	b_rat_on = netlist_button(netlist_dialog, "rat_on", "Enable for rats",
														0, NLB_FORM, NLB_FORM, 0, (XtCallbackProc) nbcb_std_callback, (void *) nbcb_rat_on);

	stdarg_n = 0;
	/*b_rat_off =*/ netlist_button(netlist_dialog, "rat_off", "Disable for rats",
														 0, NLB_FORM, b_rat_on, 0, (XtCallbackProc) nbcb_std_callback, (void *) nbcb_rat_off);

	stdarg_n = 0;
	b_sel = netlist_button(netlist_dialog, "select", "Select",
												 0, b_rat_on, NLB_FORM, 0, (XtCallbackProc) nbcb_std_callback, (void *) nbcb_select);

	stdarg_n = 0;
	b_unsel = netlist_button(netlist_dialog, "deselect", "Deselect",
													 0, b_rat_on, b_sel, 0, (XtCallbackProc) nbcb_std_callback, (void *) nbcb_deselect);

	stdarg_n = 0;
	b_find = netlist_button(netlist_dialog, "find", "Find",
													0, b_rat_on, b_unsel, 0, (XtCallbackProc) nbcb_std_callback, (void *) nbcb_find);


	stdarg_n = 0;
	/*b_ripup =*/ netlist_button(netlist_dialog, "ripup", "Rip Up", 0, b_rat_on, b_find, 0, (XtCallbackProc) nbcb_ripup, 0);

	stdarg_n = 0;
	stdarg(XmNbottomAttachment, XmATTACH_WIDGET);
	stdarg(XmNbottomWidget, b_sel);
	stdarg(XmNleftAttachment, XmATTACH_FORM);
	ops_str = XmStringCreatePCB("Operations on selected net names:");
	stdarg(XmNlabelString, ops_str);
	l_ops = XmCreateLabel(netlist_dialog, XmStrCast("ops"), stdarg_args, stdarg_n);
	XtManageChild(l_ops);

	stdarg_n = 0;
	stdarg(XmNtopAttachment, XmATTACH_FORM);
	stdarg(XmNbottomAttachment, XmATTACH_WIDGET);
	stdarg(XmNbottomWidget, l_ops);
	stdarg(XmNleftAttachment, XmATTACH_FORM);
	stdarg(XmNrightAttachment, XmATTACH_POSITION);
	stdarg(XmNrightPosition, 50);
	stdarg(XmNvisibleItemCount, 10);
	stdarg(XmNselectionPolicy, XmEXTENDED_SELECT);
	netlist_list = XmCreateScrolledList(netlist_dialog, XmStrCast("nets"), stdarg_args, stdarg_n);
	XtManageChild(netlist_list);
	XtAddCallback(netlist_list, XmNdefaultActionCallback, (XtCallbackProc) netlist_select, 0);
	XtAddCallback(netlist_list, XmNextendedSelectionCallback, (XtCallbackProc) netlist_extend, 0);

	stdarg_n = 0;
	stdarg(XmNtopAttachment, XmATTACH_FORM);
	stdarg(XmNbottomAttachment, XmATTACH_WIDGET);
	stdarg(XmNbottomWidget, l_ops);
	stdarg(XmNrightAttachment, XmATTACH_FORM);
	stdarg(XmNleftAttachment, XmATTACH_POSITION);
	stdarg(XmNleftPosition, 50);
	netnode_list = XmCreateScrolledList(netlist_dialog, XmStrCast("nodes"), stdarg_args, stdarg_n);
	XtManageChild(netnode_list);
	XtAddCallback(netnode_list, XmNbrowseSelectionCallback, (XtCallbackProc) netnode_browse, 0);

	return 0;
}

static int LesstifNetlistChanged(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	int i;
	if (!PCB->NetlistLib[PCB_NETLIST_EDITED].MenuN)
		return 0;
	if (build_netlist_dialog())
		return 0;
	last_pick = -1;
	if (netlist_strings)
		free(netlist_strings);
	netlist_strings = (XmString *) malloc(PCB->NetlistLib[PCB_NETLIST_EDITED].MenuN * sizeof(XmString));
	for (i = 0; i < PCB->NetlistLib[PCB_NETLIST_EDITED].MenuN; i++)
		netlist_strings[i] = XmStringCreatePCB(PCB->NetlistLib[PCB_NETLIST_EDITED].Menu[i].Name);
	stdarg_n = 0;
	stdarg(XmNitems, netlist_strings);
	stdarg(XmNitemCount, PCB->NetlistLib[PCB_NETLIST_EDITED].MenuN);
	XtSetValues(netlist_list, stdarg_args, stdarg_n);
	pick_net(0);
	return 0;
}

static const char netlistshow_syntax[] = "NetlistShow(pinname|netname)";

static const char netlistshow_help[] = "Selects the given pinname or netname in the netlist window.";

/* %start-doc actions NetlistShow

%end-doc */

static int LesstifNetlistShow(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	if (build_netlist_dialog())
		return 0;

	if (argc == 1) {
		pcb_lib_menu_t *net;

		net = pcb_netnode_to_netname(argv[0]);
		if (net) {
			XmString item;
			int vis = 0;

			/* Select net first, 'True' causes pick_net() to be invoked */
			item = XmStringCreatePCB(net->Name);
			XmListSelectItem(netlist_list, item, True);
			XmListSetItem(netlist_list, item);
			XmStringFree(item);

			/* Now the netnode_list has the right contents */
			item = XmStringCreatePCB(argv[0]);
			XmListSelectItem(netnode_list, item, False);

			/*
			 * Only force the item to the top if there are enough to scroll.
			 * A bug (?) in lesstif will cause the window to get ever wider
			 * if an XmList that doesn't require a scrollbar is forced to
			 * have one (when the top item is not the first item).
			 */
			stdarg_n = 0;
			stdarg(XmNvisibleItemCount, &vis);
			XtGetValues(netnode_list, stdarg_args, stdarg_n);
			if (n_netnode_strings > vis) {
				XmListSetItem(netnode_list, item);
			}
			XmStringFree(item);
		}
		else {
			/* Try the argument as a netname */
			net = pcb_netname_to_netname(argv[0]);
			if (net) {
				XmString item;

				item = XmStringCreatePCB(net->Name);
				XmListSetItem(netlist_list, item);
				XmListSelectItem(netlist_list, item, True);
				XmStringFree(item);
			}
		}
	}
	return 0;
}

void lesstif_show_netlist()
{
	build_netlist_dialog();
	XtManageChild(netlist_dialog);
}

pcb_hid_action_t lesstif_netlist_action_list[] = {
	{"NetlistChanged", 0, LesstifNetlistChanged,
	 netlistchanged_help, netlistchanged_syntax},
	{"NetlistShow", 0, LesstifNetlistShow,
	 netlistshow_help, netlistshow_syntax}
};

PCB_REGISTER_ACTIONS(lesstif_netlist_action_list, lesstif_cookie)
