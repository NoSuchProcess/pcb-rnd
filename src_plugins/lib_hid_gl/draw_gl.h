/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2009-2017 PCB Contributers (See ChangeLog for details)
 *  Copyright (C) 2017 Adrian Purser
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
 */
#ifndef HID_GL_DRAW_GL_H
#define HID_GL_DRAW_GL_H

#include "config.h"
#include "opengl.h"

void		drawgl_uninit();

void		drawgl_reserve_triangles(int count);

void		drawgl_set_colour(GLfloat r,GLfloat g,GLfloat b,GLfloat a);

void		drawgl_add_triangle(GLfloat x1,GLfloat y1,GLfloat x2,GLfloat y2,GLfloat x3,GLfloat y3);
void		drawgl_add_line(GLfloat x1,GLfloat y1,GLfloat x2,GLfloat y2);
void		drawgl_add_rectangle(GLfloat x1,GLfloat y1,GLfloat x2,GLfloat y2);
void		drawgl_add_mask_create();
void		drawgl_add_mask_destroy();
void		drawgl_add_mask_use();

void		drawgl_flush();
void		drawgl_reset();
void		drawgl_draw_all(int stencil_bits);

void		drawgl_direct_draw_rectangle(GLfloat x1,GLfloat y1,GLfloat x2,GLfloat y2);
void		drawgl_direct_draw_solid_rectangle(GLfloat x1,GLfloat y1,GLfloat x2,GLfloat y2);

void		drawgl_set_marker();
void		drawgl_rewind_to_marker();

#endif /* ! defined HID_GL_DRAW_GL_H */

