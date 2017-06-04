/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *
 *  This module, rats.c, was written and is Copyright (C) 1997 by harry eaton
 *  this module is also subject to the GNU GPL as described below
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"

#include "buffer.h"
#include "board.h"
#include "crosshair.h"
#include "data.h"
#include "error.h"
#include "obj_subc.h"

pcb_subc_t *pcb_subc_alloc(void)
{
	pcb_subc_t *sc;
	sc = calloc(sizeof(pcb_subc_t), 1);
	sc->data = pcb_data_new(NULL);
	PCB_SET_PARENT(sc->data, subc, sc);
	return sc;
}

pcb_layer_t *pcb_subc_layer_create(pcb_subc_t *sc, pcb_layer_t *src)
{
	pcb_layer_t *dst = &sc->data->Layer[sc->data->LayerN++];
	dst->parent = sc->data;
	dst->grp = src->grp;
	dst->comb = src->comb;
	
	dst->line_tree = src->line_tree;
	dst->text_tree = src->text_tree;
	dst->polygon_tree = src->polygon_tree;
	dst->subc_tree = src->subc_tree;


	dst->meta.bound.real = src;
	dst->meta.bound.type = pcb_layergrp_flags(PCB, src->grp);

	if (dst->meta.bound.type & PCB_LYT_INTERN) {
#warning TODO: calculate inner layer stack offset - needs a stack
		dst->meta.bound.stack_offs = 0;
	}
	else
		dst->meta.bound.stack_offs = 0;
	return dst;
}

int pcb_subc_convert_from_buffer(pcb_buffer_t *buffer)
{
	pcb_subc_t *sc;
	int n;

	sc = pcb_subc_alloc();
	PCB_SET_PARENT(sc->data, data, buffer->Data);
	pcb_subclist_append(&sc->data->subc, sc);

	/* create layer matches and copy objects */
	for(n = 0; n < PCB_MAX_LAYER; n++) {
		pcb_layer_t *dst, *src;
		pcb_line_t *line;
		pcb_text_t *text;
		pcb_polygon_t *poly;
		pcb_arc_t *arc;

		if (pcb_layer_is_pure_empty(&buffer->Data->Layer[n]))
			continue;

		src = &buffer->Data->Layer[n];
		dst = pcb_subc_layer_create(sc, src);

		while((line = linelist_first(&src->Line)) != NULL) {
			linelist_remove(line);
			linelist_append(&dst->Line, line);
			PCB_SET_PARENT(line, subc, sc);
		}

		while((arc = arclist_first(&src->Arc)) != NULL) {
			arclist_remove(arc);
			arclist_append(&dst->Arc, arc);
			PCB_SET_PARENT(arc, subc, sc);
		}

		while((text = textlist_first(&src->Text)) != NULL) {
			textlist_remove(text);
			textlist_append(&dst->Text, text);
			PCB_SET_PARENT(text, subc, sc);
		}

		while((poly = polylist_first(&src->Polygon)) != NULL) {
			polylist_remove(poly);
			polylist_append(&dst->Polygon, poly);
			PCB_SET_PARENT(poly, subc, sc);
		}
	}

	pcb_message(PCB_MSG_ERROR, "pcb_subc_convert_from_buffer(): not yet implemented\n");
	return -1;
}

void XORDrawSubc(pcb_subc_t *sc, pcb_coord_t DX, pcb_coord_t DY)
{
	/* draw the bounding box */
#if 0
	pcb_gui->draw_line(pcb_crosshair.GC,
								 DX + sc->BoundingBox.X1,
								 DY + sc->BoundingBox.Y1, DX + sc->BoundingBox.X1, DY + sc->BoundingBox.Y2);
	pcb_gui->draw_line(pcb_crosshair.GC,
								 DX + sc->BoundingBox.X1,
								 DY + sc->BoundingBox.Y2, DX + sc->BoundingBox.X2, DY + sc->BoundingBox.Y2);
	pcb_gui->draw_line(pcb_crosshair.GC,
								 DX + sc->BoundingBox.X2,
								 DY + sc->BoundingBox.Y2, DX + sc->BoundingBox.X2, DY + sc->BoundingBox.Y1);
	pcb_gui->draw_line(pcb_crosshair.GC,
								 DX + sc->BoundingBox.X2,
								 DY + sc->BoundingBox.Y1, DX + sc->BoundingBox.X1, DY + sc->BoundingBox.Y1);
#endif

#if 0
	{
		PCB_ELEMENT_PCB_LINE_LOOP(Element);
		{
			pcb_gui->draw_line(pcb_crosshair.GC, DX + line->Point1.X, DY + line->Point1.Y, DX + line->Point2.X, DY + line->Point2.Y);
		}
		PCB_END_LOOP;

		/* arc coordinates and angles have to be converted to X11 notation */
		PCB_ARC_LOOP(Element);
		{
			pcb_gui->draw_arc(pcb_crosshair.GC, DX + arc->X, DY + arc->Y, arc->Width, arc->Height, arc->StartAngle, arc->Delta);
		}
		PCB_END_LOOP;
	}
#endif

	/* mark */
	pcb_gui->draw_line(pcb_crosshair.GC, DX - PCB_EMARK_SIZE, DY, DX, DY - PCB_EMARK_SIZE);
	pcb_gui->draw_line(pcb_crosshair.GC, DX + PCB_EMARK_SIZE, DY, DX, DY - PCB_EMARK_SIZE);
	pcb_gui->draw_line(pcb_crosshair.GC, DX - PCB_EMARK_SIZE, DY, DX, DY + PCB_EMARK_SIZE);
	pcb_gui->draw_line(pcb_crosshair.GC, DX + PCB_EMARK_SIZE, DY, DX, DY + PCB_EMARK_SIZE);
}
