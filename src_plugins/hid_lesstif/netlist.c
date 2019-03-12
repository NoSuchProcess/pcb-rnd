#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "xincludes.h"

#include "compat_misc.h"
#include "data.h"

#include "find.h"
#include "netlist2.h"
#include "select.h"
#include "undo.h"
#include "remove.h"
#include "crosshair.h"
#include "draw.h"
#include "event.h"
#include "fptr_cast.h"

#include "hid.h"
#include "actions.h"
#include "lesstif.h"
#include "stdarg.h"

static Widget netlist_dialog = 0;
static Widget netlist_list, netnode_list;

static XmString *netlist_strings = 0;
static XmString *netnode_strings = 0;
static int n_netnode_strings;
static char *last_pick;

static void pick_net(XmString *name, int pick)
{
	char *net_name = NULL;
	pcb_net_t *net = NULL;
	pcb_net_term_t *t;
	int i;

	if (name != NULL) {
		XmStringGetLtoR(name[0], XmFONTLIST_DEFAULT_TAG, &net_name);
		net = pcb_net_get(PCB, &PCB->netlist[PCB_NETLIST_EDITED], net_name, 0);
	}

	if ((net_name != NULL) && (last_pick != NULL) && (strcmp(net_name, last_pick) == 0))
		return;
	free(last_pick);
	if (net_name == NULL)
		last_pick = NULL;
	else
		last_pick = pcb_strdup(net_name);

	if (netnode_strings)
		free(netnode_strings);			/* XXX leaked all XmStrings??? */

	if (net == NULL)
		return;

	n_netnode_strings = pcb_termlist_length(&net->conns);
	if (n_netnode_strings == 0)
		return;

	netnode_strings = (XmString *) malloc(n_netnode_strings * sizeof(XmString));
	for(t = pcb_termlist_first(&net->conns), i = 0; t != NULL; t = pcb_termlist_next(t), i++) {
		char *tmp = pcb_concat(t->refdes, "-", t->term, NULL);
		netnode_strings[i] = XmStringCreatePCB(tmp);
	}

	stdarg_n = 0;
	stdarg(XmNitems, netnode_strings);
	stdarg(XmNitemCount, n_netnode_strings);
	XtSetValues(netnode_list, stdarg_args, stdarg_n);
}

static void netlist_select(Widget w, void *v, XmListCallbackStruct * cbs)
{
#if 0
	XmString str;
	int pos = cbs->item_position;
	pcb_lib_menu_t *net = &(PCB->NetlistLib[PCB_NETLIST_EDITED].Menu[pos - 1]);
	char *name = net->Name;
TODO("netlist: do not change net name");
	if (name[0] == ' ') {
		name[0] = '*';
		net->flag = 0;
	}
	else {
		name[0] = ' ';
		net->flag = 1;
	}

	str = XmStringCreatePCB(name+2);
	XmListReplaceItemsPos(netlist_list, &str, 1, pos);
	XmStringFree(str);
	XmListSelectPos(netlist_list, pos, False);
#endif
}

static void netlist_extend(Widget w, void *v, XmListCallbackStruct * cbs)
{
	if (cbs->selected_item_count == 1)
		pick_net(cbs->selected_items, cbs->item_position - 1);
}

typedef void (*Std_Nbcb_Func) (pcb_net_t *, int);

static void nbcb_rat_on(pcb_net_t *net, int pos)
{
	net->inhibit_rats = 0;
}

static void nbcb_rat_off(pcb_net_t *net, int pos)
{
	net->inhibit_rats = 1;
}



static void nbcb_select(pcb_net_t *net, int pos)
{
	pcb_actionl("netlist", "select", net->name, NULL);
}

static void nbcb_deselect(pcb_net_t *net, int pos)
{
	pcb_actionl("netlist", "unselect", net->name, NULL);
}

static void nbcb_find(pcb_net_t *net, int pos)
{
	pcb_actionl("netlist", "find", net->name, NULL);
}

static void nbcb_std_callback(Widget w, Std_Nbcb_Func v, XmPushButtonCallbackStruct * cbs)
{
	htsp_entry_t *e;
	int *posl, posc, i, n;
	XmString **items, **selected;
	if (XmListGetSelectedPos(netlist_list, &posl, &posc) == False)
		return;
	if (v == nbcb_find)
		pcb_actionl("connection", "reset", NULL);

	for(e = htsp_first(&PCB->netlist[PCB_NETLIST_EDITED]), i = 0; e != NULL; e = htsp_next(&PCB->netlist[PCB_NETLIST_EDITED], e), i++) {
		pcb_net_t *net = e->value;
		for(n = 0; n < posc; n++)
			if (posl[n]-1 == i)
				v(net, i);
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
			pcb_remove_object(PCB_OBJ_LINE, layer, line, line);
	}
	PCB_ENDALL_LOOP;

	PCB_ARC_VISIBLE_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_FOUND, arc) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, arc))
			pcb_remove_object(PCB_OBJ_ARC, layer, arc, arc);
	}
	PCB_ENDALL_LOOP;

	if (PCB->pstk_on)
		PCB_PADSTACK_LOOP(PCB->Data);
		{
			if (PCB_FLAG_TEST(PCB_FLAG_FOUND, padstack) && !PCB_FLAG_TEST(PCB_FLAG_LOCK, padstack))
				pcb_remove_object(PCB_OBJ_PSTK, padstack, padstack, padstack);
		}
		PCB_END_LOOP;
}

static void netnode_browse(Widget w, XtPointer v, XmListCallbackStruct * cbs)
{
TODO("subc TODO")
#if 0
	pcb_net_t *net;
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
					pcb_coord_t x = (pad->Point1.X + pad->Point2.X) / 2;
					pcb_coord_t y = (pad->Point1.Y + pad->Point2.Y) / 2;
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
#endif
}

#define NLB_FORM ((Widget)(~0))
static Widget
netlist_button(Widget parent, const char *name, const char *string,
							 Widget top, Widget bottom, Widget left, Widget right, XtCallbackProc callback, Std_Nbcb_Func user_func)
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
		XtAddCallback(rv, XmNactivateCallback, callback, (XtPointer)pcb_cast_f2d(user_func));
	XmStringFree(str);
	return rv;
}

static int build_netlist_dialog()
{
	Widget b_sel, b_unsel, b_find, /*b_ripup,*/ b_rat_on, b_rat_off, l_ops;
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
														0, NLB_FORM, NLB_FORM, 0, (XtCallbackProc)nbcb_std_callback, nbcb_rat_on);
	XtSetSensitive(b_rat_on, 0);

	stdarg_n = 0;
	b_rat_off = netlist_button(netlist_dialog, "rat_off", "Disable for rats",
														 0, NLB_FORM, b_rat_on, 0, (XtCallbackProc)nbcb_std_callback, nbcb_rat_off);
	XtSetSensitive(b_rat_off, 0);

	stdarg_n = 0;
	b_sel = netlist_button(netlist_dialog, "select", "Select",
												 0, b_rat_on, NLB_FORM, 0, (XtCallbackProc)nbcb_std_callback, nbcb_select);

	stdarg_n = 0;
	b_unsel = netlist_button(netlist_dialog, "deselect", "Deselect",
													 0, b_rat_on, b_sel, 0, (XtCallbackProc)nbcb_std_callback, nbcb_deselect);

	stdarg_n = 0;
	b_find = netlist_button(netlist_dialog, "find", "Find",
													0, b_rat_on, b_unsel, 0, (XtCallbackProc)nbcb_std_callback, nbcb_find);


	stdarg_n = 0;
	/*b_ripup =*/ netlist_button(netlist_dialog, "ripup", "Rip Up", 0, b_rat_on, b_find, 0, (XtCallbackProc)nbcb_ripup, 0);

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

void LesstifNetlistChanged(void *user_data, int argc, pcb_event_arg_t argv[])
{
	htsp_entry_t *e;
	int i;
	if (PCB->netlist[PCB_NETLIST_EDITED].used < 1)
		return;
	if (build_netlist_dialog())
		return;
	free(last_pick);
	last_pick = NULL;
	if (netlist_strings)
		free(netlist_strings);
	netlist_strings = (XmString *) malloc(PCB->netlist[PCB_NETLIST_EDITED].used * sizeof(XmString));
	for(e = htsp_first(&PCB->netlist[PCB_NETLIST_EDITED]), i = 0; e != NULL; e = htsp_next(&PCB->netlist[PCB_NETLIST_EDITED], e), i++) {
		pcb_net_t *net = e->value;
		netlist_strings[i] = XmStringCreatePCB(net->name);
	}
	stdarg_n = 0;
	stdarg(XmNitems, netlist_strings);
	stdarg(XmNitemCount, PCB->netlist[PCB_NETLIST_EDITED].used);
	XtSetValues(netlist_list, stdarg_args, stdarg_n);
	pick_net(NULL, 0);
	return;
}

static const char pcb_acts_LesstifNetlistShow[] = "NetlistShow(pinname|netname)";
static const char pcb_acth_LesstifNetlistShow[] = "Selects the given pinname or netname in the netlist window.";
static fgw_error_t pcb_act_LesstifNetlistShow(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
		const char *nn;

	if (build_netlist_dialog()) {
		PCB_ACT_IRES(0);
		return 0;
	}

		PCB_ACT_CONVARG(1, FGW_STR, LesstifNetlistShow, nn = argv[1].val.str);

	if (argc == 2) {
		pcb_net_term_t *term;

		term = pcb_net_find_by_pinname(&PCB->netlist[PCB_NETLIST_EDITED], nn);
		if (term != NULL) {
			XmString item;
			int vis = 0;

			/* Select net first, 'True' causes pick_net() to be invoked */
			item = XmStringCreatePCB(term->parent.net->name);
			XmListSelectItem(netlist_list, item, True);
			XmListSetItem(netlist_list, item);
			XmStringFree(item);

			/* Now the netnode_list has the right contents */
			item = XmStringCreatePCB(nn);
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
			pcb_net_t *net = pcb_net_get(PCB, &PCB->netlist[PCB_NETLIST_EDITED], nn, 0);
			if (net != NULL) {
				XmString item;

				item = XmStringCreatePCB(net->name);
				XmListSetItem(netlist_list, item);
				XmListSelectItem(netlist_list, item, True);
				XmStringFree(item);
			}
		}
	}
	PCB_ACT_IRES(0);
	return 0;
}

void lesstif_show_netlist()
{
	build_netlist_dialog();
	XtManageChild(netlist_dialog);
	pcb_ltf_winplace(display, XtWindow(XtParent(netlist_dialog)), "netlist", 300, 300);
	XtAddEventHandler(XtParent(netlist_dialog), StructureNotifyMask, False, pcb_ltf_wplc_config_cb, "netlist");
}

pcb_action_t lesstif_netlist_action_list[] = {
	{"NetlistShow", pcb_act_LesstifNetlistShow, pcb_acth_LesstifNetlistShow, pcb_acts_LesstifNetlistShow}
};

PCB_REGISTER_ACTIONS(lesstif_netlist_action_list, lesstif_cookie)
