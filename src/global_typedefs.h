/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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

#ifndef GLOBAL_TYPEDEFS_H
#define GLOBAL_TYPEDEFS_H
#include "config.h"

typedef struct BoxType BoxType, *BoxTypePtr;
typedef struct polygon_st PolygonType, *PolygonTypePtr;
typedef struct pad_st PadType, *PadTypePtr;
typedef struct pin_st PinType, *PinTypePtr, **PinTypeHandle;
typedef struct drc_violation_st DrcViolationType, *DrcViolationTypePtr;
typedef struct rtree rtree_t;
typedef struct AttributeListType AttributeListType, *AttributeListTypePtr;
typedef struct rats_patch_line_s rats_patch_line_t;

typedef unsigned int pcb_cardinal_t;
typedef unsigned char BYTE;

#include "pcb_bool.h"

#include "unit.h"

#endif
