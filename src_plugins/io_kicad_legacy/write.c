/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2018,2019 Tibor 'Igor2' Palinkas
 *  Copyright (C) 2016 Erich S. Heinzle
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
#include <math.h>
#include "config.h"
#include "compat_misc.h"
#include "board.h"
#include "plug_io.h"
#include "error.h"
#include "data.h"
#include "write.h"
#include "layer.h"
#include "netlist2.h"
#include "macro.h"
#include "obj_pstk_inlines.h"

#include "../io_kicad/uniq_name.h"
#include "src_plugins/lib_compat_help/pstk_compat.h"

/* layer "0" is first copper layer = "0. Back - Solder"
 * and layer "15" is "15. Front - Component"
 * and layer "20" SilkScreen Back
 * and layer "21" SilkScreen Front
 */

static const char *or_empty(const char *s)
{
	if (s == NULL) return "";
	return s;
}

/* writes (eventually) de-duplicated list of element names in kicad legacy format module $INDEX */
static int io_kicad_legacy_write_subc_index(FILE *FP, pcb_data_t *Data)
{
	gdl_iterator_t eit;
	pcb_subc_t *subc;
	unm_t group1;

	unm_init(&group1);

	subclist_foreach(&Data->subc, &eit, subc) {
		if (pcb_data_is_empty(subc->data))
			continue;

TODO(": need a subc dedup")
/*		elementlist_dedup_skip(ededup, element);*/

		fprintf(FP, "%s\n", unm_name(&group1, or_empty(pcb_attribute_get(&subc->Attributes, "footprint")), subc));
	}

	unm_uninit(&group1);
	return 0;
}

/* via is: Position shape Xstart Ystart Xend Yend width
   Description layer 0 netcode timestamp status
   Shape parameter is set to 0 (reserved for future) */
static int write_kicad_legacy_layout_vias(FILE *FP, pcb_data_t *Data, pcb_coord_t xOffset, pcb_coord_t yOffset)
{
	gdl_iterator_t it;
	pcb_pstk_t *ps;
	pcb_coord_t x, y, drill_dia, pad_dia, clearance, mask;
	pcb_pstk_compshape_t cshape;
	pcb_bool plated;

	padstacklist_foreach(&Data->padstack, &it, ps) {
		if (pcb_pstk_export_compat_via(ps, &x, &y, &drill_dia, &pad_dia, &clearance, &mask, &cshape, &plated)) {
			if (cshape != PCB_PSTK_COMPAT_ROUND) {
				pcb_io_incompat_save(Data, (pcb_any_obj_t *)ps, "via", "Failed to export via: only round shaped vias, with copper ring, are supported", NULL);
				continue;
			}

			pcb_fprintf(FP, "Po 3 %.0mk %.0mk %.0mk %.0mk %.0mk\n", /* testing kicad printf */
				x + xOffset, y + yOffset, x + xOffset, y + yOffset, pad_dia);
TODO(": check if drill_dia can be applied")
TODO(": bbvia")
			pcb_fprintf(FP, "De 15 1 0 0 0\n"); /* this is equivalent to 0F, via from 15 -> 0 */
		}
	}
	return 0;
}

/* generates a default via drill size for the layout */
static int write_kicad_legacy_layout_via_drill_size(FILE *FP)
{
TODO(": do not hardwire this")
	pcb_fprintf(FP, "ViaDrill 250\n"); /* decimil format, default for now, ~= 0.635mm */
	return 0;
}

static int write_kicad_legacy_layout_tracks(FILE *FP, pcb_cardinal_t number, pcb_layer_t *layer, pcb_coord_t xOffset, pcb_coord_t yOffset)
{
	gdl_iterator_t it;
	pcb_line_t *line;
	pcb_cardinal_t currentLayer = number;

	/* write information about non empty layers */
	if (!pcb_layer_is_empty_(PCB, layer) || (layer->name && *layer->name)) {
		int localFlag = 0;
		linelist_foreach(&layer->Line, &it, line) {
			if (currentLayer < 16) { /* a copper line i.e. track */
				pcb_fprintf(FP, "Po 0 %.0mk %.0mk %.0mk %.0mk %.0mk\n", line->Point1.X + xOffset, line->Point1.Y + yOffset, line->Point2.X + xOffset, line->Point2.Y + yOffset, line->Thickness);
				pcb_fprintf(FP, "De %d 0 0 0 0\n", currentLayer); /* omitting net info */
			}
			else if ((currentLayer == 20) || (currentLayer == 21) || (currentLayer == 28)) { /* a silk line or outline */
				fputs("$DRAWSEGMENT\n", FP);
				pcb_fprintf(FP, "Po 0 %.0mk %.0mk %.0mk %.0mk %.0mk\n", line->Point1.X + xOffset, line->Point1.Y + yOffset, line->Point2.X + xOffset, line->Point2.Y + yOffset, line->Thickness);
				pcb_fprintf(FP, "De %d 0 0 0 0\n", currentLayer); /* omitting net info */
				fputs("$EndDRAWSEGMENT\n", FP);
			}
			localFlag |= 1;
		}
		return localFlag;
	}
	else {
		return 0;
	}
}

static int write_kicad_legacy_layout_arcs(FILE *FP, pcb_cardinal_t number, pcb_layer_t *layer, pcb_coord_t xOffset, pcb_coord_t yOffset)
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

			if (currentLayer < 16) { /* a copper arc, i.e. track, is unsupported by kicad, and will be exported as a line */
				kicadArcShape = 0; /* make it a line for copper layers - kicad doesn't do arcs on copper */
				pcb_fprintf(FP, "Po %d %.0mk %.0mk %.0mk %.0mk %.0mk\n", kicadArcShape, copperStartX, copperStartY, xEnd, yEnd, arc->Thickness);
				pcb_fprintf(FP, "De %d 0 0 0 0\n", currentLayer); /* in theory, copper arcs unsupported by kicad, make angle = 0 */
			}
			else if ((currentLayer == 20) || (currentLayer == 21) || (currentLayer == 28)) { /* a silk arc or outline */
				fputs("$DRAWSEGMENT\n", FP);
				pcb_fprintf(FP, "Po %d %.0mk %.0mk %.0mk %.0mk %.0mk\n", kicadArcShape, xStart, yStart, xEnd, yEnd, arc->Thickness);
				pcb_fprintf(FP, "De %d 0 %mA 0 0\n", currentLayer, arc->Delta); /* in theory, decidegrees != 900 unsupported by older kicad */
				fputs("$EndDRAWSEGMENT\n", FP);
			}
			localFlag |= 1;
		}
		return localFlag;
	}
	else {
		return 0;
	}
}

static int write_kicad_legacy_layout_text(FILE *FP, pcb_cardinal_t number, pcb_layer_t *layer, pcb_coord_t xOffset, pcb_coord_t yOffset)
{
	pcb_font_t *myfont = pcb_font(PCB, 0, 1);
	pcb_coord_t mWidth = myfont->MaxWidth; /* kicad needs the width of the widest letter */
	pcb_coord_t defaultStrokeThickness = 100 * 2540; /* use 100 mil as default 100% stroked font line thickness */
	int kicadMirrored = 1; /* 1 is not mirrored, 0  is mirrored */
	int direction;

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
		localFlag = 0;
		textlist_foreach(&layer->Text, &it, text) {
			if ((currentLayer < 16) || (currentLayer == 20) || (currentLayer == 21)) { /* copper or silk layer text */
				fputs("$TEXTPCB\nTe \"", FP);
				fputs(text->TextString, FP);
				fputs("\"\n", FP);
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

TODO("code duplication with io_kicad - clean that up after fixing textrot!")
TODO("textrot: use the angle, not n*90 deg")
				pcb_text_old_direction(&direction, text->rot);
				if (direction == 3) { /*vertical down */
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
				else if (direction == 2) { /*upside down */
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
				else if (direction == 1) { /*vertical up */
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
				else if (direction == 0) { /*normal text */
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
				pcb_fprintf(FP, "Po %.0mk %.0mk %.0mk %.0mk %.0mk %d\n", text->X + xOffset + textOffsetX, text->Y + yOffset + textOffsetY, defaultXSize, defaultYSize, strokeThickness, rotation);
				pcb_fprintf(FP, "De %d %d B98C Normal\n", currentLayer, kicadMirrored); /* timestamp made up B98C  */
				fputs("$EndTEXTPCB\n", FP);
			}
			localFlag |= 1;
		}
		return localFlag;
	}
	else {
		return 0;
	}
}

static int write_kicad_legacy_equipotential_netlists(FILE *FP, pcb_board_t *Layout)
{
	htsp_entry_t *e;
	pcb_cardinal_t netNumber = 0;

	/* first we write a default netlist for the 0 net, which is for unconnected pads in pcbnew */
	fputs("$EQUIPOT\n", FP);
	fputs("Na 0 \"\"\n", FP);
	fputs("St ~\n", FP);
	fputs("$EndEQUIPOT\n", FP);

	/* now we step through any available netlists and generate descriptors */
	for(e = htsp_first(&Layout->netlist[PCB_NETLIST_EDITED]); e != NULL; e = htsp_next(&Layout->netlist[PCB_NETLIST_EDITED], e)) {
		pcb_net_t *net = e->value;
		pcb_net_term_t *t = pcb_termlist_first(&net->conns);
		if (t != NULL) {
			netNumber++;
			fputs("$EQUIPOT\n", FP);
			fprintf(FP, "Na %d \"%s\"\n", netNumber, net->name); /* netlist 0 was used for unconnected pads  */
			fputs("St ~\n", FP);
			fputs("$EndEQUIPOT\n", FP);
			net->export_tmp = netNumber;
		}
		else
			net->export_tmp = 0;
	}
	return 0;
}

static void print_pstk_net(FILE *FP, pcb_board_t *Layout, pcb_pstk_t *ps)
{
	pcb_net_term_t *term;
	pcb_net_t *net = NULL;

	if (Layout != NULL) {
		term = pcb_net_find_by_obj(&Layout->netlist[PCB_NETLIST_EDITED], (const pcb_any_obj_t *)ps);
		if (term != NULL)
			net = term->parent.net;
	}
	if ((net != NULL) && (net->export_tmp != 0))
		fprintf(FP, "Ne %d \"%s\"\n", net->export_tmp, net->name); /* library parts have empty net descriptors, in a .brd they don't */
	else
		fprintf(FP, "Ne 0 \"\"\n"); /* unconnected pads have zero for net */
}

static int io_kicad_legacy_write_subc(FILE *FP, pcb_board_t *pcb, pcb_subc_t *subc, pcb_coord_t xOffset, pcb_coord_t yOffset, const char *uname)
{
	pcb_pstk_t *ps;
	pcb_coord_t ox, oy, sox, soy;
	int on_bottom;
	int copperLayer; /* hard coded default, 0 is bottom copper */
	gdl_iterator_t it;
	int n, silkLayer;

	if (pcb_subc_get_origin(subc, &sox, &soy) != 0) {
		pcb_io_incompat_save(subc->parent.data, (pcb_any_obj_t *)subc, "subc-place", "Failed to get origin of subcircuit", "fix the missing subc-aux layer");
		return -1;
	}
	if (pcb_subc_get_side(subc, &on_bottom) != 0) {
		pcb_io_incompat_save(subc->parent.data, (pcb_any_obj_t *)subc, "subc-place", "Failed to get placement side of subcircuit", "fix the missing subc-aux layer");
		return -1;
	}

	ox = sox + xOffset;
	oy = soy + yOffset;

	if (on_bottom) {
		copperLayer = 0;
		silkLayer = 20;
	}
	else {
		copperLayer = 15;
		silkLayer = 21;
	}

	fprintf(FP, "$MODULE %s\n", uname);
TODO(": do not hardwire time stamps")
	pcb_fprintf(FP, "Po %.0mk %.0mk 0 %d 51534DFF 00000000 ~~\n", ox, oy, copperLayer);
	fprintf(FP, "Li %s\n", uname); /* This needs to be unique */
	fprintf(FP, "Cd %s\n", uname);
	fputs("Sc 0\n", FP);
	fputs("AR\n", FP);
TODO(": is this the origin point? if so, it should be sox and soy")
	fputs("Op 0 0 0\n", FP);

TODO(": do not hardwire coords")
TODO(": figure how to turn off displaying these")
	fprintf(FP, "T0 0 -4000 600 600 0 120 N V %d N \"%s\"\n", silkLayer, or_empty(pcb_attribute_get(&subc->Attributes, "refdes")));
	fprintf(FP, "T1 0 -5000 600 600 0 120 N V %d N \"%s\"\n", silkLayer, or_empty(pcb_attribute_get(&subc->Attributes, "value")));
	fprintf(FP, "T2 0 -6000 600 600 0 120 N V %d N \"%s\"\n", silkLayer, or_empty(pcb_attribute_get(&subc->Attributes, "footprint")));

	/* export padstacks */
	padstacklist_foreach(&subc->data->padstack, &it, ps) {
		pcb_coord_t x, y, drill_dia, pad_dia, clearance, mask, x1, y1, x2, y2, thickness;
		pcb_pstk_compshape_t cshape;
		pcb_bool plated, square, nopaste;
		double psrot;

		if (ps->term == NULL) {
			pcb_io_incompat_save(subc->data, (pcb_any_obj_t *)ps, "padstack-nonterm", "can't export non-terminal padstack in subcircuit, omitting the object", NULL);
			continue;
		}

		psrot = ps->rot;
		if (ps->smirror)
			psrot = -psrot;

		if (pcb_pstk_export_compat_via(ps, &x, &y, &drill_dia, &pad_dia, &clearance, &mask, &cshape, &plated)) {
			fputs("$PAD\n", FP);
			pcb_fprintf(FP, "Po %.0mk %.0mk\n", x - sox, y - soy);  /* positions of pad */
			fputs("Sh ", FP); /* pin shape descriptor */
			pcb_print_quoted_string(FP, (char *)PCB_EMPTY(ps->term));

			if (cshape == PCB_PSTK_COMPAT_SQUARE) fputs(" R ", FP);
			else if (cshape == PCB_PSTK_COMPAT_ROUND) fputs(" C ", FP);
			else {
				pcb_io_incompat_save(subc->data, (pcb_any_obj_t *)ps, "padstack-shape", "can't export shaped pin; needs to be square or circular - using circular instead", NULL);
				fputs(" C ", FP);
			}
			pcb_fprintf(FP, "%.0mk %.0mk ", pad_dia, pad_dia); /* height = width */
			fprintf(FP, "0 0 %d\n", (int)(psrot * 10)); /* deltaX deltaY Orientation as float in decidegrees */

			/* drill details; size and x,y pos relative to pad location */
			pcb_fprintf(FP, "Dr %.0mk 0 0\n", drill_dia);

			fputs("At STD N 00E0FFFF\n", FP); /* through hole STD pin, all copper layers */

			print_pstk_net(FP, pcb, ps);
			fputs("$EndPAD\n", FP);
		}
		else if (pcb_pstk_export_compat_pad(ps, &x1, &y1, &x2, &y2, &thickness, &clearance, &mask, &square, &nopaste)) {
			/* the above check only makes sure this is a plain padstack, get the geometry from the copper layer shape */
			char shape_chr;
			int n, has_mask = 0, on_bottom;
			pcb_pstk_proto_t *proto = pcb_pstk_get_proto_(subc->data, ps->proto);
			pcb_pstk_tshape_t *tshp = &proto->tr.array[0];
			pcb_coord_t w, h, cx, cy;
			int found = 0;

			fputs("$PAD\n", FP); /* start pad descriptor for an smd pad */

TODO(": remove this code dup with io_kicad")
			for(n = 0; n < tshp->len; n++) {
				if (tshp->shape[n].layer_mask & PCB_LYT_COPPER) {
					int i;
					pcb_line_t line;
					pcb_box_t bx;
					pcb_pstk_shape_t *shape = &tshp->shape[n];
					
					if (found)
						continue;
					found = 1;

					on_bottom = tshp->shape[n].layer_mask & PCB_LYT_BOTTOM;
					if (ps->smirror) on_bottom = !on_bottom;

					switch(shape->shape) {
						case PCB_PSSH_POLY:
							bx.X1 = bx.X2 = shape->data.poly.x[0];
							bx.Y1 = bx.Y2 = shape->data.poly.y[0];
							for(i = 1; i < shape->data.poly.len; i++)
								pcb_box_bump_point(&bx, shape->data.poly.x[i], shape->data.poly.y[i]);
							w = (bx.X2 - bx.X1);
							h = (bx.Y2 - bx.Y1);
							cx = (bx.X1 + bx.X2)/2;
							cy = (bx.Y1 + bx.Y2)/2;
							shape_chr = 'R';
							break;
						case PCB_PSSH_LINE:
							line.Point1.X = shape->data.line.x1;
							line.Point1.Y = shape->data.line.y1;
							line.Point2.X = shape->data.line.x2;
							line.Point2.Y = shape->data.line.y2;
							line.Thickness = shape->data.line.thickness;
							line.Clearance = 0;
							line.Flags = pcb_flag_make(shape->data.line.square ? PCB_FLAG_SQUARE : 0);
							pcb_line_bbox(&line);
							w = (line.BoundingBox.X2 - line.BoundingBox.X1);
							h = (line.BoundingBox.Y2 - line.BoundingBox.Y1);
							cx = (shape->data.line.x1 + shape->data.line.x2)/2;
							cy = (shape->data.line.y1 + shape->data.line.y2)/2;
							shape_chr = shape->data.line.square ? 'R' : 'C';
							break;
						case PCB_PSSH_CIRC:
							w = h = shape->data.circ.dia;
							cx = shape->data.circ.x;
							cy = shape->data.circ.y;
							shape_chr = 'C';
							break;
						case PCB_PSSH_HSHADOW:
TODO("hshadow TODO")
							shape_chr = 'C';
							cx = 0;
							cy = 0;
							break;
					}
				}
				if (tshp->shape[n].layer_mask & PCB_LYT_MASK)
					has_mask = 1;
			}
			pcb_fprintf(FP, "Po %.0mk %.0mk\n", ps->x + cx - sox, ps->y + cy - soy);  /* positions of pad */

			fputs("Sh ", FP); /* pin shape descriptor */
			pcb_print_quoted_string(FP, (char *)PCB_EMPTY(ps->term));
			pcb_fprintf(FP, " %c %.0mk %.0mk ", shape_chr, w, h);

			pcb_fprintf(FP, "0 0 %d\n", (int)(psrot*10.0)); /* deltaX deltaY Orientation as float in decidegrees */

			fputs("Dr 0 0 0\n", FP); /* drill details; zero size; x,y pos vs pad location */

			 /* SMD pin, need to use right layer mask */
			if (on_bottom)
				fputs("At SMD N 00440001\n", FP);
			else
				fputs("At SMD N 00888000\n", FP);

			print_pstk_net(FP, pcb, ps);
			fputs("$EndPAD\n", FP);
		}
		else
			pcb_io_incompat_save(subc->data, (pcb_any_obj_t *)ps, "padstack-shape", "Can't convert padstack to pin or pad", "use a simpler, uniform shape");

	}

	/* export layer objects */
	for(n = 0; n < subc->data->LayerN; n++) {
		pcb_layer_t *ly = &subc->data->Layer[n];
		pcb_layer_type_t lyt = pcb_layer_flags_(ly);
		pcb_line_t *line;
		pcb_arc_t *arc;
		pcb_poly_t *poly;
		pcb_text_t *text;
		pcb_coord_t arcStartX, arcStartY, arcEndX, arcEndY;
		int silkLayer;

		if (!(lyt & PCB_LYT_SILK)) {
			linelist_foreach(&ly->Line, &it, line)
				pcb_io_incompat_save(subc->data, (pcb_any_obj_t *)line, "subc-obj", "can't save non-silk lines in subcircuits", "convert terminals to padstacks, remove the rest");
			arclist_foreach(&ly->Arc, &it, arc)
				pcb_io_incompat_save(subc->data, (pcb_any_obj_t *)arc, "subc-obj", "can't save non-silk arc in subcircuits", "remove this arc");
		}

		polylist_foreach(&ly->Polygon, &it, poly)
			pcb_io_incompat_save(subc->data, (pcb_any_obj_t *)poly, "subc-obj", "can't save polygons in subcircuits", "convert square terminals to padstacks, remove the rest");

		textlist_foreach(&ly->Text, &it, text)
			if (!PCB_FLAG_TEST(PCB_FLAG_DYNTEXT, text))
				pcb_io_incompat_save(subc->data, (pcb_any_obj_t *)text, "subc-obj", "can't save text in subcircuits", "remove the text object");

		silkLayer = (lyt & PCB_LYT_BOTTOM) ? 20 : 21;

		linelist_foreach(&ly->Line, &it, line) {
			pcb_fprintf(FP, "DS %.0mk %.0mk %.0mk %.0mk %.0mk ",
				line->Point1.X - sox, line->Point1.Y - soy,
				line->Point2.X - sox, line->Point2.Y - soy,
				line->Thickness);
			fprintf(FP, "%d\n", silkLayer);
		}

		arclist_foreach(&ly->Arc, &it, arc) {
			pcb_arc_get_end(arc, 0, &arcStartX, &arcStartY);
			pcb_arc_get_end(arc, 1, &arcEndX, &arcEndY);

			if ((arc->Delta == 360.0) || (arc->Delta == -360.0)) { /* it's a circle */
				pcb_fprintf(FP, "DC %.0mk %.0mk %.0mk %.0mk %.0mk ",
					arc->X - sox, arc->Y - soy, /* centre */
					arcStartX - sox, arcStartY - soy, /* on circle */
					arc->Thickness);
			}
			else {
				pcb_fprintf(FP, "DA %.0mk %.0mk %.0mk %.0mk %mA %.0mk ",
					arc->X - sox, arc->Y - soy, /* centre */
					arcEndX - sox, arcEndY - soy,
					arc->Delta, /* CW delta angle in decidegrees */
					arc->Thickness);
			}
			fprintf(FP, "%d\n", silkLayer); /* and now append a suitable Kicad layer, front silk = 21, back silk 20 */
		}
	}

	fprintf(FP, "$EndMODULE %s\n", uname);

	return 0;
}

static int write_kicad_legacy_layout_subcs(FILE *FP, pcb_board_t *Layout, pcb_data_t *Data, pcb_coord_t xOffset, pcb_coord_t yOffset)
{
	gdl_iterator_t sit;
	pcb_subc_t *subc;
	unm_t group1;

	unm_init(&group1);

	subclist_foreach(&Data->subc, &sit, subc) {
		const char *uname = unm_name(&group1, or_empty(pcb_attribute_get(&subc->Attributes, "footprint")), subc);
TODO(": what did we need this for?")
/*		elementlist_dedup_skip(ededup, element); skip duplicate elements */
		io_kicad_legacy_write_subc(FP, PCB, subc, xOffset, yOffset, uname);
	}

	unm_uninit(&group1);
	return 0;
}

static int write_kicad_legacy_layout_polygons(FILE *FP, pcb_cardinal_t number, pcb_layer_t *layer, pcb_coord_t xOffset, pcb_coord_t yOffset)
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
				fputs("$CZONE_OUTLINE\n", FP);
				fputs("ZInfo 478E3FC8 0 \"\"\n", FP); /* use default empty netname, net 0, not connected */
				fprintf(FP, "ZLayer %d\n", currentLayer);
				fprintf(FP, "ZAux %d E\n", polygon->PointN); /* corner count, use edge hatching for displaying the zone in pcbnew */
				fputs("ZClearance 200 X\n", FP); /* set pads/pins to not be connected to pours in the zone by default */
				fputs("ZMinThickness 190\n", FP); /* minimum copper thickness in zone, default setting */
				fputs("ZOptions 0 32 F 200 200\n", FP); /* solid fill, 32 segments per arc, antipad thickness, thermal stubs width(s) */

				/* now the zone outline is defined */

				for(i = 0, j = 0; i < polygon->PointN; i++) {
					if (i == (polygon->PointN - 1)) {
						j = 1; /* flags that this is the last vertex of the outline */
					}
					pcb_fprintf(FP, "ZCorner %.0mk %.0mk %d\n", polygon->Points[i].X + xOffset, polygon->Points[i].Y + yOffset, j);
				}

				/* in here could go additional plolygon descriptors for holes removed from  the previously defined outer polygon */
				fputs("$endCZONE_OUTLINE\n", FP);
			}
			localFlag |= 1;
		}
		return localFlag;
	}
	else {
		return 0;
	}
}

int io_kicad_legacy_write_buffer(pcb_plug_io_t *ctx, FILE *FP, pcb_buffer_t *buff, pcb_bool elem_only)
{
	if (pcb_subclist_length(&buff->Data->subc) == 0) {
		pcb_message(PCB_MSG_ERROR, "Buffer has no subcircuits!\n");
		return -1;
	}

TODO(": no hardwiring of dates")
	fputs("PCBNEW-LibModule-V1	jan 01 jan 2016 00:00:01 CET\n", FP);
	fputs("$INDEX\n", FP);
	io_kicad_legacy_write_subc_index(FP, buff->Data);
	fputs("$EndINDEX\n", FP);

	pcb_write_footprint_data(FP, buff->Data, "kicadl");

	return 0;
}

int io_kicad_legacy_write_pcb(pcb_plug_io_t *ctx, FILE *FP, const char *old_filename, const char *new_filename, pcb_bool emergency)
{
	pcb_cardinal_t i;
	int kicadLayerCount = 0;
	int physicalLayerCount = 0;
	int layer = 0;
	int currentKicadLayer = 0;
	int currentGroup = 0;
	pcb_coord_t outlineThickness = PCB_MIL_TO_COORD(10);

	int bottomCount;
	pcb_layer_id_t *bottomLayers;
	int innerCount;
	pcb_layer_id_t *innerLayers;
	int topCount;
	pcb_layer_id_t *topLayers;
	int bottomSilkCount;
	pcb_layer_id_t *bottomSilk;
	int topSilkCount;
	pcb_layer_id_t *topSilk;
	int outlineCount;
	pcb_layer_id_t *outlineLayers;

	pcb_coord_t LayoutXOffset;
	pcb_coord_t LayoutYOffset;

	/* Kicad expects a layout "sheet" size to be specified in mils, and A4, A3 etc... */
	int A4HeightMil = 8267;
	int A4WidthMil = 11700;
	int sheetHeight = A4HeightMil;
	int sheetWidth = A4WidthMil;
	int paperSize = 4; /* default paper size is A4 */

	fputs("PCBNEW-BOARD Version 1 jan 01 jan 2016 00:00:01 CET\n", FP);

	fputs("$GENERAL\n", FP);
	fputs("Ly 1FFF8001\n", FP); /* obsolete, needed for old pcbnew */
	/*puts("Units mm\n",FP); *//*decimils most universal legacy format */
	fputs("$EndGENERAL\n", FP);

	fputs("$SHEETDESCR\n", FP);

TODO(": se this from io_kicad, do not duplicate the code here")
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

	fprintf(FP, "Sheet A%d ", paperSize);
	/* we now sort out the offsets for centring the layout in the chosen sheet size here */
	if (sheetWidth > PCB_COORD_TO_MIL(PCB->MaxWidth)) { /* usually A4, bigger if needed */
		fprintf(FP, "%d ", sheetWidth); /* legacy kicad: elements decimils, sheet size mils */
		LayoutXOffset = PCB_MIL_TO_COORD(sheetWidth) / 2 - PCB->MaxWidth / 2;
	}
	else { /* the layout is bigger than A0; most unlikely, but... */
		pcb_fprintf(FP, "%.0ml ", PCB->MaxWidth);
		LayoutXOffset = 0;
	}
	if (sheetHeight > PCB_COORD_TO_MIL(PCB->MaxHeight)) {
		fprintf(FP, "%d", sheetHeight);
		LayoutYOffset = PCB_MIL_TO_COORD(sheetHeight) / 2 - PCB->MaxHeight / 2;
	}
	else { /* the layout is bigger than A0; most unlikely, but... */
		pcb_fprintf(FP, "%.0ml", PCB->MaxHeight);
		LayoutYOffset = 0;
	}
	fputs("\n", FP);
	fputs("$EndSHEETDESCR\n", FP);

	fputs("$SETUP\n", FP);
	fputs("InternalUnit 0.000100 INCH\n", FP); /* decimil is the default v1 kicad legacy unit */

	/* here we define the copper layers in the exported kicad file */
	physicalLayerCount = pcb_layergrp_list(PCB, PCB_LYT_COPPER, NULL, 0);

	fputs("Layers ", FP);
	kicadLayerCount = physicalLayerCount;
	if (kicadLayerCount % 2 == 1) {
		kicadLayerCount++; /* kicad doesn't like odd numbers of layers, has been deprecated for some time apparently */
	}

	fprintf(FP, "%d\n", kicadLayerCount);
	layer = 0;
	if (physicalLayerCount >= 1) {
		fprintf(FP, "Layer[%d] COPPER_LAYER_0 signal\n", layer);
	}
	if (physicalLayerCount > 1) { /* seems we need to ignore layers > 16 due to kicad limitation */
		for(layer = 1; (layer < (kicadLayerCount - 1)) && (layer < 15); layer++) {
			fprintf(FP, "Layer[%d] Inner%d.Cu signal\n", layer, layer);
		}
		fputs("Layer[15] COPPER_LAYER_15 signal\n", FP);
	}

	write_kicad_legacy_layout_via_drill_size(FP);

	fputs("$EndSETUP\n", FP);

	write_kicad_legacy_equipotential_netlists(FP, PCB);
	write_kicad_legacy_layout_subcs(FP, PCB, PCB->Data, LayoutXOffset, LayoutYOffset);

	/* we now need to map pcb's layer groups onto the kicad layer numbers */
	currentKicadLayer = 0;
	currentGroup = 0;

	/* figure out which pcb layers are bottom copper and make a list */
	bottomCount = pcb_layer_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_COPPER, NULL, 0);
	if (bottomCount > 0) {
		bottomLayers = malloc(sizeof(pcb_layer_id_t) * bottomCount);
		pcb_layer_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_COPPER, bottomLayers, bottomCount);
	}
	else {
		bottomLayers = NULL;
	}

	/* figure out which pcb layers are internal copper layers and make a list */
	innerCount = pcb_layer_list(PCB, PCB_LYT_INTERN | PCB_LYT_COPPER, NULL, 0);
	if (innerCount > 0) {
		innerLayers = malloc(sizeof(pcb_layer_id_t) * innerCount);
		pcb_layer_list(PCB, PCB_LYT_INTERN | PCB_LYT_COPPER, innerLayers, innerCount);
	}
	else {
		innerLayers = NULL;
	}

	/* figure out which pcb layers are top copper and make a list */
	topCount = pcb_layer_list(PCB, PCB_LYT_TOP | PCB_LYT_COPPER, NULL, 0);
	if (topCount > 0) {
		topLayers = malloc(sizeof(pcb_layer_id_t) * topCount);
		pcb_layer_list(PCB, PCB_LYT_TOP | PCB_LYT_COPPER, topLayers, topCount);
	}
	else {
		topLayers = NULL;
	}

	/* figure out which pcb layers are bottom silk and make a list */
	bottomSilkCount = pcb_layer_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_SILK, NULL, 0);
	if (bottomSilkCount > 0) {
		bottomSilk = malloc(sizeof(pcb_layer_id_t) * bottomSilkCount);
		pcb_layer_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_SILK, bottomSilk, bottomSilkCount);
	}
	else {
		bottomSilk = NULL;
	}

	/* figure out which pcb layers are top silk and make a list */
	topSilkCount = pcb_layer_list(PCB, PCB_LYT_TOP | PCB_LYT_SILK, NULL, 0);
	if (topSilkCount > 0) {
		topSilk = malloc(sizeof(pcb_layer_id_t) * topSilkCount);
		pcb_layer_list(PCB, PCB_LYT_TOP | PCB_LYT_SILK, topSilk, topSilkCount);
	}
	else {
		topSilk = NULL;
	}

	/* figure out which pcb layers are outlines and make a list */
	outlineCount = pcb_layer_list(PCB, PCB_LYT_BOUNDARY, NULL, 0);
	if (outlineCount > 0) {
		outlineLayers = malloc(sizeof(pcb_layer_id_t) * outlineCount);
		pcb_layer_list(PCB, PCB_LYT_BOUNDARY, outlineLayers, outlineCount);
	}
	else {
		outlineLayers = NULL;
	}

	/* we now proceed to write the outline tracks to the kicad file, layer by layer */
	currentKicadLayer = 28; /* 28 is the edge cuts layer in kicad */
	if (outlineCount > 0) {
		for(i = 0; i < outlineCount; i++) { /* write top copper tracks, if any */
			write_kicad_legacy_layout_tracks(FP, currentKicadLayer, &(PCB->Data->Layer[outlineLayers[i]]), LayoutXOffset, LayoutYOffset);
			write_kicad_legacy_layout_arcs(FP, currentKicadLayer, &(PCB->Data->Layer[outlineLayers[i]]), LayoutXOffset, LayoutYOffset);
		}
	}
	else { /* no outline layer per se, export the board margins instead  - obviously some scope to reduce redundant code... */
		fputs("$DRAWSEGMENT\n", FP);
		pcb_fprintf(FP, "Po 0 %.0mk %.0mk %.0mk %.0mk %.0mk\n", PCB->MaxWidth / 2 - LayoutXOffset, PCB->MaxHeight / 2 - LayoutYOffset, PCB->MaxWidth / 2 + LayoutXOffset, PCB->MaxHeight / 2 - LayoutYOffset, outlineThickness);
		pcb_fprintf(FP, "De %d 0 0 0 0\n", currentKicadLayer);
		fputs("$EndDRAWSEGMENT\n", FP);
		fputs("$DRAWSEGMENT\n", FP);
		pcb_fprintf(FP, "Po 0 %.0mk %.0mk %.0mk %.0mk %.0mk\n", PCB->MaxWidth / 2 + LayoutXOffset, PCB->MaxHeight / 2 - LayoutYOffset, PCB->MaxWidth / 2 + LayoutXOffset, PCB->MaxHeight / 2 + LayoutYOffset, outlineThickness);
		pcb_fprintf(FP, "De %d 0 0 0 0\n", currentKicadLayer);
		fputs("$EndDRAWSEGMENT\n", FP);
		fputs("$DRAWSEGMENT\n", FP);
		pcb_fprintf(FP, "Po 0 %.0mk %.0mk %.0mk %.0mk %.0mk\n", PCB->MaxWidth / 2 + LayoutXOffset, PCB->MaxHeight / 2 + LayoutYOffset, PCB->MaxWidth / 2 - LayoutXOffset, PCB->MaxHeight / 2 + LayoutYOffset, outlineThickness);
		pcb_fprintf(FP, "De %d 0 0 0 0\n", currentKicadLayer);
		fputs("$EndDRAWSEGMENT\n", FP);
		fputs("$DRAWSEGMENT\n", FP);
		pcb_fprintf(FP, "Po 0 %.0mk %.0mk %.0mk %.0mk %.0mk\n", PCB->MaxWidth / 2 - LayoutXOffset, PCB->MaxHeight / 2 + LayoutYOffset, PCB->MaxWidth / 2 - LayoutXOffset, PCB->MaxHeight / 2 - LayoutYOffset, outlineThickness);
		pcb_fprintf(FP, "De %d 0 0 0 0\n", currentKicadLayer);
		fputs("$EndDRAWSEGMENT\n", FP);
	}


	/* we now proceed to write the bottom silk lines, arcs, text to the kicad legacy file, using layer 20 */
	currentKicadLayer = 20; /* 20 is the bottom silk layer in kicad */
	for(i = 0; i < bottomSilkCount; i++) { /* write bottom silk lines, if any */
		write_kicad_legacy_layout_tracks(FP, currentKicadLayer, &(PCB->Data->Layer[bottomSilk[i]]), LayoutXOffset, LayoutYOffset);
		write_kicad_legacy_layout_arcs(FP, currentKicadLayer, &(PCB->Data->Layer[bottomSilk[i]]), LayoutXOffset, LayoutYOffset);
		write_kicad_legacy_layout_text(FP, currentKicadLayer, &(PCB->Data->Layer[bottomSilk[i]]), LayoutXOffset, LayoutYOffset);
	}

	/* we now proceed to write the bottom copper text to the kicad legacy file, layer by layer */
	currentKicadLayer = 0; /* 0 is the bottom copper layer in kicad */
	for(i = 0; i < bottomCount; i++) { /* write bottom copper tracks, if any */
		write_kicad_legacy_layout_text(FP, currentKicadLayer, &(PCB->Data->Layer[bottomLayers[i]]), LayoutXOffset, LayoutYOffset);
	} /* 0 is the bottom most track in kicad */

	/* we now proceed to write the internal copper text to the kicad file, layer by layer */
	if (innerCount > 0) {
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
		write_kicad_legacy_layout_text(FP, currentKicadLayer, &(PCB->Data->Layer[innerLayers[i]]), LayoutXOffset, LayoutYOffset);
	}

	/* we now proceed to write the top copper text to the kicad legacy file, layer by layer */
	currentKicadLayer = 15; /* 15 is the top most copper layer in kicad */
	for(i = 0; i < topCount; i++) { /* write top copper tracks, if any */
		write_kicad_legacy_layout_text(FP, currentKicadLayer, &(PCB->Data->Layer[topLayers[i]]), LayoutXOffset, LayoutYOffset);
	}

	/* we now proceed to write the top silk lines, arcs, text to the kicad legacy file, using layer 21 */
	currentKicadLayer = 21; /* 21 is the top silk layer in kicad */
	for(i = 0; i < topSilkCount; i++) { /* write top silk lines, if any */
		write_kicad_legacy_layout_tracks(FP, currentKicadLayer, &(PCB->Data->Layer[topSilk[i]]), LayoutXOffset, LayoutYOffset);
		write_kicad_legacy_layout_arcs(FP, currentKicadLayer, &(PCB->Data->Layer[topSilk[i]]), LayoutXOffset, LayoutYOffset);
		write_kicad_legacy_layout_text(FP, currentKicadLayer, &(PCB->Data->Layer[topSilk[i]]), LayoutXOffset, LayoutYOffset);
	}

	/* having done the graphical elements, we move onto tracks and vias */
	fputs("$TRACK\n", FP);
	write_kicad_legacy_layout_vias(FP, PCB->Data, LayoutXOffset, LayoutYOffset);

	/* we now proceed to write the bottom copper tracks to the kicad legacy file, layer by layer */
	currentKicadLayer = 0; /* 0 is the bottom copper layer in kicad */
	for(i = 0; i < bottomCount; i++) { /* write bottom copper tracks, if any */
		write_kicad_legacy_layout_tracks(FP, currentKicadLayer, &(PCB->Data->Layer[bottomLayers[i]]), LayoutXOffset, LayoutYOffset);
		write_kicad_legacy_layout_arcs(FP, currentKicadLayer, &(PCB->Data->Layer[bottomLayers[i]]), LayoutXOffset, LayoutYOffset);
	} /* 0 is the bottom most track in kicad */

	/* we now proceed to write the internal copper tracks to the kicad file, layer by layer */
	if (innerCount > 0) {
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
		write_kicad_legacy_layout_tracks(FP, currentKicadLayer, &(PCB->Data->Layer[innerLayers[i]]), LayoutXOffset, LayoutYOffset);
		write_kicad_legacy_layout_arcs(FP, currentKicadLayer, &(PCB->Data->Layer[innerLayers[i]]), LayoutXOffset, LayoutYOffset);
	}

	/* we now proceed to write the top copper tracks to the kicad legacy file, layer by layer */
	currentKicadLayer = 15; /* 15 is the top most copper layer in kicad */
	for(i = 0; i < topCount; i++) { /* write top copper tracks, if any */
		write_kicad_legacy_layout_tracks(FP, currentKicadLayer, &(PCB->Data->Layer[topLayers[i]]), LayoutXOffset, LayoutYOffset);
		write_kicad_legacy_layout_arcs(FP, currentKicadLayer, &(PCB->Data->Layer[topLayers[i]]), LayoutXOffset, LayoutYOffset);
	}
	fputs("$EndTRACK\n", FP);

	/*
	 * now we proceed to write polygons for each layer, and iterate much like we did for tracks
	 */

	/* we now proceed to write the bottom silk polygons  to the kicad legacy file, using layer 20 */
	currentKicadLayer = 20; /* 20 is the bottom silk layer in kicad */
	for(i = 0; i < bottomSilkCount; i++) { /* write bottom silk polygons, if any */
		write_kicad_legacy_layout_polygons(FP, currentKicadLayer, &(PCB->Data->Layer[bottomSilk[i]]), LayoutXOffset, LayoutYOffset);
	}

	/* we now proceed to write the bottom copper polygons to the kicad legacy file, layer by layer */
	currentKicadLayer = 0; /* 0 is the bottom copper layer in kicad */
	for(i = 0; i < bottomCount; i++) { /* write bottom copper polygons, if any */
		write_kicad_legacy_layout_polygons(FP, currentKicadLayer, &(PCB->Data->Layer[bottomLayers[i]]), LayoutXOffset, LayoutYOffset);
	} /* 0 is the bottom most track in kicad */

	/* we now proceed to write the internal copper polygons to the kicad file, layer by layer */
	if (innerCount > 0) {
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
		write_kicad_legacy_layout_polygons(FP, currentKicadLayer, &(PCB->Data->Layer[innerLayers[i]]), LayoutXOffset, LayoutYOffset);
	}

	/* we now proceed to write the top copper polygons to the kicad legacy file, layer by layer */
	currentKicadLayer = 15; /* 15 is the top most copper layer in kicad */
	for(i = 0; i < topCount; i++) { /* write top copper polygons, if any */
		write_kicad_legacy_layout_polygons(FP, currentKicadLayer, &(PCB->Data->Layer[topLayers[i]]), LayoutXOffset, LayoutYOffset);
	}

	/* we now proceed to write the top silk polygons to the kicad legacy file, using layer 21 */
	currentKicadLayer = 21; /* 21 is the top silk layer in kicad */
	for(i = 0; i < topSilkCount; i++) { /* write top silk polygons, if any */
		write_kicad_legacy_layout_polygons(FP, currentKicadLayer, &(PCB->Data->Layer[topSilk[i]]), LayoutXOffset, LayoutYOffset);
	}


	fputs("$EndBOARD\n", FP);

	/* now free memory from arrays that were used */
	if (bottomCount > 0) {
		free(bottomLayers);
	}
	if (innerCount > 0) {
		free(innerLayers);
	}
	if (topCount > 0) {
		free(topLayers);
	}
	if (topSilkCount > 0) {
		free(topSilk);
	}
	if (bottomSilkCount > 0) {
		free(bottomSilk);
	}
	if (outlineCount > 0) {
		free(outlineLayers);
	}
	return 0;
}

int io_kicad_legacy_write_element(pcb_plug_io_t *ctx, FILE *FP, pcb_data_t *Data)
{
	gdl_iterator_t sit;
	pcb_subc_t *subc;
	unm_t group1;
	int res = 0;

	unm_init(&group1);
	subclist_foreach(&Data->subc, &sit, subc) {
		const char *uname = unm_name(&group1, or_empty(pcb_attribute_get(&subc->Attributes, "footprint")), subc);
		res |= io_kicad_legacy_write_subc(FP, PCB, subc, 0, 0, uname);
	}
	unm_uninit(&group1);

	return res;
}
