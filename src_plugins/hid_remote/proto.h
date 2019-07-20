/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2015 Tibor 'Igor2' Palinkas
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


void remote_proto_send_ver();
void remote_proto_send_unit();
void remote_proto_send_brddim(pcb_coord_t w, pcb_coord_t h);
int remote_proto_send_ready();
void proto_send_invalidate(int l, int r, int t, int b);
void proto_send_invalidate_all(void);

int pcb_remote_new_layer_group(const char *name, pcb_layergrp_id_t idx, unsigned int flags);
int pcb_remote_new_layer(const char *name, pcb_layer_id_t idx, unsigned int group);
int proto_send_set_layer_group(pcb_layergrp_id_t group, const char *purpose, int is_empty);

int proto_send_make_gc(void);
int proto_send_del_gc(int gc);
void proto_send_set_color(int gc, const char *name);
void proto_send_set_line_cap(int gc, char style);
void proto_send_set_line_width(int gc, pcb_coord_t width);
void proto_send_set_draw_xor(int gc, int xor_set);
void proto_send_draw_line(int gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2);
void proto_send_draw_arc(int gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t width, pcb_coord_t height, pcb_angle_t start_angle, pcb_angle_t delta_angle);
void proto_send_draw_rect(int gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, int is_filled);
void proto_send_fill_circle(int gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t radius);
void proto_send_draw_poly(int gc, int n_coords, pcb_coord_t *x, pcb_coord_t *y, pcb_coord_t dx, pcb_coord_t dy);
int proto_send_set_drawing_mode(const char *name, int direct);

int remote_proto_parse_all();
