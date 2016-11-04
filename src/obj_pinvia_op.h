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

/*** Standard operations on pins and vias ***/

#include "operation.h"

void *AddViaToBuffer(pcb_opctx_t *ctx, PinTypePtr Via);
void *MoveViaToBuffer(pcb_opctx_t *ctx, PinType * via);
void *ChangeViaThermal(pcb_opctx_t *ctx, PinTypePtr Via);
void *ChangePinThermal(pcb_opctx_t *ctx, ElementTypePtr element, PinTypePtr Pin);
void *ChangeViaSize(pcb_opctx_t *ctx, PinTypePtr Via);
void *ChangeVia2ndSize(pcb_opctx_t *ctx, PinTypePtr Via);
void *ChangePin2ndSize(pcb_opctx_t *ctx, ElementTypePtr Element, PinTypePtr Pin);
void *ChangeViaClearSize(pcb_opctx_t *ctx, PinTypePtr Via);
void *ChangePinSize(pcb_opctx_t *ctx, ElementTypePtr Element, PinTypePtr Pin);
void *ChangePinClearSize(pcb_opctx_t *ctx, ElementTypePtr Element, PinTypePtr Pin);
void *ChangeViaName(pcb_opctx_t *ctx, PinTypePtr Via);
void *ChangePinName(pcb_opctx_t *ctx, ElementTypePtr Element, PinTypePtr Pin);
void *ChangePinNum(pcb_opctx_t *ctx, ElementTypePtr Element, PinTypePtr Pin);
void *ChangeViaSquare(pcb_opctx_t *ctx, PinTypePtr Via);
void *ChangePinSquare(pcb_opctx_t *ctx, ElementTypePtr Element, PinTypePtr Pin);
void *SetPinSquare(pcb_opctx_t *ctx, ElementTypePtr Element, PinTypePtr Pin);
void *ClrPinSquare(pcb_opctx_t *ctx, ElementTypePtr Element, PinTypePtr Pin);
void *ChangeViaOctagon(pcb_opctx_t *ctx, PinTypePtr Via);
void *SetViaOctagon(pcb_opctx_t *ctx, PinTypePtr Via);
void *ClrViaOctagon(pcb_opctx_t *ctx, PinTypePtr Via);
void *ChangePinOctagon(pcb_opctx_t *ctx, ElementTypePtr Element, PinTypePtr Pin);
void *SetPinOctagon(pcb_opctx_t *ctx, ElementTypePtr Element, PinTypePtr Pin);
void *ClrPinOctagon(pcb_opctx_t *ctx, ElementTypePtr Element, PinTypePtr Pin);
pcb_bool ChangeHole(PinTypePtr Via);
void *ChangePinMaskSize(pcb_opctx_t *ctx, ElementTypePtr Element, PinTypePtr Pin);
void *ChangeViaMaskSize(pcb_opctx_t *ctx, PinTypePtr Via);
void *CopyVia(pcb_opctx_t *ctx, PinTypePtr Via);
void *MoveVia(pcb_opctx_t *ctx, PinTypePtr Via);
void *DestroyVia(pcb_opctx_t *ctx, PinTypePtr Via);
void *RemoveVia(pcb_opctx_t *ctx, PinTypePtr Via);
