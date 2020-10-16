/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <genvector/vti0.h>
#include <genvector/vtp0.h>

#include <librnd/core/hid.h>
#include <librnd/core/hid_cfg.h>
#include <librnd/core/hid_dad.h>

#include <librnd/core/actions.h>
#include "board.h"
#include "data.h"
#include "event.h"
#include "layer.h"
#include "layer_grp.h"
#include "layer_ui.h"
#include "layer_vis.h"
#include <librnd/core/hidlib_conf.h>

#include "layersel.h"

static const char *grpsep[] = { /* 8 pixel high transparent for spacing */
	"1 8 1 1",
	".	c None",
	".",".",".",".",".",".",".","."
};

static const char *closed_grp_layer_unsel[] = { /* layer unselected icon in a closed layer group */
	"10 8 2 1",
	".	c None",
	"x	c #000000",
	"..........",
	"xx......xx",
	"x........x",
	"..........",
	"..........",
	"..........",
	"x........x",
	"xx......xx"
};

static const char *closed_grp_layer_sel[] = { /* layer unselected icon in a closed layer group */
	"10 8 3 1",
	".	c None",
	"x	c #000000",
	"o	c #6EA5D7",
	"..........",
	"xx.xxxx.xx",
	"x.xoooox.x",
	".xoooooox.",
	".xoooooox.",
	".xoooooox.",
	"x.xoooox.x",
	"xx.xxxx.xx"
};

static const char *closed_grp_layer_nosel[] = { /* layer unselected icon in a closed layer group */
	"10 8 1 1",
	".	c None",
	"..........",
	"..........",
	"..........",
	"..........",
	"..........",
	"..........",
	"..........",
	".........."
};

static const char *all_open_xpm[] = {
"10 10 3 1",
" 	c None",
"@	c #6EA5D7",
"+	c #000000",
"   ++++   ",
"   +@@+   ",
"   +@@+   ",
"++++@@++++",
"+@@@@@@@@+",
"+@@@@@@@@+",
"++++@@++++",
"   +@@+   ",
"   +@@+   ",
"   ++++   ",
};

static const char *all_closed_xpm[] = {
"10 10 3 1",
" 	c None",
"@	c #6EA5D7",
"+	c #000000",
"          ",
"          ",
"          ",
"++++++++++",
"+@@@@@@@@+",
"+@@@@@@@@+",
"++++++++++",
"          ",
"          ",
"          ",
};

static const char *all_vis_xpm[] = {
"12 9 3 1",
" 	c None",
"@	c #6EA5D7",
"+	c #000000",
"            ",
"    ++++    ",
"  ++@@@@++  ",
" +@@@++@@@+ ",
"+@@@++++@@@+",
" +@@@++@@@+ ",
"  ++@@@@++  ",
"    ++++    ",
"            ",
};

static const char *all_invis_xpm[] = {
"12 9 3 1",
" 	c None",
"@	c #6EA5D7",
"+	c #000000",
" @+@        ",
"  @+@+++    ",
"  +@+@@@++  ",
" +@@@+@@@@+ ",
"+@@@+@+@@@@+",
" +@@@+@+@@+ ",
"  ++@@@@+@  ",
"    ++++@+@ ",
"         @+@",
};


typedef struct {
	char buf[32][20];
	const char *xpm[32];
} gen_xpm_t;

typedef struct layersel_ctx_s layersel_ctx_t;

typedef struct {
	int wvis_on_open, wvis_off_open, wvis_on_closed, wvis_off_closed, wlab;
	int wunsel_closed, wsel_closed;
	gen_xpm_t on_open, off_open, on_closed, off_closed;
	layersel_ctx_t *ls;
	pcb_layer_t *ly;
	const pcb_menu_layers_t *ml;
	unsigned grp_vis:1;
} ls_layer_t;

/* virtual group GIDs */
typedef enum {
	LGS_invlaid = -1,
	LGS_VIRTUAL = 0,
	LGS_UI = 1,
	LGS_max
} virt_grp_id_t;

typedef struct {
	int wopen, wclosed;
	layersel_ctx_t *ls;
	rnd_layergrp_id_t gid;
	virt_grp_id_t vid;
	unsigned int is_open:1;
} ls_group_t;

struct layersel_ctx_s {
	rnd_hid_dad_subdialog_t sub;
	int sub_inited;
	int lock_vis, lock_sel;
	int w_last_sel, w_last_sel_closed, w_last_unsel_closed;
	vtp0_t real_layer, menu_layer, ui_layer; /* -> ls_layer_t */
	vtp0_t group; /* -> ls_group_t */
};

static layersel_ctx_t layersel;

static ls_layer_t *lys_get(layersel_ctx_t *ls, vtp0_t *vt, size_t idx, int alloc)
{
	ls_layer_t **res = (ls_layer_t **)vtp0_get(vt, idx, alloc);
	if (res == NULL)
		return NULL;
	if ((*res == NULL) && alloc) {
		*res = calloc(sizeof(ls_layer_t), 1);
		(*res)->ls = ls;
	}
	return *res;
}

static ls_group_t *lgs_get_(layersel_ctx_t *ls, size_t gid, int alloc)
{
	ls_group_t **res = (ls_group_t **)vtp0_get(&ls->group, gid, alloc);
	if (res == NULL)
		return NULL;
	if ((*res == NULL) && alloc) {
		*res = calloc(sizeof(ls_group_t), 1);
		(*res)->ls = ls;
		(*res)->is_open = 1;
	}
	return *res;
}

static ls_group_t *lgs_get_real(layersel_ctx_t *ls, rnd_layergrp_id_t gid, int alloc)
{
	ls_group_t *lgs = lgs_get_(ls, gid+LGS_max, alloc);
	lgs->gid = gid;
	lgs->vid = -1;
	return lgs;
}

static void lgs_reset(layersel_ctx_t *ls)
{
	ls->group.used = 0;
}


static ls_group_t *lgs_get_virt(layersel_ctx_t *ls, virt_grp_id_t vid, int alloc)
{
	ls_group_t *lgs = lgs_get_(ls, vid, alloc);
	lgs->gid = -1;
	lgs->vid = vid;
	return lgs;
}

static void locked_layersel(layersel_ctx_t *ls, int wid, int wid_unsel_closed, int wid_sel_closed)
{
	if (ls->lock_sel > 0)
		return;

	ls->lock_sel = 1;
	if (ls->w_last_sel != 0) {
		rnd_gui->attr_dlg_widget_state(ls->sub.dlg_hid_ctx, ls->w_last_sel, 1);
		rnd_gui->attr_dlg_widget_hide(ls->sub.dlg_hid_ctx, ls->w_last_unsel_closed, 0);
		rnd_gui->attr_dlg_widget_hide(ls->sub.dlg_hid_ctx, ls->w_last_sel_closed, 1);
	}
	ls->w_last_sel = wid;
	ls->w_last_sel_closed = wid_sel_closed;
	ls->w_last_unsel_closed = wid_unsel_closed;
	if (ls->w_last_sel != 0) {
		rnd_gui->attr_dlg_widget_state(ls->sub.dlg_hid_ctx, ls->w_last_sel, 2);
		rnd_gui->attr_dlg_widget_hide(ls->sub.dlg_hid_ctx, ls->w_last_unsel_closed, 1);
		rnd_gui->attr_dlg_widget_hide(ls->sub.dlg_hid_ctx, ls->w_last_sel_closed, 0);
	}
	ls->lock_sel = 0;
}

static void locked_layervis_ev(layersel_ctx_t *ls)
{
	ls->lock_vis++;
	rnd_event(&PCB->hidlib, PCB_EVENT_LAYERVIS_CHANGED, NULL);
	ls->lock_vis--;
}

static void lys_update_vis(ls_layer_t *lys, rnd_bool_t vis)
{
	rnd_gui->attr_dlg_widget_hide(lys->ls->sub.dlg_hid_ctx, lys->wvis_on_open, !vis);
	rnd_gui->attr_dlg_widget_hide(lys->ls->sub.dlg_hid_ctx, lys->wvis_on_closed, !vis);
	rnd_gui->attr_dlg_widget_hide(lys->ls->sub.dlg_hid_ctx, lys->wvis_off_open, !!vis);
	rnd_gui->attr_dlg_widget_hide(lys->ls->sub.dlg_hid_ctx, lys->wvis_off_closed, !!vis);
}

static void layer_select(ls_layer_t *lys)
{
	rnd_bool *vis = NULL;

	if (lys->ly != NULL) {
		if (lys->grp_vis) {
			rnd_layer_id_t lid = lys->ly - PCB->Data->Layer;
			pcb_layervis_change_group_vis(&PCB->hidlib, lid, 1, 1);
		}
		else {
			vis = &lys->ly->meta.real.vis;
			*vis = 1;
		}
		PCB->RatDraw = 0;
	}
	else if (lys->ml != NULL) {
		vis = (rnd_bool *)((char *)PCB + lys->ml->vis_offs);
		*vis = 1;
		rnd_actionva(&PCB->hidlib, "SelectLayer", lys->ml->select_name, NULL);
	}
	else
		return;

	rnd_hid_redraw(PCB);

	if (vis != NULL) {
		lys_update_vis(lys, *vis);
		locked_layervis_ev(lys->ls);
	}
	locked_layersel(lys->ls, lys->wlab, lys->wunsel_closed, lys->wsel_closed);
}

static void layer_sel_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	layer_select(attr->user_data);
}

static void group_sync_core(pcb_board_t *pcb, ls_group_t *lgs, int update_from_core)
{
	pcb_layergrp_t *g;

	if (lgs->gid < 0)
		return;
	g = pcb_get_layergrp(pcb, lgs->gid);
	if (g == NULL)
		return;

	if (update_from_core)
		lgs->is_open = !!g->open;
	else
		g->open = !!lgs->is_open;
}

static void group_open_close_update(ls_group_t *lgs)
{
	group_sync_core(PCB, lgs, 0);

	rnd_gui->attr_dlg_widget_hide(lgs->ls->sub.dlg_hid_ctx, lgs->wopen, !lgs->is_open);
	rnd_gui->attr_dlg_widget_hide(lgs->ls->sub.dlg_hid_ctx, lgs->wclosed, !!lgs->is_open);
}

static void all_open_close(int is_open)
{
	int n;
	for(n = 0; n < layersel.group.used; n++) {
		ls_group_t *lgs = layersel.group.array[n];
		if ((lgs != NULL) && (lgs->is_open != is_open)) {
			lgs->is_open = is_open;
			group_open_close_update(lgs);
		}
		
	}
}

static void all_open_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	all_open_close(1);
}

static void all_close_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	all_open_close(0);
}

static void all_vis_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_actionva(rnd_gui->get_dad_hidlib(hid_ctx), "ToggleView", "all", "vis", "set", NULL);
}

static void all_invis_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	rnd_actionva(rnd_gui->get_dad_hidlib(hid_ctx), "ToggleView", "all", "vis", "clear", NULL);
}

/* Select the first visible layer (physically) below the one turned off or
   reenable the original if all are off; this how we ensure the current layer
   is visible and avoid drawing on inivisible layers */
static void ensure_visible_current(pcb_board_t *pcb, layersel_ctx_t *ls)
{
	pcb_layer_t *ly;
	rnd_layer_id_t lid;
	rnd_layergrp_id_t gid;
	pcb_layer_t *l;
	ls_layer_t *lys;
	int repeat = 0;

	ly = PCB_CURRLAYER(pcb);
	if ((ly == NULL) || (ly->meta.real.vis))
		return;

	/* currently selected layer lost visible state - choose another */

	/* At the moment the layer selector displays only board layers which are always real */
	assert(!PCB_CURRLAYER(pcb)->is_bound);

	/* look for the next one to enable, group-wise */
	for(gid = PCB_CURRLAYER(pcb)->meta.real.grp + 1; gid != PCB_CURRLAYER(pcb)->meta.real.grp; gid++) {
		pcb_layergrp_t *g;
		if (gid >= pcb_max_group(pcb)) {
			gid = 0;
			repeat++;
			if (repeat > 1)
				break; /* failed to find one */
		}
		g = &pcb->LayerGroups.grp[gid];
		if (g->len < 1)
			continue;
		l = pcb->Data->Layer + g->lid[0];
		if (l->meta.real.vis)
			goto change_selection;
	}

	/* fallback: all off; turn the current one back on */
	l = PCB_CURRLAYER(pcb);

	change_selection:;
	lid = pcb_layer_id(pcb->Data, l);
	pcb_layervis_change_group_vis(&pcb->hidlib, lid, 1, 1);

	lys = lys_get(ls, &ls->real_layer, lid, 0);
	if (lys != 0)
		locked_layersel(lys->ls, lys->wlab, lys->wunsel_closed, lys->wsel_closed);
	else
		locked_layersel(lys->ls, 0, 0, 0);
}


static void layer_vis_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	ls_layer_t *lys = attr->user_data;
	rnd_bool *vis;

	if (lys->ly != NULL)
		vis = &lys->ly->meta.real.vis;
	else if (lys->ml != NULL)
		vis = (rnd_bool *)((char *)PCB + lys->ml->vis_offs);
	else
		return;

	if (lys->grp_vis) {
		rnd_layer_id_t lid = lys->ly - PCB->Data->Layer;
		pcb_layervis_change_group_vis(&PCB->hidlib, lid, !*vis, 1);
	}
	else {
		*vis = !(*vis);
		lys_update_vis(lys, *vis);
		locked_layervis_ev(lys->ls);
	}

	ensure_visible_current(PCB, lys->ls);

	rnd_hid_redraw(PCB);
}

static void layer_right_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	ls_layer_t *lys = attr->user_data;

	/* enable the popup only when there is a real layer behind the button;
	   example: rats is selectable, drawable, but there's no real layer */
	if (lys->ly == NULL)
		return;

	layer_select(lys);
	rnd_actionva(&PCB->hidlib, "Popup", "layer", NULL);
}

extern rnd_layergrp_id_t pcb_actd_EditGroup_gid;
static void group_right_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	ls_group_t *grp = attr->user_data;
	if (grp->gid < 0)
		return;
	pcb_actd_EditGroup_gid = grp->gid;
	rnd_actionva(&PCB->hidlib, "Popup", "group", NULL);
}


static void group_open_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	ls_group_t *lgs = attr->user_data;
	lgs->is_open = !lgs->is_open;

	group_open_close_update(lgs);
}

/* draw a visibility box: filled or partially filled with layer color */
static void layer_vis_box(gen_xpm_t *dst, int filled, const rnd_color_t *color, int brd, int hatch, int width, int height, int slant)
{
	int max_height = height;
	char *p;
	unsigned int w, line = 0, n;

	rnd_snprintf(dst->buf[line++], 20, "%d %d 4 1", width, height);
	strcpy(dst->buf[line++], ".	c None");
	strcpy(dst->buf[line++], "u	c None");
	strcpy(dst->buf[line++], "b	c #000000");
	rnd_snprintf(dst->buf[line++], 20, "c	c #%02X%02X%02X", color->r, color->g, color->b);

	while (height--) {
		w = width;
		p = dst->buf[line++];
		while (w--) {
			if ((height < brd) || (height >= max_height-brd) || (w < brd) || (w >= width-brd))
				*p = 'b'; /* frame */
			else if ((hatch) && (((w - height) % 4) == 0))
				*p = '.';
			else if ((width-w+slant < height) || (filled))
				*p = 'c'; /* layer color fill (full or up-left triangle) */
			else
				*p = 'u'; /* the unfilled part when triangle should be transparent */
			p++;
		}
		*p = '\0';
	}

	for(n=0; n < line; n++)
		dst->xpm[n] = dst->buf[n];
}

static void layersel_begin_grp_open(layersel_ctx_t *ls, const char *name, ls_group_t *lsg)
{
	RND_DAD_BEGIN_HBOX(ls->sub.dlg);
		lsg->wopen = RND_DAD_CURRENT(ls->sub.dlg);

		/* vertical group name */
		RND_DAD_LABEL(ls->sub.dlg, name);
			RND_DAD_COMPFLAG(ls->sub.dlg, RND_HATF_TIGHT | RND_HATF_TEXT_VERTICAL | RND_HATF_TEXT_TRUNCATED);
			RND_DAD_SET_ATTR_FIELD(ls->sub.dlg, user_data, lsg);
			RND_DAD_RIGHT_CB(ls->sub.dlg, group_right_cb);
			RND_DAD_CHANGE_CB(ls->sub.dlg, group_open_cb);
			RND_DAD_HELP(ls->sub.dlg, name);

		/* vert sep */
		RND_DAD_BEGIN_HBOX(ls->sub.dlg);
			RND_DAD_COMPFLAG(ls->sub.dlg, RND_HATF_TIGHT | RND_HATF_FRAME);
		RND_DAD_END(ls->sub.dlg);

		/* layer list box */
		RND_DAD_BEGIN_VBOX(ls->sub.dlg);
			RND_DAD_COMPFLAG(ls->sub.dlg, RND_HATF_TIGHT);
}

static void layersel_end_grp_open(layersel_ctx_t *ls)
{
		RND_DAD_END(ls->sub.dlg);
	RND_DAD_END(ls->sub.dlg);
}

static void layersel_begin_grp_closed(layersel_ctx_t *ls, const char *name, ls_group_t *lsg)
{
	RND_DAD_BEGIN_HBOX(ls->sub.dlg);
		lsg->wclosed = RND_DAD_CURRENT(ls->sub.dlg);
		RND_DAD_LABEL(ls->sub.dlg, name);
			RND_DAD_SET_ATTR_FIELD(ls->sub.dlg, user_data, lsg);
			RND_DAD_RIGHT_CB(ls->sub.dlg, group_right_cb);
			RND_DAD_CHANGE_CB(ls->sub.dlg, group_open_cb);

		/* vert sep */
		RND_DAD_BEGIN_HBOX(ls->sub.dlg);
			RND_DAD_COMPFLAG(ls->sub.dlg, RND_HATF_TIGHT | RND_HATF_FRAME);
		RND_DAD_END(ls->sub.dlg);

}

static void layersel_end_grp_closed(layersel_ctx_t *ls)
{
	RND_DAD_END(ls->sub.dlg);
}


static void layersel_create_layer_open(layersel_ctx_t *ls, ls_layer_t *lys, const char *name, const rnd_color_t *color, int brd, int hatch, int selectable)
{
	layer_vis_box(&lys->on_open, 1, color, brd, hatch, 16, 16, 5);
	layer_vis_box(&lys->off_open, 0, color, brd, hatch, 16, 16, 5);

	RND_DAD_BEGIN_HBOX(ls->sub.dlg);
		RND_DAD_PICTURE(ls->sub.dlg, lys->on_open.xpm);
			lys->wvis_on_open = RND_DAD_CURRENT(ls->sub.dlg);
			RND_DAD_SET_ATTR_FIELD(ls->sub.dlg, user_data, lys);
			RND_DAD_CHANGE_CB(ls->sub.dlg, layer_vis_cb);
		RND_DAD_PICTURE(ls->sub.dlg, lys->off_open.xpm);
			lys->wvis_off_open = RND_DAD_CURRENT(ls->sub.dlg);
			RND_DAD_SET_ATTR_FIELD(ls->sub.dlg, user_data, lys);
			RND_DAD_CHANGE_CB(ls->sub.dlg, layer_vis_cb);
		RND_DAD_LABEL(ls->sub.dlg, name);
			lys->wlab = RND_DAD_CURRENT(ls->sub.dlg);
			RND_DAD_SET_ATTR_FIELD(ls->sub.dlg, user_data, lys);
			if (selectable) {
				RND_DAD_CHANGE_CB(ls->sub.dlg, layer_sel_cb);
				RND_DAD_RIGHT_CB(ls->sub.dlg, layer_right_cb);
			}
	RND_DAD_END(ls->sub.dlg);
}

static void layersel_create_layer_closed(layersel_ctx_t *ls, ls_layer_t *lys, const char *name, const rnd_color_t *color, int brd, int hatch, int selected, int selectable)
{
	layer_vis_box(&lys->on_closed, 1, color, brd, hatch, 10, 10, 0);
	layer_vis_box(&lys->off_closed, 0, color, brd, hatch, 10, 10, 0);

	RND_DAD_BEGIN_VBOX(ls->sub.dlg);
		RND_DAD_COMPFLAG(ls->sub.dlg, RND_HATF_TIGHT);
		RND_DAD_PICTURE(ls->sub.dlg, lys->on_closed.xpm);
			lys->wvis_on_closed = RND_DAD_CURRENT(ls->sub.dlg);
			RND_DAD_SET_ATTR_FIELD(ls->sub.dlg, user_data, lys);
			RND_DAD_CHANGE_CB(ls->sub.dlg, layer_vis_cb);
			RND_DAD_HELP(ls->sub.dlg, name);
		RND_DAD_PICTURE(ls->sub.dlg, lys->off_closed.xpm);
			lys->wvis_off_closed = RND_DAD_CURRENT(ls->sub.dlg);
			RND_DAD_SET_ATTR_FIELD(ls->sub.dlg, user_data, lys);
			RND_DAD_CHANGE_CB(ls->sub.dlg, layer_vis_cb);
			RND_DAD_HELP(ls->sub.dlg, name);
		if (selectable) {
			RND_DAD_PICTURE(ls->sub.dlg, closed_grp_layer_unsel);
				if (selected)
					RND_DAD_COMPFLAG(ls->sub.dlg, RND_HATF_HIDE);
				lys->wunsel_closed = RND_DAD_CURRENT(ls->sub.dlg);
				RND_DAD_SET_ATTR_FIELD(ls->sub.dlg, user_data, lys);
				RND_DAD_CHANGE_CB(ls->sub.dlg, layer_sel_cb);
				RND_DAD_HELP(ls->sub.dlg, name);
			RND_DAD_PICTURE(ls->sub.dlg, closed_grp_layer_sel);
				if (!selected)
					RND_DAD_COMPFLAG(ls->sub.dlg, RND_HATF_HIDE);
				lys->wsel_closed = RND_DAD_CURRENT(ls->sub.dlg);
				RND_DAD_SET_ATTR_FIELD(ls->sub.dlg, user_data, lys);
				RND_DAD_CHANGE_CB(ls->sub.dlg, layer_sel_cb);
				RND_DAD_HELP(ls->sub.dlg, name);
		}
		else {
			RND_DAD_PICTURE(ls->sub.dlg, closed_grp_layer_nosel);
				lys->wunsel_closed = lys->wsel_closed = RND_DAD_CURRENT(ls->sub.dlg);
		}
	RND_DAD_END(ls->sub.dlg);
}

static void layersel_create_grp(layersel_ctx_t *ls, pcb_board_t *pcb, pcb_layergrp_t *g, ls_group_t *lgs, int is_open)
{
	rnd_cardinal_t n;
	const char *gname = g->name == NULL ? "" : g->name;

	if (is_open)
		layersel_begin_grp_open(ls, gname, lgs);
	else
		layersel_begin_grp_closed(ls, gname, lgs);
	for(n = 0; n < g->len; n++) {
		pcb_layer_t *ly = pcb_get_layer(pcb->Data, g->lid[n]);
		assert(ly != NULL);
		if (ly != NULL) {
			int brd = (((ly != NULL) && (ly->comb & PCB_LYC_SUB)) ? 2 : 1);
			int hatch = (((ly != NULL) && (ly->comb & PCB_LYC_AUTO)) ? 1 : 0);
			const rnd_color_t *clr = &ly->meta.real.color;
			ls_layer_t *lys = lys_get(ls, &ls->real_layer, g->lid[n], 1);

			lys->ly = ly;
			lys->grp_vis = 1;
			if (is_open)
				layersel_create_layer_open(ls, lys, ly->name, clr, brd, hatch, 1);
			else
				layersel_create_layer_closed(ls, lys, ly->name, clr, brd, hatch, (ly ==PCB_CURRLAYER(PCB)), 1);
		}
	}
	if (is_open)
		layersel_end_grp_open(ls);
	else
		layersel_end_grp_closed(ls);
}

static void layersel_add_grpsep(layersel_ctx_t *ls)
{
	RND_DAD_BEGIN_HBOX(ls->sub.dlg);
		RND_DAD_COMPFLAG(ls->sub.dlg, RND_HATF_EXPFILL);
		RND_DAD_PICTURE(ls->sub.dlg, grpsep);
	RND_DAD_END(ls->sub.dlg);

}

/* Editable layer groups that are part of the stack */
static void layersel_create_stack(layersel_ctx_t *ls, pcb_board_t *pcb)
{
	rnd_layergrp_id_t gid;
	pcb_layergrp_t *g;
	rnd_cardinal_t created = 0;

	for(gid = 0, g = pcb->LayerGroups.grp; gid < pcb->LayerGroups.len; gid++,g++) {
		ls_group_t *lgs;

		if (!(PCB_LAYER_IN_STACK(g->ltype)) || (g->ltype & PCB_LYT_SUBSTRATE))
			continue;
		if (created > 0)
			layersel_add_grpsep(ls);
		lgs = lgs_get_real(ls, gid, 1);
		layersel_create_grp(ls, pcb, g, lgs, 1);
		layersel_create_grp(ls, pcb, g, lgs, 0);
		created++;
	}
}

/* Editable layer groups that are not part of the stack */
static void layersel_create_global(layersel_ctx_t *ls, pcb_board_t *pcb)
{
	rnd_layergrp_id_t gid;
	pcb_layergrp_t *g;
	rnd_cardinal_t created = 0;

	for(gid = 0, g = pcb->LayerGroups.grp; gid < pcb->LayerGroups.len; gid++,g++) {
		ls_group_t *lgs;
		
		if (PCB_LAYER_IN_STACK(g->ltype))
			continue;
		if (created > 0)
			layersel_add_grpsep(ls);
		lgs = lgs_get_real(ls, gid, 1);
		layersel_create_grp(ls, pcb, g, lgs, 1);
		layersel_create_grp(ls, pcb, g, lgs, 0);
		created++;
	}
}

/* hardwired/virtual: typically Non-editable layers (no group) */
static void layersel_create_virtual(layersel_ctx_t *ls, pcb_board_t *pcb)
{
	const pcb_menu_layers_t *ml;
	int n;
	ls_group_t *lgs = lgs_get_virt(ls, LGS_VIRTUAL, 1);

	layersel_begin_grp_open(ls, "Virtual", lgs);
	for(n = 0, ml = pcb_menu_layers; ml->name != NULL; n++,ml++) {
		ls_layer_t *lys = lys_get(ls, &ls->menu_layer, n, 1);
		lys->ml = ml;
		lys->grp_vis = 0;
		layersel_create_layer_open(ls, lys, ml->name, ml->force_color, 1, 0, (ml->sel_offs != 0));
	}
	layersel_end_grp_open(ls);

	layersel_begin_grp_closed(ls, "Virtual", lgs);
	for(n = 0, ml = pcb_menu_layers; ml->name != NULL; n++,ml++) {
		ls_layer_t *lys = lys_get(ls, &ls->menu_layer, n, 0);
		layersel_create_layer_closed(ls, lys, ml->name, ml->force_color, 1, 0, 0, (ml->sel_offs != 0));
	}
	layersel_end_grp_closed(ls);
}

/* user-interface layers (no group) */
static void layersel_create_ui(layersel_ctx_t *ls, pcb_board_t *pcb)
{
	ls_group_t *lgs = lgs_get_virt(ls, LGS_UI, 1);
	int n;

	layersel_begin_grp_open(ls, "UI", lgs);
	if (vtp0_len(&pcb_uilayers) == 0)
		RND_DAD_LABEL(ls->sub.dlg, "(no layers)");
	for(n = 0; n < vtp0_len(&pcb_uilayers); n++) {
		pcb_layer_t *ly = pcb_uilayers.array[n];
		if ((ly != NULL) && (ly->name != NULL)) {
			ls_layer_t *lys = lys_get(ls, &ls->ui_layer, n, 1);
			lys->ly = ly;
			lys->grp_vis = 0;
			layersel_create_layer_open(ls, lys, ly->name, &ly->meta.real.color, 1, 0, 0);
		}
	}
	layersel_end_grp_open(ls);

	layersel_begin_grp_closed(ls, "UI", lgs);
	for(n = 0; n < vtp0_len(&pcb_uilayers); n++) {
		pcb_layer_t *ly = pcb_uilayers.array[n];
		if ((ly != NULL) && (ly->name != NULL)) {
			ls_layer_t *lys = lys_get(ls, &ls->ui_layer, n, 0);
			layersel_create_layer_closed(ls, lys, ly->name, &ly->meta.real.color, 1, 0, 0, 0);
		}
	}
	layersel_end_grp_closed(ls);
}

static void hsep(layersel_ctx_t *ls)
{
	RND_DAD_BEGIN_VBOX(ls->sub.dlg);
		RND_DAD_COMPFLAG(ls->sub.dlg, RND_HATF_TIGHT | RND_HATF_FRAME);
	RND_DAD_END(ls->sub.dlg);
}

static void layersel_docked_create(layersel_ctx_t *ls, pcb_board_t *pcb)
{
	lgs_reset(ls);
	RND_DAD_BEGIN_VBOX(ls->sub.dlg);
		RND_DAD_COMPFLAG(ls->sub.dlg, RND_HATF_EXPFILL | RND_HATF_TIGHT | RND_HATF_SCROLL);
		layersel_create_stack(&layersel, pcb);
		hsep(&layersel);
		layersel_create_global(&layersel, pcb);
		hsep(&layersel);
		layersel_create_virtual(&layersel, pcb);
		layersel_add_grpsep(ls);
		layersel_create_ui(&layersel, pcb);
		layersel_add_grpsep(ls);
		RND_DAD_BEGIN_HBOX(ls->sub.dlg);
			RND_DAD_PICBUTTON(ls->sub.dlg, all_open_xpm);
				RND_DAD_HELP(ls->sub.dlg, "expand/open all layer groups\nso that layer names are\ndisplayed, one layer per row");
				RND_DAD_CHANGE_CB(ls->sub.dlg, all_open_cb);
			RND_DAD_PICBUTTON(ls->sub.dlg, all_closed_xpm);
				RND_DAD_HELP(ls->sub.dlg, "collapse/close all layer groups\nso that layer names are\nnot displayed,\neach row is a layer group");
				RND_DAD_CHANGE_CB(ls->sub.dlg, all_close_cb);
			RND_DAD_PICBUTTON(ls->sub.dlg, all_vis_xpm);
				RND_DAD_HELP(ls->sub.dlg, "all layers visible");
				RND_DAD_CHANGE_CB(ls->sub.dlg, all_vis_cb);
			RND_DAD_PICBUTTON(ls->sub.dlg, all_invis_xpm);
				RND_DAD_HELP(ls->sub.dlg, "all layers invisible\nexcept for the current layer group");
				RND_DAD_CHANGE_CB(ls->sub.dlg, all_invis_cb);
			RND_DAD_BEGIN_HBOX(ls->sub.dlg);
				RND_DAD_COMPFLAG(ls->sub.dlg, RND_HATF_EXPFILL);
			RND_DAD_END(ls->sub.dlg);
		RND_DAD_END(ls->sub.dlg);
	RND_DAD_END(ls->sub.dlg);
	RND_DAD_DEFSIZE(ls->sub.dlg, 180, 200);
	RND_DAD_MINSIZE(ls->sub.dlg, 100, 100);
	ls->w_last_sel = 0;
}

static void layersel_update_vis(layersel_ctx_t *ls, pcb_board_t *pcb)
{
	pcb_layer_t *ly = pcb->Data->Layer;
	ls_layer_t **lys = (ls_layer_t **)ls->real_layer.array;
	ls_group_t **lgs = (ls_group_t **)ls->group.array;
	const pcb_menu_layers_t *ml;
	rnd_cardinal_t n;

	if (lys == NULL)
		return;

	for(n = 0; n < pcb->Data->LayerN; n++,ly++,lys++) {
		if (*lys == NULL)
			continue;
		lys_update_vis(*lys, ly->meta.real.vis);
	}


	lys = (ls_layer_t **)ls->menu_layer.array;
	for(ml = pcb_menu_layers; ml->name != NULL; ml++,lys++) {
		rnd_bool *b;
		if (*lys == NULL)
			continue;
		b = (rnd_bool *)((char *)pcb + ml->vis_offs);
		lys_update_vis(*lys, *b);
	}

	lys = (ls_layer_t **)ls->ui_layer.array;
	for(n = 0; n < vtp0_len(&pcb_uilayers); n++,lys++) {
		pcb_layer_t *ly = pcb_uilayers.array[n];
		if (ly != NULL)
			lys_update_vis(*lys, ly->meta.real.vis);
	}

	/* update group open/close hides */
	for(n = 0; n < vtp0_len(&ls->group); n++,lgs++) {
		if (*lgs == NULL)
			continue;

		group_sync_core(pcb, *lgs, 1);
		rnd_gui->attr_dlg_widget_hide(ls->sub.dlg_hid_ctx, (*lgs)->wopen, !(*lgs)->is_open);
		rnd_gui->attr_dlg_widget_hide(ls->sub.dlg_hid_ctx, (*lgs)->wclosed, (*lgs)->is_open);
	}

	{ /* ifPCB_CURRLAYER(PCB) is not selected, select it */
		ls_layer_t *lys = lys_get(ls, &ls->real_layer, pcb_layer_id(pcb->Data,PCB_CURRLAYER(pcb)), 0);
		if ((lys != NULL) && (lys->wlab != ls->w_last_sel))
			locked_layersel(ls, lys->wlab, lys->wunsel_closed, lys->wsel_closed);
	}

	ensure_visible_current(pcb, ls);
}

static void layersel_build(void)
{
	layersel_docked_create(&layersel, PCB);
	if (rnd_hid_dock_enter(&layersel.sub, RND_HID_DOCK_LEFT, "layersel") == 0) {
		layersel.sub_inited = 1;
		layersel_update_vis(&layersel, PCB);
	}
}

void pcb_layersel_gui_init_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	if ((RND_HAVE_GUI_ATTR_DLG) && (rnd_gui->get_menu_cfg != NULL))
		layersel_build();
}

void pcb_layersel_vis_chg_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	pcb_board_t *pcb = (pcb_board_t *)hidlib;
	if ((!layersel.sub_inited) || (layersel.lock_vis > 0))
		return;
	layersel_update_vis(&layersel, pcb);
}

void pcb_layersel_stack_chg_ev(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	if ((RND_HAVE_GUI_ATTR_DLG) && (rnd_gui->get_menu_cfg != NULL) && (layersel.sub_inited)) {
		rnd_hid_dock_leave(&layersel.sub);
		layersel.sub_inited = 0;
		layersel_build();
	}
}
