/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2016..2020 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#ifndef GLOBAL_TYPEDEFS_H
#define GLOBAL_TYPEDEFS_H

#include <librnd/core/global_typedefs.h>

typedef struct pcb_board_s pcb_board_t;
typedef struct pcb_data_s pcb_data_t;
typedef struct pcb_layer_stack_s pcb_layer_stack_t;
typedef struct pcb_layer_s pcb_layer_t;
typedef struct pcb_layergrp_s pcb_layergrp_t;
typedef struct pcb_buffer_s pcb_buffer_t;
typedef struct pcb_net_s pcb_net_t;
typedef struct pcb_net_term_s pcb_net_term_t;
typedef struct pcb_oldnet_s pcb_oldnet_t;
typedef struct pcb_connection_s pcb_connection_t;
typedef struct pcb_font_s pcb_font_t;
typedef struct pcb_fontkit_s pcb_fontkit_t;
typedef struct pcb_line_s pcb_line_t;
typedef struct pcb_arc_s pcb_arc_t;
typedef struct pcb_gfx_s pcb_gfx_t;
typedef struct pcb_rat_line_s pcb_rat_t;

typedef struct pcb_poly_s pcb_poly_t;
typedef struct pcb_pstk_s pcb_pstk_t;
typedef struct pcb_pstk_proto_s pcb_pstk_proto_t;

typedef struct pcb_ratspatch_line_s pcb_ratspatch_line_t;
typedef struct pcb_subc_s pcb_subc_t;
typedef struct pcb_text_s pcb_text_t;

typedef struct pcb_any_obj_s pcb_any_obj_t;
typedef struct pcb_any_line_s pcb_any_line_t;

typedef union pcb_parent_s pcb_parent_t;

typedef struct pcb_plug_io_s pcb_plug_io_t;
typedef struct pcb_plug_fp_map_s pcb_plug_fp_map_t;

typedef struct pcb_view_s pcb_view_t;
#endif
