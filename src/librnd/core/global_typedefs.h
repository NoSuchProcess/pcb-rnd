/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#ifndef GLOBAL_TYPEDEFS_H
#define GLOBAL_TYPEDEFS_H
#include "config.h"

struct pcb_box_s {        /* a bounding box */
	pcb_coord_t X1, Y1;     /* upper left */
	pcb_coord_t X2, Y2;     /* and lower right corner */
};

typedef struct pcb_hidlib_s pcb_hidlib_t;


/* typedef ... pcb_coord_t; pcb base unit, typedef'd in config.h */
typedef double pcb_angle_t; /* degrees */
typedef struct pcb_unit_s pcb_unit_t;

typedef struct pcb_board_s pcb_board_t;
typedef struct pcb_data_s pcb_data_t;
typedef struct pcb_layer_stack_s pcb_layer_stack_t;
typedef struct pcb_layer_s pcb_layer_t;
typedef struct pcb_layergrp_s pcb_layergrp_t;
typedef long int pcb_layer_id_t;
typedef long int pcb_layergrp_id_t;
typedef struct pcb_polyarea_s pcb_polyarea_t;
typedef struct pcb_buffer_s pcb_buffer_t;
typedef struct pcb_net_s pcb_net_t;
typedef struct pcb_net_term_s pcb_net_term_t;
typedef struct pcb_oldnet_s pcb_oldnet_t;
typedef struct pcb_connection_s pcb_connection_t;
typedef struct pcb_box_s pcb_box_t;
typedef struct pcb_box_list_s pcb_box_list_t;
typedef struct pcb_font_s pcb_font_t;
typedef struct pcb_fontkit_s pcb_fontkit_t;
typedef struct pcb_line_s pcb_line_t;
typedef struct pcb_arc_s pcb_arc_t;
typedef struct pcb_point_s pcb_point_t;
typedef struct pcb_rat_line_s pcb_rat_t;

typedef struct pcb_poly_s pcb_poly_t;
typedef struct pcb_pstk_s pcb_pstk_t;
typedef struct pcb_rtree_s pcb_rtree_t;
typedef struct pcb_rtree_it_s pcb_rtree_it_t;
typedef struct pcb_ratspatch_line_s pcb_ratspatch_line_t;
typedef struct pcb_subc_s pcb_subc_t;
typedef struct pcb_text_s pcb_text_t;

typedef struct pcb_any_obj_s pcb_any_obj_t;
typedef struct pcb_any_line_s pcb_any_line_t;

typedef union pcb_parent_s pcb_parent_t;

typedef struct pcb_plug_io_s pcb_plug_io_t;

typedef struct pcb_hid_cfg_s pcb_hid_cfg_t;

typedef unsigned int pcb_cardinal_t;
typedef struct pcb_color_s pcb_color_t;
typedef struct pcb_clrcache_s pcb_clrcache_t;
typedef struct pcb_pixmap_s pcb_pixmap_t;

typedef struct pcb_action_s pcb_action_t;

typedef struct pcb_view_s pcb_view_t;

typedef struct pcb_hid_dad_subdialog_s pcb_hid_dad_subdialog_t;

typedef struct pcb_event_arg_s pcb_event_arg_t;

typedef struct pcb_hid_expose_ctx_s pcb_hid_expose_ctx_t;
typedef struct pcb_hid_s pcb_hid_t;
typedef struct pcb_xform_s pcb_xform_t;

/* This graphics context is an opaque pointer defined by the HID.  GCs
   are HID-specific; attempts to use one HID's GC for a different HID
   will result in a fatal error.  The first field must be:
   pcb_core_gc_t core_gc; */
typedef struct hid_gc_s *pcb_hid_gc_t;

typedef struct pcb_hid_attr_val_s  pcb_hid_attr_val_t;
typedef struct pcb_hid_attribute_s pcb_hid_attribute_t;
typedef struct pcb_export_opt_s pcb_export_opt_t;

#include "pcb_bool.h"

#endif
