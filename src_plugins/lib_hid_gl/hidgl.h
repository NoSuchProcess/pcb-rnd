/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2009-2011 PCB Contributors (See ChangeLog for details).
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

#ifndef PCB_HID_COMMON_HIDGL_H
#define PCB_HID_COMMON_HIDGL_H

/*extern float global_depth;*/
void hidgl_draw_local_grid(rnd_hidlib_t *hidlib, rnd_coord_t cx, rnd_coord_t cy, int radius);
void hidgl_draw_grid(rnd_hidlib_t *hidlib, rnd_box_t *drawn_area);
void hidgl_set_depth(float depth);
void hidgl_draw_line(int cap, rnd_coord_t width, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, double scale);
void hidgl_draw_arc(rnd_coord_t width, rnd_coord_t vx, rnd_coord_t vy, rnd_coord_t vrx, rnd_coord_t vry, pcb_angle_t start_angle, pcb_angle_t delta_angle, double scale);
void hidgl_draw_rect(rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2);
void hidgl_draw_texture_rect(	rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, unsigned long texture_id );
void hidgl_fill_circle(rnd_coord_t vx, rnd_coord_t vy, rnd_coord_t vr, double scale);
void hidgl_fill_polygon(int n_coords, rnd_coord_t *x, rnd_coord_t *y);
void hidgl_fill_polygon_offs(int n_coords, rnd_coord_t *x, rnd_coord_t *y, rnd_coord_t dx, rnd_coord_t dy);
void hidgl_fill_pcb_polygon(pcb_polyarea_t *poly, const rnd_box_t *clip_box, double scale, int fullpoly);
void hidgl_fill_rect(rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2);
void hidgl_init(void);
void hidgl_set_drawing_mode(pcb_hid_t *hid, pcb_composite_op_t op, rnd_bool direct, const rnd_box_t *screen);
pcb_composite_op_t hidgl_get_drawing_mode();

#endif /* PCB_HID_COMMON_HIDGL_H  */
