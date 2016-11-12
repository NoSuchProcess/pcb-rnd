/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* functions used to copy pins, elements ...
 * it's necessary to copy data by calling create... since the base pointer
 * may change cause of dynamic memory allocation
 */

#include "config.h"
#include "conf_core.h"

#include "board.h"
#include "data.h"
#include "draw.h"
#include "select.h"
#include "undo.h"
#include "obj_all_op.h"

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static pcb_opfunc_t CopyFunctions = {
	CopyLine,
	CopyText,
	CopyPolygon,
	CopyVia,
	CopyElement,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	CopyArc,
	NULL
};

/* ---------------------------------------------------------------------------
 * pastes the contents of the buffer to the layout. Only visible objects
 * are handled by the routine.
 */
pcb_bool CopyPastebufferToLayout(pcb_coord_t X, pcb_coord_t Y)
{
	pcb_cardinal_t i;
	pcb_bool changed = pcb_false;
	pcb_opctx_t ctx;

#ifdef DEBUG
	printf("Entering CopyPastebufferToLayout.....\n");
#endif

	/* set movement vector */
	ctx.copy.pcb = PCB;
	ctx.copy.DeltaX = X - PASTEBUFFER->X;
	ctx.copy.DeltaY = Y - PASTEBUFFER->Y;

	/* paste all layers */
	for (i = 0; i < max_copper_layer + 2; i++) {
		pcb_layer_t *sourcelayer = &PASTEBUFFER->Data->Layer[i], *destlayer = LAYER_PTR(i);

		if (destlayer->On) {
			changed = changed || (!LAYER_IS_EMPTY(sourcelayer));
			LINE_LOOP(sourcelayer);
			{
				CopyLine(&ctx, destlayer, line);
			}
			END_LOOP;
			ARC_LOOP(sourcelayer);
			{
				CopyArc(&ctx, destlayer, arc);
			}
			END_LOOP;
			TEXT_LOOP(sourcelayer);
			{
				CopyText(&ctx, destlayer, text);
			}
			END_LOOP;
			POLYGON_LOOP(sourcelayer);
			{
				CopyPolygon(&ctx, destlayer, polygon);
			}
			END_LOOP;
		}
	}

	/* paste elements */
	if (PCB->PinOn && PCB->ElementOn) {
		ELEMENT_LOOP(PASTEBUFFER->Data);
		{
#ifdef DEBUG
			printf("In CopyPastebufferToLayout, pasting element %s\n", element->Name[1].TextString);
#endif
			if (FRONT(element) || PCB->InvisibleObjectsOn) {
				CopyElement(&ctx, element);
				changed = pcb_true;
			}
		}
		END_LOOP;
	}

	/* finally the vias */
	if (PCB->ViaOn) {
		changed |= (pinlist_length(&(PASTEBUFFER->Data->Via)) != 0);
		VIA_LOOP(PASTEBUFFER->Data);
		{
			CopyVia(&ctx, via);
		}
		END_LOOP;
	}

	if (changed) {
		Draw();
		IncrementUndoSerialNumber();
	}

#ifdef DEBUG
	printf("  .... Leaving CopyPastebufferToLayout.\n");
#endif

	return (changed);
}

/* ---------------------------------------------------------------------------
 * copies the object identified by its data pointers and the type
 * the new objects is moved by DX,DY
 * I assume that the appropriate layer ... is switched on
 */
void *CopyObject(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_coord_t DX, pcb_coord_t DY)
{
	void *ptr;
	pcb_opctx_t ctx;

	ctx.copy.pcb = PCB;
	ctx.copy.DeltaX = DX;
	ctx.copy.DeltaY = DY;

	/* the subroutines add the objects to the undo-list */
	ptr = ObjectOperation(&CopyFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	IncrementUndoSerialNumber();
	return (ptr);
}
