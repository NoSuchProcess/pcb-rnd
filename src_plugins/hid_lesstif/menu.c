#include "xincludes.h"

#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "data.h"
#include "error.h"
#include "pcb-printf.h"
#include "layer.h"

#include "hid.h"
#include "hid_cfg.h"
#include "hid_cfg_action.h"
#include "hid_cfg_input.h"
#include "lesstif.h"
#include "paths.h"
#include "hid_actions.h"
#include "hid_flags.h"
#include "stdarg.h"
#include "event.h"
#include "compat_misc.h"
#include "layer_vis.h"
#include <genht/hash.h>

Widget lesstif_menubar;
pcb_hid_cfg_t *lesstif_cfg;

#ifndef R_OK
/* Common value for systems that don't define it.  */
#define R_OK 4
#endif

static Colormap cmap;

static void note_accelerator(const lht_node_t *node);
static int note_widget_flag(Widget w, char *type, const char *name);

static const char getxy_syntax[] = "GetXY()";

static const char getxy_help[] = "Get a coordinate.";

/* %start-doc actions GetXY

Prompts the user for a coordinate, if one is not already selected.

%end-doc */

static int GetXY(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	return 0;
}

/*-----------------------------------------------------------------------------*/

#define LB_RATS	(PCB_MAX_LAYER+0)
#define LB_NUMPICK (LB_RATS+1)
/* more */
#define LB_BACK	(PCB_MAX_LAYER+1)
#define LB_SUBC	(PCB_MAX_LAYER+2)
#define LB_SUBC_PARTS	(PCB_MAX_LAYER+3)
#define LB_PSTK_MARKS	(PCB_MAX_LAYER+4)
#define LB_HOLES	(PCB_MAX_LAYER+5)
#define LB_NUM  (PCB_MAX_LAYER+6)

typedef struct {
	Widget w[LB_NUM];
	int is_pick;
} LayerButtons;

static LayerButtons *layer_button_list = 0;
static int num_layer_buttons = 0;
static int fg_colors[LB_NUM];
static int bg_color;

extern Widget lesstif_m_layer;

void LesstifLayersChanged(void *user_data, int argc, pcb_event_arg_t argv[])
{
	int l, i, set;
	const char *name;
	int current_layer;

	if (!layer_button_list)
		return;
	if (PCB && PCB->Data) {
		pcb_data_t *d = PCB->Data;
		for (i = 0; i < PCB_MAX_LAYER; i++) {
			pcb_layer_type_t lyt = pcb_layer_flags(PCB, i);
#warning layer TODO: hardwired layer colors
			if (lyt & PCB_LYT_SILK) fg_colors[i] = lesstif_parse_color(conf_core.appearance.color.element);
			else if (lyt & PCB_LYT_MASK) fg_colors[i] = lesstif_parse_color(conf_core.appearance.color.mask);
			else if (lyt & PCB_LYT_PASTE) fg_colors[i] = lesstif_parse_color(conf_core.appearance.color.paste);
			else fg_colors[i] = lesstif_parse_color(d->Layer[i].meta.real.color);
		}

		fg_colors[LB_RATS] = lesstif_parse_color(conf_core.appearance.color.rat);
		fg_colors[LB_SUBC] = lesstif_parse_color(conf_core.appearance.color.subc);
		fg_colors[LB_SUBC_PARTS] = lesstif_parse_color(conf_core.appearance.color.subc);
		fg_colors[LB_PSTK_MARKS] = lesstif_parse_color(conf_core.appearance.color.subc);
		fg_colors[LB_HOLES] = lesstif_parse_color(conf_core.appearance.color.via);
		fg_colors[LB_BACK] = lesstif_parse_color(conf_core.appearance.color.invisible_objects);
		bg_color = lesstif_parse_color(conf_core.appearance.color.background);
	}
	else {
		for (i = 0; i < PCB_MAX_LAYER; i++)
			fg_colors[i] = lesstif_parse_color(conf_core.appearance.color.layer[i]);
		fg_colors[LB_RATS] = lesstif_parse_color(conf_core.appearance.color.rat);
		fg_colors[LB_SUBC] = lesstif_parse_color(conf_core.appearance.color.subc);
		fg_colors[LB_SUBC_PARTS] = lesstif_parse_color(conf_core.appearance.color.subc);
		fg_colors[LB_PSTK_MARKS] = lesstif_parse_color(conf_core.appearance.color.subc);
		fg_colors[LB_HOLES] = lesstif_parse_color(conf_core.appearance.color.via);
		fg_colors[LB_BACK] = lesstif_parse_color(conf_core.appearance.color.invisible_objects);
		bg_color = lesstif_parse_color(conf_core.appearance.color.background);
	}

	if (PCB->RatDraw)
		current_layer = LB_RATS;
	else
		current_layer = pcb_layer_stack[0];

	for (l = 0; l < num_layer_buttons; l++) {
		LayerButtons *lb = layer_button_list + l;
		for (i = 0; i < (lb->is_pick ? LB_NUMPICK : LB_NUM); i++) {
			switch (i) {
			case LB_RATS:
				set = PCB->RatOn;
				break;
			case LB_SUBC:
				set = PCB->SubcOn;
				break;
			case LB_SUBC_PARTS:
				set = PCB->SubcPartsOn;
				break;
			case LB_PSTK_MARKS:
				set = PCB->padstack_mark_on;
				break;
			case LB_BACK:
				set = PCB->InvisibleObjectsOn;
				break;
			default:									/* layers */
				set = PCB->Data->Layer[i].meta.real.vis;
				break;
			}

			stdarg_n = 0;
			if (i < PCB_MAX_LAYER && PCB->Data->Layer[i].name) {
				XmString s = XmStringCreatePCB(PCB->Data->Layer[i].name);
				stdarg(XmNlabelString, s);
			}
			if (!lb->is_pick) {
				if (set) {
					stdarg(XmNforeground, bg_color);
					stdarg(XmNbackground, fg_colors[i]);
				}
				else {
					stdarg(XmNforeground, fg_colors[i]);
					stdarg(XmNbackground, bg_color);
				}
				stdarg(XmNset, set);
			}
			else {
				stdarg(XmNforeground, bg_color);
				stdarg(XmNbackground, fg_colors[i]);
				stdarg(XmNset, current_layer == i ? True : False);
			}
			XtSetValues(lb->w[i], stdarg_args, stdarg_n);

			if (i >= pcb_max_layer && i < PCB_MAX_LAYER)
				XtUnmanageChild(lb->w[i]);
			else
				XtManageChild(lb->w[i]);
		}
	}
	if (lesstif_m_layer) {
		switch (current_layer) {
		case LB_RATS:
			name = "Rats";
			break;
		default:
			name = PCB->Data->Layer[current_layer].name;
			break;
		}
		stdarg_n = 0;
		stdarg(XmNbackground, fg_colors[current_layer]);
		stdarg(XmNforeground, bg_color);
		stdarg(XmNlabelString, XmStringCreatePCB(name));
		XtSetValues(lesstif_m_layer, stdarg_args, stdarg_n);
	}

	lesstif_update_layer_groups();

	return;
}

static void show_one_layer_button(int layer, int set)
{
	int l;

	stdarg_n = 0;
	if (set) {
		stdarg(XmNforeground, bg_color);
		stdarg(XmNbackground, fg_colors[layer]);
	}
	else {
		stdarg(XmNforeground, fg_colors[layer]);
		stdarg(XmNbackground, bg_color);
	}
	stdarg(XmNset, set);

	for (l = 0; l < num_layer_buttons; l++) {
		LayerButtons *lb = layer_button_list + l;
		if (!lb->is_pick)
			XtSetValues(lb->w[layer], stdarg_args, stdarg_n);
	}
}

static void layer_button_callback(Widget w, int layer, XmPushButtonCallbackStruct * pbcs)
{
	int l, set;
	switch (layer) {
	case LB_RATS:
		set = PCB->RatOn = !PCB->RatOn;
		break;
	case LB_SUBC:
		set = PCB->SubcOn = !PCB->SubcOn;
		break;
	case LB_SUBC_PARTS:
		set = PCB->SubcPartsOn = !PCB->SubcPartsOn;
		break;
	case LB_PSTK_MARKS:
		set = PCB->padstack_mark_on = !PCB->padstack_mark_on;
		break;
	case LB_HOLES:
		set = PCB->hole_on = !PCB->hole_on;
		break;
	case LB_BACK:
		set = PCB->InvisibleObjectsOn = !PCB->InvisibleObjectsOn;
		break;
	default:											/* layers */
		set = PCB->Data->Layer[layer].meta.real.vis = !PCB->Data->Layer[layer].meta.real.vis;
		break;
	}

	show_one_layer_button(layer, set);
	if (layer < pcb_max_layer) {
		int i;
		pcb_layergrp_id_t group = pcb_layer_get_group(PCB, layer);
		for (i = 0; i < PCB->LayerGroups.grp[group].len; i++) {
			l = PCB->LayerGroups.grp[group].lid[i];
			if (l != layer) {
				show_one_layer_button(l, set);
				PCB->Data->Layer[l].meta.real.vis = set;
			}
		}
	}
	lesstif_invalidate_all();
}

static void layerpick_button_callback(Widget w, int layer, XmPushButtonCallbackStruct * pbcs)
{
	int l, i;
	const char *name;
	PCB->RatDraw = (layer == LB_RATS);
	if (layer < pcb_max_layer)
		pcb_layervis_change_group_vis(layer, 1, 1);
	for (l = 0; l < num_layer_buttons; l++) {
		LayerButtons *lb = layer_button_list + l;
		if (!lb->is_pick)
			continue;
		for (i = 0; i < LB_NUMPICK; i++)
			XmToggleButtonSetState(lb->w[i], layer == i, False);
	}
	switch (layer) {
	case LB_RATS:
		name = "Rats";
		break;
	default:
		name = PCB->Data->Layer[layer].name;
		break;
	}
	stdarg_n = 0;
	stdarg(XmNbackground, fg_colors[layer]);
	stdarg(XmNforeground, bg_color);
	stdarg(XmNlabelString, XmStringCreatePCB(name));
	XtSetValues(lesstif_m_layer, stdarg_args, stdarg_n);
	lesstif_invalidate_all();
}

static void insert_layerview_buttons(Widget menu)
{
	int i, s;
	LayerButtons *lb;

	num_layer_buttons++;
	s = num_layer_buttons * sizeof(LayerButtons);
	if (layer_button_list)
		layer_button_list = (LayerButtons *) realloc(layer_button_list, s);
	else
		layer_button_list = (LayerButtons *) malloc(s);
	lb = layer_button_list + num_layer_buttons - 1;

	for (i = 0; i < LB_NUM; i++) {
		static char namestr[] = "Label ";
		const char *name = namestr;
		Widget btn;
		namestr[5] = 'A' + i;
		switch (i) {
		case LB_RATS:
			name = "Rat Lines";
			break;
		case LB_SUBC:
			name = "Subcircuits";
			break;
		case LB_SUBC_PARTS:
			name = "Subc. parts";
			break;
		case LB_PSTK_MARKS:
			name = "Pstk. Marks";
			break;
		case LB_HOLES:
			name = "Holes";
			break;
		case LB_BACK:
			name = "Far Side";
			break;
		}
		stdarg_n = 0;
		btn = XmCreateToggleButton(menu, XmStrCast(name), stdarg_args, stdarg_n);
		XtManageChild(btn);
		XtAddCallback(btn, XmNvalueChangedCallback, (XtCallbackProc) layer_button_callback, (XtPointer) (size_t) i);
		lb->w[i] = btn;
	}
	lb->is_pick = 0;
	LesstifLayersChanged(0, 0, 0);
}

static void insert_layerpick_buttons(Widget menu)
{
	int i, s;
	LayerButtons *lb;

	num_layer_buttons++;
	s = num_layer_buttons * sizeof(LayerButtons);
	if (layer_button_list)
		layer_button_list = (LayerButtons *) realloc(layer_button_list, s);
	else
		layer_button_list = (LayerButtons *) malloc(s);
	lb = layer_button_list + num_layer_buttons - 1;

	for (i = 0; i < LB_NUMPICK; i++) {
		static char namestr[] = "Label ";
		const char *name = namestr;
		char av[30];
		Widget btn;
		namestr[5] = 'A' + i;
		switch (i) {
		case LB_RATS:
			name = "Rat Lines";
			strcpy(av, "SelectLayer(Rats)");
			break;
		default:
			sprintf(av, "SelectLayer(%d)", i + 1);
			break;
		}
		stdarg_n = 0;
		stdarg(XmNindicatorType, XmONE_OF_MANY);
		btn = XmCreateToggleButton(menu, XmStrCast(name), stdarg_args, stdarg_n);
		XtManageChild(btn);
		XtAddCallback(btn, XmNvalueChangedCallback, (XtCallbackProc) layerpick_button_callback, (XtPointer) (size_t) i);
		lb->w[i] = btn;
	}
	lb->is_pick = 1;
	LesstifLayersChanged(0, 0, 0);
}

/*-----------------------------------------------------------------------------*/

typedef struct {
	Widget w;
	const char *flagname;
	int oldval;
	char *xres;
} Widgetpcb_flag_t;

static Widgetpcb_flag_t *wflags = 0;
static int n_wflags = 0;
static int max_wflags = 0;

static int note_widget_flag(Widget w, char *type, const char *name)
{
	int idx;

	/* look for a free slot to reuse */
	for(idx = 0; idx < n_wflags; idx++)
		if (wflags[idx].w == NULL)
			goto add;

	/* no free slot, alloc a new one */
	if (n_wflags >= max_wflags) {
		max_wflags += 20;
		wflags = (Widgetpcb_flag_t *) realloc(wflags, max_wflags * sizeof(Widgetpcb_flag_t));
	}
	idx = n_wflags++;

	add:;
	wflags[idx].w = w;
	wflags[idx].flagname = name;
	wflags[idx].oldval = -1;
	wflags[idx].xres = type;
	return idx;
}

static int del_widget_flag(int idx)
{
	wflags[idx].w = NULL;
	wflags[idx].flagname = NULL;
	wflags[idx].xres = NULL;
	return 0;
}

void lesstif_update_widget_flags()
{
	int i;
	for (i = 0; i < n_wflags; i++) {
		int v;
		Arg args[2];

		if (wflags[i].w == NULL)
			continue;

		v = pcb_hid_get_flag(wflags[i].flagname);
		if (v < 0) {
			XtSetArg(args[0], wflags[i].xres, 0);
			XtSetArg(args[1], XtNsensitive, 0);
			XtSetValues(wflags[i].w, args, 2);
		}
		else {
			XtSetArg(args[0], wflags[i].xres, v ? 1 : 0);
			XtSetValues(wflags[i].w, args, 1);
		}
		wflags[i].oldval = v;
	}
}

/*-----------------------------------------------------------------------------*/

pcb_hid_action_t lesstif_menu_action_list[] = {
	{"GetXY", "", GetXY,
	 getxy_help, getxy_syntax},
};

PCB_REGISTER_ACTIONS(lesstif_menu_action_list, lesstif_cookie)
#if 0
		 static void
		   stdarg_do_color(char *value, char *which)
{
	XColor color;
	if (XParseColor(display, cmap, value, &color))
		if (XAllocColor(display, cmap, &color)) {
			stdarg(which, color.pixel);
		}
}
#endif

static int need_xy = 0, have_xy = 0, action_x, action_y;

#if 0
typedef struct ToggleItem {
	struct ToggleItem *next;
	Widget w;
	char *group, *item;
	XtCallbackProc callback;
	lht_node_t *node;
} ToggleItem;
static ToggleItem *toggle_items = 0;

static void radio_callback(Widget toggle, ToggleItem * me, XmToggleButtonCallbackStruct * cbs)
{
	if (!cbs->set)								/* uh uh, can't turn it off */
		XmToggleButtonSetState(toggle, 1, 0);
	else {
		ToggleItem *ti;
		for (ti = toggle_items; ti; ti = ti->next)
			if (strcmp(me->group, ti->group) == 0) {
				if (me->item == ti->item || strcmp(me->item, ti->item) == 0)
					XmToggleButtonSetState(ti->w, 1, 0);
				else
					XmToggleButtonSetState(ti->w, 0, 0);
			}
		me->callback(toggle, me->node, cbs);
	}
}
#endif

int lesstif_button_event(Widget w, XEvent * e)
{
	have_xy = 1;
	action_x = e->xbutton.x;
	action_y = e->xbutton.y;
	if (!need_xy)
		return 0;
	if (w != work_area)
		return 1;
	return 0;
}

void lesstif_get_xy(const char *message)
{
	XmString ls = XmStringCreatePCB(message);

	XtManageChild(m_click);
	stdarg_n = 0;
	stdarg(XmNlabelString, ls);
	XtSetValues(m_click, stdarg_args, stdarg_n);
	/*printf("need xy: msg `%s'\n", msg); */
	need_xy = 1;
	XBell(display, 100);
	while (!have_xy) {
		XEvent e;
		XtAppNextEvent(app_context, &e);
		XtDispatchEvent(&e);
	}
	need_xy = 0;
	have_xy = 1;
	XtUnmanageChild(m_click);
}

void lesstif_get_coords(const char *msg, pcb_coord_t * px, pcb_coord_t * py)
{
	if (!have_xy && msg)
		lesstif_get_xy(msg);
	if (have_xy)
		lesstif_coords_to_pcb(action_x, action_y, px, py);
}

static void callback(Widget w, lht_node_t * node, XmPushButtonCallbackStruct * pbcs)
{
	have_xy = 0;
	lesstif_show_crosshair(0);
	if (pbcs->event && pbcs->event->type == KeyPress) {
		Dimension wx, wy;
		Widget aw = XtWindowToWidget(display, pbcs->event->xkey.window);
		action_x = pbcs->event->xkey.x;
		action_y = pbcs->event->xkey.y;
		if (aw) {
			Widget p = work_area;
			while (p && p != aw) {
				stdarg_n = 0;
				stdarg(XmNx, &wx);
				stdarg(XmNy, &wy);
				XtGetValues(p, stdarg_args, stdarg_n);
				action_x -= wx;
				action_y -= wy;
				p = XtParent(p);
			}
			if (p == aw)
				have_xy = 1;
		}
		/*pcb_printf("have xy from %s: %$mD\n", XtName(aw), action_x, action_y); */
	}

	lesstif_need_idle_proc();
	pcb_hid_cfg_action(node);
}

static void note_accelerator(const lht_node_t *node)
{
	lht_node_t *anode, *knode;
	assert(node != NULL);
	anode = pcb_hid_cfg_menu_field(node, PCB_MF_ACTION, NULL);
	knode = pcb_hid_cfg_menu_field(node, PCB_MF_ACCELERATOR, NULL);
	if ((anode != NULL) && (knode != NULL))
		pcb_hid_cfg_keys_add_by_desc(&lesstif_keymap, knode, anode, NULL, 0);
	else
		pcb_hid_cfg_error(node, "No action specified for key accel\n");
}

int lesstif_key_event(XKeyEvent * e)
{
	char buf[10];
	KeySym sym;
	int slen;
	int mods = 0;
	static pcb_hid_cfg_keyseq_t *seq[32];
	static int seq_len = 0;

	if (e->state & ShiftMask)
		mods |= PCB_M_Shift;
	if (e->state & ControlMask)
		mods |= PCB_M_Ctrl;
	if (e->state & Mod1Mask)
		mods |= PCB_M_Alt;

	e->state &= ~(ControlMask | Mod1Mask);

	if (e->state & ShiftMask)
		e->state &= ~ShiftMask;
	slen = XLookupString(e, buf, sizeof(buf), &sym, NULL);

	/* Ignore these.  */
	switch (sym) {
	case XK_Shift_L:
	case XK_Shift_R:
	case XK_Control_L:
	case XK_Control_R:
	case XK_Caps_Lock:
	case XK_Shift_Lock:
	case XK_Meta_L:
	case XK_Meta_R:
	case XK_Alt_L:
	case XK_Alt_R:
	case XK_Super_L:
	case XK_Super_R:
	case XK_Hyper_L:
	case XK_Hyper_R:
	case XK_ISO_Level3_Shift:
		return 1;
	}

/* TODO#3: this works only on US keyboard */
	if (mods & PCB_M_Shift) {
		static const char *lower = "`1234567890-=[]\\;',./";
		static const char *upper = "~!@#$%^&*()_+{}|:\"<>?";
		char *l;
		if ((sym >= 'A') && (sym <= 'Z'))
			sym = tolower(sym);
		else if ((l = strchr(lower, sym)) != NULL) {
			sym = upper[l - lower];
			mods &= ~PCB_M_Shift;
		}
	}

/*	printf("KEY lookup: mod=%x sym=%x/%d\n", mods, sym, slen); */
#warning TODO#3: pass on raw and translated keys
	slen = pcb_hid_cfg_keys_input(&lesstif_keymap, mods, sym, sym, seq, &seq_len);
	if (slen <= 0)
		return 1;

	if (e->window == XtWindow(work_area)) {
		have_xy = 1;
		action_x = e->x;
		action_y = e->y;
	}
	else
		have_xy = 0;

	/* Parsing actions may not return until more user interaction
	   happens.  */
	pcb_hid_cfg_keys_action(seq, slen);

	return 1;
}

static void add_node_to_menu(Widget menu, lht_node_t *node, XtCallbackProc callback, int level);

typedef struct {
	Widget sub;     /* the open menu pane that hosts all the submenus */
	Widget btn;     /* the button in the menu line */
	int wflag_idx;  /* index in the wflags[] array */
} menu_data_t;

menu_data_t *menu_data_alloc(void)
{
	menu_data_t *md = calloc(sizeof(menu_data_t), 1);
	md->wflag_idx = -1;
	return md;
}

static int del_menu(void *ctx, lht_node_t *node)
{
	menu_data_t *md = node->user_data;

	if (md == NULL)
		return 0;

	if (md->wflag_idx >= 0)
		del_widget_flag(md->wflag_idx);

	if (md->sub != NULL) {
		XtUnmanageChild(md->sub);
		XtDestroyWidget(md->sub);
	}
	if (md->btn != NULL) {
		XtUnmanageChild(md->btn);
		XtDestroyWidget(md->btn);
	}
	free(md);

	node->user_data = NULL;
	return 0;
}

static void add_res2menu_main(Widget menu, lht_node_t *node, XtCallbackProc callback)
{
	menu_data_t *md = menu_data_alloc();

	stdarg_n = 0;
	stdarg(XmNtearOffModel, XmTEAR_OFF_ENABLED);
	md->sub = XmCreatePulldownMenu(menu, node->name, stdarg_args, stdarg_n);
	XtSetValues(md->sub, stdarg_args, stdarg_n);
	stdarg_n = 0;
	stdarg(XmNsubMenuId, md->sub);
	md->btn = XmCreateCascadeButton(menu, node->name, stdarg_args, stdarg_n);
	XtManageChild(md->btn);

	node->user_data = md;

	if (pcb_hid_cfg_has_submenus(node)) {
		lht_node_t *i;
		i = pcb_hid_cfg_menu_field(node, PCB_MF_SUBMENU, NULL);
		for(i = i->data.list.first; i != NULL; i = i->next)
			add_node_to_menu(md->sub, i, callback, 1);
	}
}

static void add_res2menu_named(Widget menu, lht_node_t *node, XtCallbackProc callback, int level)
{
	const char *v;
	lht_node_t *act, *kacc;
	menu_data_t *md;

	stdarg_n = 0;
	v = pcb_hid_cfg_menu_field_str(node, PCB_MF_FOREGROUND);
	if (v != NULL)
		stdarg_do_color(v, XmNforeground);

	v = pcb_hid_cfg_menu_field_str(node, PCB_MF_BACKGROUND);
	if (v != NULL)
		stdarg_do_color(v, XmNbackground);

	v = pcb_hid_cfg_menu_field_str(node, PCB_MF_FONT);
	if (v != NULL) {
		XFontStruct *fs = XLoadQueryFont(display, v);
		if (fs) {
			XmFontList fl = XmFontListCreate(fs, XmSTRING_DEFAULT_CHARSET);
			stdarg(XmNfontList, fl);
		}
	}

	v = pcb_hid_cfg_menu_field_str(node, PCB_MF_MNEMONIC);
	if (v != NULL)
		stdarg(XmNmnemonic, v);

	kacc = pcb_hid_cfg_menu_field(node, PCB_MF_ACCELERATOR, NULL);
	if (kacc != NULL) {
		char *acc_str = pcb_hid_cfg_keys_gen_accel(&lesstif_keymap, kacc, 1, NULL);

		if (acc_str != NULL) {
			XmString as = XmStringCreatePCB(acc_str);
			stdarg(XmNacceleratorText, as);
		}

#warning TODO: remove this call
		note_accelerator(node);
	}

	v = node->name;
	stdarg(XmNlabelString, XmStringCreatePCB(pcb_strdup(v)));

	md = menu_data_alloc();
	if (pcb_hid_cfg_has_submenus(node)) {
		int nn = stdarg_n;
		lht_node_t *i;
		const char *field_name;
		lht_node_t *submenu_node = pcb_hid_cfg_menu_field(node, PCB_MF_SUBMENU, &field_name);

		stdarg(XmNtearOffModel, XmTEAR_OFF_ENABLED);
		md->sub = XmCreatePulldownMenu(menu, pcb_strdup(v), stdarg_args + nn, stdarg_n - nn);
		node->user_data = md;
		stdarg_n = nn;
		stdarg(XmNsubMenuId, md->sub);
		md->btn = XmCreateCascadeButton(menu, XmStrCast("menubutton"), stdarg_args, stdarg_n);
		XtManageChild(md->btn);

		/* assume submenu is a list, pcb_hid_cfg_has_submenus() already checked that */
		for(i = submenu_node->data.list.first; i != NULL; i = i->next)
			add_node_to_menu(md->sub, i, callback, level+1);
	}
	else {
		/* doesn't have submenu */
		const char *checked = pcb_hid_cfg_menu_field_str(node, PCB_MF_CHECKED);
		const char *label = pcb_hid_cfg_menu_field_str(node, PCB_MF_SENSITIVE);
#if 0
/* Do not support radio for now: the gtk HID doesn't have it either */
		Resource *radio = resource_subres(node->v[i].subres, "radio");
		if (radio) {
			ToggleItem *ti = (ToggleItem *) malloc(sizeof(ToggleItem));
			ti->next = toggle_items;
			ti->group = radio->v[0].value;
			ti->item = radio->v[1].value;
			ti->callback = callback;
			ti->node = node->v[i].subres;
			toggle_items = ti;

			if (resource_value(node->v[i].subres, "set")) {
				stdarg(XmNset, True);
			}
			stdarg(XmNindicatorType, XmONE_OF_MANY);
			btn = XmCreateToggleButton(menu, "menubutton", args, n);
			ti->w = btn;
			XtAddCallback(btn, XmNvalueChangedCallback, (XtCallbackProc) radio_callback, (XtPointer) ti);
		}
		else
#endif
		act = pcb_hid_cfg_menu_field(node, PCB_MF_ACTION, NULL);
		if (checked) {
			if (strchr(checked, '='))
				stdarg(XmNindicatorType, XmONE_OF_MANY);
			else
				stdarg(XmNindicatorType, XmN_OF_MANY);
			md->btn = XmCreateToggleButton(menu, XmStrCast("menubutton"), stdarg_args, stdarg_n);
			if (act != NULL)
				XtAddCallback(md->btn, XmNvalueChangedCallback, callback, (XtPointer) act);
		}
		else if (label && strcmp(label, "false") == 0) {
			stdarg(XmNalignment, XmALIGNMENT_BEGINNING);
			md->btn = XmCreateLabel(menu, XmStrCast("menulabel"), stdarg_args, stdarg_n);
		}
		else {
			md->btn = XmCreatePushButton(menu, XmStrCast("menubutton"), stdarg_args, stdarg_n);
			XtAddCallback(md->btn, XmNactivateCallback, callback, (XtPointer) act);
		}

		v = pcb_hid_cfg_menu_field_str(node, PCB_MF_CHECKED);
		if (v != NULL)
			md->wflag_idx = note_widget_flag(md->btn, XmNset, v);

		v = pcb_hid_cfg_menu_field_str(node, PCB_MF_ACTIVE);
		if (v != NULL)
			note_widget_flag(md->btn, XmNsensitive, v);

		XtManageChild(md->btn);
		node->user_data = md;
	}
}

static void add_res2menu_text_special(Widget menu, lht_node_t *node, XtCallbackProc callback)
{
#warning TODO: make this a flag hash, also in the gtk hid
	Widget btn = NULL;
	stdarg_n = 0;
	if (*node->data.text.value == '@') {
		if (strcmp(node->data.text.value, "@layerview") == 0)
			insert_layerview_buttons(menu);
		if (strcmp(node->data.text.value, "@layerpick") == 0)
			insert_layerpick_buttons(menu);
		if (strcmp(node->data.text.value, "@routestyles") == 0)
			lesstif_insert_style_buttons(menu);
	}
	else if ((strcmp(node->data.text.value, "-") == 0) || (strcmp(node->data.text.value, "-"))) {
		btn = XmCreateSeparator(menu, XmStrCast("sep"), stdarg_args, stdarg_n);
		XtManageChild(btn);
	}
}

static void add_node_to_menu(Widget in_menu, lht_node_t *node, XtCallbackProc callback, int level)
{
	if (level == 0) {
		add_res2menu_main(in_menu, node, callback);
		return;
	}

	switch(node->type) {
		case LHT_HASH: add_res2menu_named(in_menu, node, callback, level); break;
		case LHT_TEXT: add_res2menu_text_special(in_menu, node, callback); break;
		default: /* ignore them */;
	}
}

extern char *lesstif_pcbmenu_path;
extern const char *lesstif_menu_default;


Widget lesstif_menu(Widget parent, const char *name, Arg * margs, int mn)
{
	Widget mb = XmCreateMenuBar(parent, XmStrCast(name), margs, mn);
	int screen;
	lht_node_t *mr;

	display = XtDisplay(mb);
	screen = DefaultScreen(display);
	cmap = DefaultColormap(display, screen);

	lesstif_cfg = pcb_hid_cfg_load("lesstif", 0, lesstif_menu_default);
	if (lesstif_cfg == NULL) {
		pcb_message(PCB_MSG_ERROR, "FATAL: can't load the lesstif menu res either from file or from hardwired default.");
		abort();
	}

	mr = pcb_hid_cfg_get_menu(lesstif_cfg, "/main_menu");
	if (mr != NULL) {
		if (mr->type == LHT_LIST) {
			lht_node_t *n;
			for(n = mr->data.list.first; n != NULL; n = n->next)
				add_node_to_menu(mb, n, (XtCallbackProc) callback, 0);
		}
		else
			pcb_hid_cfg_error(mr, "/main_menu should be a list");
	}


	hid_cfg_mouse_init(lesstif_cfg, &lesstif_mouse);

	return mb;
}

static int lesstif_create_menu_widget(void *ctx, const char *path, const char *name, int is_main, lht_node_t *parent, lht_node_t *menu_item)
{
	Widget w = (is_main) ? lesstif_menubar : ((menu_data_t *)parent->user_data)->sub;

	if (strncmp(path, "/popups", 7) == 0)
		return -1; /* there's no popup support in lesstif */

	add_node_to_menu(w, menu_item, (XtCallbackProc) callback, is_main ? 0 : 2);

	return 0;
}


void lesstif_create_menu(const char *menu_path, const char *action, const char *mnemonic, const char *accel, const char *tip, const char *cookie)
{
	pcb_hid_cfg_create_menu(lesstif_cfg, menu_path, action, mnemonic, accel, tip, cookie, lesstif_create_menu_widget, NULL);
}

void lesstif_remove_menu(const char *menu_path)
{
	pcb_hid_cfg_remove_menu(lesstif_cfg, menu_path, del_menu, NULL);
}

void lesstif_uninit_menu(void)
{
	XtDestroyWidget(lesstif_menubar);
}
