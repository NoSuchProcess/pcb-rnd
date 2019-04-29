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
#include "conf_hid.h"
#include "lesstif.h"
#include "paths.h"
#include "actions.h"
#include "stdarg.h"
#include "event.h"
#include "compat_misc.h"
#include "layer_vis.h"
#include <genht/hash.h>

#include "../src_plugins/lib_hid_common/menu_helper.h"

Widget lesstif_menubar;
pcb_hid_cfg_t *lesstif_cfg;
conf_hid_id_t lesstif_menuconf_id = -1;
htsp_t ltf_popups; /* popup_name -> Widget */

#ifndef R_OK
/* Common value for systems that don't define it.  */
#define R_OK 4
#endif

static Colormap cmap;

static void note_accelerator(const lht_node_t *node);
static int note_widget_flag(Widget w, char *type, const char *name);

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

void lesstif_update_widget_flags(const char *cookie)
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

#if 0
static void stdarg_do_color(char *value, char *which)
{
	XColor color;
	if (XParseColor(display, cmap, value, &color)) {
		if (XAllocColor(display, cmap, &color)) {
			stdarg(which, color.pixel);
		}
	}
}
#endif

static int need_xy = 0, have_xy = 0, block_xy = 0, action_x, action_y;

int lesstif_button_event(Widget w, XEvent * e)
{
	have_xy = 1;
	action_x = e->xbutton.x;
	action_y = e->xbutton.y;
	if (!need_xy)
		return 0;
	if (w != work_area)
		return 1;
	if (block_xy)
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

void lesstif_get_coords(const char *msg, pcb_coord_t *px, pcb_coord_t *py, int force)
{
	if ((force || !have_xy) && msg) {
		if (force) {
			have_xy = 0;
			block_xy = 1;
		}
		lesstif_get_xy(msg);
		block_xy = 0;
	}
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
		pcb_hid_cfg_keys_add_by_desc(&lesstif_keymap, knode, anode);
	else
		pcb_hid_cfg_error(node, "No action specified for key accel\n");
}

int lesstif_key_event(XKeyEvent * e)
{
	char buf[10];
	KeySym sym;
	int slen;
	int mods = 0;

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

	switch (sym) {
	/* Ignore these.  */
		case XK_Shift_L:    case XK_Shift_R:
		case XK_Control_L:  case XK_Control_R:
		case XK_Caps_Lock:  case XK_Shift_Lock:
		case XK_Meta_L:     case XK_Meta_R:
		case XK_Alt_L:      case XK_Alt_R:
		case XK_Super_L:    case XK_Super_R:
		case XK_Hyper_L:    case XK_Hyper_R:
		case XK_ISO_Level3_Shift:
			return 1;

		case XK_KP_Add: sym = '+'; break;
		case XK_KP_Subtract: sym = '-'; break;
		case XK_KP_Multiply: sym = '*'; break;
		case XK_KP_Divide: sym = '/'; break;
		case XK_KP_Enter: sym = XK_Return; break;
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
TODO("TODO#3: pass on raw and translated keys")
	slen = pcb_hid_cfg_keys_input(&lesstif_keymap, mods, sym, sym);
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
	pcb_hid_cfg_keys_action(&lesstif_keymap);

	return 1;
}

static void add_node_to_menu(Widget menu, lht_node_t *ins_after, lht_node_t *node, XtCallbackProc callback, int level);

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
			add_node_to_menu(md->sub, NULL, i, callback, 1);
	}
}

/* when ins_after is specified, calculate where the new node needs to be
   inserted and set the XmNpositionIndex accordingly */
static void set_ins_after(Widget menu, lht_node_t *ins_after)
{
	WidgetList ch;
	Cardinal n, nch;
	lht_node_t *nd;

	if (ins_after == NULL)
		return;

	XtVaGetValues(menu, XmNchildren, &ch, XmNnumChildren, &nch, NULL);
	assert(ins_after->parent->type == LHT_LIST);
	for(n = 0, nd = ins_after->parent->data.list.first; n < nch; n++,nd = nd->next) {
		short pos;
		
		XtVaGetValues(ch[n], XmNpositionIndex, &pos, NULL);
		if (nd == ins_after) {
			stdarg(XmNpositionIndex, pos);
			break;
		}
	}
}

static void lesstif_confchg_checkbox(conf_native_t *cfg, int arr_idx)
{
	lesstif_update_widget_flags(NULL);
}

static void add_res2menu_named(Widget menu, lht_node_t *ins_after, lht_node_t *node, XtCallbackProc callback, int level)
{
	const char *v;
	lht_node_t *act, *kacc;
	menu_data_t *md;

	stdarg_n = 0;
	v = pcb_hid_cfg_menu_field_str(node, PCB_MF_FOREGROUND);
	if (v != NULL)
		stdarg_do_color_str(v, XmNforeground);

	v = pcb_hid_cfg_menu_field_str(node, PCB_MF_BACKGROUND);
	if (v != NULL)
		stdarg_do_color_str(v, XmNbackground);

	v = pcb_hid_cfg_menu_field_str(node, PCB_MF_FONT);
	if (v != NULL) {
		XFontStruct *fs = XLoadQueryFont(display, v);
		if (fs) {
			XmFontList fl = XmFontListCreate(fs, XmSTRING_DEFAULT_CHARSET);
			stdarg(XmNfontList, fl);
		}
	}

	kacc = pcb_hid_cfg_menu_field(node, PCB_MF_ACCELERATOR, NULL);
	if (kacc != NULL) {
		char *acc_str = pcb_hid_cfg_keys_gen_accel(&lesstif_keymap, kacc, 1, NULL);

		if (acc_str != NULL) {
			XmString as = XmStringCreatePCB(acc_str);
			stdarg(XmNacceleratorText, as);
		}

TODO(": remove this call")
		note_accelerator(node);
	}

	v = node->name;
	stdarg(XmNlabelString, XmStringCreatePCB(pcb_strdup(v)));
	set_ins_after(menu, ins_after);

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
			add_node_to_menu(md->sub, NULL, i, callback, level+1);
	}
	else {
		/* doesn't have submenu */
		const char *checked = pcb_hid_cfg_menu_field_str(node, PCB_MF_CHECKED);
		const char *label = pcb_hid_cfg_menu_field_str(node, PCB_MF_SENSITIVE);

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
		if (v != NULL) {
			const char *uo;

			md->wflag_idx = note_widget_flag(md->btn, XmNset, v);

			/* set up the update-on callback */
			uo = pcb_hid_cfg_menu_field_str(node, PCB_MF_UPDATE_ON);
			if (uo == NULL)
				uo = pcb_hid_cfg_menu_field_str(node, PCB_MF_CHECKED);
			if (uo != NULL) {
				static conf_hid_callbacks_t cbs;
				static int cbs_inited = 0;
				conf_native_t *nat = conf_get_field(uo);

				if (nat != NULL) {
					if (!cbs_inited) {
						memset(&cbs, 0, sizeof(conf_hid_callbacks_t));
						cbs.val_change_post = lesstif_confchg_checkbox;
						cbs_inited = 1;
					}
					conf_hid_set_cb(nat, lesstif_menuconf_id, &cbs);
				}
				else {
					if (*uo != '\0')
						pcb_message(PCB_MSG_WARNING, "Checkbox menu item %s not updated on any conf change - try to use the update_on field\n", checked);
				}
			}
		}

		v = pcb_hid_cfg_menu_field_str(node, PCB_MF_ACTIVE);
		if (v != NULL)
			note_widget_flag(md->btn, XmNsensitive, v);

		XtManageChild(md->btn);
		node->user_data = md;
	}
}

static void add_res2menu_text_special(Widget menu, lht_node_t *node, XtCallbackProc callback)
{
TODO(": make this a flag hash, also in the gtk hid")
	Widget btn = NULL;
	stdarg_n = 0;
	if (*node->data.text.value == '@') {
		/* ignore anchors */
	}
	else if ((strcmp(node->data.text.value, "-") == 0) || (strcmp(node->data.text.value, "-"))) {
		btn = XmCreateSeparator(menu, XmStrCast("sep"), stdarg_args, stdarg_n);
		XtManageChild(btn);
	}
}

static void add_node_to_menu(Widget in_menu, lht_node_t *ins_after, lht_node_t *node, XtCallbackProc callback, int level)
{
	if (level == 0) {
		add_res2menu_main(in_menu, node, callback);
		return;
	}

	switch(node->type) {
		case LHT_HASH: add_res2menu_named(in_menu, ins_after, node, callback, level); break;
		case LHT_TEXT: add_res2menu_text_special(in_menu, node, callback); break;
		default: /* ignore them */;
	}
}

extern char *lesstif_pcbmenu_path;
extern const char *pcb_menu_default;
extern pcb_hid_t lesstif_hid;

Widget lesstif_menu(Widget parent, const char *name, Arg * margs, int mn)
{
	Widget mb = XmCreateMenuBar(parent, XmStrCast(name), margs, mn);
	int screen;
	lht_node_t *mr;

	display = XtDisplay(mb);
	screen = DefaultScreen(display);
	cmap = DefaultColormap(display, screen);

	lesstif_cfg = pcb_hid_cfg_load("lesstif", 0, pcb_menu_default);
	lesstif_hid.hid_cfg = lesstif_cfg;
	if (lesstif_cfg == NULL) {
		pcb_message(PCB_MSG_ERROR, "FATAL: can't load the lesstif menu res either from file or from hardwired default.");
		abort();
	}

	mr = pcb_hid_cfg_get_menu(lesstif_cfg, "/main_menu");
	if (mr != NULL) {
		if (mr->type == LHT_LIST) {
			lht_node_t *n;
			for(n = mr->data.list.first; n != NULL; n = n->next)
				add_node_to_menu(mb, NULL, n, (XtCallbackProc) callback, 0);
		}
		else
			pcb_hid_cfg_error(mr, "/main_menu should be a list");
	}

	htsp_init(&ltf_popups, strhash, strkeyeq);
	mr = pcb_hid_cfg_get_menu(lesstif_cfg, "/popups");
	if (mr != NULL) {
		if (mr->type == LHT_LIST) {
			lht_node_t *n;

			for(n = mr->data.list.first; n != NULL; n = n->next) {
				menu_data_t *md;
				Widget pmw, sh;
				lht_node_t *i;

				md = calloc(sizeof(menu_data_t), 1);
				md->sub = XtCreatePopupShell("popupshell", topLevelShellWidgetClass, parent, margs, mn);
/*				md->sub = XtCreatePopupShell("popupshell", xmMenuShellWidgetClass, parent, margs, mn); -> size 0 error */
				{
					Arg a[2];
					int an = 0;
/*					XtSetArg(a[0], XmNrowColumnType, XmMENU_POPUP); an++; -> inappropriate menu shell error */
					pmw = XmCreateRowColumn(md->sub, XmStrCast(n->name), a, an);
				}

				i = pcb_hid_cfg_menu_field(n, PCB_MF_SUBMENU, NULL);
				for(i = i->data.list.first; i != NULL; i = i->next)
					add_node_to_menu(pmw, NULL, i, callback, 1);

				XtManageChild(md->sub);
				XtManageChild(pmw);
				n->user_data = md;
				md->btn = pmw;
				htsp_set(&ltf_popups, n->name, pmw);
			}
		}
		else
			pcb_hid_cfg_error(mr, "/popups should be a list");
	}


	hid_cfg_mouse_init(lesstif_cfg, &lesstif_mouse);

	return mb;
}

int ltf_open_popup(const char *menupath)
{
	menu_data_t *md;
	lht_node_t *menu_node = pcb_hid_cfg_get_menu(lesstif_cfg, menupath);

	pcb_trace("ltf_open_popup: %s: %p\n", menupath, menu_node);

	if (menu_node == NULL)
		return -1;

	md = menu_node->user_data;
	XtPopup(md->sub, XtGrabExclusive);
	return 0;
}

static int lesstif_create_menu_widget(void *ctx, const char *path, const char *name, int is_main, lht_node_t *parent, lht_node_t *ins_after, lht_node_t *menu_item)
{
	Widget w = (is_main) ? lesstif_menubar : ((menu_data_t *)parent->user_data)->sub;

TODO("addin popup menu should be possible now TODO#59");
	if (strncmp(path, "/popups", 7) == 0)
		return -1; /* there's no popup support in lesstif */

	add_node_to_menu(w, ins_after, menu_item, (XtCallbackProc) callback, is_main ? 0 : 2);

	return 0;
}


void lesstif_create_menu(const char *menu_path, const pcb_menu_prop_t *props)
{
	pcb_hid_cfg_create_menu(lesstif_cfg, menu_path, props, lesstif_create_menu_widget, NULL);
}

int lesstif_remove_menu(const char *menu_path)
{
	return pcb_hid_cfg_remove_menu(lesstif_cfg, menu_path, del_menu, NULL);
}

int lesstif_remove_menu_node(lht_node_t *node)
{
	return pcb_hid_cfg_remove_menu_node(lesstif_cfg, node, del_menu, NULL);
}

pcb_hid_cfg_t *lesstif_get_menu_cfg(void)
{
	return lesstif_cfg;
}

static const char *lesstif_menu_cookie = "hid_lesstif_menu";
void lesstif_init_menu(void)
{
	if (lesstif_menuconf_id < 0)
		lesstif_menuconf_id = conf_hid_reg(lesstif_menu_cookie, NULL);
}

void lesstif_uninit_menu(void)
{
	conf_hid_unreg(lesstif_menu_cookie);
	XtDestroyWidget(lesstif_menubar);
	lesstif_menuconf_id = -1;
}
