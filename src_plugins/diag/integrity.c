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
		pcb_pin_t *via;
		pcb_element_t *elem;

		/* check layers */
		if (data->Layer[n].parent != data)
			pcb_message(PCB_MSG_ERROR, CHK "%s layer %ld/%s parent proken (%p != %p)\n", whose, n, data->Layer[n].Name, data->Layer[n].parent, data);
		if (name_chk && ((data->Layer[n].Name == NULL) || (*data->Layer[n].Name == '\0')))
			pcb_message(PCB_MSG_ERROR, CHK "%s layer %ld has invalid name\n", whose, n);

		/* check global objects */
		for(via = pinlist_first(&data->Via); via != NULL; via = pinlist_next(via))
			check_parent("via", via, PCB_PARENT_DATA, data);

		for(elem = elementlist_first(&data->Element); elem != NULL; elem = elementlist_next(elem))
			check_parent("element", elem, PCB_PARENT_DATA, data);

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
