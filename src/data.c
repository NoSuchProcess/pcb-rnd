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

/* just defines common identifiers
 */
#include "config.h"

#include "board.h"
#include "data.h"

/* ---------------------------------------------------------------------------
 * some shared identifiers
 */

CrosshairType Crosshair;				/* information about cursor settings */
MarkType Marked;								/* a cross-hair mark */
PCBTypePtr PCB;									/* pointer to layout struct */

int LayerStack[MAX_LAYER];			/* determines the layer draw order */

BufferType Buffers[MAX_BUFFER]; /* my buffers */
pcb_bool Bumped;                /* if the undo serial number has changed */

int addedLines;


/* callback based loops */
void pcb_loop_layers(void *ctx, pcb_layer_cb_t lacb, pcb_line_cb_t lcb, pcb_arc_cb_t acb, pcb_text_cb_t tcb, pcb_poly_cb_t pocb)
{
	if ((lacb != NULL) || (lcb != NULL) || (acb != NULL) || (tcb != NULL) || (pocb != NULL)) {
		LAYER_LOOP(PCB->Data, max_copper_layer + 2);
		{
			if (lacb != NULL)
				if (lacb(ctx, PCB, layer, 1))
					continue;
			if (lcb != NULL) {
				LINE_LOOP(layer);
				{
					lcb(ctx, PCB, layer, line);
				}
				END_LOOP;
			}

			if (acb != NULL) {
				ARC_LOOP(layer);
				{
					acb(ctx, PCB, layer, arc);
				}
				END_LOOP;
			}

			if (tcb != NULL) {
				TEXT_LOOP(layer);
				{
					tcb(ctx, PCB, layer, text);
				}
				END_LOOP;
			}

			if (pocb != NULL) {
				POLYGON_LOOP(layer);
				{
					pocb(ctx, PCB, layer, polygon);
				}
				END_LOOP;
			}
			if (lacb != NULL)
				lacb(ctx, PCB, layer, 0);
		}
		END_LOOP;
	}
}

void pcb_loop_elements(void *ctx, pcb_element_cb_t ecb, pcb_eline_cb_t elcb, pcb_earc_cb_t eacb, pcb_etext_cb_t etcb, pcb_epin_cb_t epicb, pcb_epad_cb_t epacb)
{
	if ((ecb != NULL) || (elcb != NULL) || (eacb != NULL)  || (etcb != NULL) || (epicb != NULL) || (epacb != NULL)) {
		ELEMENT_LOOP(PCB->Data);
		{
			if (ecb != NULL)
				if (ecb(ctx, PCB, element, 1))
					continue;

			if (elcb != NULL) {
				ELEMENTLINE_LOOP(element);
				{
					elcb(ctx, PCB, element, line);
				}
				END_LOOP;
			}

			if (eacb != NULL) {
				ELEMENTARC_LOOP(element);
				{
					eacb(ctx, PCB, element, arc);
				}
				END_LOOP;
			}

			if (etcb != NULL) {
				ELEMENTTEXT_LOOP(element);
				{
					etcb(ctx, PCB, element, text);
				}
				END_LOOP;
			}

			if (epicb != NULL) {
				PIN_LOOP(element);
				{
					epicb(ctx, PCB, element, pin);
				}
				END_LOOP;
			}


			if (epacb != NULL) {
				PAD_LOOP(element);
				{
					epacb(ctx, PCB, element, pad);
				}
				END_LOOP;
			}

			if (ecb != NULL)
				ecb(ctx, PCB, element, 0);
		}
		END_LOOP;
	}
}

void pcb_loop_vias(void *ctx, pcb_via_cb_t vcb)
{
	if (vcb != NULL) {
		VIA_LOOP(PCB->Data);
		{
			vcb(ctx, PCB, via);
		}
		END_LOOP;
	}
}

void pcb_loop_all(void *ctx,
	pcb_layer_cb_t lacb, pcb_line_cb_t lcb, pcb_arc_cb_t acb, pcb_text_cb_t tcb, pcb_poly_cb_t pocb,
	pcb_element_cb_t ecb, pcb_eline_cb_t elcb, pcb_earc_cb_t eacb, pcb_etext_cb_t etcb, pcb_epin_cb_t epicb, pcb_epad_cb_t epacb,
	pcb_via_cb_t vcb
	)
{
	pcb_loop_layers(ctx, lacb, lcb, acb, tcb, pocb);
	pcb_loop_elements(ctx, ecb, elcb, eacb, etcb, epicb, epacb);
	pcb_loop_vias(ctx, vcb);
}
