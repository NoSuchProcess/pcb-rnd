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
	int wvis_on_open, wvis_off_open, wvis_on_closed, wvis_off_closed, wlab;
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
	pcb_layergrp_id_t gid;
	virt_grp_id_t vid;
	unsigned int is_open:1;
} ls_group_t;

struct layersel_ctx_s {
	pcb_hid_dad_subdialog_t sub;
	int sub_inited;
	int lock_vis, lock_sel;
	int w_last_sel;
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

static ls_group_t *lgs_get_real(layersel_ctx_t *ls, pcb_layergrp_id_t gid, int alloc)
{
	ls_group_t *lgs = lgs_get_(ls, gid+LGS_max, alloc);
	lgs->gid = gid;
	lgs->vid = -1;
	return lgs;
}

static ls_group_t *lgs_get_virt(layersel_ctx_t *ls, virt_grp_id_t vid, int alloc)
{
	ls_group_t *lgs = lgs_get_(ls, vid, alloc);
	lgs->gid = -1;
	lgs->vid = vid;
	return lgs;
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

static void lys_update_vis(ls_layer_t *lys, pcb_bool_t vis)
{
	pcb_gui->attr_dlg_widget_hide(lys->ls->sub.dlg_hid_ctx, lys->wvis_on_open, !vis);
	pcb_gui->attr_dlg_widget_hide(lys->ls->sub.dlg_hid_ctx, lys->wvis_on_closed, !vis);
	pcb_gui->attr_dlg_widget_hide(lys->ls->sub.dlg_hid_ctx, lys->wvis_off_open, !!vis);
	pcb_gui->attr_dlg_widget_hide(lys->ls->sub.dlg_hid_ctx, lys->wvis_off_closed, !!vis);
}

static void layer_select(ls_layer_t *lys)
{
	pcb_bool *vis = NULL;

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
		lys_update_vis(lys, *vis);
		locked_layervis_ev(lys->ls);
	}
	locked_layersel(lys->ls, lys->wlab);
}

static void layer_sel_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	layer_select(attr->user_data);
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
		return;

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
		lys_update_vis(lys, *vis);
		locked_layervis_ev(lys->ls);
	}

	ensure_visible_current(lys->ls);

	pcb_hid_redraw(PCB);
}

static void layer_right_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	layer_select(attr->user_data);
	pcb_actionl("Popup", "layer", NULL);
}

extern pcb_layergrp_id_t pcb_actd_EditGroup_gid;
static void group_right_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	ls_group_t *grp = attr->user_data;
	if (grp->gid < 0)
		return;
	pcb_actd_EditGroup_gid = grp->gid;
	pcb_actionl("Popup", "group", NULL);
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
		lgs->is_open = g->open;
	else
		g->open = lgs->is_open;
}

static void group_open_cb(void *hid_ctx, void *caller_data, pcb_hid_attribute_t *attr)
{
	ls_group_t *lgs = attr->user_data;
	lgs->is_open = !lgs->is_open;
	group_sync_core(PCB, lgs, 0);

	pcb_gui->attr_dlg_widget_hide(lgs->ls->sub.dlg_hid_ctx, lgs->wopen, !lgs->is_open);
	pcb_gui->attr_dlg_widget_hide(lgs->ls->sub.dlg_hid_ctx, lgs->wclosed, lgs->is_open);
}

/* draw a visibility box: filled or partially filled with layer color */
static void layer_vis_box(gen_xpm_t *dst, int filled, const pcb_color_t *color, int brd, int hatch, int width)
{
	int height = 16, max_height = 16;
	char *p;
	unsigned int w, line = 0, n;

	pcb_snprintf(dst->buf[line++], 20, "%d 16 4 1", width);
	strcpy(dst->buf[line++], ".	c None");
	strcpy(dst->buf[line++], "u	c None");
	strcpy(dst->buf[line++], "b	c #000000");
	pcb_snprintf(dst->buf[line++], 20, "c	c #%02X%02X%02X", color->r, color->g, color->b);

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

static void layersel_begin_grp_open(layersel_ctx_t *ls, const char *name, ls_group_t *lsg)
{
	PCB_DAD_BEGIN_HBOX(ls->sub.dlg);
		lsg->wopen = PCB_DAD_CURRENT(ls->sub.dlg);

		/* vertical group name */
		PCB_DAD_LABEL(ls->sub.dlg, name);
			PCB_DAD_COMPFLAG(ls->sub.dlg, PCB_HATF_TIGHT | PCB_HATF_TEXT_VERTICAL | PCB_HATF_TEXT_TRUNCATED);
			PCB_DAD_SET_ATTR_FIELD(ls->sub.dlg, user_data, lsg);
			PCB_DAD_RIGHT_CB(ls->sub.dlg, group_right_cb);
			PCB_DAD_CHANGE_CB(ls->sub.dlg, group_open_cb);

		/* vert sep */
		PCB_DAD_BEGIN_HBOX(ls->sub.dlg);
			PCB_DAD_COMPFLAG(ls->sub.dlg, PCB_HATF_TIGHT | PCB_HATF_FRAME);
		PCB_DAD_END(ls->sub.dlg);

		/* layer list box */
		PCB_DAD_BEGIN_VBOX(ls->sub.dlg);
			PCB_DAD_COMPFLAG(ls->sub.dlg, PCB_HATF_TIGHT);
}

static void layersel_end_grp_open(layersel_ctx_t *ls)
{
		PCB_DAD_END(ls->sub.dlg);
	PCB_DAD_END(ls->sub.dlg);
}

static void layersel_begin_grp_closed(layersel_ctx_t *ls, const char *name, ls_group_t *lsg)
{
	PCB_DAD_BEGIN_HBOX(ls->sub.dlg);
		lsg->wclosed = PCB_DAD_CURRENT(ls->sub.dlg);
		PCB_DAD_LABEL(ls->sub.dlg, name);
			PCB_DAD_SET_ATTR_FIELD(ls->sub.dlg, user_data, lsg);
			PCB_DAD_RIGHT_CB(ls->sub.dlg, group_right_cb);
			PCB_DAD_CHANGE_CB(ls->sub.dlg, group_open_cb);

		/* vert sep */
		PCB_DAD_BEGIN_HBOX(ls->sub.dlg);
			PCB_DAD_COMPFLAG(ls->sub.dlg, PCB_HATF_TIGHT | PCB_HATF_FRAME);
		PCB_DAD_END(ls->sub.dlg);

}

static void layersel_end_grp_closed(layersel_ctx_t *ls)
{
	PCB_DAD_END(ls->sub.dlg);
}


static void layersel_create_layer_open(layersel_ctx_t *ls, ls_layer_t *lys, const char *name, const pcb_color_t *color, int brd, int hatch)
{
	layer_vis_box(&lys->on_open, 1, color, brd, hatch, 16);
	layer_vis_box(&lys->off_open, 0, color, brd, hatch, 16);

	PCB_DAD_BEGIN_HBOX(ls->sub.dlg);
		PCB_DAD_PICTURE(ls->sub.dlg, lys->on_open.xpm);
			lys->wvis_on_open = PCB_DAD_CURRENT(ls->sub.dlg);
			PCB_DAD_SET_ATTR_FIELD(ls->sub.dlg, user_data, lys);
			PCB_DAD_CHANGE_CB(ls->sub.dlg, layer_vis_cb);
		PCB_DAD_PICTURE(ls->sub.dlg, lys->off_open.xpm);
			lys->wvis_off_open = PCB_DAD_CURRENT(ls->sub.dlg);
			PCB_DAD_SET_ATTR_FIELD(ls->sub.dlg, user_data, lys);
			PCB_DAD_CHANGE_CB(ls->sub.dlg, layer_vis_cb);
		PCB_DAD_LABEL(ls->sub.dlg, name);
			lys->wlab = PCB_DAD_CURRENT(ls->sub.dlg);
			PCB_DAD_SET_ATTR_FIELD(ls->sub.dlg, user_data, lys);
			PCB_DAD_CHANGE_CB(ls->sub.dlg, layer_sel_cb);
			PCB_DAD_RIGHT_CB(ls->sub.dlg, layer_right_cb);
	PCB_DAD_END(ls->sub.dlg);
}

static void layersel_create_layer_closed(layersel_ctx_t *ls, ls_layer_t *lys, const char *name, const pcb_color_t *color, int brd, int hatch)
{
	layer_vis_box(&lys->on_closed, 1, color, brd, hatch, 8);
	layer_vis_box(&lys->off_closed, 0, color, brd, hatch, 8);

	PCB_DAD_BEGIN_HBOX(ls->sub.dlg);
		PCB_DAD_PICTURE(ls->sub.dlg, lys->on_closed.xpm);
			lys->wvis_on_closed = PCB_DAD_CURRENT(ls->sub.dlg);
			PCB_DAD_SET_ATTR_FIELD(ls->sub.dlg, user_data, lys);
			PCB_DAD_CHANGE_CB(ls->sub.dlg, layer_vis_cb);
		PCB_DAD_PICTURE(ls->sub.dlg, lys->off_closed.xpm);
			lys->wvis_off_closed = PCB_DAD_CURRENT(ls->sub.dlg);
			PCB_DAD_SET_ATTR_FIELD(ls->sub.dlg, user_data, lys);
			PCB_DAD_CHANGE_CB(ls->sub.dlg, layer_vis_cb);
	PCB_DAD_END(ls->sub.dlg);
}

static void layersel_create_grp(layersel_ctx_t *ls, pcb_board_t *pcb, pcb_layergrp_t *g, ls_group_t *lgs, int is_open)
{
	pcb_cardinal_t n;

	if (is_open)
		layersel_begin_grp_open(ls, g->name, lgs);
	else
		layersel_begin_grp_closed(ls, g->name, lgs);
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
			if (is_open)
				layersel_create_layer_open(ls, lys, ly->name, clr, brd, hatch);
			else
				layersel_create_layer_closed(ls, lys, ly->name, clr, brd, hatch);
		}
	}
	if (is_open)
		layersel_end_grp_open(ls);
	else
		layersel_end_grp_closed(ls);
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
	pcb_layergrp_id_t gid;
	pcb_layergrp_t *g;
	pcb_cardinal_t created = 0;

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
		layersel_create_layer_open(ls, lys, ml->name, ml->force_color, 1, 0);
	}
	layersel_end_grp_open(ls);

	layersel_begin_grp_closed(ls, "Virtual", lgs);
	for(n = 0, ml = pcb_menu_layers; ml->name != NULL; n++,ml++) {
		ls_layer_t *lys = lys_get(ls, &ls->menu_layer, n, 0);
		layersel_create_layer_closed(ls, lys, ml->name, ml->force_color, 1, 0);
	}
	layersel_end_grp_closed(ls);
}

/* user-interface layers (no group) */
static void layersel_create_ui(layersel_ctx_t *ls, pcb_board_t *pcb)
{
	ls_group_t *lgs = lgs_get_virt(ls, LGS_UI, 1);
	int n;

	if (vtp0_len(&pcb_uilayers) <= 0)
		return;

	layersel_begin_grp_open(ls, "UI", lgs);
	for(n = 0; n < vtp0_len(&pcb_uilayers); n++) {
		pcb_layer_t *ly = pcb_uilayers.array[n];
		if ((ly != NULL) && (ly->name != NULL)) {
			ls_layer_t *lys = lys_get(ls, &ls->ui_layer, n, 1);
			lys->ly = ly;
			lys->grp_vis = 0;
			layersel_create_layer_open(ls, lys, ly->name, &ly->meta.real.color, 1, 0);
		}
	}
	layersel_end_grp_open(ls);

	layersel_begin_grp_closed(ls, "UI", lgs);
	for(n = 0; n < vtp0_len(&pcb_uilayers); n++) {
		pcb_layer_t *ly = pcb_uilayers.array[n];
		if ((ly != NULL) && (ly->name != NULL)) {
			ls_layer_t *lys = lys_get(ls, &ls->ui_layer, n, 0);
			layersel_create_layer_closed(ls, lys, ly->name, &ly->meta.real.color, 1, 0);
		}
	}
	layersel_end_grp_closed(ls);
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
		layersel_add_grpsep(ls);
		layersel_create_ui(&layersel, pcb);
	PCB_DAD_END(ls->sub.dlg);
	ls->w_last_sel = 0;
}

static void layersel_update_vis(layersel_ctx_t *ls, pcb_board_t *pcb)
{
	pcb_layer_t *ly = pcb->Data->Layer;
	ls_layer_t **lys = (ls_layer_t **)ls->real_layer.array;
	ls_group_t **lgs = (ls_group_t **)ls->group.array;
	const pcb_menu_layers_t *ml;
	pcb_cardinal_t n;

	if (lys == NULL)
		return;

	for(n = 0; n < pcb->Data->LayerN; n++,ly++,lys++) {
		if (*lys == NULL)
			continue;
		lys_update_vis(*lys, ly->meta.real.vis);
	}


	lys = (ls_layer_t **)ls->menu_layer.array;
	for(ml = pcb_menu_layers; ml->name != NULL; ml++,lys++) {
		pcb_bool *b;
		if (*lys == NULL)
			continue;
		b = (pcb_bool *)((char *)PCB + ml->vis_offs);
		lys_update_vis(*lys, *b);
	}

	lys = (ls_layer_t **)ls->ui_layer.array;
	for(n = 0; n < vtp0_len(&pcb_uilayers); n++,lys++) {
		pcb_layer_t *ly = pcb_uilayers.array[n];
		lys_update_vis(*lys, ly->meta.real.vis);
	}

	/* update group open/close hides */
	for(n = 0; n < vtp0_len(&ls->group); n++,lgs++) {
		if (*lgs == NULL)
			continue;

		group_sync_core(pcb, *lgs, 1);
		pcb_gui->attr_dlg_widget_hide(ls->sub.dlg_hid_ctx, (*lgs)->wopen, !(*lgs)->is_open);
		pcb_gui->attr_dlg_widget_hide(ls->sub.dlg_hid_ctx, (*lgs)->wclosed, (*lgs)->is_open);
	}

	{ /* if CURRENT is not selected, select it */
		ls_layer_t *lys = lys_get(ls, &ls->real_layer, pcb_layer_id(PCB->Data, CURRENT), 0);
		if ((lys != NULL) && (lys->wlab != ls->w_last_sel))
			locked_layersel(ls, lys->wlab);
	}

	ensure_visible_current(ls);
}

static void layersel_build(void)
{
	layersel_docked_create(&layersel, PCB);
	if (pcb_hid_dock_enter(&layersel.sub, PCB_HID_DOCK_LEFT, "layersel") == 0) {
		layersel.sub_inited = 1;
		layersel_update_vis(&layersel, PCB);
	}
}

void pcb_layersel_gui_init_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	if ((PCB_HAVE_GUI_ATTR_DLG) && (pcb_gui->get_menu_cfg != NULL))
		layersel_build();
}

void pcb_layersel_vis_chg_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	if ((!layersel.sub_inited) || (layersel.lock_vis > 0))
		return;
	layersel_update_vis(&layersel, PCB);
}

void pcb_layersel_stack_chg_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	if ((PCB_HAVE_GUI_ATTR_DLG) && (pcb_gui->get_menu_cfg != NULL) && (layersel.sub_inited)) {
		pcb_hid_dock_leave(&layersel.sub);
		layersel.sub_inited = 0;
		layersel_build();
	}
}
