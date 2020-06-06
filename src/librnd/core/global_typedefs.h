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

#ifndef RND_GLOBAL_TYPEDEFS_H
#define RND_GLOBAL_TYPEDEFS_H
#include <librnd/config.h>

struct rnd_box_s {        /* a bounding box */
	rnd_coord_t X1, Y1;     /* upper left */
	rnd_coord_t X2, Y2;     /* and lower right corner */
};

typedef struct rnd_hidlib_s rnd_hidlib_t;


/* typedef ... rnd_coord_t; pcb base unit, typedef'd in config.h */
typedef double rnd_angle_t; /* degrees */
typedef struct rnd_unit_s rnd_unit_t;

typedef struct rnd_point_s rnd_point_t;
typedef struct rnd_box_s rnd_box_t;
typedef struct rnd_box_list_s rnd_box_list_t;
typedef struct rnd_polyarea_s rnd_polyarea_t;

typedef struct rnd_rtree_s rnd_rtree_t;
typedef struct rnd_rtree_it_s rnd_rtree_it_t;

typedef struct rnd_hid_cfg_s rnd_hid_cfg_t;

typedef unsigned int rnd_cardinal_t;
typedef struct rnd_color_s rnd_color_t;
typedef struct rnd_clrcache_s rnd_clrcache_t;
typedef struct rnd_pixmap_s rnd_pixmap_t;

typedef struct rnd_action_s rnd_action_t;

typedef struct rnd_hid_dad_subdialog_s rnd_hid_dad_subdialog_t;

typedef struct rnd_event_arg_s rnd_event_arg_t;

typedef struct rnd_hid_expose_ctx_s rnd_hid_expose_ctx_t;
typedef struct rnd_hid_s rnd_hid_t;
typedef struct rnd_xform_s rnd_xform_t; /* declared by the app */

/* This graphics context is an opaque pointer defined by the HID.  GCs
   are HID-specific; attempts to use one HID's GC for a different HID
   will result in a fatal error.  The first field must be:
   rnd_core_gc_t core_gc; */
typedef struct rnd_hid_gc_s *rnd_hid_gc_t;

typedef struct rnd_hid_attr_val_s  rnd_hid_attr_val_t;
typedef struct rnd_hid_attribute_s rnd_hid_attribute_t;
typedef struct rnd_export_opt_s rnd_export_opt_t;

typedef long int rnd_layer_id_t;
typedef long int rnd_layergrp_id_t;

#include <librnd/core/rnd_bool.h>

#endif
