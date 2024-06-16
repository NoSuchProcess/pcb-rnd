/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,2004,2006 Thomas Nau
 *  Copyright (C) 2016,2021 Tibor 'Igor2' Palinkas
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
 *
 */
#include <ctype.h>
#include "config.h"
#include <librnd/core/rnd_printf.h>
#include "genvector/gds_char.h"
#include "route_style.h"
#include <librnd/core/error.h>
#include <librnd/core/conf.h>
#include <librnd/core/misc_util.h>
#include "board.h"
#include "undo.h"
#include "funchash_core.h"
#include "conf_core.h"
#include "event.h"

static const char rst_cookie[] = "core route style";

void pcb_use_route_style_(pcb_route_style_t * rst)
{
	rnd_conf_set_design("design/line_thickness", "%$mS", rst->Thick);
	rnd_conf_set_design("design/text_scale", "%d", rst->texts <= 0 ? 100 : rst->texts);
	rnd_conf_set_design("design/text_thickness", "%$mS", rst->textt);
	if (rst->fid != -1)
		rnd_conf_set_design("design/text_font_id", "%ld", rst->fid);
	if (rst->via_proto_set)
		rnd_conf_set_design("design/via_proto", "%ld", (long)rst->via_proto);
	rnd_conf_set_design("design/clearance", "%$mS", rst->Clearance);
	PCB->pen_attr = &rst->attr;
}

int pcb_use_route_style_idx_(vtroutestyle_t *styles, int idx)
{
	if ((idx < 0) || (idx >= vtroutestyle_len(styles)))
		return -1;
	pcb_use_route_style_(styles->array+idx);
	return 0;
}

#define cmp(a,b) (((a) != -1) && (coord_abs((a)-(b)) > 32))
#define cmpi0(a,b) (((a) > 0) && (strict || ((b) > 0)) && ((a) != (b)))
#define cmpi(a,b) (((a) != -1) && (strict || ((b) != -1)) && ((a) != (b)))
#define cmps(a,b) (((a) != NULL) && (strcmp((a), (b)) != 0))
RND_INLINE int pcb_route_style_match_(pcb_route_style_t *rst, int strict, rnd_coord_t Thick, rnd_coord_t textt, int texts, rnd_font_id_t fid, rnd_coord_t Clearance, rnd_cardinal_t via_proto, char *Name)
{
	if (cmp(Thick, rst->Thick)) return 0;
	if (cmp(textt, rst->textt)) return 0;
	if (cmpi0(texts, rst->texts)) return 0;
	if (cmpi(fid, rst->fid)) return 0;
	if (strict || rst->via_proto_set)
		if (cmpi(via_proto, rst->via_proto)) return 0;
	if (cmp(Clearance, rst->Clearance)) return 0;
	if (cmps(Name, rst->name)) return 0;
	return 1;
}

int pcb_route_style_match(pcb_route_style_t *rst, rnd_coord_t Thick, rnd_coord_t textt, int texts, rnd_font_id_t fid, rnd_coord_t Clearance, rnd_cardinal_t via_proto, char *Name)
{
	return pcb_route_style_match_(rst, 0, Thick, textt, texts, fid, Clearance, via_proto, Name);
}

#undef cmp
#undef cmpi
#undef cmpi0
#undef cmps

int pcb_route_style_lookup(vtroutestyle_t *styles, int hint, rnd_coord_t Thick, rnd_coord_t textt, int texts, rnd_font_id_t fid, rnd_coord_t Clearance, rnd_cardinal_t via_proto, char *Name)
{
	int n, len = vtroutestyle_len(styles);

	if ((hint >= 0) && (hint <len))
		if (pcb_route_style_match_(&styles->array[hint], 0, Thick, textt, texts, fid, Clearance, via_proto, Name))
			return hint;

	for (n = 0; n < len; n++)
		if (pcb_route_style_match_(&styles->array[n], 0, Thick, textt, texts, fid, Clearance, via_proto, Name))
			return n;

	return -1;
}


int pcb_route_style_lookup_strict(vtroutestyle_t *styles, int hint, rnd_coord_t Thick, rnd_coord_t textt, int texts, rnd_font_id_t fid, rnd_coord_t Clearance, rnd_cardinal_t via_proto, char *Name)
{
	int n, len = vtroutestyle_len(styles);

	if ((hint >= 0) && (hint <len))
		if (pcb_route_style_match_(&styles->array[hint], 1, Thick, textt, texts, fid, Clearance, via_proto, Name))
			return hint;

	for (n = 0; n < vtroutestyle_len(styles); n++)
		if (pcb_route_style_match_(&styles->array[n], 1, Thick, textt, texts, fid, Clearance, via_proto, Name))
			return n;

	return -1;
}

int pcb_lookup_route_style_pen_bestfit(pcb_board_t *pcb)
{
	int res = PCB_LOOKUP_ROUTE_STYLE_PEN_STRICT(pcb);
	if (res == -1) res = PCB_LOOKUP_ROUTE_STYLE_PEN(pcb);
	return res;
}

int pcb_get_style_size(int funcid, rnd_coord_t * out, int type, int size_id)
{
	switch (funcid) {
	case F_Object:
		switch (type) {
		case PCB_OBJ_LINE:
			return pcb_get_style_size(F_SelectedLines, out, 0, size_id);
		case PCB_OBJ_ARC:
			return pcb_get_style_size(F_SelectedArcs, out, 0, size_id);
		}
		rnd_message(RND_MSG_ERROR, "Sorry, can't fetch the style of that object type (%x)\n", type);
		return -1;
	case F_SelectedPads:
		if (size_id != 2) /* don't mess with pad size */
			return -1;
	case F_SelectedVias:
	case F_SelectedPins:
	case F_SelectedObjects:
	case F_Selected:
	case F_SelectedElements:
		if ((size_id == 0) || (size_id == 1)) {
			rnd_message(RND_MSG_ERROR, "pcb_get_style_size(Selected*) for size_id %d is not supported (padstack prototypes don't have diameter)\n", size_id);
			*out = 0;
		}
		else
			*out = conf_core.design.clearance;
		break;
	case F_SelectedArcs:
	case F_SelectedLines:
		if (size_id == 2)
			*out = conf_core.design.clearance;
		else
			*out = conf_core.design.line_thickness;
		return 0;
	case F_SelectedTexts:
	case F_SelectedNames:
		rnd_message(RND_MSG_ERROR, "Sorry, can't change style of every selected object\n");
		return -1;
	}
	return 0;
}



typedef struct {
	pcb_board_t *pcb;
	int idx;
	pcb_route_style_t rst;
	enum { RST_SET, RST_NEW, RST_DEL } cmd;
} undo_rst_t;

static int undo_rst_swap(void *udata)
{
	undo_rst_t *g = udata;
	pcb_route_style_t *rst;

	if (g->cmd != RST_NEW) {
		rst = vtroutestyle_get(&g->pcb->RouteStyle, g->idx, 0);
		if (rst == NULL) {
			rnd_message(RND_MSG_ERROR, "undo_rst_swap(): internal error: route %d does not exist\b", g->idx);
			return -1;
		}
	}
	else
		rst = vtroutestyle_alloc_insert(&g->pcb->RouteStyle, g->idx, 1);

	rnd_swap(pcb_route_style_t, *rst, g->rst);

	if (g->cmd == RST_DEL)
		vtroutestyle_remove(&g->pcb->RouteStyle, g->idx, 1);

	/* invert new/del */
	if (g->cmd == RST_NEW) g->cmd = RST_DEL;
	else if (g->cmd == RST_DEL) g->cmd = RST_NEW;


	rnd_event(&g->pcb->hidlib, PCB_EVENT_ROUTE_STYLES_CHANGED, NULL);
	pcb_board_set_changed_flag(g->pcb, 1);

	return 0;
}

static void undo_rst_print(void *udata, char *dst, size_t dst_len)
{
	undo_rst_t *g = udata;

	rnd_snprintf(dst, dst_len, "route style: thick=%.03$mH textt=%.03$mH texts=%.03$mH Clearance=%.03$mH texts=%.03$mH proto=%ld",
		g->rst.Thick, g->rst.textt, g->rst.texts, g->rst.Clearance, (long)(g->rst.via_proto_set ? g->rst.via_proto : -1));
}

static const uundo_oper_t undo_rst = {
	rst_cookie,
	NULL,
	undo_rst_swap,
	undo_rst_swap,
	undo_rst_print
};


int pcb_route_style_change(pcb_board_t *pcb, int rstidx, rnd_coord_t *thick, rnd_coord_t *textt, int *texts, rnd_font_id_t *fid, rnd_coord_t *clearance, rnd_cardinal_t *via_proto, rnd_bool undoable)
{
	undo_rst_t gtmp, *g = &gtmp;
	pcb_route_style_t *rst = vtroutestyle_get(&pcb->RouteStyle, rstidx, 0);

	if (rst == NULL)
		return -1;

	if (undoable) g = pcb_undo_alloc(pcb, &undo_rst, sizeof(undo_rst_t));

	g->pcb = pcb;
	g->idx = rstidx;
	g->rst = *rst;
	g->cmd = RST_SET;
	if (thick != NULL)      g->rst.Thick = *thick;
	if (textt != NULL)      g->rst.textt = *textt;
	if (texts != NULL)      g->rst.texts = *texts;
	if (fid != NULL)        g->rst.fid = *fid;
	if (clearance != NULL)  g->rst.Clearance = *clearance;
	if (via_proto != NULL) {
		g->rst.via_proto = *via_proto;
		g->rst.via_proto_set = (*via_proto != -1);
			
	}

	undo_rst_swap(g);
	if (undoable) pcb_undo_inc_serial();

	return 0;
}

int pcb_route_style_change_name(pcb_board_t *pcb, int rstidx, const char *new_name, rnd_bool undoable)
{
	undo_rst_t gtmp, *g = &gtmp;
	pcb_route_style_t *rst = vtroutestyle_get(&pcb->RouteStyle, rstidx, 0);

	if (rst == NULL)
		return -1;

	if (undoable) g = pcb_undo_alloc(pcb, &undo_rst, sizeof(undo_rst_t));

	g->pcb = pcb;
	g->idx = rstidx;
	g->rst = *rst;
	g->cmd = RST_SET;
	strncpy(g->rst.name, new_name, sizeof(g->rst.name));
	undo_rst_swap(g);
	if (undoable) pcb_undo_inc_serial();

	return 0;
}

int pcb_route_style_new(pcb_board_t *pcb, const char *name, rnd_bool undoable)
{

	undo_rst_t gtmp, *g = &gtmp;
	if (undoable) g = pcb_undo_alloc(pcb, &undo_rst, sizeof(undo_rst_t));

	g->pcb = pcb;
	g->idx = vtroutestyle_len(&PCB->RouteStyle);
	g->cmd = RST_NEW;

	memset(&g->rst, 0, sizeof(g->rst));

	strncpy(g->rst.name, name, sizeof(g->rst.name));

	g->rst.Thick = conf_core.design.line_thickness;
	g->rst.textt = conf_core.design.text_thickness;
	g->rst.texts = conf_core.design.text_scale;
	g->rst.Clearance = conf_core.design.clearance;
	g->rst.fid = -1; /* leave font unset so saving in old lihata format won't trigger a warning */

	undo_rst_swap(g);
	if (undoable) pcb_undo_inc_serial();

	return g->idx;
}

int pcb_route_style_del(pcb_board_t *pcb, int idx, rnd_bool undoable)
{
	undo_rst_t gtmp, *g = &gtmp;
	if (undoable) g = pcb_undo_alloc(pcb, &undo_rst, sizeof(undo_rst_t));

	g->pcb = pcb;
	g->idx = idx;
	g->cmd = RST_DEL;

	memset(&g->rst, 0, sizeof(g->rst));

	undo_rst_swap(g);
	if (undoable) pcb_undo_inc_serial();

	return g->idx;
}

