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

#include "board.h"
#include "event.h"
#include "layer.h"
#include "layer_grp.h"
#include "layer_ui.h"
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

typedef struct {
	int wvis_on, wvis_off, wlab;
	gen_xpm_t on, off;
} ls_layer_t;

typedef struct {
	pcb_hid_dad_subdialog_t sub;
	int sub_inited;
	vtp0_t real_layer, menu_layer, ui_layer;
} layersel_ctx_t;

static layersel_ctx_t layersel;

static ls_layer_t *lys_get(vtp0_t *vt, size_t idx, int alloc)
{
	ls_layer_t **res = vtp0_get(vt, idx, alloc);
	if (res == NULL)
		return NULL;
	if ((*res == NULL) && alloc)
		*res = calloc(sizeof(ls_layer_t), 1);
	return *res;
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

	for(n=0; n < line; n++) {
		dst->xpm[n] = dst->buf[n];
/*		printf("  \"%s\"\n", dst->xpm[n]);*/
	}
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
		PCB_DAD_PICTURE(ls->sub.dlg, lys->off.xpm);
		PCB_DAD_LABEL(ls->sub.dlg, name);
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
			ls_layer_t *lys = lys_get(&ls->real_layer, g->lid[n], 1);

/*			static pcb_color_t clr_invalid;
			static int clr_invalid_inited = 0;
			if (ly == NULL)
				clr = &clr_invalid;
			if (!clr_invalid_inited) {
				pcb_color_load_str(&clr_invalid, "#aaaa00");
				clr_invalid_inited = 1;
			}
*/
			layersel_create_layer(ls, lys, ly->name, clr, brd, hatch);
		}
	}
	layersel_end_grp(ls);
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
		if (created > 0) {
			PCB_DAD_BEGIN_HBOX(ls->sub.dlg);
				PCB_DAD_COMPFLAG(ls->sub.dlg, PCB_HATF_EXPFILL);
				PCB_DAD_PICTURE(ls->sub.dlg, grpsep);
			PCB_DAD_END(ls->sub.dlg);
		}
		layersel_create_grp(ls, pcb, g, gid);
		created++;
	}
}

/* Editable layer groups that are not part of the stack */
static void layersel_create_global(layersel_ctx_t *ls, pcb_board_t *pcb)
{
	pcb_layergrp_id_t gid;
	pcb_layergrp_t *g;

	for(gid = 0, g = pcb->LayerGroups.grp; gid < pcb->LayerGroups.len; gid++,g++) {
		if (PCB_LAYER_IN_STACK(g->ltype))
			continue;
		layersel_create_grp(ls, pcb, g, gid);
	}
}

/* hardwired/virtual: typically Non-editable layers (no group) */
static void layersel_create_virtual(layersel_ctx_t *ls, pcb_board_t *pcb)
{
	const pcb_menu_layers_t *ml;
	int n;

	layersel_begin_grp(ls, "Virtual");
	for(n = 0, ml = pcb_menu_layers; ml->name != NULL; n++,ml++) {
		ls_layer_t *lys = lys_get(&ls->menu_layer, n, 1);
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
			ls_layer_t *lys = lys_get(&ls->ui_layer, n, 1);
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
}


void pcb_layersel_gui_init_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
	if ((PCB_HAVE_GUI_ATTR_DLG) && (pcb_gui->get_menu_cfg != NULL)) {
		layersel_docked_create(&layersel, PCB);
		if (pcb_hid_dock_enter(&layersel.sub, PCB_HID_DOCK_LEFT, "layersel") == 0) {
			layersel.sub_inited = 1;
/*			layersel_pcb2dlg(&layersel, PCB);*/
		}
	}
}

void pcb_layersel_vis_chg_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
/*	layersel_pcb2dlg(&layersel, PCB);*/
}

void pcb_layersel_stack_chg_ev(pcb_hidlib_t *hidlib, void *user_data, int argc, pcb_event_arg_t argv[])
{
TODO("rebuild the whole widget");
/*	layersel_pcb2dlg(&layersel, PCB);*/
}
