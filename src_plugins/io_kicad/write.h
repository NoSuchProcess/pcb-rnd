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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

#include "config.h"
#include <stdio.h>
#include "data.h"

int io_kicad_write_element(plug_io_t *ctx, FILE * FP, pcb_data_t *Data);
int io_kicad_write_buffer(plug_io_t *ctx, FILE * FP, BufferType *buff);
int io_kicad_write_pcb(plug_io_t *ctx, FILE * FP, const char *old_filename, const char *new_filename, pcb_bool emergency);
int write_kicad_module_header(FILE * FP, pcb_cardinal_t indentation);
int write_kicad_layout_header(FILE * FP, pcb_cardinal_t indentation);
int write_kicad_layout_vias(FILE * FP, pcb_data_t *Data, Coord MaxWidth, 										Coord MaxHeight, pcb_cardinal_t indentation);
int write_kicad_layout_tracks(FILE * FP, pcb_cardinal_t number, pcb_layer_t *layer,
						Coord MaxWidth, Coord MaxHeight, pcb_cardinal_t indentation);
int write_kicad_layout_arcs(FILE * FP, pcb_cardinal_t number, pcb_layer_t *layer,
						Coord xOffset, Coord yOffset, pcb_cardinal_t indentation);
int write_kicad_layout_text(FILE * FP, pcb_cardinal_t number, pcb_layer_t *layer,
						Coord xOffset, Coord yOffset, pcb_cardinal_t indentation);
int write_kicad_equipotential_netlists(FILE * FP, pcb_board_t *Layout, pcb_cardinal_t indentation);
int write_kicad_layout_elements(FILE * FP, pcb_board_t *Layout, pcb_data_t *Data, Coord xOffset, Coord yOffset, pcb_cardinal_t indentation);
int write_kicad_layout_polygons(FILE * FP, pcb_cardinal_t number, pcb_layer_t *layer,								Coord xOffset, Coord yOffset, pcb_cardinal_t indentation);

