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
typedef struct pcb_layer_stack_s pcb_layer_stack_t;
typedef struct pcb_layer_s pcb_layer_t;
typedef struct pcb_buffer_s pcb_buffer_t;
typedef struct pcb_net_s pcb_net_t;
typedef struct pcb_connection_s pcb_connection_t;
typedef struct pcb_box_s pcb_box_t;
typedef struct pcb_box_list_s pcb_box_list_t;
typedef struct pcb_font_s pcb_font_t;
typedef struct pcb_fontkit_s pcb_fontkit_t;
typedef struct pcb_line_s pcb_line_t;
typedef struct pcb_arc_s pcb_arc_t;
typedef struct pcb_point_s pcb_point_t;
typedef struct pcb_rat_line_s pcb_rat_t;

typedef struct pcb_polygon_s pcb_polygon_t;
typedef struct pcb_pad_s pcb_pad_t;
typedef struct pcb_pin_s pcb_pin_t;
typedef struct pcb_rtree_s pcb_rtree_t;
typedef struct pcb_rtree_it_s pcb_rtree_it_t;
typedef struct pcb_ratspatch_line_s pcb_ratspatch_line_t;
typedef struct pcb_element_s pcb_element_t;
typedef struct pcb_subc_s pcb_subc_t;
typedef struct pcb_text_s pcb_text_t;

typedef union pcb_parent_s pcb_parent_t;

typedef struct pcb_plug_io_s pcb_plug_io_t;

typedef unsigned int pcb_cardinal_t;
typedef unsigned char pcb_uint8_t;   /* Don't use in new code. */

#include "pcb_bool.h"

#include "unit.h"

#endif
