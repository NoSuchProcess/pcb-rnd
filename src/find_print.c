/*
 *
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include "data_it.h"

/* Connection export/output functions */

static void PrepareNextLoop(void);

/* Count terminals that are found, up to maxt */
static void count_terms(pcb_data_t *data, pcb_cardinal_t maxt, pcb_cardinal_t *cnt)
{
	pcb_any_obj_t *o;
	pcb_data_it_t it;

	for(o = pcb_data_first(&it, data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it)) {
		if ((o->term != NULL) && PCB_FLAG_TEST(PCB_FLAG_FOUND, o)) {
char *rf = "<none>";
if (data->parent_type == PCB_PARENT_SUBC) rf = data->parent.subc->refdes;
printf("  %s-%s\n", rf, o->term);
			(*cnt)++;
		}
		if (o->type == PCB_OBJ_SUBC)
			count_terms(((pcb_subc_t *)o)->data, maxt, cnt);
		if (*cnt >= maxt)
			return *cnt;
	}
	return *cnt;
}

/* prints all unused pins of an element to file FP */
static pcb_bool print_select_unused_subc_terms(pcb_subc_t *subc, FILE * FP)
{
	pcb_any_obj_t *o;
	pcb_data_it_t it;
	pcb_bool first = pcb_true;

	for(o = pcb_data_first(&it, subc->data, PCB_OBJ_CLASS_REAL); o != NULL; o = pcb_data_next(&it)) {
		pcb_cardinal_t number = 0;
		if (o->term == NULL) /* consider named terminals only */
			continue;

		/* reset found objects for the next pin */
		PrepareNextLoop();

		ListStart(o);
		DoIt(pcb_true, pcb_true);

		printf("%s-%s:\n", subc->refdes, o->term);
		count_terms(PCB->Data, 2, &number);
		printf(" number=%d\n", number);
		if (number <= 1) {
			/* output of element name if not already done */
			if (first) {
				pcb_print_conn_subc_name(subc, FP);
				first = pcb_false;
			}

			/* write name to list */
			fputc('\t', FP);
			pcb_print_quoted_string(FP, (char *)PCB_EMPTY(o->term));
			fputc('\n', FP);
			PCB_FLAG_SET(PCB_FLAG_SELECTED, o);
			pcb_draw_obj(o);
		}
	}

	/* print separator if element has unused pins or pads */
	if (!first) {
		fputs("}\n\n", FP);
		SEPARATE(FP);
	}
	return pcb_false;
}

/* ---------------------------------------------------------------------------
 * resets some flags for looking up the next pin/pad
 */
static void PrepareNextLoop(void)
{
	pcb_cardinal_t layer;

	/* reset found LOs for the next pin */
	for (layer = 0; layer < pcb_max_layer; layer++) {
		LineList[layer].Location = LineList[layer].Number = 0;
		ArcList[layer].Location = ArcList[layer].Number = 0;
		PolygonList[layer].Location = PolygonList[layer].Number = 0;
	}

	/* reset Padstacks */
	PadstackList.Number = PadstackList.Location = 0;
	pcb_reset_conns(pcb_false);
}

/* ---------------------------------------------------------------------------
 * finds all connections to the pins of the passed element.
 * The result is written to file FP
 * Returns pcb_true if operation was aborted
 */
static pcb_bool pcb_print_subc_conns(pcb_subc_t *subc, FILE * FP, pcb_bool AndDraw)
{
	pcb_print_conn_subc_name(subc, FP);

#warning padstack TODO:
#if 0
	/* check all pins in element */
	PCB_PIN_LOOP(Element);
	{
		/* pin might have been checked before, add to list if not */
		if (PCB_FLAG_TEST(TheFlag, pin)) {
			PrintConnectionListEntry((char *) PCB_EMPTY(pin->Name), NULL, pcb_true, FP);
			fputs("\t\t__CHECKED_BEFORE__\n\t}\n", FP);
			continue;
		}
		if (ADD_PV_TO_LIST(pin, PCB_TYPE_ELEMENT, Element, PCB_FCT_ELEMENT))
			return pcb_true;
		DoIt(pcb_true, AndDraw);
		/* printout all found connections */
		PrintPinConnections(FP, pcb_true);
		PrintPadConnections(PCB_COMPONENT_SIDE, FP, pcb_false);
		PrintPadConnections(PCB_SOLDER_SIDE, FP, pcb_false);
		fputs("\t}\n", FP);
		PrepareNextLoop();
	}
	PCB_END_LOOP;

	/* check all pads in element */
	PCB_PAD_LOOP(Element);
	{
		pcb_cardinal_t layer;
		/* pad might have been checked before, add to list if not */
		if (PCB_FLAG_TEST(TheFlag, pad)) {
			PrintConnectionListEntry((char *) PCB_EMPTY(pad->Name), NULL, pcb_true, FP);
			fputs("\t\t__CHECKED_BEFORE__\n\t}\n", FP);
			continue;
		}
		layer = PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad) ? PCB_SOLDER_SIDE : PCB_COMPONENT_SIDE;
		if (ADD_PAD_TO_LIST(layer, pad, PCB_TYPE_ELEMENT, Element, PCB_FCT_ELEMENT))
			return pcb_true;
		DoIt(pcb_true, AndDraw);
		/* print all found connections */
		PrintPadConnections(layer, FP, pcb_true);
		PrintPadConnections(layer == (PCB_COMPONENT_SIDE ? PCB_SOLDER_SIDE : PCB_COMPONENT_SIDE), FP, pcb_false);
		PrintPinConnections(FP, pcb_false);
		fputs("\t}\n", FP);
		PrepareNextLoop(FP);
	}
	PCB_END_LOOP;
#endif
	fputs("}\n\n", FP);
	return pcb_false;
}

/* ---------------------------------------------------------------------------
 * find all unused pins of all element
 */
void pcb_lookup_unused_pins(FILE * FP)
{
	/* reset all currently marked connections */
	User = pcb_true;
	pcb_reset_conns(pcb_true);
	pcb_conn_lookup_init();

	PCB_SUBC_LOOP(PCB->Data);
	{
		/* break if abort dialog returned pcb_true;
		 * passing NULL as filedescriptor discards the normal output */
		if (print_select_unused_subc_terms(subc, FP))
			break;
	}
	PCB_END_LOOP;

	if (conf_core.editor.beep_when_finished)
		pcb_gui->beep();
	pcb_conn_lookup_uninit();
	pcb_undo_inc_serial();
	User = pcb_false;
	pcb_draw();
}

/* ---------------------------------------------------------------------------
 * find all connections to pins within one element
 */
void pcb_lookup_element_conns(pcb_element_t *Element, FILE * FP)
{
	/* reset all currently marked connections */
	User = pcb_true;
	TheFlag = PCB_FLAG_FOUND;
	pcb_reset_conns(pcb_true);
	pcb_conn_lookup_init();
	pcb_print_subc_conns(Element, FP, pcb_true);
	pcb_board_set_changed_flag(pcb_true);
	if (conf_core.editor.beep_when_finished)
		pcb_gui->beep();
	pcb_conn_lookup_uninit();
	pcb_undo_inc_serial();
	User = pcb_false;
	pcb_draw();
}


/* ---------------------------------------------------------------------------
 * find all connections to pins of all element
 */
void pcb_lookup_conns_to_all_elements(FILE * FP)
{
	/* reset all currently marked connections */
	User = pcb_false;
	TheFlag = PCB_FLAG_FOUND;
	pcb_reset_conns(pcb_false);
	pcb_conn_lookup_init();

#warning subc TODO: rewrite for subcs
#if 0
	PCB_ELEMENT_LOOP(PCB->Data);
	{
		/* break if abort dialog returned pcb_true */
		if (pcb_print_subc_conns(element, FP, pcb_false))
			break;
		SEPARATE(FP);
		if (conf_core.editor.reset_after_element && gdl_it_idx(&__it__) != 1)
			pcb_reset_conns(pcb_false);
	}
	PCB_END_LOOP;
#endif
	if (conf_core.editor.beep_when_finished)
		pcb_gui->beep();
	pcb_reset_conns(pcb_false);
	pcb_conn_lookup_uninit();
	pcb_redraw();
}
