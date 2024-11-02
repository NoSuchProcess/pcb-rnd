/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
#include "config.h"
#include "board.h"
#include "data.h"
#include "conf_core.h"
#include <librnd/core/rnd_conf.h>
#include "plug_io.h"
#include <librnd/core/compat_misc.h>
#include <librnd/core/actions.h>
#include <librnd/core/paths.h>
#include <librnd/hid/hid_menu.h>
#include <librnd/poly/rtree.h>
#include "undo.h"
#include "draw.h"
#include "event.h"
#include <librnd/core/safe_fs.h>
#include <librnd/hid/tool.h>
#include "layer.h"
#include "netlist.h"

pcb_board_t *PCB;

static const char core_board_cookie[] = "core/board";

void pcb_board_free(pcb_board_t * pcb)
{
	int i;

	if (pcb == NULL)
		return;

	free(pcb->line_mod_merge);
	free(pcb->hidlib.name);
	free(pcb->hidlib.loadname);
	free(pcb->PrintFilename);
	pcb_ratspatch_destroy(pcb);
	pcb_data_free(pcb->Data);
	pcb_data_free(pcb->uilayer_data);

	/* release font symbols */
	pcb_fontkit_free(&pcb->fontkit);
	for (i = 0; i < PCB_NUM_NETLISTS; i++)
		pcb_netlist_uninit(&(pcb->netlist[i]));
	vtroutestyle_uninit(&pcb->RouteStyle);
	pcb_attribute_free(&pcb->Attributes);

	pcb_layergroup_free_stack(&pcb->LayerGroups);

	/* clear struct */
	memset(pcb, 0, sizeof(pcb_board_t));
}

/* creates a new PCB */
pcb_board_t *pcb_board_new_(rnd_bool SetDefaultNames)
{
	pcb_board_t *ptr;
	int i;

	/* allocate memory, switch all layers on and copy resources */
	ptr = calloc(1, sizeof(pcb_board_t));
	ptr->Data = pcb_buffer_new(ptr);
	ptr->uilayer_data = pcb_buffer_new(ptr);

	for(i = 0; i < PCB_NUM_NETLISTS; i++)
		pcb_netlist_init(&(ptr->netlist[i]));

	rnd_conf_set(RND_CFR_INTERNAL, "design/poly_isle_area", -1, "200000000", RND_POL_OVERWRITE);

	ptr->RatDraw = rnd_false;

	/* NOTE: we used to set all the pcb flags on ptr here, but we don't need to do that anymore due to the new conf system */
	ptr->hidlib.grid = rnd_conf.editor.grid;

	ptr->hidlib.dwg.Y1 = ptr->hidlib.dwg.X1 = 0;
	ptr->hidlib.dwg.Y2 = ptr->hidlib.dwg.X2 = RND_MM_TO_COORD(20); /* should be overriden by the default design */
	ptr->ID = pcb_create_ID_get();
	ptr->ThermScale = 0.5;

	pcb_font_create_default(ptr);

	return ptr;
}

#include "defpcb_internal.c"

pcb_board_t *pcb_board_new(int inhibit_events)
{
	pcb_board_t *old, *nw = NULL;
	int dpcb;
	unsigned int inh = inhibit_events ? PCB_INHIBIT_BOARD_CHANGED : 0;

	old = PCB;
	PCB = NULL;

	dpcb = -1;
	pcb_io_err_inhibit_inc();
	rnd_hid_menu_merge_inhibit_inc();
	rnd_conf_list_foreach_path_first(NULL, dpcb, &conf_core.rc.default_pcb_file, pcb_load_pcb(__path__, NULL, rnd_false, 1 | 0x10 | inh));
	rnd_hid_menu_merge_inhibit_dec();
	pcb_io_err_inhibit_dec();

	if (dpcb != 0) { /* no default PCB in file, use embedded version */
		FILE *f;
		char *efn;
		const char *tmp_fn = ".pcb-rnd.default.pcb";

		/* We can parse from file only, make a temp file */
		f = rnd_fopen_fn(NULL, tmp_fn, "wb", &efn);
		if (f != NULL) {
			fwrite(default_pcb_internal, strlen(default_pcb_internal), 1, f);
			fclose(f);
			dpcb = pcb_load_pcb(efn, NULL, rnd_false, 1 | 0x10);
			if (dpcb == 0)
				rnd_message(RND_MSG_WARNING, "Couldn't find default.pcb - using the embedded fallback\n");
			else
				rnd_message(RND_MSG_ERROR, "Couldn't find default.pcb and failed to load the embedded fallback\n");
			rnd_remove(NULL, efn);
			free(efn);
		}
	}

	if (dpcb == 0) {
		nw = PCB;
		if (nw->hidlib.loadname != NULL) {
			/* make sure the new PCB doesn't inherit the name and loader of the default pcb */
			free(nw->hidlib.loadname);
			nw->hidlib.loadname = NULL;
			nw->Data->loader = NULL;
		}
	}

	PCB = old;
	return nw;
}

int pcb_board_new_postproc(pcb_board_t *pcb, int use_defaults)
{
	int n;

	/* copy default settings */
	pcb_layer_colors_from_conf(pcb, 0);

	for(n = 0; n < PCB_MAX_BUFFER; n++) {
		if (pcb_buffers[n].Data != NULL) {
			pcb_data_unbind_layers(pcb_buffers[n].Data);
			pcb_data_binding_update(pcb, pcb_buffers[n].Data);
		}
	}

	if (pcb == PCB)
		rnd_event(&PCB->hidlib, PCB_EVENT_ROUTE_STYLES_CHANGED, NULL);

	return 0;
}

void pcb_layer_colors_from_conf(pcb_board_t *ptr, int force)
{
	int i;

	/* copy default settings */
	for (i = 0; i < PCB_MAX_LAYER; i++)
		if (force || (ptr->Data->Layer[i].meta.real.color.str[0] == '\0'))
			memcpy(&ptr->Data->Layer[i].meta.real.color, pcb_layer_default_color(i, pcb_layer_flags(ptr, i)), sizeof(rnd_color_t));
}

typedef struct {
	int nplated;
	int nunplated;
} hole_count_t;

#include "obj_pstk_inlines.h"
static rnd_rtree_dir_t hole_counting_callback(void *cl, void *obj, const rnd_rtree_box_t *box)
{
	pcb_pstk_t *ps = (pcb_pstk_t *)obj;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);
	hole_count_t *hcs = (hole_count_t *)cl;

	if ((proto != NULL) && (proto->hdia > 0)) {
		if (proto->hplated)
			hcs->nplated++;
		else
			hcs->nunplated++;
	}
	return rnd_RTREE_DIR_FOUND_CONT;
}

static rnd_rtree_dir_t slot_counting_callback(void *cl, void *obj, const rnd_rtree_box_t *box)
{
	pcb_pstk_t *ps = (pcb_pstk_t *)obj;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);
	hole_count_t *hcs = (hole_count_t *)cl;

	if ((proto != NULL) && (proto->mech_idx >= 0)) {
		if (proto->hplated)
			hcs->nplated++;
		else
			hcs->nunplated++;
	}
	return rnd_RTREE_DIR_FOUND_CONT;
}

void pcb_board_count_holes(pcb_board_t *pcb, int *plated, int *unplated, const rnd_box_t *within_area)
{
	hole_count_t hcs = { 0, 0 };

	rnd_rtree_search_any(pcb->Data->padstack_tree, (rnd_rtree_box_t *)within_area, NULL, hole_counting_callback, &hcs, NULL);

	if (plated != NULL)
		*plated = hcs.nplated;
	if (unplated != NULL)
		*unplated = hcs.nunplated;
}

void pcb_board_count_slots(pcb_board_t *pcb, int *plated, int *unplated, const rnd_box_t *within_area)
{
	hole_count_t hcs = { 0, 0 };

	rnd_rtree_search_any(pcb->Data->padstack_tree, (rnd_rtree_box_t *)within_area, NULL, slot_counting_callback, &hcs, NULL);

	if (plated != NULL)
		*plated = hcs.nplated;
	if (unplated != NULL)
		*unplated = hcs.nunplated;
}

const char *pcb_board_get_filename(void)
{
	return PCB->hidlib.loadname;
}

const char *pcb_board_get_name(void)
{
	return PCB->hidlib.name;
}

rnd_bool pcb_board_change_name(char *Name)
{
	free(PCB->hidlib.name);
	PCB->hidlib.name = Name;
	rnd_event(&PCB->hidlib, RND_EVENT_DESIGN_META_CHANGED, NULL, NULL);
	return rnd_true;
}

static void pcb_board_resize_(pcb_board_t *pcb, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2)
{
	pcb->hidlib.dwg.X1 = x1;
	pcb->hidlib.dwg.Y1 = y1;
	pcb->hidlib.dwg.X2 = x2;
	pcb->hidlib.dwg.Y2 = y2;

	if (pcb == PCB) {
		rnd_event(&pcb->hidlib, RND_EVENT_DESIGN_META_CHANGED, NULL);
		rnd_event(&pcb->hidlib, PCB_EVENT_BOARD_EDITED, NULL);
		rnd_gui->invalidate_all(rnd_gui);
	}
}

/*** undoable board resize ***/

typedef struct {
	pcb_board_t *pcb;
	rnd_coord_t x1, y1, x2, y2;
	double thermal_scale;
} undo_board_size_t;

static int undo_board_size_swap(void *udata)
{
	undo_board_size_t *s = udata;

	if ((s->x1 != s->pcb->hidlib.dwg.X1) || (s->y1 != s->pcb->hidlib.dwg.Y1) || (s->x2 != s->pcb->hidlib.dwg.X2) || (s->y2 != s->pcb->hidlib.dwg.Y2)) {
		rnd_coord_t old_x1 = s->pcb->hidlib.dwg.X1, old_y1 = s->pcb->hidlib.dwg.Y1;
		rnd_coord_t old_x2 = s->pcb->hidlib.dwg.X2, old_y2 = s->pcb->hidlib.dwg.Y2;
		pcb_board_resize_(s->pcb, s->x1, s->y1, s->x2, s->y2);
		s->x1 = old_x1; s->y1 = old_y1;
		s->x2 = old_x2; s->y2 = old_y2;
	}
	if (s->thermal_scale != s->pcb->ThermScale) {
		rnd_swap(double, s->thermal_scale, s->pcb->ThermScale);
		pcb_data_clip_polys(s->pcb->Data);
		rnd_gui->invalidate_all(rnd_gui);
	}
	return 0;
}

static void undo_board_size_print(void *udata, char *dst, size_t dst_len)
{
	undo_board_size_t *s = udata;
	if ((s->x1 != s->pcb->hidlib.dwg.X1) || (s->y1 != s->pcb->hidlib.dwg.Y1) || (s->x2 != s->pcb->hidlib.dwg.X2) || (s->y2 != s->pcb->hidlib.dwg.Y2))
		rnd_snprintf(dst, dst_len, "board_size: %mm;%mm .. %mm;%mm ", s->x1, s->y1, s->x2, s->y2);
	if (s->thermal_scale != s->pcb->ThermScale)
		rnd_snprintf(dst, dst_len, "thermal scale: %f", s->thermal_scale);
}

static const uundo_oper_t undo_board_size = {
	core_board_cookie,
	NULL, /* free */
	undo_board_size_swap,
	undo_board_size_swap,
	undo_board_size_print
};

void pcb_board_resize(rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, int undoable)
{
	undo_board_size_t *s;

	if (!undoable) {
		pcb_board_resize_(PCB, x1, y1, x2, y2);
		return;
	}

	if ((x1 != 0) || (y1 != 0))
		rnd_message(RND_MSG_ERROR, "pcb_board_resize(): drawing area should start at 0;0 for compatibility reasons\n");

	s = pcb_undo_alloc(PCB, &undo_board_size, sizeof(undo_board_size_t));
	s->pcb = PCB;
	s->x1 = x1;
	s->y1 = y1;
	s->x2 = x2;
	s->y2 = y2;
	s->thermal_scale = PCB->ThermScale;
	undo_board_size_swap(s);
}

void pcb_board_chg_thermal_scale(double thermal_scale, int undoable)
{
	undo_board_size_t *s;

	if (!undoable) {
		PCB->ThermScale = thermal_scale;
		pcb_data_clip_polys(PCB->Data);
		rnd_gui->invalidate_all(rnd_gui);
		return;
	}

	s = pcb_undo_alloc(PCB, &undo_board_size, sizeof(undo_board_size_t));
	s->pcb = PCB;
	s->x1 = PCB->hidlib.dwg.X1;
	s->y1 = PCB->hidlib.dwg.Y1;
	s->x2 = PCB->hidlib.dwg.X2;
	s->y2 = PCB->hidlib.dwg.Y2;
	s->thermal_scale = thermal_scale;
	undo_board_size_swap(s);
}


void pcb_board_remove(pcb_board_t *Ptr)
{
	pcb_undo_clear_list(rnd_true);
	pcb_board_free(Ptr);
	free(Ptr);
}

/* sets or resets changed flag and redraws status */
void pcb_board_set_changed_flag(pcb_board_t *pcb, rnd_bool New)
{
	rnd_bool old = pcb->Changed;

	pcb->Changed = New;

	if (old != New)
		rnd_event(&pcb->hidlib, RND_EVENT_DESIGN_META_CHANGED, NULL);

	if (New)
		rnd_event(&pcb->hidlib, PCB_EVENT_BOARD_EDITED, NULL);
}


void pcb_board_replaced(void)
{
	rnd_single_switch_to(&PCB->hidlib);
}

int pcb_board_normalize(pcb_board_t *pcb)
{
	rnd_box_t b;
	int chg = 0;
	
	if (pcb_data_bbox(&b, pcb->Data, rnd_false) == NULL)
		return -1;

	if ((b.X2 - b.X1) > pcb->hidlib.dwg.X2) {
		pcb->hidlib.dwg.X2 = b.X2 - b.X1;
		chg++;
	}

	if ((b.Y2 - b.Y1) > pcb->hidlib.dwg.Y2) {
		pcb->hidlib.dwg.Y2 = b.Y2 - b.Y1;
		chg++;
	}

	chg += pcb_data_normalize_(pcb->Data, &b);

	return chg;
}

rnd_coord_t pcb_stack_thickness(pcb_board_t *pcb, const char *namespace, pcb_board_thickness_flags_t flags, rnd_layergrp_id_t from, rnd_bool include_from, rnd_layergrp_id_t to, rnd_bool include_to, pcb_layer_type_t accept)
{
	rnd_layergrp_id_t gid;
	pcb_layergrp_t *grp;
	rnd_coord_t curr, total = 0;

	if (from > to) {
		rnd_swap(rnd_layergrp_id_t, from, to);
		rnd_swap(int, include_from, include_to);
	}

	if (!include_from)
		from++;

	if (!include_to)
		to--;

	if (to < from)
		return -1;

	for(gid = from, grp = pcb->LayerGroups.grp+from; gid <= to; gid++,grp++) {
		const char *s;

		if (!(grp->ltype & accept))
			continue;

		curr = 0;
		s = pcb_layergrp_thickness_attr(grp, namespace);
		if (s != NULL)
			curr = rnd_get_value(s, NULL, NULL, NULL);
		if (curr <= 0) {
			if (grp->ltype & PCB_LYT_SUBSTRATE) {
				if (flags & PCB_BRDTHICK_PRINT_ERROR) {
					if (namespace != NULL)
						rnd_message(RND_MSG_ERROR, "%s: ", namespace);
					rnd_message(RND_MSG_ERROR, "can not determine substrate thickness on layer group %ld - total board thickness is probably wrong\n", (long)gid);
				}
				if (flags & PCB_BRDTHICK_TOLERANT)
					continue;
				return -1;
			}
			else
				continue;
		}
		total += curr;
	}
	return total;
}

rnd_coord_t pcb_board_thickness(pcb_board_t *pcb, const char *namespace, pcb_board_thickness_flags_t flags)
{
	return pcb_stack_thickness(pcb, namespace, flags, 0, rnd_true, pcb->LayerGroups.len-1, rnd_true, PCB_LYT_COPPER | PCB_LYT_SUBSTRATE);
}

void pcb_board_changed_meta_ev(rnd_design_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	pcb_board_replaced();
}

void pcb_board_init(void)
{
	rnd_event_bind(RND_EVENT_DESIGN_META_CHANGED, pcb_board_changed_meta_ev, NULL, core_board_cookie);
}

void pcb_board_uninit(void)
{
	rnd_event_unbind_allcookie(core_board_cookie);
}
