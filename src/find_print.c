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

static bool PrepareNextLoop(FILE * FP);

/* ---------------------------------------------------------------------------
 * prints all unused pins of an element to file FP
 */
static bool PrintAndSelectUnusedPinsAndPadsOfElement(ElementTypePtr Element, FILE * FP)
{
	bool first = true;
	Cardinal number;

	/* check all pins in element */

	PIN_LOOP(Element);
	{
		if (!TEST_FLAG(HOLEFLAG, pin)) {
			/* pin might have bee checked before, add to list if not */
			if (!TEST_FLAG(TheFlag, pin) && FP) {
				int i;
				if (ADD_PV_TO_LIST(pin, 0, NULL, 0))
					return true;
				DoIt(true, true);
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
						first = false;
					}

					/* write name to list and draw selected object */
					fputc('\t', FP);
					PrintQuotedString(FP, (char *) EMPTY(pin->Name));
					fputc('\n', FP);
					SET_FLAG(SELECTEDFLAG, pin);
					DrawPin(pin);
				}

				/* reset found objects for the next pin */
				if (PrepareNextLoop(FP))
					return (true);
			}
		}
	}
	END_LOOP;

	/* check all pads in element */
	PAD_LOOP(Element);
	{
		/* lookup pad in list */
		/* pad might has bee checked before, add to list if not */
		if (!TEST_FLAG(TheFlag, pad) && FP) {
			int i;
			if (ADD_PAD_TO_LIST(TEST_FLAG(ONSOLDERFLAG, pad)
													? SOLDER_LAYER : COMPONENT_LAYER, pad, 0, NULL, 0))
				return true;
			DoIt(true, true);
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
					first = false;
				}

				/* write name to list and draw selected object */
				fputc('\t', FP);
				PrintQuotedString(FP, (char *) EMPTY(pad->Name));
				fputc('\n', FP);

				SET_FLAG(SELECTEDFLAG, pad);
				DrawPad(pad);
			}

			/* reset found objects for the next pin */
			if (PrepareNextLoop(FP))
				return (true);
		}
	}
	END_LOOP;

	/* print separator if element has unused pins or pads */
	if (!first) {
		fputs("}\n\n", FP);
		SEPARATE(FP);
	}
	return (false);
}

/* ---------------------------------------------------------------------------
 * resets some flags for looking up the next pin/pad
 */
static bool PrepareNextLoop(FILE * FP)
{
	Cardinal layer;

	/* reset found LOs for the next pin */
	for (layer = 0; layer < max_copper_layer; layer++) {
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

	return (false);
}

/* ---------------------------------------------------------------------------
 * finds all connections to the pins of the passed element.
 * The result is written to file FP
 * Returns true if operation was aborted
 */
static bool PrintElementConnections(ElementTypePtr Element, FILE * FP, bool AndDraw)
{
	PrintConnectionElementName(Element, FP);

	/* check all pins in element */
	PIN_LOOP(Element);
	{
		/* pin might have been checked before, add to list if not */
		if (TEST_FLAG(TheFlag, pin)) {
			PrintConnectionListEntry((char *) EMPTY(pin->Name), NULL, true, FP);
			fputs("\t\t__CHECKED_BEFORE__\n\t}\n", FP);
			continue;
		}
		if (ADD_PV_TO_LIST(pin, ELEMENT_TYPE, Element, FCT_ELEMENT))
			return true;
		DoIt(true, AndDraw);
		/* printout all found connections */
		PrintPinConnections(FP, true);
		PrintPadConnections(COMPONENT_LAYER, FP, false);
		PrintPadConnections(SOLDER_LAYER, FP, false);
		fputs("\t}\n", FP);
		if (PrepareNextLoop(FP))
			return (true);
	}
	END_LOOP;

	/* check all pads in element */
	PAD_LOOP(Element);
	{
		Cardinal layer;
		/* pad might have been checked before, add to list if not */
		if (TEST_FLAG(TheFlag, pad)) {
			PrintConnectionListEntry((char *) EMPTY(pad->Name), NULL, true, FP);
			fputs("\t\t__CHECKED_BEFORE__\n\t}\n", FP);
			continue;
		}
		layer = TEST_FLAG(ONSOLDERFLAG, pad) ? SOLDER_LAYER : COMPONENT_LAYER;
		if (ADD_PAD_TO_LIST(layer, pad, ELEMENT_TYPE, Element, FCT_ELEMENT))
			return true;
		DoIt(true, AndDraw);
		/* print all found connections */
		PrintPadConnections(layer, FP, true);
		PrintPadConnections(layer == (COMPONENT_LAYER ? SOLDER_LAYER : COMPONENT_LAYER), FP, false);
		PrintPinConnections(FP, false);
		fputs("\t}\n", FP);
		if (PrepareNextLoop(FP))
			return (true);
	}
	END_LOOP;
	fputs("}\n\n", FP);
	return (false);
}

/* ---------------------------------------------------------------------------
 * find all unused pins of all element
 */
void LookupUnusedPins(FILE * FP)
{
	/* reset all currently marked connections */
	User = true;
	ResetConnections(true);
	InitConnectionLookup();

	ELEMENT_LOOP(PCB->Data);
	{
		/* break if abort dialog returned true;
		 * passing NULL as filedescriptor discards the normal output
		 */
		if (PrintAndSelectUnusedPinsAndPadsOfElement(element, FP))
			break;
	}
	END_LOOP;

	if (conf_core.editor.beep_when_finished)
		gui->beep();
	FreeConnectionLookupMemory();
	IncrementUndoSerialNumber();
	User = false;
	Draw();
}

/* ---------------------------------------------------------------------------
 * find all connections to pins within one element
 */
void LookupElementConnections(ElementTypePtr Element, FILE * FP)
{
	/* reset all currently marked connections */
	User = true;
	TheFlag = FOUNDFLAG;
	ResetConnections(true);
	InitConnectionLookup();
	PrintElementConnections(Element, FP, true);
	SetChangedFlag(true);
	if (conf_core.editor.beep_when_finished)
		gui->beep();
	FreeConnectionLookupMemory();
	IncrementUndoSerialNumber();
	User = false;
	Draw();
}


/* ---------------------------------------------------------------------------
 * find all connections to pins of all element
 */
void LookupConnectionsToAllElements(FILE * FP)
{
	/* reset all currently marked connections */
	User = false;
	TheFlag = FOUNDFLAG;
	ResetConnections(false);
	InitConnectionLookup();

	ELEMENT_LOOP(PCB->Data);
	{
		/* break if abort dialog returned true */
		if (PrintElementConnections(element, FP, false))
			break;
		SEPARATE(FP);
		if (conf_core.editor.reset_after_element && gdl_it_idx(&__it__) != 1)
			ResetConnections(false);
	}
	END_LOOP;
	if (conf_core.editor.beep_when_finished)
		gui->beep();
	ResetConnections(false);
	FreeConnectionLookupMemory();
	Redraw();
}

