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

typedef struct pcb_board_s pcb_board_t;
typedef struct pcb_data_s pcb_data_t;
typedef struct pcb_layer_group_s pcb_layer_group_t;
typedef struct pcb_layer_s LayerType, *LayerTypePtr;
typedef struct pcb_buffer_s BufferType, *BufferTypePtr;
typedef struct pcb_net_s NetType, *NetTypePtr;
typedef struct pcb_connection_s  ConnectionType, *ConnectionTypePtr;
typedef struct pcb_box_s BoxType, *BoxTypePtr;
typedef struct pcb_boxlist_s  BoxListType, *BoxListTypePtr;
typedef struct pcb_font_s  FontType, *FontTypePtr;
typedef struct pcb_line_s LineType, *LineTypePtr;
typedef struct pcb_arc_s ArcType, *ArcTypePtr;
typedef struct pcb_point_s PointType, *PointTypePtr;
typedef struct pcb_rat_line_s RatType, *RatTypePtr;

typedef struct polygon_st PolygonType, *PolygonTypePtr;
typedef struct pad_st PadType, *PadTypePtr;
typedef struct pin_st PinType, *PinTypePtr, **PinTypeHandle;
typedef struct rtree rtree_t;
typedef struct rats_patch_line_s rats_patch_line_t;
typedef struct element_st ElementType, *ElementTypePtr, **ElementTypeHandle;
typedef struct pcb_text_s  TextType, *TextTypePtr;

typedef struct plug_io_s plug_io_t;

typedef unsigned int pcb_cardinal_t;
typedef unsigned char pcb_uint8_t;   /* Don't use in new code. */

#include "pcb_bool.h"

#include "unit.h"

#endif
