/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *
 *  This module, rats.c, was written and is Copyright (C) 1997 by harry eaton
 *  this module is also subject to the GNU GPL as described below
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

#ifndef PCB_OBJ_SUBC_OP_H
#define PCB_OBJ_SUBC_OP_H

#include "operation.h"

void *CopySubc(pcb_opctx_t *ctx, pcb_subc_t *src);
void *MoveSubc(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *Rotate90Subc(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *MoveSubcToBuffer(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *AddSubcToBuffer(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *ChangeSubcSize(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *ChangeSubcClearSize(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *ChangeSubc1stSize(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *ChangeSubc2ndSize(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *ChangeSubcNonetlist(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *ChangeSubcName(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *DestroySubc(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *RemoveSubc_op(pcb_opctx_t *ctx, pcb_subc_t *sc);

void *ClrSubcOctagon(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *SetSubcOctagon(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *ChangeSubcOctagon(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *ClrSubcSquare(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *SetSubcSquare(pcb_opctx_t *ctx, pcb_subc_t *sc);
void *ChangeSubcSquare(pcb_opctx_t *ctx, pcb_subc_t *sc);


#endif
