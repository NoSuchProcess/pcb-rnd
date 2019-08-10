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
#include "hidlib_conf.h"
#include "plug_io.h"
#include "compat_misc.h"
#include "actions.h"
#include "paths.h"
#include "rtree.h"
#include "undo.h"
#include "draw.h"
#include "event.h"
#include "safe_fs.h"
#include "tool.h"
#include "layer.h"
#include "netlist.h"

pcb_board_t *PCB;

void pcb_board_free(pcb_board_t * pcb)
{
	int i;

	if (pcb == NULL)
		return;

	free(pcb->hidlib.name);
	free(pcb->hidlib.filename);
	free(pcb->PrintFilename);
	pcb_ratspatch_destroy(pcb);
	pcb_data_free(pcb->Data);

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
pcb_board_t *pcb_board_new_(pcb_bool SetDefaultNames)
{
	pcb_board_t *ptr;
	int i;

	/* allocate memory, switch all layers on and copy resources */
	ptr = calloc(1, sizeof(pcb_board_t));
	ptr->Data = pcb_buffer_new(ptr);

	for(i = 0; i < PCB_NUM_NETLISTS; i++)
		pcb_netlist_init(&(ptr->netlist[i]));

	pcb_conf_set(CFR_INTERNAL, "design/poly_isle_area", -1, "200000000", POL_OVERWRITE);

	ptr->RatDraw = pcb_false;

	/* NOTE: we used to set all the pcb flags on ptr here, but we don't need to do that anymore due to the new conf system */
	ptr->hidlib.grid = pcbhl_conf.editor.grid;

	ptr->hidlib.size_y = ptr->hidlib.size_x = PCB_MM_TO_COORD(20); /* should be overriden by the default design */
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
	conf_list_foreach_path_first(NULL, dpcb, &conf_core.rc.default_pcb_file, pcb_load_pcb(__path__, NULL, pcb_false, 1 | 0x10 | inh));
	pcb_io_err_inhibit_dec();

	if (dpcb != 0) { /* no default PCB in file, use embedded version */
		FILE *f;
		char *efn;
		const char *tmp_fn = ".pcb-rnd.default.pcb";

		/* We can parse from file only, make a temp file */
		f = pcb_fopen_fn(NULL, tmp_fn, "wb", &efn);
		if (f != NULL) {
			fwrite(default_pcb_internal, strlen(default_pcb_internal), 1, f);
			fclose(f);
			dpcb = pcb_load_pcb(efn, NULL, pcb_false, 1 | 0x10);
			if (dpcb == 0)
				pcb_message(PCB_MSG_WARNING, "Couldn't find default.pcb - using the embedded fallback\n");
			else
				pcb_message(PCB_MSG_ERROR, "Couldn't find default.pcb and failed to load the embedded fallback\n");
			pcb_remove(NULL, efn);
			free(efn);
		}
	}

	if (dpcb == 0) {
		nw = PCB;
		if (nw->hidlib.filename != NULL) {
			/* make sure the new PCB doesn't inherit the name and loader of the default pcb */
			free(nw->hidlib.filename);
			nw->hidlib.filename = NULL;
			nw->Data->loader = NULL;
		}
	}

	PCB = old;
	pcb_event(&PCB->hidlib, PCB_EVENT_ROUTE_STYLES_CHANGED, NULL);
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

	return 0;
}

void pcb_layer_colors_from_conf(pcb_board_t *ptr, int force)
{
	int i;

	/* copy default settings */
	for (i = 0; i < PCB_MAX_LAYER; i++)
		if (force || (ptr->Data->Layer[i].meta.real.color.str[0] == '\0'))
			memcpy(&ptr->Data->Layer[i].meta.real.color, pcb_layer_default_color(i, pcb_layer_flags(ptr, i)), sizeof(pcb_color_t));
}

typedef struct {
	int nplated;
	int nunplated;
} HoleCountStruct;

TODO("padstack: move this to obj_pstk.c after pinvia removal")
#include "obj_pstk_inlines.h"
static pcb_r_dir_t hole_counting_callback(const pcb_box_t * b, void *cl)
{
	pcb_pstk_t *ps = (pcb_pstk_t *)b;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);
	HoleCountStruct *hcs = (HoleCountStruct *)cl;

	if ((proto != NULL) && (proto->hdia > 0)) {
		if (proto->hplated)
			hcs->nplated++;
		else
			hcs->nunplated++;
	}
	return PCB_R_DIR_FOUND_CONTINUE;
}

static pcb_r_dir_t slot_counting_callback(const pcb_box_t *b, void *cl)
{
	pcb_pstk_t *ps = (pcb_pstk_t *)b;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);
	HoleCountStruct *hcs = (HoleCountStruct *)cl;

	if ((proto != NULL) && (proto->mech_idx >= 0)) {
		if (proto->hplated)
			hcs->nplated++;
		else
			hcs->nunplated++;
	}
	return PCB_R_DIR_FOUND_CONTINUE;
}

void pcb_board_count_holes(pcb_board_t *pcb, int *plated, int *unplated, const pcb_box_t *within_area)
{
	HoleCountStruct hcs = { 0, 0 };

	pcb_r_search(pcb->Data->padstack_tree, within_area, NULL, hole_counting_callback, &hcs, NULL);

	if (plated != NULL)
		*plated = hcs.nplated;
	if (unplated != NULL)
		*unplated = hcs.nunplated;
}

void pcb_board_count_slots(pcb_board_t *pcb, int *plated, int *unplated, const pcb_box_t *within_area)
{
	HoleCountStruct hcs = { 0, 0 };

	pcb_r_search(pcb->Data->padstack_tree, within_area, NULL, slot_counting_callback, &hcs, NULL);

	if (plated != NULL)
		*plated = hcs.nplated;
	if (unplated != NULL)
		*unplated = hcs.nunplated;
}

const char *pcb_board_get_filename(void)
{
	return PCB->hidlib.filename;
}

const char *pcb_board_get_name(void)
{
	return PCB->hidlib.name;
}

pcb_bool pcb_board_change_name(char *Name)
{
	free(PCB->hidlib.name);
	PCB->hidlib.name = Name;
	pcb_board_changed(0);
	return pcb_true;
}

void pcb_board_resize(pcb_coord_t Width, pcb_coord_t Height)
{
	PCB->hidlib.size_x = Width;
	PCB->hidlib.size_y = Height;

	/* crosshair range is different if pastebuffer-mode
	 * is enabled
	 */
	if (pcbhl_conf.editor.mode == PCB_MODE_PASTE_BUFFER)
		pcb_crosshair_set_range(PCB_PASTEBUFFER->X - PCB_PASTEBUFFER->BoundingBox.X1,
											PCB_PASTEBUFFER->Y - PCB_PASTEBUFFER->BoundingBox.Y1,
											MAX(0,
													Width - (PCB_PASTEBUFFER->BoundingBox.X2 -
																	 PCB_PASTEBUFFER->X)), MAX(0, Height - (PCB_PASTEBUFFER->BoundingBox.Y2 - PCB_PASTEBUFFER->Y)));
	else
		pcb_crosshair_set_range(0, 0, Width, Height);

	pcb_board_changed(0);
}

void pcb_board_remove(pcb_board_t *Ptr)
{
	pcb_undo_clear_list(pcb_true);
	pcb_board_free(Ptr);
	free(Ptr);
}

/* sets a new line thickness */
void pcb_board_set_line_width(pcb_coord_t Size)
{
	if (Size >= PCB_MIN_LINESIZE && Size <= PCB_MAX_LINESIZE) {
		conf_set_design("design/line_thickness", "%$mS", Size);
		if (conf_core.editor.auto_drc)
			pcb_crosshair_grid_fit(pcb_crosshair.X, pcb_crosshair.Y);
	}
}

/* sets a new via thickness */
void pcb_board_set_via_size(pcb_coord_t Size, pcb_bool Force)
{
	if (Force || (Size <= PCB_MAX_PINORVIASIZE && Size >= PCB_MIN_PINORVIASIZE && Size >= conf_core.design.via_drilling_hole + PCB_MIN_PINORVIACOPPER)) {
		conf_set_design("design/via_thickness", "%$mS", Size);
	}
}

/* sets a new via drilling hole */
void pcb_board_set_via_drilling_hole(pcb_coord_t Size, pcb_bool Force)
{
	if (Force || (Size <= PCB_MAX_PINORVIASIZE && Size >= PCB_MIN_PINORVIAHOLE && Size <= conf_core.design.via_thickness - PCB_MIN_PINORVIACOPPER)) {
		conf_set_design("design/via_drilling_hole", "%$mS", Size);
	}
}

/* sets a clearance width */
void pcb_board_set_clearance(pcb_coord_t Width)
{
	if (Width <= PCB_MAX_LINESIZE) {
		conf_set_design("design/clearance", "%$mS", Width);
	}
}

/* sets a text scaling */
void pcb_board_set_text_scale(int Scale)
{
	if (Scale <= PCB_MAX_TEXTSCALE && Scale >= PCB_MIN_TEXTSCALE) {
		conf_set_design("design/text_scale", "%d", Scale);
	}
}

/* sets or resets changed flag and redraws status */
void pcb_board_set_changed_flag(pcb_bool New)
{
	pcb_bool old = PCB->Changed;

	PCB->Changed = New;

	if (old != New)
		pcb_event(&PCB->hidlib, PCB_EVENT_BOARD_META_CHANGED, NULL);
}


void pcb_board_changed(int reverted)
{
	if ((pcb_gui != NULL) && (pcb_gui->set_hidlib != NULL))
		pcb_gui->set_hidlib(pcb_gui, &PCB->hidlib);
	pcb_event(&PCB->hidlib, PCB_EVENT_BOARD_CHANGED, "i", reverted);
}

int pcb_board_normalize(pcb_board_t *pcb)
{
	pcb_box_t b;
	int chg = 0;
	
	if (pcb_data_bbox(&b, pcb->Data, pcb_false) == NULL)
		return -1;

	if ((b.X2 - b.X1) > pcb->hidlib.size_x) {
		pcb->hidlib.size_x = b.X2 - b.X1;
		chg++;
	}

	if ((b.Y2 - b.Y1) > pcb->hidlib.size_y) {
		pcb->hidlib.size_y = b.Y2 - b.Y1;
		chg++;
	}

	chg += pcb_data_normalize_(pcb->Data, &b);

	return chg;
}

pcb_pixmap_t *pcb_pixmap_insert_or_free(pcb_board_t *pcb, pcb_pixmap_t *pm)
{

}

