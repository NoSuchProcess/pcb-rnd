/*
 *			    COPYRIGHT
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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include "config.h"
#include <stdio.h>
#include "data.h"

int io_kicad_write_element(pcb_plug_io_t *ctx, FILE *FP, pcb_data_t *Data);
int io_kicad_write_buffer_subc(pcb_plug_io_t *ctx, FILE *FP, pcb_buffer_t *buff, long idx);
int io_kicad_write_pcb(pcb_plug_io_t *ctx, FILE *FP, const char *old_filename, const char *new_filename, pcb_bool emergency);


