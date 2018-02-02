/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
 *  Copyright (C) 2016 Erich S. Heinzle
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include "config.h"
#include <stdio.h>
#include "data.h"

int io_kicad_legacy_write_element(pcb_plug_io_t *ctx, FILE *FP, pcb_data_t *Data);
int io_kicad_legacy_write_buffer(pcb_plug_io_t *ctx, FILE *FP, pcb_buffer_t *buff, pcb_bool elem_only);
int io_kicad_legacy_write_pcb(pcb_plug_io_t *ctx, FILE *FP, const char *old_filename, const char *new_filename, pcb_bool emergency);
int write_kicad_legacy_module_header(FILE *FP);
int write_kicad_legacy_layout_header(FILE *FP);
int write_kicad_legacy_layout_vias(FILE *FP, pcb_data_t *Data, pcb_coord_t MaxWidth, pcb_coord_t MaxHeight);
int write_kicad_legacy_layout_tracks(FILE *FP, pcb_cardinal_t number, pcb_layer_t *layer, pcb_coord_t MaxWidth, pcb_coord_t MaxHeight);
int write_kicad_legacy_layout_arcs(FILE *FP, pcb_cardinal_t number, pcb_layer_t *layer, pcb_coord_t xOffset, pcb_coord_t yOffset);
int write_kicad_legacy_layout_text(FILE *FP, pcb_cardinal_t number, pcb_layer_t *layer, pcb_coord_t xOffset, pcb_coord_t yOffset);
int write_kicad_legacy_equipotential_netlists(FILE *FP, pcb_board_t *Layout);
int write_kicad_legacy_layout_elements(FILE *FP, pcb_board_t *Layout, pcb_data_t *Data, pcb_coord_t xOffset, pcb_coord_t yOffset);
int write_kicad_legacy_layout_polygons(FILE *FP, pcb_cardinal_t number, pcb_layer_t *layer, pcb_coord_t xOffset, pcb_coord_t yOffset);
