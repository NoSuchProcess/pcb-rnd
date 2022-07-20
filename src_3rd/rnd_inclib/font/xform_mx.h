/*
 *                            COPYRIGHT
 *
 *  librnd, modular 2D CAD framework - 2d matrix transformations
 *  librnd Copyright (C) 2022 Tibor 'Igor2' Palinkas
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
 *    Project page: http://repo.hu/projects/librnd
 *    lead developer: http://repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

typedef double rnd_xform_mx_t[9];
#define RND_XFORM_MX_IDENT {1,0,0,   0,1,0,   0,0,1}

/* Standard 2d matrix transformation using mx as rnd_xform_mx_t */
#define rnd_xform_x(mx, x_in, y_in) ((double)(x_in) * mx[0] + (double)(y_in) * mx[1] + mx[2])
#define rnd_xform_y(mx, x_in, y_in) ((double)(x_in) * mx[3] + (double)(y_in) * mx[4] + mx[5])

void rnd_xform_mx_rotate(rnd_xform_mx_t mx, double deg);
void rnd_xform_mx_translate(rnd_xform_mx_t mx, double xt, double yt);
void rnd_xform_mx_scale(rnd_xform_mx_t mx, double st, double sy);
void rnd_xform_mx_shear(rnd_xform_mx_t mx, double sx, double sy);
void rnd_xform_mx_mirrorx(rnd_xform_mx_t mx); /* mirror over the x axis (flip y coords) */
