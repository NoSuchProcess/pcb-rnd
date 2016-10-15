/*
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996, 2005 Thomas Nau
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Contact addresses for paper mail and Email:
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 * Thomas.Nau@rz.uni-ulm.de
 */

/*!
 * \brief Resets all used flags of pins and vias.
 */
pcb_bool ClearFlagOnPinsViasAndPads(pcb_bool AndDraw, int flag)
{
	pcb_bool change = pcb_false;

	VIA_LOOP(PCB->Data);
	{
		if (TEST_FLAG(flag, via)) {
			if (AndDraw)
				AddObjectToFlagUndoList(PCB_TYPE_VIA, via, via, via);
			CLEAR_FLAG(flag, via);
			if (AndDraw)
				DrawVia(via);
			change = pcb_true;
		}
	}
	END_LOOP;
	ELEMENT_LOOP(PCB->Data);
	{
		PIN_LOOP(element);
		{
			if (TEST_FLAG(flag, pin)) {
				if (AndDraw)
					AddObjectToFlagUndoList(PCB_TYPE_PIN, element, pin, pin);
				CLEAR_FLAG(flag, pin);
				if (AndDraw)
					DrawPin(pin);
				change = pcb_true;
			}
		}
		END_LOOP;
		PAD_LOOP(element);
		{
			if (TEST_FLAG(flag, pad)) {
				if (AndDraw)
					AddObjectToFlagUndoList(PCB_TYPE_PAD, element, pad, pad);
				CLEAR_FLAG(flag, pad);
				if (AndDraw)
					DrawPad(pad);
				change = pcb_true;
			}
		}
		END_LOOP;
	}
	END_LOOP;
	if (change)
		SetChangedFlag(pcb_true);
	return change;
}

/*!
 * \brief Resets all used flags of LOs.
 */
pcb_bool ClearFlagOnLinesAndPolygons(pcb_bool AndDraw, int flag)
{
	pcb_bool change = pcb_false;

	RAT_LOOP(PCB->Data);
	{
		if (TEST_FLAG(flag, line)) {
			if (AndDraw)
				AddObjectToFlagUndoList(PCB_TYPE_RATLINE, line, line, line);
			CLEAR_FLAG(flag, line);
			if (AndDraw)
				DrawRat(line);
			change = pcb_true;
		}
	}
	END_LOOP;
	COPPERLINE_LOOP(PCB->Data);
	{
		if (TEST_FLAG(flag, line)) {
			if (AndDraw)
				AddObjectToFlagUndoList(PCB_TYPE_LINE, layer, line, line);
			CLEAR_FLAG(flag, line);
			if (AndDraw)
				DrawLine(layer, line);
			change = pcb_true;
		}
	}
	ENDALL_LOOP;
	COPPERARC_LOOP(PCB->Data);
	{
		if (TEST_FLAG(flag, arc)) {
			if (AndDraw)
				AddObjectToFlagUndoList(PCB_TYPE_ARC, layer, arc, arc);
			CLEAR_FLAG(flag, arc);
			if (AndDraw)
				DrawArc(layer, arc);
			change = pcb_true;
		}
	}
	ENDALL_LOOP;
	COPPERPOLYGON_LOOP(PCB->Data);
	{
		if (TEST_FLAG(flag, polygon)) {
			if (AndDraw)
				AddObjectToFlagUndoList(PCB_TYPE_POLYGON, layer, polygon, polygon);
			CLEAR_FLAG(flag, polygon);
			if (AndDraw)
				DrawPolygon(layer, polygon);
			change = pcb_true;
		}
	}
	ENDALL_LOOP;
	if (change)
		SetChangedFlag(pcb_true);
	return change;
}

/*!
 * \brief Resets all found connections.
 */
pcb_bool ClearFlagOnAllObjects(pcb_bool AndDraw, int flag)
{
	pcb_bool change = pcb_false;

	change = ClearFlagOnPinsViasAndPads(AndDraw, flag) || change;
	change = ClearFlagOnLinesAndPolygons(AndDraw, flag) || change;

	return change;
}
