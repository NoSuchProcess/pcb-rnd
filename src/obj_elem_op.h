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

/*** Standard operations on elements ***/

#include "operation.h"

void *AddElementToBuffer(pcb_opctx_t *ctx, pcb_element_t *Element);
void *MoveElementToBuffer(pcb_opctx_t *ctx, pcb_element_t * element);
void *ClrElementOctagon(pcb_opctx_t *ctx, pcb_element_t *Element);
void *SetElementOctagon(pcb_opctx_t *ctx, pcb_element_t *Element);
void *ChangeElementOctagon(pcb_opctx_t *ctx, pcb_element_t *Element);
void *ClrElementSquare(pcb_opctx_t *ctx, pcb_element_t *Element);
void *SetElementSquare(pcb_opctx_t *ctx, pcb_element_t *Element);
void *ChangeElementSquare(pcb_opctx_t *ctx, pcb_element_t *Element);
void *ChangeElementNonetlist(pcb_opctx_t *ctx, pcb_element_t *Element);
void *ChangeElementName(pcb_opctx_t *ctx, pcb_element_t *Element);
void *ChangeElementNameSize(pcb_opctx_t *ctx, pcb_element_t *Element);
void *ChangeElementSize(pcb_opctx_t *ctx, pcb_element_t *Element);
void *ChangeElementClearSize(pcb_opctx_t *ctx, pcb_element_t *Element);
void *ChangeElement1stSize(pcb_opctx_t *ctx, pcb_element_t *Element);
void *ChangeElement2ndSize(pcb_opctx_t *ctx, pcb_element_t *Element);
void *CopyElement(pcb_opctx_t *ctx, pcb_element_t *Element);
void *MoveElementName(pcb_opctx_t *ctx, pcb_element_t *Element);
void *MoveElement(pcb_opctx_t *ctx, pcb_element_t *Element);
void *DestroyElement(pcb_opctx_t *ctx, pcb_element_t *Element);
void *RemoveElement_op(pcb_opctx_t *ctx, pcb_element_t *Element);
void *RotateElement(pcb_opctx_t *ctx, pcb_element_t *Element);
void *RotateElementName(pcb_opctx_t *ctx, pcb_element_t *Element);

