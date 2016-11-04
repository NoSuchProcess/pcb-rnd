/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2004 Thomas Nau
 *  15 Oct 2008 Ineiev: add different crosshair shapes
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

#ifndef PCB_OPERATION_H
#define PCB_OPERATION_H

#include "global_typedefs.h"
#include "global_element.h"

typedef struct {
	PCBType *pcb;
} pcb_opctx_copy_t;

typedef struct {
	PCBType *pcb;
} pcb_opctx_move_t;

typedef union {
	pcb_opctx_copy_t copy;
	pcb_opctx_move_t move;
} pcb_opctx_t;

/* pointer to low-level operation (copy, move and rotate) functions */
typedef struct {
	void *(*Line)(pcb_opctx_t *ctx, LayerTypePtr, LineTypePtr);
	void *(*Text)(pcb_opctx_t *ctx, LayerTypePtr, TextTypePtr);
	void *(*Polygon)(pcb_opctx_t *ctx, LayerTypePtr, PolygonTypePtr);
	void *(*Via)(pcb_opctx_t *ctx, PinTypePtr);
	void *(*Element)(pcb_opctx_t *ctx, ElementTypePtr);
	void *(*ElementName)(pcb_opctx_t *ctx, ElementTypePtr);
	void *(*Pin)(pcb_opctx_t *ctx, ElementTypePtr, PinTypePtr);
	void *(*Pad)(pcb_opctx_t *ctx, ElementTypePtr, PadTypePtr);
	void *(*LinePoint)(pcb_opctx_t *ctx, LayerTypePtr, LineTypePtr, PointTypePtr);
	void *(*Point)(pcb_opctx_t *ctx, LayerTypePtr, PolygonTypePtr, PointTypePtr);
	void *(*Arc)(pcb_opctx_t *ctx, LayerTypePtr, ArcTypePtr);
	void *(*Rat)(pcb_opctx_t *ctx, RatTypePtr);
} pcb_opfunc_t;

#endif
