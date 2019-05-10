/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2009-2011 PCB Contributers (See ChangeLog for details)
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

#include "config.h"
#include "hidlib_conf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "grid.h"
#include "hid.h"
#include "hidgl.h"
#include "rtree.h"

#include "draw_gl.c"

#include "stencil_gl.h"

void hidgl_init(void)
{
	stencilgl_init();
}

static pcb_composite_op_t composite_op = PCB_HID_COMP_RESET;
static pcb_bool direct_mode = pcb_true;
static int comp_stencil_bit = 0;

static GLfloat *grid_points = NULL;
static int grid_point_capacity = 0;


static inline void mode_reset(pcb_bool direct, const pcb_box_t *screen)
{
	drawgl_flush();
	drawgl_reset();
	glColorMask(0, 0, 0, 0); /* Disable colour drawing */
	stencilgl_reset_stencil_usage();
	glDisable(GL_COLOR_LOGIC_OP);
	comp_stencil_bit = 0;
}

static inline void mode_positive(pcb_bool direct, const pcb_box_t *screen)
{
	if (comp_stencil_bit == 0)
		comp_stencil_bit = stencilgl_allocate_clear_stencil_bit();
	else
		drawgl_flush();

	glEnable(GL_STENCIL_TEST);
	glDisable(GL_COLOR_LOGIC_OP);
	stencilgl_mode_write_set(comp_stencil_bit);
}

static inline void mode_positive_xor(pcb_bool direct, const pcb_box_t *screen)
{
	drawgl_flush();
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_STENCIL_TEST);
	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_EQUIV);
}

static inline void mode_negative(pcb_bool direct, const pcb_box_t *screen)
{
	glEnable(GL_STENCIL_TEST);
	glDisable(GL_COLOR_LOGIC_OP);
	
	if (comp_stencil_bit == 0) {
		/* The stencil isn't valid yet which means that this is the first pos/neg mode
		 * set since the reset. The compositing stencil bit will be allocated. Because 
		 * this is a negative mode and it's the first mode to be set, the stencil buffer
		 * will be set to all ones.
		 */
		comp_stencil_bit = stencilgl_allocate_clear_stencil_bit();
		stencilgl_mode_write_set(comp_stencil_bit);
		drawgl_direct_draw_solid_rectangle(screen->X1, screen->Y1, screen->X2, screen->Y2);
	}
	else
		drawgl_flush();

	stencilgl_mode_write_clear(comp_stencil_bit);
	drawgl_set_marker();
}

static inline void mode_flush(pcb_bool direct, pcb_bool xor_mode, const pcb_box_t *screen)
{
	drawgl_flush();
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	if (comp_stencil_bit) {
		glEnable(GL_STENCIL_TEST);

		/* Setup the stencil to allow writes to the colour buffer if the 
		 * comp_stencil_bit is set. After the operation, the comp_stencil_bit
		 * will be cleared so that any further writes to this pixel are disabled.
		 */
		glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
		glStencilMask(comp_stencil_bit);
		glStencilFunc(GL_EQUAL, comp_stencil_bit, comp_stencil_bit);

		/* Draw all primtives through the stencil to the colour buffer. */
		drawgl_draw_all(comp_stencil_bit);
	}

	glDisable(GL_STENCIL_TEST);
	stencilgl_reset_stencil_usage();
	comp_stencil_bit = 0;
}

void hidgl_set_drawing_mode(pcb_composite_op_t op, pcb_bool direct, const pcb_box_t *screen)
{
	pcb_bool xor_mode = (composite_op == PCB_HID_COMP_POSITIVE_XOR ? pcb_true : pcb_false);

	/* If the previous mode was NEGATIVE then all of the primitives drawn
	 * in that mode were used only for creating the stencil and will not be 
	 * drawn directly to the colour buffer. Therefore these primitives can be 
	 * discarded by rewinding the primitive buffer to the marker that was
	 * set when entering NEGATIVE mode.
	 */
	if (composite_op == PCB_HID_COMP_NEGATIVE) {
		drawgl_flush();
		drawgl_rewind_to_marker();
	}

	composite_op = op;
	direct_mode = direct;

	switch (op) {
		case PCB_HID_COMP_RESET:
			mode_reset(direct, screen);
			break;
		case PCB_HID_COMP_POSITIVE_XOR:
			mode_positive_xor(direct, screen);
			break;
		case PCB_HID_COMP_POSITIVE:
			mode_positive(direct, screen);
			break;
		case PCB_HID_COMP_NEGATIVE:
			mode_negative(direct, screen);
			break;
		case PCB_HID_COMP_FLUSH:
			mode_flush(direct, xor_mode, screen);
			break;
		default:
			break;
	}

}


void hidgl_fill_rect(pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	drawgl_add_triangle(x1, y1, x1, y2, x2, y2);
	drawgl_add_triangle(x2, y1, x2, y2, x1, y1);
}


static inline void reserve_grid_points(int n)
{
	if (n > grid_point_capacity) {
		grid_point_capacity = n + 10;
		grid_points = realloc(grid_points, grid_point_capacity * 2 * sizeof(GLfloat));
	}
}

void hidgl_draw_local_grid(pcb_hidlib_t *hidlib, pcb_coord_t cx, pcb_coord_t cy, int radius)
{
	int npoints = 0;
	pcb_coord_t x, y;

	/* PI is approximated with 3.25 here - allows a minimal overallocation, speeds up calculations */
	const int r2 = radius * radius;
	const int n = r2 * 3 + r2 / 4 + 1;

	reserve_grid_points(n);

	for(y = -radius; y <= radius; y++) {
		int y2 = y * y;
		for(x = -radius; x <= radius; x++) {
			if (x * x + y2 < r2) {
				grid_points[npoints * 2] = x * hidlib->grid + cx;
				grid_points[npoints * 2 + 1] = y * hidlib->grid + cy;
				npoints++;
			}
		}
	}

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, grid_points);
	glDrawArrays(GL_POINTS, 0, npoints);
	glDisableClientState(GL_VERTEX_ARRAY);

}

void hidgl_draw_grid(pcb_hidlib_t *hidlib, pcb_box_t *drawn_area)
{
	pcb_coord_t x1, y1, x2, y2, n, i;
	double x, y;

	x1 = pcb_grid_fit(MAX(0, drawn_area->X1), hidlib->grid, hidlib->grid_ox);
	y1 = pcb_grid_fit(MAX(0, drawn_area->Y1), hidlib->grid, hidlib->grid_oy);
	x2 = pcb_grid_fit(MIN(hidlib->size_x, drawn_area->X2), hidlib->grid, hidlib->grid_ox);
	y2 = pcb_grid_fit(MIN(hidlib->size_y, drawn_area->Y2), hidlib->grid, hidlib->grid_oy);

	if (x1 > x2) {
		pcb_coord_t tmp = x1;
		x1 = x2;
		x2 = tmp;
	}

	if (y1 > y2) {
		pcb_coord_t tmp = y1;
		y1 = y2;
		y2 = tmp;
	}

	n = (int)((x2 - x1) / hidlib->grid + 0.5) + 1;
	reserve_grid_points(n);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, grid_points);

	n = 0;
	for(x = x1; x <= x2; x += hidlib->grid, ++n)
		grid_points[2 * n + 0] = x;

	for(y = y1; y <= y2; y += hidlib->grid) {
		for(i = 0; i < n; i++)
			grid_points[2 * i + 1] = y;
		glDrawArrays(GL_POINTS, 0, n);
	}

	glDisableClientState(GL_VERTEX_ARRAY);
}

#define MAX_PIXELS_ARC_TO_CHORD 0.5
#define MIN_SLICES 6
int calc_slices(float pix_radius, float sweep_angle)
{
	float slices;

	if (pix_radius <= MAX_PIXELS_ARC_TO_CHORD)
		return MIN_SLICES;

	slices = sweep_angle / acosf(1 - MAX_PIXELS_ARC_TO_CHORD / pix_radius) / 2.;
	return (int)ceilf(slices);
}

#define MIN_TRIANGLES_PER_CAP 3
#define MAX_TRIANGLES_PER_CAP 90
static void draw_cap(pcb_coord_t width, pcb_coord_t x, pcb_coord_t y, pcb_angle_t angle, double scale)
{
	float last_capx, last_capy;
	float capx, capy;
	float radius = width / 2.;
	int slices = calc_slices(radius / scale, M_PI);
	int i;

	if (slices < MIN_TRIANGLES_PER_CAP)
		slices = MIN_TRIANGLES_PER_CAP;

	if (slices > MAX_TRIANGLES_PER_CAP)
		slices = MAX_TRIANGLES_PER_CAP;

	drawgl_reserve_triangles(slices);

	last_capx = radius * cosf(angle * M_PI / 180.) + x;
	last_capy = -radius * sinf(angle * M_PI / 180.) + y;
	for(i = 0; i < slices; i++) {
		capx = radius * cosf(angle * M_PI / 180. + ((float)(i + 1)) * M_PI / (float)slices) + x;
		capy = -radius * sinf(angle * M_PI / 180. + ((float)(i + 1)) * M_PI / (float)slices) + y;
		drawgl_add_triangle(last_capx, last_capy, capx, capy, x, y);
		last_capx = capx;
		last_capy = capy;
	}
}

#define NEEDS_CAP(width, coord_per_pix) (width > coord_per_pix)

void hidgl_draw_line(int cap, pcb_coord_t width, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, double scale)
{
	double angle;
	float deltax, deltay, length;
	float wdx, wdy;
	int circular_caps = 0;
	pcb_coord_t orig_width = width;

	if ((width == 0) || (!NEEDS_CAP(orig_width, scale)))
		drawgl_add_line(x1, y1, x2, y2);
	else {
		if (width < scale)
			width = scale;

		deltax = x2 - x1;
		deltay = y2 - y1;

		length = sqrt(deltax * deltax + deltay * deltay);

		if (length == 0) {
			/* Assume the orientation of the line is horizontal */
			angle = 0;
			wdx = 0;
			wdy = width / 2.;
			length = 1.;
			deltax = 1.;
			deltay = 0.;
		}
		else {
			wdy = deltax * width / 2. / length;
			wdx = -deltay * width / 2. / length;

			if (deltay == 0.)
				angle = (deltax < 0) ? 270. : 90.;
			else
				angle = 180. / M_PI * atanl(deltax / deltay);

			if (deltay < 0)
				angle += 180.;
		}

		switch (cap) {
			case pcb_cap_round:
				circular_caps = 1;
				break;

			case pcb_cap_square:
				x1 -= deltax * width / 2. / length;
				y1 -= deltay * width / 2. / length;
				x2 += deltax * width / 2. / length;
				y2 += deltay * width / 2. / length;
				break;

			default:
				assert(!"unhandled cap");
				circular_caps = 1;
		}

		drawgl_add_triangle(x1 - wdx, y1 - wdy, x2 - wdx, y2 - wdy, x2 + wdx, y2 + wdy);
		drawgl_add_triangle(x1 - wdx, y1 - wdy, x2 + wdx, y2 + wdy, x1 + wdx, y1 + wdy);

		if (circular_caps) {
			draw_cap(width, x1, y1, angle, scale);
			draw_cap(width, x2, y2, angle + 180., scale);
		}
	}
}

#define MIN_SLICES_PER_ARC 6
#define MAX_SLICES_PER_ARC 360
void hidgl_draw_arc(pcb_coord_t width, pcb_coord_t x, pcb_coord_t y, pcb_coord_t rx, pcb_coord_t ry, pcb_angle_t start_angle, pcb_angle_t delta_angle, double scale)
{
	float last_inner_x, last_inner_y;
	float last_outer_x, last_outer_y;
	float inner_x, inner_y;
	float outer_x, outer_y;
	float inner_r;
	float outer_r;
	float cos_ang, sin_ang;
	float start_angle_rad;
	float delta_angle_rad;
	float angle_incr_rad;
	int slices;
	int i;
	int hairline = 0;
	pcb_coord_t orig_width = width;

	/* TODO: Draw hairlines as lines instead of triangles ? */

	if (width == 0.0)
		hairline = 1;

	if (width < scale)
		width = scale;

	inner_r = rx - width / 2.;
	outer_r = rx + width / 2.;

	if (delta_angle < 0) {
		start_angle += delta_angle;
		delta_angle = -delta_angle;
	}

	start_angle_rad = start_angle * M_PI / 180.;
	delta_angle_rad = delta_angle * M_PI / 180.;

	slices = calc_slices((rx + width / 2.) / scale, delta_angle_rad);

	if (slices < MIN_SLICES_PER_ARC)
		slices = MIN_SLICES_PER_ARC;

	if (slices > MAX_SLICES_PER_ARC)
		slices = MAX_SLICES_PER_ARC;

	drawgl_reserve_triangles(2 * slices);

	angle_incr_rad = delta_angle_rad / (float)slices;

	cos_ang = cosf(start_angle_rad);
	sin_ang = sinf(start_angle_rad);
	last_inner_x = -inner_r * cos_ang + x;
	last_inner_y = inner_r * sin_ang + y;
	last_outer_x = -outer_r * cos_ang + x;
	last_outer_y = outer_r * sin_ang + y;
	for(i = 1; i <= slices; i++) {
		cos_ang = cosf(start_angle_rad + ((float)(i)) * angle_incr_rad);
		sin_ang = sinf(start_angle_rad + ((float)(i)) * angle_incr_rad);
		inner_x = -inner_r * cos_ang + x;
		inner_y = inner_r * sin_ang + y;
		outer_x = -outer_r * cos_ang + x;
		outer_y = outer_r * sin_ang + y;

		drawgl_add_triangle(last_inner_x, last_inner_y, last_outer_x, last_outer_y, outer_x, outer_y);
		drawgl_add_triangle(last_inner_x, last_inner_y, inner_x, inner_y, outer_x, outer_y);

		last_inner_x = inner_x;
		last_inner_y = inner_y;
		last_outer_x = outer_x;
		last_outer_y = outer_y;
	}

	/* Don't bother capping hairlines */
	if (hairline)
		return;

	if (NEEDS_CAP(orig_width, scale)) {
		draw_cap(width, x + rx * -cosf(start_angle_rad), y + rx * sinf(start_angle_rad), start_angle, scale);
		draw_cap(width, x + rx * -cosf(start_angle_rad + delta_angle_rad), y + rx * sinf(start_angle_rad + delta_angle_rad), start_angle + delta_angle + 180., scale);
	}
}

void hidgl_draw_rect(pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	drawgl_add_rectangle(x1, y1, x2, y2);
}

void hidgl_fill_circle(pcb_coord_t vx, pcb_coord_t vy, pcb_coord_t vr, double scale)
{
#define MIN_TRIANGLES_PER_CIRCLE 6
#define MAX_TRIANGLES_PER_CIRCLE 360

	float last_x, last_y;
	float radius = vr;
	int slices;
	int i;

	assert((composite_op == PCB_HID_COMP_POSITIVE) || (composite_op == PCB_HID_COMP_POSITIVE_XOR) || (composite_op == PCB_HID_COMP_NEGATIVE));

	slices = calc_slices(vr / scale, 2 * M_PI);

	if (slices < MIN_TRIANGLES_PER_CIRCLE)
		slices = MIN_TRIANGLES_PER_CIRCLE;

	if (slices > MAX_TRIANGLES_PER_CIRCLE)
		slices = MAX_TRIANGLES_PER_CIRCLE;

	drawgl_reserve_triangles(slices);

	last_x = vx + vr;
	last_y = vy;

	for(i = 0; i < slices; i++) {
		float x, y;
		x = radius * cosf(((float)(i + 1)) * 2. * M_PI / (float)slices) + vx;
		y = radius * sinf(((float)(i + 1)) * 2. * M_PI / (float)slices) + vy;
		drawgl_add_triangle(vx, vy, last_x, last_y, x, y);
		last_x = x;
		last_y = y;
	}
}


#define MAX_COMBINED_MALLOCS 2500
static void *combined_to_free[MAX_COMBINED_MALLOCS];
static int combined_num_to_free = 0;

static GLenum tessVertexType;
static int stashed_vertices;
static int triangle_comp_idx;

static void myError(GLenum errno)
{
	printf("gluTess error: %s\n", gluErrorString(errno));
}

static void myFreeCombined()
{
	while(combined_num_to_free)
		free(combined_to_free[--combined_num_to_free]);
}

static void myCombine(GLdouble coords[3], void *vertex_data[4], GLfloat weight[4], void **dataOut)
{
#define MAX_COMBINED_VERTICES 2500
	static GLdouble combined_vertices[3 *MAX_COMBINED_VERTICES];
	static int num_combined_vertices = 0;

	GLdouble *new_vertex;

	if (num_combined_vertices < MAX_COMBINED_VERTICES) {
		new_vertex = &combined_vertices[3 * num_combined_vertices];
		num_combined_vertices++;
	}
	else {
		new_vertex = malloc(3 * sizeof(GLdouble));

		if (combined_num_to_free < MAX_COMBINED_MALLOCS)
			combined_to_free[combined_num_to_free++] = new_vertex;
		else
			printf("myCombine leaking %lu bytes of memory\n", 3 * sizeof(GLdouble));
	}

	new_vertex[0] = coords[0];
	new_vertex[1] = coords[1];
	new_vertex[2] = coords[2];

	*dataOut = new_vertex;
}

static void myBegin(GLenum type)
{
	tessVertexType = type;
	stashed_vertices = 0;
	triangle_comp_idx = 0;
}

static double global_scale;

static void myVertex(GLdouble *vertex_data)
{
	static GLfloat triangle_vertices[2 *3];

	if (tessVertexType == GL_TRIANGLE_STRIP || tessVertexType == GL_TRIANGLE_FAN) {
		if (stashed_vertices < 2) {
			triangle_vertices[triangle_comp_idx++] = vertex_data[0];
			triangle_vertices[triangle_comp_idx++] = vertex_data[1];
			stashed_vertices++;
		}
		else {
			drawgl_add_triangle(triangle_vertices[0], triangle_vertices[1], triangle_vertices[2], triangle_vertices[3], vertex_data[0], vertex_data[1]);
			if (tessVertexType == GL_TRIANGLE_STRIP) {
				/* STRIP saves the last two vertices for re-use in the next triangle */
				triangle_vertices[0] = triangle_vertices[2];
				triangle_vertices[1] = triangle_vertices[3];
			}
			/* Both FAN and STRIP save the last vertex for re-use in the next triangle */
			triangle_vertices[2] = vertex_data[0];
			triangle_vertices[3] = vertex_data[1];
		}
	}
	else if (tessVertexType == GL_TRIANGLES) {
		triangle_vertices[triangle_comp_idx++] = vertex_data[0];
		triangle_vertices[triangle_comp_idx++] = vertex_data[1];
		stashed_vertices++;
		if (stashed_vertices == 3) {
			drawgl_add_triangle(triangle_vertices[0], triangle_vertices[1], triangle_vertices[2], triangle_vertices[3], triangle_vertices[4], triangle_vertices[5]);
			triangle_comp_idx = 0;
			stashed_vertices = 0;
		}
	}
	else
		printf("Vertex received with unknown type\n");
}

/* Intentaional code duplication for performance */
void hidgl_fill_polygon(int n_coords, pcb_coord_t *x, pcb_coord_t *y)
{
	int i;
	GLUtesselator *tobj;
	GLdouble *vertices;

	assert(n_coords > 0);

	vertices = malloc(sizeof(GLdouble) * n_coords * 3);

	tobj = gluNewTess();
	gluTessCallback(tobj, GLU_TESS_BEGIN, (_GLUfuncptr) myBegin);
	gluTessCallback(tobj, GLU_TESS_VERTEX, (_GLUfuncptr) myVertex);
	gluTessCallback(tobj, GLU_TESS_COMBINE, (_GLUfuncptr) myCombine);
	gluTessCallback(tobj, GLU_TESS_ERROR, (_GLUfuncptr) myError);

	gluTessBeginPolygon(tobj, NULL);
	gluTessBeginContour(tobj);

	for(i = 0; i < n_coords; i++) {
		vertices[0 + i * 3] = x[i];
		vertices[1 + i * 3] = y[i];
		vertices[2 + i * 3] = 0.;
		gluTessVertex(tobj, &vertices[i * 3], &vertices[i * 3]);
	}

	gluTessEndContour(tobj);
	gluTessEndPolygon(tobj);
	gluDeleteTess(tobj);

	myFreeCombined();
	free(vertices);
}

/* Intentaional code duplication for performance */
void hidgl_fill_polygon_offs(int n_coords, pcb_coord_t *x, pcb_coord_t *y, pcb_coord_t dx, pcb_coord_t dy)
{
	int i;
	GLUtesselator *tobj;
	GLdouble *vertices;

	assert(n_coords > 0);

	vertices = malloc(sizeof(GLdouble) * n_coords * 3);

	tobj = gluNewTess();
	gluTessCallback(tobj, GLU_TESS_BEGIN, (_GLUfuncptr) myBegin);
	gluTessCallback(tobj, GLU_TESS_VERTEX, (_GLUfuncptr) myVertex);
	gluTessCallback(tobj, GLU_TESS_COMBINE, (_GLUfuncptr) myCombine);
	gluTessCallback(tobj, GLU_TESS_ERROR, (_GLUfuncptr) myError);

	gluTessBeginPolygon(tobj, NULL);
	gluTessBeginContour(tobj);

	for(i = 0; i < n_coords; i++) {
		vertices[0 + i * 3] = x[i] + dx;
		vertices[1 + i * 3] = y[i] + dy;
		vertices[2 + i * 3] = 0.;
		gluTessVertex(tobj, &vertices[i * 3], &vertices[i * 3]);
	}

	gluTessEndContour(tobj);
	gluTessEndPolygon(tobj);
	gluDeleteTess(tobj);

	myFreeCombined();
	free(vertices);
}

void tesselate_contour(GLUtesselator *tobj, pcb_pline_t *contour, GLdouble *vertices, double scale)
{
	pcb_vnode_t *vn = &contour->head;
	int offset = 0;
	pcb_coord_t lastx = PCB_MAX_COORD, lasty = PCB_MAX_COORD, mindist = scale * 2;

	/* If the contour is round, and hidgl_fill_circle would use
	 * less slices than we have vertices to draw it, then call
	 * hidgl_fill_circle to draw this contour.
	 */
	if (contour->is_round) {
		double slices = calc_slices(contour->radius / scale, 2 * M_PI);
		if (slices < contour->Count) {
			hidgl_fill_circle(contour->cx, contour->cy, contour->radius, scale);
			return;
		}
	}

	gluTessBeginPolygon(tobj, NULL);
	gluTessBeginContour(tobj);
	do {
		if ((offset > 3) && (PCB_ABS(vn->point[0] - lastx) < mindist) && (PCB_ABS(vn->point[1] - lasty) < mindist))
			continue;
		vertices[0 + offset] = vn->point[0];
		vertices[1 + offset] = vn->point[1];
		vertices[2 + offset] = 0.;
		lastx = vn->point[0];
		lasty = vn->point[1];
		gluTessVertex(tobj, &vertices[offset], &vertices[offset]);
		offset += 3;
	} while((vn = vn->next) != &contour->head);
	gluTessEndContour(tobj);
	gluTessEndPolygon(tobj);
}

struct do_hole_info {
	GLUtesselator *tobj;
	GLdouble *vertices;
	double scale;
};

static pcb_r_dir_t do_hole(const pcb_box_t *b, void *cl)
{
	struct do_hole_info *info = cl;
	pcb_pline_t *curc = (pcb_pline_t *) b;

	/* Ignore the outer contour - we draw it first explicitly */
	if (curc->Flags.orient == PCB_PLF_DIR) {
		return PCB_R_DIR_NOT_FOUND;
	}

	tesselate_contour(info->tobj, curc, info->vertices, info->scale);
	return PCB_R_DIR_FOUND_CONTINUE;
}


void hidgl_fill_pcb_polygon(pcb_polyarea_t *poly, const pcb_box_t *clip_box, double scale, int fullpoly)
{
	int vertex_count = 0;
	pcb_pline_t *contour;
	struct do_hole_info info;
	pcb_polyarea_t *p_area;

	info.scale = scale;
	global_scale = scale;

	if (poly == NULL) {
		fprintf(stderr, "hidgl_fill_pcb_polygon: polyarea == NULL\n");
		return;
	}

	drawgl_flush();

	/* Walk the polygon structure, counting vertices */
	/* This gives an upper bound on the amount of storage required */
	p_area = poly;
	do {
		for(contour = p_area->contours; contour != NULL; contour = contour->next)
			vertex_count = MAX(vertex_count, contour->Count);
		p_area = p_area->f;
	} while(fullpoly && (p_area != poly));

	/* Allocate a vertex buffer */
	info.vertices = malloc(sizeof(GLdouble) * vertex_count * 3);

	/* Setup the tesselator */
	info.tobj = gluNewTess();
	gluTessCallback(info.tobj, GLU_TESS_BEGIN, (_GLUfuncptr) myBegin);
	gluTessCallback(info.tobj, GLU_TESS_VERTEX, (_GLUfuncptr) myVertex);
	gluTessCallback(info.tobj, GLU_TESS_COMBINE, (_GLUfuncptr) myCombine);
	gluTessCallback(info.tobj, GLU_TESS_ERROR, (_GLUfuncptr) myError);

	/* Walk the polygon structure. Each iteration draws an island. For each island, all 
	 * of the island's cutouts are drawn to the stencil buffer as a '1'. Then the island
	 * is drawn and areas where the stencil is set to a '1' are masked and not drawn.
	 */
	p_area = poly;
	do {

		/* Create a MASK (stencil). All following drawing will affect the mask only */
		drawgl_add_mask_create();

		/* Find and draw the cutouts */
		pcb_r_search(p_area->contour_tree, clip_box, NULL, do_hole, &info, NULL);
		drawgl_flush();

		/* Use the mask while updating the colour buffer. All following drawing will update the 
		 * colour buffer wherever the mask is not set
		 */
		drawgl_add_mask_use();

		/* Draw the island to the colour buffer */
		tesselate_contour(info.tobj, p_area->contours, info.vertices, scale);

		/* destroy the current MASK */
		drawgl_add_mask_destroy();
		drawgl_flush();

		p_area = p_area->f;
	} while(fullpoly && (p_area != poly));

	gluDeleteTess(info.tobj);
	myFreeCombined();
	free(info.vertices);
}
