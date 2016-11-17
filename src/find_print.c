/*
 *
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* Connection export/output functions */

static pcb_bool PrepareNextLoop(FILE * FP);

/* ---------------------------------------------------------------------------
 * prints all unused pins of an element to file FP
 */
static pcb_bool PrintAndSelectUnusedPinsAndPadsOfElement(pcb_element_t *Element, FILE * FP)
{
	pcb_bool first = pcb_true;
	pcb_cardinal_t number;

	/* check all pins in element */

	PCB_PIN_LOOP(Element);
	{
		if (!PCB_FLAG_TEST(PCB_FLAG_HOLE, pin)) {
			/* pin might have bee checked before, add to list if not */
			if (!PCB_FLAG_TEST(TheFlag, pin) && FP) {
				int i;
				if (ADD_PV_TO_LIST(pin, 0, NULL, 0))
					return pcb_true;
				DoIt(pcb_true, pcb_true);
				number = PadList[COMPONENT_LAYER].Number + PadList[SOLDER_LAYER].Number + PVList.Number;
				/* the pin has no connection if it's the only
				 * list entry; don't count vias
				 */
				for (i = 0; i < PVList.Number; i++)
					if (!PVLIST_ENTRY(i)->Element)
						number--;
				if (number == 1) {
					/* output of element name if not already done */
					if (first) {
						PrintConnectionElementName(Element, FP);
						first = pcb_false;
					}

					/* write name to list and draw selected object */
					fputc('\t', FP);
					pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pin->Name));
					fputc('\n', FP);
					PCB_FLAG_SET(PCB_FLAG_SELECTED, pin);
					DrawPin(pin);
				}

				/* reset found objects for the next pin */
				if (PrepareNextLoop(FP))
					return (pcb_true);
			}
		}
	}
	END_LOOP;

	/* check all pads in element */
	PCB_PAD_LOOP(Element);
	{
		/* lookup pad in list */
		/* pad might has bee checked before, add to list if not */
		if (!PCB_FLAG_TEST(TheFlag, pad) && FP) {
			int i;
			if (ADD_PAD_TO_LIST(PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad)
													? SOLDER_LAYER : COMPONENT_LAYER, pad, 0, NULL, 0))
				return pcb_true;
			DoIt(pcb_true, pcb_true);
			number = PadList[COMPONENT_LAYER].Number + PadList[SOLDER_LAYER].Number + PVList.Number;
			/* the pin has no connection if it's the only
			 * list entry; don't count vias
			 */
			for (i = 0; i < PVList.Number; i++)
				if (!PVLIST_ENTRY(i)->Element)
					number--;
			if (number == 1) {
				/* output of element name if not already done */
				if (first) {
					PrintConnectionElementName(Element, FP);
					first = pcb_false;
				}

				/* write name to list and draw selected object */
				fputc('\t', FP);
				pcb_print_quoted_string(FP, (char *) PCB_EMPTY(pad->Name));
				fputc('\n', FP);

				PCB_FLAG_SET(PCB_FLAG_SELECTED, pad);
				DrawPad(pad);
			}

			/* reset found objects for the next pin */
			if (PrepareNextLoop(FP))
				return (pcb_true);
		}
	}
	END_LOOP;

	/* print separator if element has unused pins or pads */
	if (!first) {
		fputs("}\n\n", FP);
		SEPARATE(FP);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * resets some flags for looking up the next pin/pad
 */
static pcb_bool PrepareNextLoop(FILE * FP)
{
	pcb_cardinal_t layer;

	/* reset found LOs for the next pin */
	for (layer = 0; layer < pcb_max_copper_layer; layer++) {
		LineList[layer].Location = LineList[layer].Number = 0;
		ArcList[layer].Location = ArcList[layer].Number = 0;
		PolygonList[layer].Location = PolygonList[layer].Number = 0;
	}

	/* reset found pads */
	for (layer = 0; layer < 2; layer++)
		PadList[layer].Location = PadList[layer].Number = 0;

	/* reset PVs */
	PVList.Number = PVList.Location = 0;
	RatList.Number = RatList.Location = 0;

	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * finds all connections to the pins of the passed element.
 * The result is written to file FP
 * Returns pcb_true if operation was aborted
 */
static pcb_bool PrintElementConnections(pcb_element_t *Element, FILE * FP, pcb_bool AndDraw)
{
	PrintConnectionElementName(Element, FP);

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
		PrintPadConnections(COMPONENT_LAYER, FP, pcb_false);
		PrintPadConnections(SOLDER_LAYER, FP, pcb_false);
		fputs("\t}\n", FP);
		if (PrepareNextLoop(FP))
			return (pcb_true);
	}
	END_LOOP;

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
		layer = PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, pad) ? SOLDER_LAYER : COMPONENT_LAYER;
		if (ADD_PAD_TO_LIST(layer, pad, PCB_TYPE_ELEMENT, Element, PCB_FCT_ELEMENT))
			return pcb_true;
		DoIt(pcb_true, AndDraw);
		/* print all found connections */
		PrintPadConnections(layer, FP, pcb_true);
		PrintPadConnections(layer == (COMPONENT_LAYER ? SOLDER_LAYER : COMPONENT_LAYER), FP, pcb_false);
		PrintPinConnections(FP, pcb_false);
		fputs("\t}\n", FP);
		if (PrepareNextLoop(FP))
			return (pcb_true);
	}
	END_LOOP;
	fputs("}\n\n", FP);
	return (pcb_false);
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

	PCB_ELEMENT_LOOP(PCB->Data);
	{
		/* break if abort dialog returned pcb_true;
		 * passing NULL as filedescriptor discards the normal output
		 */
		if (PrintAndSelectUnusedPinsAndPadsOfElement(element, FP))
			break;
	}
	END_LOOP;

	if (conf_core.editor.beep_when_finished)
		gui->beep();
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
	PrintElementConnections(Element, FP, pcb_true);
	pcb_board_set_changed_flag(pcb_true);
	if (conf_core.editor.beep_when_finished)
		gui->beep();
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

	PCB_ELEMENT_LOOP(PCB->Data);
	{
		/* break if abort dialog returned pcb_true */
		if (PrintElementConnections(element, FP, pcb_false))
			break;
		SEPARATE(FP);
		if (conf_core.editor.reset_after_element && gdl_it_idx(&__it__) != 1)
			pcb_reset_conns(pcb_false);
	}
	END_LOOP;
	if (conf_core.editor.beep_when_finished)
		gui->beep();
	pcb_reset_conns(pcb_false);
	pcb_conn_lookup_uninit();
	pcb_redraw();
}
