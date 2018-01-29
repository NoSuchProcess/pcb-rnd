/*
 *														COPYRIGHT
 *
 *	pcb-rnd, interactive printed circuit board design
 *	Copyright (C) 2016 Tibor 'Igor2' Palinkas
 *	Copyright (C) 2016 Erich S. Heinzle
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */
#include <math.h>
#include "config.h"
#include "compat_misc.h"
#include "board.h"
#include "plug_io.h"
#include "error.h"
#include "uniq_name.h"
#include "data.h"
#include "write.h"
#include "layer.h"
#include "const.h"
#include "netlist.h"
#include "obj_all.h"

#include "../src_plugins/lib_compat_help/pstk_compat.h"

/* layer "0" is first copper layer = "0. Back - Solder"
 * and layer "15" is "15. Front - Component"
 * and layer "20" SilkScreen Back
 * and layer "21" SilkScreen Front
 */


/* generates a default via drill size for the layout */
static int write_kicad_layout_via_drill_size(FILE *FP, pcb_cardinal_t indentation);

/* writes the buffer to file */
int io_kicad_write_buffer(pcb_plug_io_t *ctx, FILE *FP, pcb_buffer_t *buff, pcb_bool elem_only)
{
	pcb_message(PCB_MSG_ERROR, "can't save buffer in s-expr yet, please use kicad legacy for this\n");
	return -1;
}

#define KICAD_MAX_LAYERS 64
typedef struct {
	FILE *f;                                     /* output file */
	pcb_board_t *pcb;                            /* board being exported */
	struct {
		pcb_layergrp_t *grp;
		char name[32];             /* kicad layer name */
		const char *param;
	} layer[KICAD_MAX_LAYERS];   /* pcb-rnd layer groups indexed by kicad's stackup */
	int num_layers;              /* number of groups in use */
} wctx_t;

typedef enum {
	FLP_COP_FIRST,
	FLP_COP_INT,
	FLP_COP_LAST,
	FLP_MISC
} fixed_layer_place_t;

typedef struct {
	char *name;
	char *param;
	pcb_layer_type_t type;
	fixed_layer_place_t place;
} fixed_layer_t;

static fixed_layer_t fixed_layers[] = {
	{"F.Cu",       "signal", PCB_LYT_COPPER | PCB_LYT_TOP,    FLP_COP_FIRST },
	{"Inner%d.Cu", "signal", PCB_LYT_COPPER | PCB_LYT_BOTTOM, FLP_COP_INT },
	{"B.Cu",       "signal", PCB_LYT_COPPER | PCB_LYT_BOTTOM, FLP_COP_LAST },
	{"F.Paste",    "user",   PCB_LYT_PASTE | PCB_LYT_TOP,     FLP_MISC },
	{"B.Paste",    "user",   PCB_LYT_PASTE | PCB_LYT_BOTTOM,  FLP_MISC },
	{"F.SilkS",    "user",   PCB_LYT_SILK | PCB_LYT_TOP,      FLP_MISC },
	{"B.SilkS",    "user",   PCB_LYT_SILK | PCB_LYT_BOTTOM,   FLP_MISC },
	{"F.Maks",     "user",   PCB_LYT_MASK | PCB_LYT_TOP,      FLP_MISC },
	{"B.Mask",     "user",   PCB_LYT_MASK | PCB_LYT_BOTTOM,   FLP_MISC },
	{"Edge.Cuts",  "user",   PCB_LYT_OUTLINE,                 FLP_MISC },
	{NULL, NULL, 0}
};

/* number of copper layers, where to start inners, how to increment iners */
#define KICAD_COPPERS       16
#define KICAD_FIRST_INNER   1
#define KICAD_NEXT_INNER   +1
#define KICAD_FIRST_MISC    KICAD_COPPERS

#define grp_idx(grp) ((grp) - ctx->pcb->LayerGroups->grp)

/* Make the mapping between pcb-rnd layer groups and kicad layer stackup
   Assumption: the first KICAD_COPPERS layers are the copper layers */
static int kicad_map_layers(wctx_t *ctx)
{

	fixed_layer_t *fl;
	int n;
	int inner = KICAD_FIRST_INNER;
	int misc = KICAD_FIRST_MISC;
	char *mapped;

	mapped = calloc(ctx->pcb->LayerGroups.len, 1);
	ctx->num_layers = -1;

	for(fl = fixed_layers; fl->name != NULL; fl++) {
		pcb_layergrp_id_t gid[KICAD_COPPERS];
		int idx, ngrp;

		gid[0] = -1;
		ngrp = pcb_layergrp_list(ctx->pcb, fl->type, gid, KICAD_COPPERS);
		if ((fl->place != FLP_COP_INT) && (ngrp > 1))
			pcb_io_incompat_save(ctx->pcb->Data, NULL, "Multiple layer groups of the same type - exporting only the first", "Merge the groups");

		switch(fl->place) {
			case FLP_COP_FIRST:
				idx = 0;
				break;
			case FLP_COP_INT:
				/* iterate over all internal groups */
				for(n = 0; n < ngrp; n++) {
					idx = inner;
					if (inner >= KICAD_COPPERS-1) {
						pcb_io_incompat_save(ctx->pcb->Data, NULL, "Too many internal layer groups, omitting some", "Use fewer groups");
						continue;
					}
					sprintf(ctx->layer[idx].name, fl->name, idx);
					if (gid[n] >= 0) {
						inner += KICAD_NEXT_INNER;
						ctx->layer[idx].grp = &ctx->pcb->LayerGroups.grp[gid[n]];
						ctx->layer[idx].param = fl->param;
						mapped[gid[n]]++;
						if (idx > ctx->num_layers)
							ctx->num_layers = idx;
					}
				}
				continue; /* the normal layer insertion does only a single group, skip it */
			case FLP_COP_LAST:
				idx = KICAD_COPPERS - 1;
				break;
			case FLP_MISC:
				idx = misc;
				if (gid[0] >= 0)
					misc++;
				if (misc >= KICAD_MAX_LAYERS-1) {
					pcb_io_incompat_save(ctx->pcb->Data, NULL, "Too many internal layer groups, omitting some", "Use fewer groups");
					continue;
				}
				break;
		}

		/* normal, single group mapping */
		if (gid[0] >= 0) {
			strcpy(ctx->layer[idx].name, fl->name);
			ctx->layer[idx].grp = &ctx->pcb->LayerGroups.grp[gid[0]];
			ctx->layer[idx].param = fl->param;
			mapped[gid[0]]++;
			if (idx > ctx->num_layers)
				ctx->num_layers = idx;
		}
	}

	for(n = 0; n < ctx->pcb->LayerGroups.len; n++) {
		if (mapped[n] == 0)
			pcb_io_incompat_save(ctx->pcb->Data, NULL, "Failed to map layer for export - layer omitted", NULL);
		else if (mapped[n] > 1)
			pcb_io_incompat_save(ctx->pcb->Data, NULL, "Mapped layer for export multiple times", NULL);
	}

	free(mapped);
	ctx->num_layers++;

	if (ctx->num_layers < 3)
		pcb_io_incompat_save(ctx->pcb->Data, NULL, "Warning: low output layer count", NULL);
	return 0;
}

static void kicad_print_layers(wctx_t *ctx, int ind)
{
	int n;

	fprintf(ctx->f, "\n%*s(layers\n", ind, "");
	for(n = 0; n < ctx->num_layers; n++)
		if (ctx->layer[n].param != NULL)
			fprintf(ctx->f, "%*s(%d %s %s)\n", ind+2, "", n, ctx->layer[n].name, ctx->layer[n].param);
	fprintf(ctx->f, "%*s)\n", ind, "");
}

static const char *kicad_sexpr_layer_to_text(wctx_t *ctx, int layer)
{
	if ((layer < 0) || (layer >= ctx->num_layers)) {
		assert(!"invalid layer");
		return "";
	}
	return ctx->layer[layer].name;
}

/* ---------------------------------------------------------------------------
 * writes PCB to file in s-expression format
 */
int io_kicad_write_pcb(pcb_plug_io_t *ctx, FILE *FP, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	wctx_t wctx;

	int baseSExprIndent = 2;

	pcb_cardinal_t i;
	int physicalLayerCount = 0;
	int layer = 0;
	int currentKicadLayer = 0;
	int currentGroup = 0;
	pcb_coord_t outlineThickness = PCB_MIL_TO_COORD(10);

	pcb_coord_t LayoutXOffset;
	pcb_coord_t LayoutYOffset;

	/* Kicad expects a layout "sheet" size to be specified in mils, and A4, A3 etc... */
	int A4HeightMil = 8267;
	int A4WidthMil = 11700;
	int sheetHeight = A4HeightMil;
	int sheetWidth = A4WidthMil;
	int paperSize = 4; /* default paper size is A4 */

	memset(&wctx, 0, sizeof(wctx));
	wctx.pcb = PCB;
	wctx.f = FP;

	/* Kicad string quoting pattern: protect parenthesis, whitespace, quote and backslash */
	pcb_printf_slot[4] = "%{\\()\t\r\n \"}mq";

#warning TODO: DO NOT fake we are kicad - print pcb-rnd and pcb-rnd version info in the quotes
	fputs("(kicad_pcb (version 3) (host pcbnew \"(2013-02-20 BZR 3963)-testing\")", FP);

	fprintf(FP, "\n%*s(general\n", baseSExprIndent, "");
	fprintf(FP, "%*s)\n", baseSExprIndent, "");

#warning TODO: rewrite this: rather have a table and a loop that hardwired calculations in code
	/* we sort out the needed kicad sheet size here, using A4, A3, A2, A1 or A0 size as needed */
	if (PCB_COORD_TO_MIL(PCB->MaxWidth) > A4WidthMil || PCB_COORD_TO_MIL(PCB->MaxHeight) > A4HeightMil) {
		sheetHeight = A4WidthMil; /* 11.7" */
		sheetWidth = 2 * A4HeightMil; /* 16.5" */
		paperSize = 3; /* this is A3 size */
	}
	if (PCB_COORD_TO_MIL(PCB->MaxWidth) > sheetWidth || PCB_COORD_TO_MIL(PCB->MaxHeight) > sheetHeight) {
		sheetHeight = 2 * A4HeightMil; /* 16.5" */
		sheetWidth = 2 * A4WidthMil; /* 23.4" */
		paperSize = 2; /* this is A2 size */
	}
	if (PCB_COORD_TO_MIL(PCB->MaxWidth) > sheetWidth || PCB_COORD_TO_MIL(PCB->MaxHeight) > sheetHeight) {
		sheetHeight = 2 * A4WidthMil; /* 23.4" */
		sheetWidth = 4 * A4HeightMil; /* 33.1" */
		paperSize = 1; /* this is A1 size */
	}
	if (PCB_COORD_TO_MIL(PCB->MaxWidth) > sheetWidth || PCB_COORD_TO_MIL(PCB->MaxHeight) > sheetHeight) {
		sheetHeight = 4 * A4HeightMil; /* 33.1" */
		sheetWidth = 4 * A4WidthMil; /* 46.8"  */
		paperSize = 0; /* this is A0 size; where would you get it made ?!?! */
	}
	fprintf(FP, "\n%*s(page A%d)\n", baseSExprIndent, "", paperSize);


	/* we now sort out the offsets for centring the layout in the chosen sheet size here */
	if (sheetWidth > PCB_COORD_TO_MIL(PCB->MaxWidth)) { /* usually A4, bigger if needed */
		/* fprintf(FP, "%d ", sheetWidth);  legacy kicad: elements decimils, sheet size mils */
		LayoutXOffset = PCB_MIL_TO_COORD(sheetWidth) / 2 - PCB->MaxWidth / 2;
	}
	else { /* the layout is bigger than A0; most unlikely, but... */
		/* pcb_fprintf(FP, "%.0ml ", PCB->MaxWidth); */
		LayoutXOffset = 0;
	}
	if (sheetHeight > PCB_COORD_TO_MIL(PCB->MaxHeight)) {
		/* fprintf(FP, "%d", sheetHeight); */
		LayoutYOffset = PCB_MIL_TO_COORD(sheetHeight) / 2 - PCB->MaxHeight / 2;
	}
	else { /* the layout is bigger than A0; most unlikely, but... */
		/* pcb_fprintf(FP, "%.0ml", PCB->MaxHeight); */
		LayoutYOffset = 0;
	}



	kicad_map_layers(&wctx);
	kicad_print_layers(&wctx, baseSExprIndent);

	/* setup section */
	fprintf(FP, "\n%*s(setup\n", baseSExprIndent, "");
	write_kicad_layout_via_drill_size(FP, baseSExprIndent + 2);
	fprintf(FP, "%*s)\n", baseSExprIndent, "");

	/* now come the netlist "equipotential" descriptors */

	write_kicad_equipotential_netlists(FP, PCB, baseSExprIndent);

	/* module descriptions come next */
	fputs("\n", FP);
	write_kicad_layout_elements(FP, PCB, PCB->Data, LayoutXOffset, LayoutYOffset, baseSExprIndent);
	write_kicad_layout_subc(FP, PCB, PCB->Data, LayoutXOffset, LayoutYOffset, baseSExprIndent);



#if 0
old, per layer export

	/* we now proceed to write the bottom silk lines, arcs, text to the kicad legacy file, using layer 20 */
	currentKicadLayer = 20; /* 20 is the bottom silk layer in kicad */
	for(i = 0; i < bottomSilkCount; i++) { /* write bottom silk lines, if any */
		write_kicad_layout_tracks(FP, currentKicadLayer, &(PCB->Data->Layer[bottomSilk[i]]), LayoutXOffset, LayoutYOffset, baseSExprIndent);
		write_kicad_layout_arcs(FP, currentKicadLayer, &(PCB->Data->Layer[bottomSilk[i]]), LayoutXOffset, LayoutYOffset, baseSExprIndent);
		write_kicad_layout_text(FP, currentKicadLayer, &(PCB->Data->Layer[bottomSilk[i]]), LayoutXOffset, LayoutYOffset, baseSExprIndent);
	}

	/* we now proceed to write the bottom copper text to the kicad legacy file, layer by layer */
	currentKicadLayer = 0; /* 0 is the bottom copper layer in kicad */
	for(i = 0; i < bottomCount; i++) { /* write bottom copper tracks, if any */
		write_kicad_layout_text(FP, currentKicadLayer, &(PCB->Data->Layer[bottomLayers[i]]), LayoutXOffset, LayoutYOffset, baseSExprIndent);
	} /* 0 is the bottom most track in kicad */

	/* we now proceed to write the internal copper text to the kicad file, layer by layer */
	if (innerCount != 0) {
		currentGroup = pcb_layer_get_group(PCB, innerLayers[0]);
	}
	for(i = 0, currentKicadLayer = 1; i < innerCount; i++) { /* write inner copper text, group by group */
		if (currentGroup != pcb_layer_get_group(PCB, innerLayers[i])) {
			currentGroup = pcb_layer_get_group(PCB, innerLayers[i]);
			currentKicadLayer++;
			if (currentKicadLayer > 14) {
				currentKicadLayer = 14; /* kicad 16 layers in total, 0...15 */
			}
		}
		write_kicad_layout_text(FP, currentKicadLayer, &(PCB->Data->Layer[innerLayers[i]]), LayoutXOffset, LayoutYOffset, baseSExprIndent);
	}

	/* we now proceed to write the top copper text to the kicad legacy file, layer by layer */
	currentKicadLayer = 15; /* 15 is the top most copper layer in kicad */
	for(i = 0; i < topCount; i++) { /* write top copper tracks, if any */
		write_kicad_layout_text(FP, currentKicadLayer, &(PCB->Data->Layer[topLayers[i]]), LayoutXOffset, LayoutYOffset, baseSExprIndent);
	}

	/* we now proceed to write the top silk lines, arcs, text to the kicad legacy file, using layer 21 */
	currentKicadLayer = 21; /* 21 is the top silk layer in kicad */
	for(i = 0; i < topSilkCount; i++) { /* write top silk lines, if any */
		write_kicad_layout_tracks(FP, currentKicadLayer, &(PCB->Data->Layer[topSilk[i]]), LayoutXOffset, LayoutYOffset, baseSExprIndent);
		write_kicad_layout_arcs(FP, currentKicadLayer, &(PCB->Data->Layer[topSilk[i]]), LayoutXOffset, LayoutYOffset, baseSExprIndent);
		write_kicad_layout_text(FP, currentKicadLayer, &(PCB->Data->Layer[topSilk[i]]), LayoutXOffset, LayoutYOffset, baseSExprIndent);
	}

	/* having done the graphical elements, we move onto tracks and vias */

	fputs("\n", FP); /* move onto tracks and vias */
	write_kicad_layout_vias(FP, PCB->Data, LayoutXOffset, LayoutYOffset, baseSExprIndent);

	/* we now proceed to write the bottom copper tracks to the kicad legacy file, layer by layer */
	currentKicadLayer = 0; /* 0 is the bottom copper layer in kicad */
	for(i = 0; i < bottomCount; i++) { /* write bottom copper tracks, if any */
		write_kicad_layout_tracks(FP, currentKicadLayer, &(PCB->Data->Layer[bottomLayers[i]]), LayoutXOffset, LayoutYOffset, baseSExprIndent);
		write_kicad_layout_arcs(FP, currentKicadLayer, &(PCB->Data->Layer[bottomLayers[i]]), LayoutXOffset, LayoutYOffset, baseSExprIndent);
	} /* 0 is the bottom most track in kicad */

	/* we now proceed to write the internal copper tracks to the kicad file, layer by layer */
	if (innerCount != 0) {
		currentGroup = pcb_layer_get_group(PCB, innerLayers[0]);
	}
	for(i = 0, currentKicadLayer = 1; i < innerCount; i++) { /* write inner copper tracks, group by group */
		if (currentGroup != pcb_layer_get_group(PCB, innerLayers[i])) {
			currentGroup = pcb_layer_get_group(PCB, innerLayers[i]);
			currentKicadLayer++;
			if (currentKicadLayer > 14) {
				currentKicadLayer = 14; /* kicad 16 layers in total, 0...15 */
			}
		}
		write_kicad_layout_tracks(FP, currentKicadLayer, &(PCB->Data->Layer[innerLayers[i]]), LayoutXOffset, LayoutYOffset, baseSExprIndent);
		write_kicad_layout_arcs(FP, currentKicadLayer, &(PCB->Data->Layer[innerLayers[i]]), LayoutXOffset, LayoutYOffset, baseSExprIndent);
	}

	/* we now proceed to write the top copper tracks to the kicad legacy file, layer by layer */
	currentKicadLayer = 15; /* 15 is the top most copper layer in kicad */
	for(i = 0; i < topCount; i++)
	{ /* write top copper tracks, if any */
		write_kicad_layout_tracks(FP, currentKicadLayer, &(PCB->Data->Layer[topLayers[i]]), LayoutXOffset, LayoutYOffset, baseSExprIndent);
		write_kicad_layout_arcs(FP, currentKicadLayer, &(PCB->Data->Layer[topLayers[i]]), LayoutXOffset, LayoutYOffset, baseSExprIndent);
	}

	/* we now proceed to write the outline tracks to the kicad file, layer by layer */
	currentKicadLayer = 28; /* 28 is the edge cuts layer in kicad */
	if (outlineCount > 0) {
		for(i = 0; i < outlineCount; i++) { /* write outline tracks, if any */
			write_kicad_layout_tracks(FP, currentKicadLayer, &(PCB->Data->Layer[outlineLayers[i]]), LayoutXOffset, LayoutYOffset, baseSExprIndent);
			write_kicad_layout_arcs(FP, currentKicadLayer, &(PCB->Data->Layer[outlineLayers[i]]), LayoutXOffset, LayoutYOffset, baseSExprIndent);
		}
	}
	else { /* no outline layer per se, export the board margins instead  - obviously some scope to reduce redundant code... */
		fprintf(FP, "%*s", baseSExprIndent, "");
		pcb_fprintf(FP, "(segment (start %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n",
								PCB->MaxWidth / 2 - LayoutXOffset, PCB->MaxHeight / 2 - LayoutYOffset, PCB->MaxWidth / 2 + LayoutXOffset, PCB->MaxHeight / 2 - LayoutYOffset, kicad_sexpr_layer_to_text(currentKicadLayer), outlineThickness);
		fprintf(FP, "%*s", baseSExprIndent, "");
		pcb_fprintf(FP, "(segment (start %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n",
								PCB->MaxWidth / 2 + LayoutXOffset, PCB->MaxHeight / 2 - LayoutYOffset, PCB->MaxWidth / 2 + LayoutXOffset, PCB->MaxHeight / 2 + LayoutYOffset, kicad_sexpr_layer_to_text(currentKicadLayer), outlineThickness);
		fprintf(FP, "%*s", baseSExprIndent, "");
		pcb_fprintf(FP, "(segment (start %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n",
								PCB->MaxWidth / 2 + LayoutXOffset, PCB->MaxHeight / 2 + LayoutYOffset, PCB->MaxWidth / 2 - LayoutXOffset, PCB->MaxHeight / 2 + LayoutYOffset, kicad_sexpr_layer_to_text(currentKicadLayer), outlineThickness);
		fprintf(FP, "%*s", baseSExprIndent, "");
		pcb_fprintf(FP, "(segment (start %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n",
								PCB->MaxWidth / 2 - LayoutXOffset, PCB->MaxHeight / 2 + LayoutYOffset, PCB->MaxWidth / 2 - LayoutXOffset, PCB->MaxHeight / 2 - LayoutYOffset, kicad_sexpr_layer_to_text(currentKicadLayer), outlineThickness);
	}

	fputs("\n", FP); /* finished  tracks and vias */
	/*
	 * now we proceed to write polygons for each layer, and iterate much like we did for tracks
	 */

	/* we now proceed to write the bottom silk polygons  to the kicad legacy file, using layer 20 */
	currentKicadLayer = 20; /* 20 is the bottom silk layer in kicad */
	for(i = 0; i < bottomSilkCount; i++) { /* write bottom silk polygons, if any */
		write_kicad_layout_polygons(FP, currentKicadLayer, &(PCB->Data->Layer[bottomSilk[i]]), LayoutXOffset, LayoutYOffset, baseSExprIndent);
	}

	/* we now proceed to write the bottom copper polygons to the kicad legacy file, layer by layer */
	currentKicadLayer = 0; /* 0 is the bottom copper layer in kicad */
	for(i = 0; i < bottomCount; i++) { /* write bottom copper polygons, if any */
		write_kicad_layout_polygons(FP, currentKicadLayer, &(PCB->Data->Layer[bottomLayers[i]]), LayoutXOffset, LayoutYOffset, baseSExprIndent);
	} /* 0 is the bottom most track in kicad */

	/* we now proceed to write the internal copper polygons to the kicad file, layer by layer */
	if (innerCount != 0) {
		currentGroup = pcb_layer_get_group(PCB, innerLayers[0]);
	}
	for(i = 0, currentKicadLayer = 1; i < innerCount; i++) { /* write inner copper polygons, group by group */
		if (currentGroup != pcb_layer_get_group(PCB, innerLayers[i])) {
			currentGroup = pcb_layer_get_group(PCB, innerLayers[i]);
			currentKicadLayer++;
			if (currentKicadLayer > 14) {
				currentKicadLayer = 14; /* kicad 16 layers in total, 0...15 */
			}
		}
		write_kicad_layout_polygons(FP, currentKicadLayer, &(PCB->Data->Layer[innerLayers[i]]), LayoutXOffset, LayoutYOffset, baseSExprIndent);
	}

	/* we now proceed to write the top copper polygons to the kicad legacy file, layer by layer */
	currentKicadLayer = 15; /* 15 is the top most copper layer in kicad */
	for(i = 0; i < topCount; i++) { /* write top copper polygons, if any */
		write_kicad_layout_polygons(FP, currentKicadLayer, &(PCB->Data->Layer[topLayers[i]]), LayoutXOffset, LayoutYOffset, baseSExprIndent);
	}

	/* we now proceed to write the top silk polygons to the kicad legacy file, using layer 21 */
	currentKicadLayer = 21; /* 21 is the top silk layer in kicad */
	for(i = 0; i < topSilkCount; i++) { /* write top silk polygons, if any */
		write_kicad_layout_polygons(FP, currentKicadLayer, &(PCB->Data->Layer[topSilk[i]]), LayoutXOffset, LayoutYOffset, baseSExprIndent);
	}
#endif


	fputs(")\n", FP); /* finish off the board */


	return 0;
}


/* ---------------------------------------------------------------------------
 * writes kicad format via data
 For a track segment:
 Position shape Xstart Ystart Xend Yend width
 Description layer 0 netcode timestamp status
 Shape parameter is set to 0 (reserved for futu
*/


int write_kicad_layout_vias(FILE *FP, pcb_data_t *Data, pcb_coord_t xOffset, pcb_coord_t yOffset, pcb_cardinal_t indentation)
{
	gdl_iterator_t it;
	pcb_pin_t *via;
	pcb_pstk_t *ps;

	/* write information about vias */
	pinlist_foreach(&Data->Via, &it, via) {
		fprintf(FP, "%*s", indentation, "");
		pcb_fprintf(FP, "(via (at %.3mm %.3mm) (size %.3mm) (layers %s %s))\n", via->X + xOffset, via->Y + yOffset, via->Thickness, kicad_sexpr_layer_to_text(NULL, 0), kicad_sexpr_layer_to_text(NULL, 15)); /* skip (net 0) for now */
	}
	padstacklist_foreach(&Data->padstack, &it, ps) {
		int klayer_from = 0, klayer_to = 15;
		pcb_coord_t x, y, drill_dia, pad_dia, clearance, mask;
		pcb_pstk_compshape_t cshape;
		pcb_bool plated;

		if (!pcb_pstk_export_compat_via(ps, &x, &y, &drill_dia, &pad_dia, &clearance, &mask, &cshape, &plated)) {
			pcb_io_incompat_save(Data, (pcb_any_obj_t *)ps, "Can not convert padstack to old-style via", "Use round, uniform-shaped vias only");
			continue;
		}

		if (cshape != PCB_PSTK_COMPAT_ROUND) {
			pcb_io_incompat_save(Data, (pcb_any_obj_t *)ps, "Can not convert padstack to via", "only round vias are supported");
			continue;
		}
#warning TODO: set klayer_from and klayer_to using bb span of ps

		fprintf(FP, "%*s", indentation, "");
		pcb_fprintf(FP, "(via (at %.3mm %.3mm) (size %.3mm) (layers %s %s))\n",
			x + xOffset, y + yOffset, pad_dia,
			kicad_sexpr_layer_to_text(NULL, klayer_from), kicad_sexpr_layer_to_text(NULL, klayer_to)
			); /* skip (net 0) for now */
	}
	return 0;
}


static int write_kicad_layout_via_drill_size(FILE *FP, pcb_cardinal_t indentation)
{
	fprintf(FP, "%*s", indentation, "");
	pcb_fprintf(FP, "(via_drill 0.635)\n"); /* mm format, default for now, ~= 0.635mm */
	return 0;
}

int write_kicad_layout_tracks(FILE *FP, pcb_cardinal_t number, pcb_layer_t *layer, pcb_coord_t xOffset, pcb_coord_t yOffset, pcb_cardinal_t indentation)
{
	gdl_iterator_t it;
	pcb_line_t *line;
	pcb_cardinal_t currentLayer = number;

	/* write information about non empty layers */
	if (!pcb_layer_is_empty_(PCB, layer) || (layer->name && *layer->name)) {
		/*
		   fprintf(FP, "Layer(%i ", (int) Number + 1);
		   pcb_print_quoted_string(FP, (char *) PCB_EMPTY(layer->name));
		   fputs(")\n(\n", FP);
		   WriteAttributeList(FP, &layer->Attributes, "\t");
		 */
		int localFlag = 0;
		linelist_foreach(&layer->Line, &it, line) {
			if ((currentLayer < 16) || (currentLayer == 28)) { /* a copper line i.e. track, or an outline edge cut */
				fprintf(FP, "%*s", indentation, "");
				pcb_fprintf(FP, "(segment (start %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n", line->Point1.X + xOffset, line->Point1.Y + yOffset, line->Point2.X + xOffset, line->Point2.Y + yOffset, kicad_sexpr_layer_to_text(NULL, currentLayer), line->Thickness); /* neglect (net ___ ) for now */
			}
			else if ((currentLayer == 20) || (currentLayer == 21) || (currentLayer == 28)) { /* a silk line or outline line */
				fprintf(FP, "%*s", indentation, "");
				pcb_fprintf(FP, "(gr_line (start %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n",
										line->Point1.X + xOffset, line->Point1.Y + yOffset, line->Point2.X + xOffset, line->Point2.Y + yOffset, kicad_sexpr_layer_to_text(NULL, currentLayer), line->Thickness);
			}
			localFlag |= 1;
		}
		return localFlag;
	}
	else {
		return 0;
	}
}

int write_kicad_layout_arcs(FILE *FP, pcb_cardinal_t number, pcb_layer_t *layer, pcb_coord_t xOffset, pcb_coord_t yOffset, pcb_cardinal_t indentation)
{
	gdl_iterator_t it;
	pcb_arc_t *arc;
	pcb_arc_t localArc; /* for converting ellipses to circular arcs */
	pcb_cardinal_t currentLayer = number;
	pcb_coord_t radius, xStart, yStart, xEnd, yEnd;
	int copperStartX; /* used for mapping geda copper arcs onto kicad copper lines */
	int copperStartY; /* used for mapping geda copper arcs onto kicad copper lines */

	/* write information about non empty layers */
	if (!pcb_layer_is_empty_(PCB, layer) || (layer->name && *layer->name)) {
		/*
		   fprintf(FP, "Layer(%i ", (int) Number + 1);
		   pcb_print_quoted_string(FP, (char *) PCB_EMPTY(layer->name));
		   fputs(")\n(\n", FP);
		   WriteAttributeList(FP, &layer->Attributes, "\t");
		 */
		int localFlag = 0;
		int kicadArcShape = 2; /* 3 = circle, and 2 = arc, 1= rectangle used in eeschema only */
		arclist_foreach(&layer->Arc, &it, arc) {
			localArc = *arc;
			if (arc->Width > arc->Height) {
				radius = arc->Height;
				localArc.Width = radius;
			}
			else {
				radius = arc->Width;
				localArc.Height = radius;
			}

/* Return the x;y coordinate of the endpoint of an arc; if which is 0, return
   the endpoint that corresponds to StartAngle, else return the end angle's.
void pcb_arc_get_end(pcb_arc_t *Arc, int which, pcb_coord_t *x, pcb_coord_t *y);

Obsolete: please use pcb_arc_get_end() instead
pcb_box_t *pcb_arc_get_ends(pcb_arc_t *Arc); */

			if (arc->Delta == 360.0 || arc->Delta == -360.0) { /* it's a circle */
				kicadArcShape = 3;
			}
			else { /* it's an arc */
				kicadArcShape = 2;
			}
			xStart = localArc.X + xOffset;
			yStart = localArc.Y + yOffset;
			pcb_arc_get_end(&localArc, 1, &xEnd, &yEnd);
			xEnd += xOffset;
			yEnd += yOffset;
			pcb_arc_get_end(&localArc, 0, &copperStartX, &copperStartY);
			copperStartX += xOffset;
			copperStartY += yOffset;
			if ((currentLayer < 16)) { /*|| (currentLayer == 28)) {  a copper arc, i.e. track, but not edge cut, is unsupported by kicad, and will be exported as a line */
				fprintf(FP, "%*s", indentation, "");
				pcb_fprintf(FP, "(segment (start %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n", copperStartX, copperStartY, xEnd, yEnd, kicad_sexpr_layer_to_text(NULL, currentLayer), arc->Thickness); /* neglect (net ___ ) for now */
			}
			else if ((currentLayer == 20) || (currentLayer == 21) || (currentLayer == 28)) { /* a silk arc, or outline */
				fprintf(FP, "%*s", indentation, "");
				pcb_fprintf(FP, "(gr_arc (start %.3mm %.3mm) (end %.3mm %.3mm) (angle %ma) (layer %s) (width %.3mm))\n", xStart, yStart, xEnd, yEnd, arc->Delta, kicad_sexpr_layer_to_text(NULL, currentLayer), arc->Thickness);
			}
			localFlag |= 1;
		}
		return localFlag;
	}
	else {
		return 0;
	}
}

int write_kicad_layout_text(FILE *FP, pcb_cardinal_t number, pcb_layer_t *layer, pcb_coord_t xOffset, pcb_coord_t yOffset, pcb_cardinal_t indentation)
{
	pcb_font_t *myfont = pcb_font(PCB, 0, 1);
	pcb_coord_t mWidth = myfont->MaxWidth; /* kicad needs the width of the widest letter */
	pcb_coord_t defaultStrokeThickness = 100 * 2540; /* use 100 mil as default 100% stroked font line thickness */
	int kicadMirrored = 1; /* 1 is not mirrored, 0  is mirrored */

	pcb_coord_t defaultXSize;
	pcb_coord_t defaultYSize;
	pcb_coord_t strokeThickness;
	int rotation;
	pcb_coord_t textOffsetX;
	pcb_coord_t textOffsetY;
	pcb_coord_t halfStringWidth;
	pcb_coord_t halfStringHeight;
	int localFlag;

	gdl_iterator_t it;
	pcb_text_t *text;
	pcb_cardinal_t currentLayer = number;

	/* write information about non empty layers */
	if (!pcb_layer_is_empty_(PCB, layer) || (layer->name && *layer->name)) {
		/*
		   fprintf(FP, "Layer(%i ", (int) Number + 1);
		   pcb_print_quoted_string(FP, (char *) PCB_EMPTY(layer->name));
		   fputs(")\n(\n", FP);
		   WriteAttributeList(FP, &layer->Attributes, "\t");
		 */
		localFlag = 0;
		textlist_foreach(&layer->Text, &it, text) {
			if ((currentLayer < 16) || (currentLayer == 20) || (currentLayer == 21)) { /* copper or silk layer text */
				pcb_fprintf(FP, "%*s(gr_text %[4] ", indentation, "", text->TextString);
				defaultXSize = 5 * PCB_SCALE_TEXT(mWidth, text->Scale) / 6; /* IIRC kicad treats this as kerned width of upper case m */
				defaultYSize = defaultXSize;
				strokeThickness = PCB_SCALE_TEXT(defaultStrokeThickness, text->Scale / 2);
				rotation = 0;
				textOffsetX = 0;
				textOffsetY = 0;
				halfStringWidth = (text->BoundingBox.X2 - text->BoundingBox.X1) / 2;
				if (halfStringWidth < 0) {
					halfStringWidth = -halfStringWidth;
				}
				halfStringHeight = (text->BoundingBox.Y2 - text->BoundingBox.Y1) / 2;
				if (halfStringHeight < 0) {
					halfStringHeight = -halfStringHeight;
				}
				if (text->Direction == 3) { /*vertical down */
					if (currentLayer == 0 || currentLayer == 20) { /* back copper or silk */
						rotation = 2700;
						kicadMirrored = 0; /* mirrored */
						textOffsetY -= halfStringHeight;
						textOffsetX -= 2 * halfStringWidth; /* was 1*hsw */
					}
					else { /* front copper or silk */
						rotation = 2700;
						kicadMirrored = 1; /* not mirrored */
						textOffsetY = halfStringHeight;
						textOffsetX -= halfStringWidth;
					}
				}
				else if (text->Direction == 2) { /*upside down */
					if (currentLayer == 0 || currentLayer == 20) { /* back copper or silk */
						rotation = 0;
						kicadMirrored = 0; /* mirrored */
						textOffsetY += halfStringHeight;
					}
					else { /* front copper or silk */
						rotation = 1800;
						kicadMirrored = 1; /* not mirrored */
						textOffsetY -= halfStringHeight;
					}
					textOffsetX = -halfStringWidth;
				}
				else if (text->Direction == 1) { /*vertical up */
					if (currentLayer == 0 || currentLayer == 20) { /* back copper or silk */
						rotation = 900;
						kicadMirrored = 0; /* mirrored */
						textOffsetY = halfStringHeight;
						textOffsetX += halfStringWidth;
					}
					else { /* front copper or silk */
						rotation = 900;
						kicadMirrored = 1; /* not mirrored */
						textOffsetY = -halfStringHeight;
						textOffsetX = 0; /* += halfStringWidth; */
					}
				}
				else if (text->Direction == 0) { /*normal text */
					if (currentLayer == 0 || currentLayer == 20) { /* back copper or silk */
						rotation = 1800;
						kicadMirrored = 0; /* mirrored */
						textOffsetY -= halfStringHeight;
					}
					else { /* front copper or silk */
						rotation = 0;
						kicadMirrored = 1; /* not mirrored */
						textOffsetY += halfStringHeight;
					}
					textOffsetX = halfStringWidth;
				}
/*				printf("\"%s\" direction field: %d\n", text->TextString, text->Direction);
				printf("textOffsetX: %d,  textOffsetY: %d\n", textOffsetX, textOffsetY);     TODO need to sort out rotation */
				pcb_fprintf(FP, "(at %.3mm %.3mm", text->X + xOffset + textOffsetX, text->Y + yOffset + textOffsetY);
				if (rotation != 0) {
					fprintf(FP, " %d", rotation / 10); /* convert decidegrees to degrees */
				}
				pcb_fprintf(FP, ") (layer %s)\n", kicad_sexpr_layer_to_text(NULL, currentLayer));
				fprintf(FP, "%*s", indentation + 2, "");
				pcb_fprintf(FP, "(effects (font (size %.3mm %.3mm) (thickness %.3mm))", defaultXSize, defaultYSize, strokeThickness); /* , rotation */
				if (kicadMirrored == 0) {
					fprintf(FP, " (justify mirror)");
				}
				fprintf(FP, ")\n%*s)\n", indentation, "");
			}
			localFlag |= 1;
		}
		return localFlag;
	}
	else {
		return 0;
	}
}

/* ---------------------------------------------------------------------------
 * writes element data in kicad legacy format for use in a .mod library
 */
int io_kicad_write_element(pcb_plug_io_t *ctx, FILE *FP, pcb_data_t *Data)
{
	if (elementlist_length(&Data->Element) > 1) {
		pcb_message(PCB_MSG_ERROR, "Can't save multiple modules (footprints) in a single s-experssion mod file\n");
		return -1;
	}

#warning TODO: make this initialization a common function with write_kicad_layout()
	pcb_printf_slot[4] = "%{\\()\t\r\n \"}mq";

	return write_kicad_layout_elements(FP, PCB, Data, 0, 0, 0);
}


/* ---------------------------------------------------------------------------
 * writes netlist data in kicad legacy format for use in a layout .brd file
 */

int write_kicad_equipotential_netlists(FILE *FP, pcb_board_t *Layout, pcb_cardinal_t indentation)
{
	int n; /* code mostly lifted from netlist.c */
	int netNumber;
	pcb_lib_menu_t *menu;
	pcb_lib_entry_t *netlist;

	/* first we write a default netlist for the 0 net, which is for unconnected pads in pcbnew */
	fprintf(FP, "\n%*s(net 0 \"\")\n", indentation, "");

	/* now we step through any available netlists and generate descriptors */
	for(n = 0, netNumber = 1; n < Layout->NetlistLib[PCB_NETLIST_EDITED].MenuN; n++, netNumber++) {
		menu = &Layout->NetlistLib[PCB_NETLIST_EDITED].Menu[n];
		netlist = &menu->Entry[0];
		if (netlist != NULL) {
			fprintf(FP, "%*s(net %d ", indentation, "", netNumber); /* netlist 0 was used for unconnected pads  */
			pcb_fprintf(FP, "%[4])\n", pcb_netlist_name(menu));
		}
	}
	return 0;
}

/* may need to export a netclass or two 
(net_class Default "Ceci est la Netclass par dÃ©faut"
(clearance 0.254)
(trace_width 0.254)
(via_dia 0.889)
(via_drill 0.635)
(uvia_dia 0.508)
(uvia_drill 0.127)
(add_net "")
*/


/* ---------------------------------------------------------------------------
 * writes element data in kicad legacy format for use in a layout .brd file
 */
int write_kicad_layout_elements(FILE *FP, pcb_board_t *Layout, pcb_data_t *Data, pcb_coord_t xOffset, pcb_coord_t yOffset, pcb_cardinal_t indentation)
{

	gdl_iterator_t eit;
	pcb_line_t *line;
	pcb_arc_t *arc;
	pcb_coord_t arcStartX, arcStartY, arcEndX, arcEndY; /* for arc rendering */
	pcb_coord_t xPos, yPos;

	pcb_element_t *element;
	unm_t group1; /* group used to deal with missing names and provide unique ones if needed */
	const char *currentElementName;
	const char *currentElementRef;
	const char *currentElementVal;

	pcb_lib_menu_t *current_pin_menu;
	pcb_lib_menu_t *current_pad_menu;

	int silkLayer = 21; /* hard coded default, 20 is bottom silk */
	int copperLayer = 15; /* hard coded default, 0 is bottom copper */

	elementlist_dedup_initializer(ededup);
	/* Now initialize the group with defaults */
	unm_init(&group1);

	elementlist_foreach(&Data->Element, &eit, element) {
		gdl_iterator_t it;
		pcb_pin_t *pin;
		pcb_pad_t *pad;

		/* elementlist_dedup_skip(ededup, element);  */
		/* let's not skip duplicate elements for layout export */

		/* TOOD: Footprint name element->Name[0].TextString */

		/* only non empty elements */
		if (!linelist_length(&element->Line) && !pinlist_length(&element->Pin) && !arclist_length(&element->Arc) && !padlist_length(&element->Pad))
			continue;
		/* the coordinates and text-flags are the same for
		 * both names of an element
		 */

		xPos = element->MarkX + xOffset;
		yPos = element->MarkY + yOffset;
		silkLayer = 21; /* default */
		copperLayer = 15; /* default */

		if (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, element)) {
			silkLayer = 20;
			copperLayer = 0;
		}

		currentElementName = unm_name(&group1, element->Name[0].TextString, element);
		if (currentElementName == NULL) {
			currentElementName = "unknown";
		}
		currentElementRef = element->Name[PCB_ELEMNAME_IDX_REFDES].TextString;
		if (currentElementRef == NULL) {
			currentElementRef = "unknown";
		}
		currentElementVal = element->Name[PCB_ELEMNAME_IDX_VALUE].TextString;
		if (currentElementVal == NULL) {
			currentElementVal = "unknown";
		}

		fprintf(FP, "%*s", indentation, "");
		pcb_fprintf(FP, "(module %[4] (layer %s) (tedit 4E4C0E65) (tstamp 5127A136)\n", currentElementName, kicad_sexpr_layer_to_text(NULL, copperLayer));
		fprintf(FP, "%*s", indentation + 2, "");
		pcb_fprintf(FP, "(at %.3mm %.3mm)\n", xPos, yPos);

		fprintf(FP, "%*s", indentation + 2, "");
		pcb_fprintf(FP, "(descr %[4])\n", currentElementName);

		fprintf(FP, "%*s", indentation + 2, "");

		pcb_fprintf(FP, "(fp_text reference %[4] (at 0.0 -2.56) ", currentElementRef);
		pcb_fprintf(FP, "(layer %s)\n", kicad_sexpr_layer_to_text(NULL, silkLayer));

		fprintf(FP, "%*s", indentation + 4, "");
		fprintf(FP, "(effects (font (size 1.397 1.27) (thickness 0.2032)))\n");
		fprintf(FP, "%*s)\n", indentation + 2, "");

		fprintf(FP, "%*s", indentation + 2, "");
		printf("Element SilkLayer: %s\n", kicad_sexpr_layer_to_text(NULL, silkLayer));

		pcb_fprintf(FP, "(fp_text value %[4] (at 0.0 -1.27) ", currentElementVal);
		pcb_fprintf(FP, "(layer %s)\n", kicad_sexpr_layer_to_text(NULL, silkLayer));

		fprintf(FP, "%*s", indentation + 4, "");
		fprintf(FP, "(effects (font (size 1.397 1.27) (thickness 0.2032)))\n");
		fprintf(FP, "%*s)\n", indentation + 2, "");

		linelist_foreach(&element->Line, &it, line) {
			fprintf(FP, "%*s", indentation + 2, "");
			pcb_fprintf(FP, "(fp_line (start %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n",
									line->Point1.X - element->MarkX, line->Point1.Y - element->MarkY, line->Point2.X - element->MarkX, line->Point2.Y - element->MarkY, kicad_sexpr_layer_to_text(NULL, silkLayer), line->Thickness);
		}

		arclist_foreach(&element->Arc, &it, arc) {

			pcb_arc_get_end(arc, 0, &arcStartX, &arcStartY);
			pcb_arc_get_end(arc, 1, &arcEndX, &arcEndY);

			if ((arc->Delta == 360.0) || (arc->Delta == -360.0)) { /* it's a circle */
				fprintf(FP, "%*s", indentation + 2, "");
				pcb_fprintf(FP, "(fp_circle (center %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n", arc->X - element->MarkX, /* x_1 centre */
										arc->Y - element->MarkY, /* y_2 centre */
										arcStartX - element->MarkX, /* x on circle */
										arcStartY - element->MarkY, /* y on circle */
										kicad_sexpr_layer_to_text(NULL, silkLayer), arc->Thickness); /* stroke thickness */
			}
			else {
				fprintf(FP, "%*s", indentation + 2, "");
				pcb_fprintf(FP, "(fp_arc (start %.3mm %.3mm) (end %.3mm %.3mm) (angle %ma) (layer %s) (width %.3mm))\n", arc->X - element->MarkX, /* x_1 centre */
										arc->Y - element->MarkY, /* y_2 centre */
										arcEndX - element->MarkX, /* x on arc */
										arcEndY - element->MarkY, /* y on arc */
										arc->Delta, /* CW delta angle in decidegrees */
										kicad_sexpr_layer_to_text(NULL, silkLayer), arc->Thickness); /* stroke thickness */
			}
		}


		pinlist_foreach(&element->Pin, &it, pin) {
			fprintf(FP, "%*s", indentation + 2, "");
			fputs("(pad ", FP);
			pcb_print_quoted_string(FP, (char *)PCB_EMPTY(pin->Number));
			if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, pin)) {
				fputs(" thru_hole rect ", FP); /* square */
			}
			else {
				fputs(" thru_hole circle ", FP); /* circular */
			}
			pcb_fprintf(FP, "(at %.3mm %.3mm)", /* positions of pad */
									pin->X - element->MarkX, pin->Y - element->MarkY);
			/* pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pin->Number)); */
			pcb_fprintf(FP, " (size %.3mm %.3mm)", pin->Thickness, pin->Thickness); /* height = width */
			/* drill details; size */
			pcb_fprintf(FP, " (drill %.3mm)\n", pin->DrillingHole);
			fprintf(FP, "%*s", indentation + 4, "");
			fprintf(FP, "(layers *.Cu *.Mask)\n"); /* define included layers for pin */
			current_pin_menu = pcb_netlist_find_net4pin(Layout, pin);
			fprintf(FP, "%*s", indentation + 4, "");
			if ((current_pin_menu != NULL) && (pcb_netlist_net_idx(Layout, current_pin_menu) != PCB_NETLIST_INVALID_INDEX)) {
				pcb_fprintf(FP, "(net %d %[4])\n", (1 + pcb_netlist_net_idx(Layout, current_pin_menu)), pcb_netlist_name(current_pin_menu)); /* library parts have empty net descriptors, in a .brd they don't */
			}
			else {
				fprintf(FP, "(net 0 \"\")\n"); /* unconnected pads have zero for net */
			}
			fprintf(FP, "%*s)\n", indentation + 2, "");
			/*
			   pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pin->Name));
			   fprintf(FP, " %s\n", F2S(pin, PCB_TYPE_PIN));
			 */
		}
		padlist_foreach(&element->Pad, &it, pad) {
			fprintf(FP, "%*s", indentation + 2, "");
			fputs("(pad ", FP);
			pcb_print_quoted_string(FP, (char *)PCB_EMPTY(pad->Number));
			fputs(" smd rect ", FP); /* square for now */
			pcb_fprintf(FP, "(at %.3mm %.3mm)", /* positions of pad */
									(pad->Point1.X + pad->Point2.X) / 2 - element->MarkX, (pad->Point1.Y + pad->Point2.Y) / 2 - element->MarkY);
			pcb_fprintf(FP, " (size ");
			if ((pad->Point1.X - pad->Point2.X) <= 0 && (pad->Point1.Y - pad->Point2.Y) <= 0) {
				pcb_fprintf(FP, "%.3mm %.3mm)\n", pad->Point2.X - pad->Point1.X + pad->Thickness, /* width */
										pad->Point2.Y - pad->Point1.Y + pad->Thickness); /* height */
			}
			else if ((pad->Point1.X - pad->Point2.X) <= 0 && (pad->Point1.Y - pad->Point2.Y) > 0) {
				pcb_fprintf(FP, "%.3mm %.3mm)\n", pad->Point2.X - pad->Point1.X + pad->Thickness, /* width */
										pad->Point1.Y - pad->Point2.Y + pad->Thickness); /* height */
			}
			else if ((pad->Point1.X - pad->Point2.X) > 0 && (pad->Point1.Y - pad->Point2.Y) > 0) {
				pcb_fprintf(FP, "%.3mm %.3mm)\n", pad->Point1.X - pad->Point2.X + pad->Thickness, /* width */
										pad->Point1.Y - pad->Point2.Y + pad->Thickness); /* height */
			}
			else if ((pad->Point1.X - pad->Point2.X) > 0 && (pad->Point1.Y - pad->Point2.Y) <= 0) {
				pcb_fprintf(FP, "%.3mm %.3mm)\n", pad->Point1.X - pad->Point2.X + pad->Thickness, /* width */
										pad->Point2.Y - pad->Point1.Y + pad->Thickness); /* height */
			}

			fprintf(FP, "%*s", indentation + 4, "");
			if (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad)) {
				fprintf(FP, "(layers B.Cu B.Paste B.Mask)\n"); /* May break if odd layer names */
			}
			else {
				fprintf(FP, "(layers F.Cu F.Paste F.Mask)\n"); /* May break if odd layer names */
			}

			current_pin_menu = pcb_netlist_find_net4pad(Layout, pad);
			fprintf(FP, "%*s", indentation + 4, "");
			if ((current_pin_menu != NULL) && (pcb_netlist_net_idx(Layout, current_pin_menu) != PCB_NETLIST_INVALID_INDEX)) {
				pcb_fprintf(FP, "(net %d %[4])\n", (1 + pcb_netlist_net_idx(Layout, current_pin_menu)), pcb_netlist_name(current_pin_menu)); /* library parts have empty net descriptors, in a .brd they don't */
			}
			else {
				fprintf(FP, "(net 0 \"\")\n"); /* unconnected pads have zero for net */
			}
			fprintf(FP, "%*s)\n", indentation + 2, "");

		}

		fprintf(FP, "%*s)\n\n", indentation, ""); /*  finish off module */
	}
	/* Release unique name utility memory */
	unm_uninit(&group1);
	/* free the state used for deduplication */
	elementlist_dedup_free(ededup);

	return 0;
}

int write_kicad_layout_subc(FILE *FP, pcb_board_t *Layout, pcb_data_t *Data, pcb_coord_t xOffset, pcb_coord_t yOffset, pcb_cardinal_t indentation)
{
	gdl_iterator_t sit;
	pcb_line_t *line;
	pcb_arc_t *arc;
	pcb_coord_t arcStartX, arcStartY, arcEndX, arcEndY; /* for arc rendering */
	pcb_coord_t xPos, yPos;

	pcb_subc_t *subc;
	unm_t group1; /* group used to deal with missing names and provide unique ones if needed */
	const char *currentElementName;
	const char *currentElementRef;
	const char *currentElementVal;

	pcb_lib_menu_t *current_pin_menu;
	pcb_lib_menu_t *current_pad_menu;


	elementlist_dedup_initializer(ededup);
	/* Now initialize the group with defaults */
	unm_init(&group1);

	subclist_foreach(&Data->subc, &sit, subc) {
		gdl_iterator_t it;
		pcb_pin_t *pin;
		pcb_pad_t *pad;
		pcb_coord_t xPos, yPos;
		int on_bottom;
		double rot;


#warning TODO: get this from data table (see also #1)
		int silkLayer = 21; /* hard coded default, 20 is bottom silk */
		int copperLayer = 15; /* hard coded default, 0 is bottom copper */

		/* elementlist_dedup_skip(ededup, element);  */
#warning TODO: why?
		/* let's not skip duplicate elements for layout export */

		if (pcb_subc_get_origin(subc, &xPos, &yPos) != 0) {
			pcb_io_incompat_save(Data, (pcb_any_obj_t *)subc, "Failed to get origin of subcircuit", "fix the missing subc-aux layer");
			continue;
		}
		if (pcb_subc_get_side(subc, &on_bottom) != 0) {
			pcb_io_incompat_save(Data, (pcb_any_obj_t *)subc, "Failed to get placement side of subcircuit", "fix the missing subc-aux layer");
			continue;
		}

		xPos += xOffset;
		yPos += yOffset;

		if (on_bottom) {
			silkLayer = 20;
			copperLayer = 0;
		}

#warning TODO: we should probably do unm_name() on the refdes, not on footprint-name?
#warning TODO: the unique name makes no sense if we override it with unknown - if the unique name is NULL, it's more likely a save-incompatibility error
		currentElementName = unm_name(&group1, pcb_attribute_get(&subc->Attributes, "footprint"), subc);
		if (currentElementName == NULL) {
			currentElementName = "unknown";
		}
		currentElementRef = pcb_attribute_get(&subc->Attributes, "refdes");
		if (currentElementRef == NULL) {
			currentElementRef = "unknown";
		}
		currentElementVal = pcb_attribute_get(&subc->Attributes, "value");
		if (currentElementVal == NULL) {
			currentElementVal = "unknown";
		}

#warning TODO: why the heck do we hardwire timestamps?!!?!?!
		fprintf(FP, "%*s", indentation, "");
		pcb_fprintf(FP, "(module %[4] (layer %s) (tedit 4E4C0E65) (tstamp 5127A136)\n", currentElementName, kicad_sexpr_layer_to_text(NULL, copperLayer));
		fprintf(FP, "%*s", indentation + 2, "");
		pcb_fprintf(FP, "(at %.3mm %.3mm)\n", xPos, yPos);

		fprintf(FP, "%*s", indentation + 2, "");
		pcb_fprintf(FP, "(descr %[4])\n", currentElementName);

		fprintf(FP, "%*s", indentation + 2, "");

#warning TODO: do not hardwire these coords, look up the first silk dyntext coords instead
		pcb_fprintf(FP, "(fp_text reference %[4] (at 0.0 -2.56) ", currentElementRef);
		pcb_fprintf(FP, "(layer %s)\n", kicad_sexpr_layer_to_text(NULL, silkLayer));

#warning TODO: do not hardwire font sizes here, look up the first silk dyntext sizes instead
		fprintf(FP, "%*s", indentation + 4, "");
		fprintf(FP, "(effects (font (size 1.397 1.27) (thickness 0.2032)))\n");
		fprintf(FP, "%*s)\n", indentation + 2, "");

#warning TODO: do not hardwire these coords, look up the first silk dyntext coords instead
		fprintf(FP, "%*s", indentation + 2, "");
		pcb_fprintf(FP, "(fp_text value %[4] (at 0.0 -1.27) ", currentElementVal);
		pcb_fprintf(FP, "(layer %s)\n", kicad_sexpr_layer_to_text(NULL, silkLayer));

#warning TODO: do not hardwire font sizes here, look up the first silk dyntext sizes instead
		fprintf(FP, "%*s", indentation + 4, "");
		fprintf(FP, "(effects (font (size 1.397 1.27) (thickness 0.2032)))\n");
		fprintf(FP, "%*s)\n", indentation + 2, "");

#if 0
		linelist_foreach(&element->Line, &it, line) {
			fprintf(FP, "%*s", indentation + 2, "");
			pcb_fprintf(FP, "(fp_line (start %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n",
									line->Point1.X - element->MarkX, line->Point1.Y - element->MarkY, line->Point2.X - element->MarkX, line->Point2.Y - element->MarkY, kicad_sexpr_layer_to_text(NULL, silkLayer), line->Thickness);
		}

		arclist_foreach(&element->Arc, &it, arc) {

			pcb_arc_get_end(arc, 0, &arcStartX, &arcStartY);
			pcb_arc_get_end(arc, 1, &arcEndX, &arcEndY);

			if ((arc->Delta == 360.0) || (arc->Delta == -360.0)) { /* it's a circle */
				fprintf(FP, "%*s", indentation + 2, "");
				pcb_fprintf(FP, "(fp_circle (center %.3mm %.3mm) (end %.3mm %.3mm) (layer %s) (width %.3mm))\n", arc->X - element->MarkX, /* x_1 centre */
										arc->Y - element->MarkY, /* y_2 centre */
										arcStartX - element->MarkX, /* x on circle */
										arcStartY - element->MarkY, /* y on circle */
										kicad_sexpr_layer_to_text(NULL, silkLayer), arc->Thickness); /* stroke thickness */
			}
			else {
				fprintf(FP, "%*s", indentation + 2, "");
				pcb_fprintf(FP, "(fp_arc (start %.3mm %.3mm) (end %.3mm %.3mm) (angle %ma) (layer %s) (width %.3mm))\n", arc->X - element->MarkX, /* x_1 centre */
										arc->Y - element->MarkY, /* y_2 centre */
										arcEndX - element->MarkX, /* x on arc */
										arcEndY - element->MarkY, /* y on arc */
										arc->Delta, /* CW delta angle in decidegrees */
										kicad_sexpr_layer_to_text(NULL, silkLayer), arc->Thickness); /* stroke thickness */
			}
		}


		pinlist_foreach(&element->Pin, &it, pin) {
			fprintf(FP, "%*s", indentation + 2, "");
			fputs("(pad ", FP);
			pcb_print_quoted_string(FP, (char *)PCB_EMPTY(pin->Number));
			if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, pin)) {
				fputs(" thru_hole rect ", FP); /* square */
			}
			else {
				fputs(" thru_hole circle ", FP); /* circular */
			}
			pcb_fprintf(FP, "(at %.3mm %.3mm)", /* positions of pad */
									pin->X - element->MarkX, pin->Y - element->MarkY);
			/* pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pin->Number)); */
			pcb_fprintf(FP, " (size %.3mm %.3mm)", pin->Thickness, pin->Thickness); /* height = width */
			/* drill details; size */
			pcb_fprintf(FP, " (drill %.3mm)\n", pin->DrillingHole);
			fprintf(FP, "%*s", indentation + 4, "");
			fprintf(FP, "(layers *.Cu *.Mask)\n"); /* define included layers for pin */
			current_pin_menu = pcb_netlist_find_net4pin(Layout, pin);
			fprintf(FP, "%*s", indentation + 4, "");
			if ((current_pin_menu != NULL) && (pcb_netlist_net_idx(Layout, current_pin_menu) != PCB_NETLIST_INVALID_INDEX)) {
				pcb_fprintf(FP, "(net %d %[4])\n", (1 + pcb_netlist_net_idx(Layout, current_pin_menu)), pcb_netlist_name(current_pin_menu)); /* library parts have empty net descriptors, in a .brd they don't */
			}
			else {
				fprintf(FP, "(net 0 \"\")\n"); /* unconnected pads have zero for net */
			}
			fprintf(FP, "%*s)\n", indentation + 2, "");
			/*
			   pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pin->Name));
			   fprintf(FP, " %s\n", F2S(pin, PCB_TYPE_PIN));
			 */
		}
		padlist_foreach(&element->Pad, &it, pad) {
			fprintf(FP, "%*s", indentation + 2, "");
			fputs("(pad ", FP);
			pcb_print_quoted_string(FP, (char *)PCB_EMPTY(pad->Number));
			fputs(" smd rect ", FP); /* square for now */
			pcb_fprintf(FP, "(at %.3mm %.3mm)", /* positions of pad */
									(pad->Point1.X + pad->Point2.X) / 2 - element->MarkX, (pad->Point1.Y + pad->Point2.Y) / 2 - element->MarkY);
			pcb_fprintf(FP, " (size ");
			if ((pad->Point1.X - pad->Point2.X) <= 0 && (pad->Point1.Y - pad->Point2.Y) <= 0) {
				pcb_fprintf(FP, "%.3mm %.3mm)\n", pad->Point2.X - pad->Point1.X + pad->Thickness, /* width */
										pad->Point2.Y - pad->Point1.Y + pad->Thickness); /* height */
			}
			else if ((pad->Point1.X - pad->Point2.X) <= 0 && (pad->Point1.Y - pad->Point2.Y) > 0) {
				pcb_fprintf(FP, "%.3mm %.3mm)\n", pad->Point2.X - pad->Point1.X + pad->Thickness, /* width */
										pad->Point1.Y - pad->Point2.Y + pad->Thickness); /* height */
			}
			else if ((pad->Point1.X - pad->Point2.X) > 0 && (pad->Point1.Y - pad->Point2.Y) > 0) {
				pcb_fprintf(FP, "%.3mm %.3mm)\n", pad->Point1.X - pad->Point2.X + pad->Thickness, /* width */
										pad->Point1.Y - pad->Point2.Y + pad->Thickness); /* height */
			}
			else if ((pad->Point1.X - pad->Point2.X) > 0 && (pad->Point1.Y - pad->Point2.Y) <= 0) {
				pcb_fprintf(FP, "%.3mm %.3mm)\n", pad->Point1.X - pad->Point2.X + pad->Thickness, /* width */
										pad->Point2.Y - pad->Point1.Y + pad->Thickness); /* height */
			}

			fprintf(FP, "%*s", indentation + 4, "");
			if (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad)) {
				fprintf(FP, "(layers B.Cu B.Paste B.Mask)\n"); /* May break if odd layer names */
			}
			else {
				fprintf(FP, "(layers F.Cu F.Paste F.Mask)\n"); /* May break if odd layer names */
			}

			current_pin_menu = pcb_netlist_find_net4pad(Layout, pad);
			fprintf(FP, "%*s", indentation + 4, "");
			if ((current_pin_menu != NULL) && (pcb_netlist_net_idx(Layout, current_pin_menu) != PCB_NETLIST_INVALID_INDEX)) {
				pcb_fprintf(FP, "(net %d %[4])\n", (1 + pcb_netlist_net_idx(Layout, current_pin_menu)), pcb_netlist_name(current_pin_menu)); /* library parts have empty net descriptors, in a .brd they don't */
			}
			else {
				fprintf(FP, "(net 0 \"\")\n"); /* unconnected pads have zero for net */
			}
			fprintf(FP, "%*s)\n", indentation + 2, "");
		}
#endif

		fprintf(FP, "%*s)\n\n", indentation, ""); /*  finish off module */
	}
	/* Release unique name utility memory */
	unm_uninit(&group1);
	/* free the state used for deduplication */
	elementlist_dedup_free(ededup);

	return 0;
}


/* ---------------------------------------------------------------------------
 * writes polygon data in kicad legacy format for use in a layout .brd file
 */

int write_kicad_layout_polygons(FILE *FP, pcb_cardinal_t number, pcb_layer_t *layer, pcb_coord_t xOffset, pcb_coord_t yOffset, pcb_cardinal_t indentation)
{
	int i, j;
	gdl_iterator_t it;
	pcb_poly_t *polygon;
	pcb_cardinal_t currentLayer = number;

	/* write information about non empty layers */
	if (!pcb_layer_is_empty_(PCB, layer) || (layer->name && *layer->name)) {
		int localFlag = 0;
		polylist_foreach(&layer->Polygon, &it, polygon) {
			if (polygon->HoleIndexN == 0) { /* no holes defined within polygon, which we implement support for first */

				/* preliminaries for zone settings */

				fprintf(FP, "%*s(zone (net 0) (net_name \"\") (layer %s) (tstamp 478E3FC8) (hatch edge 0.508)\n", indentation, "", kicad_sexpr_layer_to_text(NULL, currentLayer));
				fprintf(FP, "%*s(connect_pads no (clearance 0.508))\n", indentation + 2, "");
				fprintf(FP, "%*s(min_thickness 0.4826)\n", indentation + 2, "");
				fprintf(FP, "%*s(fill (arc_segments 32) (thermal_gap 0.508) (thermal_bridge_width 0.508))\n", indentation + 2, "");
				fprintf(FP, "%*s(polygon\n", indentation + 2, "");
				fprintf(FP, "%*s(pts\n", indentation + 4, "");

				/* now the zone outline is defined */

				for(i = 0; i < polygon->PointN; i = i + 5) { /* kicad exports five coords per line in s-expr files */
					fprintf(FP, "%*s", indentation + 6, ""); /* pcb_fprintf does not support %*s   */
					for(j = 0; (j < polygon->PointN) && (j < 5); j++) {
						pcb_fprintf(FP, "(xy %.3mm %.3mm)", polygon->Points[i + j].X + xOffset, polygon->Points[i + j].Y + yOffset);
						if ((j < 4) && ((i + j) < (polygon->PointN - 1))) {
							fputs(" ", FP);
						}
					}
					fputs("\n", FP);
				}
				fprintf(FP, "%*s)\n", indentation + 4, "");
				fprintf(FP, "%*s)\n", indentation + 2, "");
				fprintf(FP, "%*s)\n", indentation, ""); /* end zone */
				/*
				 *   in here could go additional polygon descriptors for holes removed from  the previously defined outer polygon
				 */

			}
			localFlag |= 1;
		}
		return localFlag;
	}
	else {
		return 0;
	}
}
