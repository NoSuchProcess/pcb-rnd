/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include "board.h"
#include "data.h"
#include "error.h"

#define CHK "Broken integrity: "

#define check_parent(name, obj, pt, prnt) \
	do { \
		if (obj->parent_type != pt) \
			pcb_message(PCB_MSG_ERROR, CHK "%s " name " %ld parent type proken (%d != %d)\n", whose, obj->ID, obj->parent_type, pt); \
		else if (obj->parent.any != prnt) \
			pcb_message(PCB_MSG_ERROR, CHK "%s " name " %ld parent type proken (%p != %p)\n", whose, obj->ID, obj->parent.any, prnt); \
	} while(0)

#define check_field_eq(name, obj, st1, st2, fld, fmt) \
	do { \
		if ((st1)->fld != (st2)->fld) \
			pcb_message(PCB_MSG_ERROR, CHK "%s " name " field ." #fld " value mismatch (" fmt " != " fmt ")\n", whose, obj->ID, (st1)->fld, (st2)->fld); \
	} while(0)


static void chk_element(const char *whose, pcb_element_t *elem)
{
	int n;
	pcb_pin_t *pin;
	pcb_pad_t *pad;
	pcb_line_t *lin;
	pcb_arc_t *arc;

	for(pin = pinlist_first(&elem->Pin); pin != NULL; pin = pinlist_next(pin))
		check_parent("pin", pin, PCB_PARENT_ELEMENT, elem);

	for(pad = padlist_first(&elem->Pad); pad != NULL; pad = padlist_next(pad))
		check_parent("pad", pad, PCB_PARENT_ELEMENT, elem);

	for(lin = linelist_first(&elem->Line); lin != NULL; lin = linelist_next(lin))
		check_parent("line", lin, PCB_PARENT_ELEMENT, elem);

	for(arc = arclist_first(&elem->Arc); arc != NULL; arc = arclist_next(arc))
		check_parent("arc", arc, PCB_PARENT_ELEMENT, elem);

	for(n = 1; n < PCB_MAX_ELEMENTNAMES; n++) {
		check_field_eq("element name", elem, &elem->Name[n], &elem->Name[0], X, "%mm");
		check_field_eq("element name", elem, &elem->Name[n], &elem->Name[0], Y, "%mm");
		check_field_eq("element name", elem, &elem->Name[n], &elem->Name[0], Direction, "%d");
		check_field_eq("element name", elem, &elem->Name[n], &elem->Name[0], fid, "%d");
	}
}

/* Check layers and objects: walk the tree top->down and check ->parent
   references from any node */
static void chk_layers(const char *whose, pcb_data_t *data, pcb_parenttype_t pt, void *parent, int name_chk)
{
	pcb_layer_id_t n;

	if (data->parent_type != pt)
		pcb_message(PCB_MSG_ERROR, CHK "%s data: parent type proken (%d != %d)\n", whose, data->parent_type, pt);
	else if (data->parent.any != parent)
		pcb_message(PCB_MSG_ERROR, CHK "%s data: parent proken (%p != %p)\n", whose, data->parent, parent);


	for(n = 0; n < pcb_max_layer; n++) {
		pcb_line_t *lin;
		pcb_text_t *txt;
		pcb_arc_t *arc;
		pcb_polygon_t *poly;
	
		/* check layers */
		if (data->Layer[n].parent != data)
			pcb_message(PCB_MSG_ERROR, CHK "%s layer %ld/%s parent proken (%p != %p)\n", whose, n, data->Layer[n].meta.real.name, data->Layer[n].parent, data);
		if (name_chk && ((data->Layer[n].meta.real.name == NULL) || (*data->Layer[n].meta.real.name == '\0')))
			pcb_message(PCB_MSG_ERROR, CHK "%s layer %ld has invalid name\n", whose, n);

		/* check layer objects */
		for(lin = linelist_first(&data->Layer[n].Line); lin != NULL; lin = linelist_next(lin))
			check_parent("line", lin, PCB_PARENT_LAYER, &data->Layer[n]);

		for(txt = textlist_first(&data->Layer[n].Text); txt != NULL; txt = textlist_next(txt))
			check_parent("text", txt, PCB_PARENT_LAYER, &data->Layer[n]);

		for(poly = polylist_first(&data->Layer[n].Polygon); poly != NULL; poly = polylist_next(poly))
			check_parent("polygon", poly, PCB_PARENT_LAYER, &data->Layer[n]);

		for(arc = arclist_first(&data->Layer[n].Arc); arc != NULL; arc = arclist_next(arc))
			check_parent("arc", arc, PCB_PARENT_LAYER, &data->Layer[n]);

	}

	/* check global objects */
	{
		pcb_pin_t *via;
		pcb_element_t *elem;

		for(via = pinlist_first(&data->Via); via != NULL; via = pinlist_next(via))
			check_parent("via", via, PCB_PARENT_DATA, data);

		for(elem = elementlist_first(&data->Element); elem != NULL; elem = elementlist_next(elem)) {
			check_parent("element", elem, PCB_PARENT_DATA, data);
			chk_element(whose, elem);
		}
	}
}

void pcb_check_integrity(pcb_board_t *pcb)
{
	int n;
	chk_layers("board", pcb->Data, PCB_PARENT_BOARD, pcb, 1);

	for (n = 0; n < PCB_MAX_BUFFER; n++) {
		char bn[16];
		sprintf(bn, "buffer #%d", n);
		chk_layers(bn, pcb_buffers[n].Data, PCB_PARENT_INVALID, NULL, 0);
	}
}
