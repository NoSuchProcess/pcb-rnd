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

#include "hid.h"
#include "hid_cfg.h"
#include "hid_dad.h"

#include "actions.h"
#include "board.h"
#include "data.h"
#include "event.h"
#include "layer.h"
#include "layer_grp.h"
#include "layer_ui.h"
#include "layer_vis.h"
#include "hidlib_conf.h"

#include "layersel.h"

static const char *grpsep[] = { /* 8 pixel high transparent for spacing */
	"1 8 1 1",
	".	c None",
	".",".",".",".",".",".",".","."
};

typedef struct {
	char buf[32][20];
	const char *xpm[32];
} gen_xpm_t;

typedef struct layersel_ctx_s layersel_ctx_t;

typedef struct {
	int wvis_on, wvis_off, wlab;
	gen_xpm_t on, off;
	layersel_ctx_t *ls;
	pcb_layer_t *ly;
	const pcb_menu_layers_t *ml;
	unsigned grp_vis:1;
} ls_layer_t;

struct layersel_ctx_s {
	pcb_hid_dad_subdialog_t sub;
	int sub_inited;
	int lock_vis, lock_sel;
	int w_last_sel;
	vtp0_t real_layer, menu_layer, ui_layer;
};

static layersel_ctx_t layersel;

static ls_layer_t *lys_get(layersel_ctx_t *ls, vtp0_t *vt, size_t idx, int alloc)
{
	ls_layer_t **res = vtp0_get(vt, idx, alloc);
	if (res == NULL)
		return NULL;
	if ((*res == NULL) && alloc) {
		*res = calloc(sizeof(ls_layer_t), 1);
		(*res)->ls = ls;
	}
	return *res;
}

static void locked_layersel(layersel_ctx_t *ls, int wid)
{
	if (ls->lock_sel > 0)
		return;

	ls->lock_sel = 1;
	if (ls->w_last_sel != 0)
		pcb_gui->attr_dlg_widget_state(ls->sub.dlg_hid_ctx, ls->w_last_sel, 1);
	ls->w_last_sel = wid;
	if (ls->w_last_sel != 0)
		pcb_gui->attr_dlg_widget_state(ls->sub.dlg_hid_ctx, ls->w_last_sel, 2);
	ls->lock_sel = 0;
}

static void locked_layervis_ev(layersel_ctx_t *ls)
{
	ls->lock_vis++;
	pcb_event(&PCB->hidlib, PCB_EVENT_LAYERVIS_CHANGED, NULL);
	ls->lock_vis--;
}

static void layer_sel_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_bool *vis = NULL;
	ls_layer_t *lys = attr->user_data;

	if (lys->ly != NULL) {
		if (lys->grp_vis) {
			pcb_layer_id_t lid = lys->ly - PCB->Data->Layer;
			pcb_layervis_change_group_vis(lid, 1, 1);
		}
		else {
			vis = &lys->ly->meta.real.vis;
			*vis = 1;
		}
	}
	else if (lys->ml != NULL) {
		vis = (pcb_bool *)((char *)PCB + lys->ml->vis_offs);
		*vis = 1;
		pcb_actionl("SelectLayer", lys->ml->select_name, NULL);
	}
	else
		return;

	pcb_hid_redraw(PCB);

	if (vis != NULL) {
		pcb_gui->attr_dlg_widget_hide(lys->ls->sub.dlg_hid_ctx, lys->wvis_on, !(*vis));
		pcb_gui->attr_dlg_widget_hide(lys->ls->sub.dlg_hid_ctx, lys->wvis_off, !!(*vis));
		locked_layervis_ev(lys->ls);
	}
	locked_layersel(lys->ls, lys->wlab);
}

/* Select the first visible layer (physically) below the one turned off or
   reenable the original if all are off; this how we ensure the CURRENT layer
   is visible and avoid drawing on inivisible layers */
static void ensure_visible_current(layersel_ctx_t *ls)
{
	pcb_layer_t *ly;
	pcb_layer_id_t lid;
	pcb_layergrp_id_t gid;
	pcb_layer_t *l;
	ls_layer_t *lys;

	ly = LAYER_ON_STACK(0);
	if ((ly == NULL) || (ly->meta.real.vis))
		return

	/* currently selected layer lost visible state - choose another */

	/* At the moment the layer selector displays only board layers which are always real */
	assert(!CURRENT->is_bound);

	/* look for the next one to enable, group-vise */
	for(gid = CURRENT->meta.real.grp + 1; gid != CURRENT->meta.real.grp; gid++) {
		pcb_layergrp_t *g;
		if (gid >= pcb_max_group(PCB))
			gid = 0;
		g = &PCB->LayerGroups.grp[gid];
		if (g->len < 1)
			continue;
		l = PCB->Data->Layer + g->lid[0];
		if (l->meta.real.vis)
			goto change_selection;
	}

	/* fallback: all off; turn the current one back on */
	l = CURRENT;

	change_selection:;
	lid = pcb_layer_id(PCB->Data, l);
	pcb_layervis_change_group_vis(lid, 1, 1);

	lys = lys_get(ls, &ls->real_layer, lid, 0);
	if (lys != 0)
		locked_layersel(lys->ls, lys->wlab);
	else
		locked_layersel(lys->ls, 0);
}


static void layer_vis_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	pcb_layer_t *ly;
	ls_layer_t *lys = attr->user_data;
	pcb_bool *vis;

	if (lys->ly != NULL)
		vis = &lys->ly->meta.real.vis;
	else if (lys->ml != NULL)
		vis = (pcb_bool *)((char *)PCB + lys->ml->vis_offs);
	else
		return;

	if (lys->grp_vis) {
		pcb_layer_id_t lid = lys->ly - PCB->Data->Layer;
		pcb_layervis_change_group_vis(lid, !*vis, 1);
	}
	else {
		*vis = !(*vis);
		pcb_gui->attr_dlg_widget_hide(lys->ls->sub.dlg_hid_ctx, lys->wvis_on, !(*vis));
		pcb_gui->attr_dlg_widget_hide(lys->ls->sub.dlg_hid_ctx, lys->wvis_off, !!(*vis));
		locked_layervis_ev(lys->ls);
	}

	ensure_visible_current(lys->ls);

	pcb_hid_redraw(PCB);
}

/* draw a visibility box: filled or partially filled with layer color */
static void layer_vis_box(gen_xpm_t *dst, int filled, const pcb_color_t *color, int brd, int hatch)
{
	int width = 16, height = 16, max_height = 16;
	char *p;
	unsigned int w, line = 0, n;

	strcpy(dst->buf[line++], "16 16 4 1");
	strcpy(dst->buf[line++], ".	c None");
	strcpy(dst->buf[line++], "u	c None");
	pcb_sprintf(dst->buf[line++], "b	c #000000");
	pcb_sprintf(dst->buf[line++], "c	c #%02X%02X%02X", color->r, color->g, color->b);

	while (height--) {
		w = width;
		p = dst->buf[line++];
		while (w--) {
			if ((height < brd) || (height >= max_height-brd) || (w < brd) || (w >= width-brd))
				*p = 'b'; /* frame */
			else if ((hatch) && (((w - height) % 4) == 0))
				*p = '.';
			else if ((width-w+5 < height) || (filled))
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

static void layersel_begin_grp(layersel_ctx_t *ls, const char *name)
{
	PCB_DAD_BEGIN_HBOX(ls->sub.dlg);
		/* vertical group name */
		PCB_DAD_LABEL(ls->sub.dlg, name);
			PCB_DAD_COMPFLAG(ls->sub.dlg, PCB_HATF_TIGHT | PCB_HATF_TEXT_VERTICAL | PCB_HATF_TEXT_TRUNCATED);
		
		/* vert sep */
		PCB_DAD_BEGIN_HBOX(ls->sub.dlg);
			PCB_DAD_COMPFLAG(ls->sub.dlg, PCB_HATF_TIGHT | PCB_HATF_FRAME);
		PCB_DAD_END(ls->sub.dlg);

		/* layer list box */
		PCB_DAD_BEGIN_VBOX(ls->sub.dlg);
			PCB_DAD_COMPFLAG(ls->sub.dlg, PCB_HATF_TIGHT);
}

static void layersel_end_grp(layersel_ctx_t *ls)
{
		PCB_DAD_END(ls->sub.dlg);
	PCB_DAD_END(ls->sub.dlg);
}

static void layersel_create_layer(layersel_ctx_t *ls, ls_layer_t *lys, const char *name, const pcb_color_t *color, int brd, int hatch)
{
	layer_vis_box(&lys->on, 1, color, brd, hatch);
	layer_vis_box(&lys->off, 0, color, brd, hatch);

	PCB_DAD_BEGIN_HBOX(ls->sub.dlg);
		PCB_DAD_PICTURE(ls->sub.dlg, lys->on.xpm);
			lys->wvis_on = PCB_DAD_CURRENT(ls->sub.dlg);
			PCB_DAD_SET_ATTR_FIELD(ls->sub.dlg, user_data, lys);
			PCB_DAD_CHANGE_CB(ls->sub.dlg, layer_vis_cb);
		PCB_DAD_PICTURE(ls->sub.dlg, lys->off.xpm);
			lys->wvis_off = PCB_DAD_CURRENT(ls->sub.dlg);
			PCB_DAD_SET_ATTR_FIELD(ls->sub.dlg, user_data, lys);
			PCB_DAD_CHANGE_CB(ls->sub.dlg, layer_vis_cb);
		PCB_DAD_LABEL(ls->sub.dlg, name);
			lys->wlab = PCB_DAD_CURRENT(ls->sub.dlg);
			PCB_DAD_SET_ATTR_FIELD(ls->sub.dlg, user_data, lys);
			PCB_DAD_CHANGE_CB(ls->sub.dlg, layer_sel_cb);
	PCB_DAD_END(ls->sub.dlg);
}

static void layersel_create_grp(layersel_ctx_t *ls, pcb_board_t *pcb, pcb_layergrp_t *g, pcb_layergrp_id_t gid)
{
	pcb_cardinal_t n;

	layersel_begin_grp(ls, g->name);
	for(n = 0; n < g->len; n++) {
		pcb_layer_t *ly = pcb_get_layer(pcb->Data, g->lid[n]);
		assert(ly != NULL);
		if (ly != NULL) {
			int brd = (((ly != NULL) && (ly->comb & PCB_LYC_SUB)) ? 2 : 1);
			int hatch = (((ly != NULL) && (ly->comb & PCB_LYC_AUTO)) ? 1 : 0);
			const pcb_color_t *clr = &ly->meta.real.color;
			ls_layer_t *lys = lys_get(ls, &ls->real_layer, g->lid[n], 1);

			lys->ly = ly;
			lys->grp_vis = 1;
			layersel_create_layer(ls, lys, ly->name, clr, brd, hatch);
		}
	}
	layersel_end_grp(ls);
}

static void layersel_add_grpsep(layersel_ctx_t *ls)
{
	PCB_DAD_BEGIN_HBOX(ls->sub.dlg);
		PCB_DAD_COMPFLAG(ls->sub.dlg, PCB_HATF_EXPFILL);
		PCB_DAD_PICTURE(ls->sub.dlg, grpsep);
	PCB_DAD_END(ls->sub.dlg);

}

/* Editable layer groups that are part of the stack */
static void layersel_create_stack(layersel_ctx_t *ls, pcb_board_t *pcb)
{
	pcb_layergrp_id_t gid;
	pcb_layergrp_t *g;
	pcb_cardinal_t created = 0;

	for(gid = 0, g = pcb->LayerGroups.grp; gid < pcb->LayerGroups.len; gid++,g++) {
		if (!(PCB_LAYER_IN_STACK(g->ltype)) || (g->ltype & PCB_LYT_SUBSTRATE))
			continue;
		if (created > 0)
			layersel_add_grpsep(ls);
		layersel_create_grp(ls, pcb, g, gid);
		created++;
	}
}

/* Editable layer groups that are not part of the stack */
static void layersel_create_global(layersel_ctx_t *ls, pcb_board_t *pcb)
{
	pcb_layergrp_id_t gid;
	pcb_layergrp_t *g;
	pcb_cardinal_t created = 0;

	for(gid = 0, g = pcb->LayerGroups.grp; gid < pcb->LayerGroups.len; gid++,g++) {
		if (PCB_LAYER_IN_STACK(g->ltype))
			continue;
		if (created > 0)
			layersel_add_grpsep(ls);
		layersel_create_grp(ls, pcb, g, gid);
		created++;
	}
}

/* hardwired/virtual: typically Non-editable layers (no group) */
static void layersel_create_virtual(layersel_ctx_t *ls, pcb_board_t *pcb)
{
	const pcb_menu_layers_t *ml;
	int n;

	layersel_begin_grp(ls, "Virtual");
	for(n = 0, ml = pcb_menu_layers; ml->name != NULL; n++,ml++) {
		ls_layer_t *lys = lys_get(ls, &ls->menu_layer, n, 1);
		lys->ml = ml;
		lys->grp_vis = 0;
		layersel_create_layer(ls, lys, ml->name, ml->force_color, 1, 0);
	}
	layersel_end_grp(ls);
}

/* user-interface layers (no group) */
static void layersel_create_ui(layersel_ctx_t *ls, pcb_board_t *pcb)
{
		int n;

	if (vtp0_len(&pcb_uilayers) <= 0)
		return;

	layersel_begin_grp(ls, "UI");
	for(n = 0; n < vtp0_len(&pcb_uilayers); n++) {
		pcb_layer_t *ly = pcb_uilayers.array[n];
		if ((ly != NULL) && (ly->name != NULL)) {
			ls_layer_t *lys = lys_get(ls, &ls->ui_layer, n, 1);
			lys->ly = ly;
			lys->grp_vis = 0;
			layersel_create_layer(ls, lys, ly->name, &ly->meta.real.color, 1, 0);
		}
	}
	layersel_end_grp(ls);
}

static void hsep(layersel_ctx_t *ls)
{
	PCB_DAD_BEGIN_VBOX(ls->sub.dlg);
		PCB_DAD_COMPFLAG(ls->sub.dlg, PCB_HATF_TIGHT | PCB_HATF_FRAME);
	PCB_DAD_END(ls->sub.dlg);
}

static void layersel_docked_create(layersel_ctx_t *ls, pcb_board_t *pcb)
{
	PCB_DAD_BEGIN_VBOX(ls->sub.dlg);
		PCB_DAD_COMPFLAG(ls->sub.dlg, PCB_HATF_EXPFILL | PCB_HATF_TIGHT /*| PCB_HATF_SCROLL*/);
		layersel_create_stack(&layersel, pcb);
		hsep(&layersel);
		layersel_create_global(&layersel, pcb);
		hsep(&layersel);
		layersel_create_virtual(&layersel, pcb);
		layersel_create_ui(&layersel, pcb);
	PCB_DAD_END(ls->sub.dlg);
	ls->w_last_sel = 0;
}

static void layersel_update_vis(layersel_ctx_t *ls, pcb_board_t *pcb)
{
	pcb_layer_t *ly = pcb->Data->Layer;
	ls_layer_t **lys = ls->real_layer.array;
	const pcb_menu_layers_t *ml;
	pcb_cardinal_t n;

	if (lys == NULL)
		return;

	for(n = 0; n < pcb->Data->LayerN; n++,ly++,lys++) {
		if (*lys == NULL)
			continue;
		pcb_gui->attr_dlg_widget_hide(ls->sub.dlg_hid_ctx, (*lys)->wvis_on, !ly->meta.real.vis);
		pcb_gui->attr_dlg_widget_hide(ls->sub.dlg_hid_ctx, (*lys)->wvis_off, !!ly->meta.real.vis);
	}


	lys = ls->menu_layer.array;
	for(ml = pcb_menu_layers; ml->name != NULL; ml++,lys++) {
		if (*lys == NULL)
			continue;
		pcb_bool *b = (pcb_bool *)((char *)PCB + ml->vis_offs);
		pcb_gui->attr_dlg_widget_hide(ls->sub.dlg_hid_ctx, (*lys)->wvis_on, !(*b));
		pcb_gui->attr_dlg_widget_hide(ls->sub.dlg_hid_ctx, (*lys)->wvis_off, !!(*b));
	}

	lys = ls->ui_layer.array;
	for(n = 0; n < vtp0_len(&pcb_uilayers); n++,lys++) {
		pcb_layer_t *ly = pcb_uilayers.array[n];
		pcb_gui->attr_dlg_widget_hide(ls->sub.dlg_hid_ctx, (*lys)->wvis_on, !ly->meta.real.vis);
		pcb_gui->attr_dlg_widget_hide(ls->sub.dlg_hid_ctx, (*lys)->wvis_off, !!ly->meta.real.vis);
	}

	{ /* if CURRENT is not selected, select it */
		ls_layer_t *lys = lys_get(ls, &ls->real_layer, pcb_layer_id(PCB->Data, CURRENT), 0);
		if ((lys != NULL) && (lys->wlab != ls->w_last_sel))
			locked_layersel(ls, lys->wlab);
	}

	ensure_visible_current(ls);
}

void pcb_layersel_gui_init_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	if ((PCB_HAVE_GUI_ATTR_DLG) && (pcb_gui->get_menu_cfg != NULL)) {
		layersel_docked_create(&layersel, PCB);
		if (pcb_hid_dock_enter(&layersel.sub, PCB_HID_DOCK_LEFT, "layersel") == 0) {
			layersel.sub_inited = 1;
			layersel_update_vis(&layersel, PCB);
		}
	}
}

void pcb_layersel_vis_chg_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	if (layersel.lock_vis > 0)
		return;
	layersel_update_vis(&layersel, PCB);
}

void pcb_layersel_stack_chg_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
TODO("rebuild the whole widget");
	layersel_update_vis(&layersel, PCB);
}
