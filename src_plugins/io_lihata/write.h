/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
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

#include "plug_io.h"

int io_lihata_write_pcb_v1(pcb_plug_io_t *ctx, FILE *FP, const char *old_filename, const char *new_filename, pcb_bool emergency);
int io_lihata_write_pcb_v2(pcb_plug_io_t *ctx, FILE *FP, const char *old_filename, const char *new_filename, pcb_bool emergency);
int io_lihata_write_pcb_v3(pcb_plug_io_t *ctx, FILE *FP, const char *old_filename, const char *new_filename, pcb_bool emergency);
int io_lihata_write_pcb_v4(pcb_plug_io_t *ctx, FILE *FP, const char *old_filename, const char *new_filename, pcb_bool emergency);
int io_lihata_write_pcb_v5(pcb_plug_io_t *ctx, FILE *FP, const char *old_filename, const char *new_filename, pcb_bool emergency);
int io_lihata_write_pcb_v6(pcb_plug_io_t *ctx, FILE *FP, const char *old_filename, const char *new_filename, pcb_bool emergency);
int io_lihata_write_font(pcb_plug_io_t *ctx, pcb_font_t *font, const char *Filename);
int io_lihata_write_buffer(pcb_plug_io_t *ctx, FILE *f, pcb_buffer_t *buff, pcb_bool elem_only);
int io_lihata_write_element(pcb_plug_io_t *ctx, FILE *f, pcb_data_t *dt);

void *io_lihata_save_as_subd_init(const pcb_plug_io_t *ctx, pcb_hid_dad_subdialog_t *sub, pcb_plug_iot_t type);
void io_lihata_save_as_subd_uninit(const pcb_plug_io_t *ctx, void *plg_ctx, pcb_hid_dad_subdialog_t *sub, pcb_bool apply);
void io_lihata_save_as_fmt_changed(const pcb_plug_io_t *ctx, void *plg_ctx, pcb_hid_dad_subdialog_t *sub);
