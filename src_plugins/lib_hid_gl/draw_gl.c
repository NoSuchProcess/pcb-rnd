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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "config.h"

#include "stencil_gl.h"

#define MARKER_STACK_SIZE				16
#define RESERVE_VERTEX_EXTRA		1024
#define RESERVE_PRIMITIVE_EXTRA	256

enum
{
	PRIM_MASK_CREATE		= 1000,
	PRIM_MASK_DESTROY,
	PRIM_MASK_USE
};


/* Vertex Buffer Data
 * The vertex buffer is a dynamic array of vertices. Each vertex contains 
 * position and colour information.
 */ 

typedef struct
{
	GLfloat		x;
	GLfloat		y;
	GLfloat		r;
	GLfloat		g;
	GLfloat		b;
	GLfloat		a;
} vertex_t;


typedef struct
{
	vertex_t * 		data;						/* A dynamic array of vertices. */
	int						capacity;				/* The capacity of the buffer (by number of vertices) */
	int 					size;						/* The number of vertices in the buffer. */
	int 					marker;					/* An index that allows vertices after the marker to be removed. */
} vertex_buffer_t;

static vertex_buffer_t	vertex_buffer = {0};

/* Primitive Buffer Data */

typedef struct
{
	int				type;			/* The type of the primitive, eg. GL_LINES, GL_TRIANGLES, GL_TRIANGLE_FAN */
	GLint			first;		/* The index of the first vertex in the vertex buffer. */
	GLsizei		count;		/* The number of vertices */
} primitive_t;

typedef struct
{
	primitive_t * 	data;															/* A dynamic array of primitives */
	int							capacity;													/* The number of primitives that can fit into the primitive_data */
	int							size;															/* The actual number of primitives in the primitive buffer */
	int 						marker;														/* An index that allows primitives after the marker to be removed. */
	int 						dirty_index;											/* The index of the first primitive that hasn't been drawn yet. */
} primitive_buffer_t;

static primitive_buffer_t primitive_buffer = {0};

static GLfloat red 		= 0.0f;
static GLfloat green 	= 0.0f;
static GLfloat blue 	= 0.0f;
static GLfloat alpha 	= 0.75f;
	
static int mask_stencil = 0;

static inline void
vertex_buffer_clear()
{
	vertex_buffer.size = 0;
	vertex_buffer.marker = 0;
}

static void
vertex_buffer_destroy()
{
	vertex_buffer_clear();
	if(vertex_buffer.data) {
		free(vertex_buffer.data);
		vertex_buffer.data = NULL;
	}
}

/* Ensure that the total capacity of the vertex buffer is at least 'size' vertices.
 * When the buffer is reallocated, extra vertices will be added to avoid many small 
 * allocations beingi made.
 */
static int
vertex_buffer_reserve(int size)
{
	int result = 0;
	if(size > vertex_buffer.capacity)	{
		vertex_t * p_data = realloc(vertex_buffer.data,(size + RESERVE_VERTEX_EXTRA) * sizeof(vertex_t));
		if(p_data == NULL)
			result = -1;
		else {
			vertex_buffer.data = p_data;
			vertex_buffer.capacity = size + RESERVE_VERTEX_EXTRA;
		}
	}
	return result;
}

/* Ensure that the capacity of the vertex buffer can accomodate an allocation
 * of at least 'size' vertices.
 */
static inline int 
vertex_buffer_reserve_extra(int size)
{
	return vertex_buffer_reserve(vertex_buffer.size + size);
}

/* Set the marker to the end of the active vertex data. This allows vertices 
 * added after the marker has been set to be discarded. This is required when 
 * temporary vertices are required to draw something that will not be required
 * for the final render pass.
 */
static inline int
vertex_buffer_set_marker()
{
	vertex_buffer.marker = vertex_buffer.size;
	return vertex_buffer.marker;
}

/* Discard vertices added after the marker was set. The end of the buffer will then be
 * the position of the marker.
 */
static inline void
vertex_buffer_rewind()
{
	vertex_buffer.size = vertex_buffer.marker;
}

static inline vertex_t * 
vertex_buffer_allocate(int size)
{
	vertex_t * p_vertex = NULL;			
	if(vertex_buffer_reserve_extra(size) == 0) {
		p_vertex = &vertex_buffer.data[vertex_buffer.size];
		vertex_buffer.size += size;
	}
	
	return p_vertex;
}

static inline void
vertex_buffer_add(GLfloat x,GLfloat y)
{
	vertex_t * p_vert = vertex_buffer_allocate(1);
	if(p_vert) {
		p_vert->x = x;
		p_vert->y = y;
		p_vert->r = red;
		p_vert->g = green;
		p_vert->b = blue;
		p_vert->a = alpha;
	}
}

static inline void
primitive_buffer_clear()
{
	primitive_buffer.size = 0;
	primitive_buffer.dirty_index = 0;
	primitive_buffer.marker = 0;
}

static void
primitive_buffer_destroy()
{
	primitive_buffer_clear();
	if(primitive_buffer.data)	{
		free(primitive_buffer.data);
		primitive_buffer.data = NULL;
	}
}

/* Ensure that the total capacity of the primitive buffer is at least 'size' 
 * primitives.  When reallocating the buffer, extra primitives will be added
 * to avoid many small reallocations.
 */
static int
primitive_buffer_reserve(int size)
{
	int result = 0;

	if(size > primitive_buffer.capacity) {
		primitive_t * p_data = realloc(primitive_buffer.data,(size + RESERVE_PRIMITIVE_EXTRA) * sizeof(primitive_t));
		if(p_data == NULL)
			result = -1;
		else {
			primitive_buffer.data = p_data;
			primitive_buffer.capacity = size + RESERVE_PRIMITIVE_EXTRA;
		}
	}

	return result;
}

/* Ensure that the capacity of the primitive buffer can accomodate an allocation
 * of at least 'size' primitives.
 */
static inline int 
primitive_buffer_reserve_extra(int size)
{
	return primitive_buffer_reserve(primitive_buffer.size + size);
}

/* Set the marker to the end of the active primitive data. This allows primitives 
 * added after the marker to be discarded. This is required when temporary primitives 
 * are required to draw something that will not be required for the final render pass.
 */
static inline int
primitive_buffer_set_marker()
{
	primitive_buffer.marker = primitive_buffer.size;
	return primitive_buffer.marker;
}

/* Discard primitives added after the marker was set. The end of the buffer will then be
 * the position of the marker.
 */
static inline void
primitive_buffer_rewind()
{
	primitive_buffer.size = primitive_buffer.marker;
}

static inline primitive_t *
primitive_buffer_back()
{
	return (primitive_buffer.size > 0) && (primitive_buffer.data) ? &primitive_buffer.data[primitive_buffer.size-1] : NULL;
}

static inline int
primitive_dirty_count()
{
		return primitive_buffer.size - primitive_buffer.dirty_index;
}

static void
primitive_buffer_add(int type,GLint first,GLsizei count)
{
	primitive_t * last = primitive_buffer_back();

	/* If the last primitive is the same type AND that type can be extended 
	 * AND the last primitive is dirty AND 'first' follows the last vertex of
	 * the previous primitive THEN we can simply append the new primitive to
	 * the last one.
	 */
	if(	last && 
			(primitive_dirty_count()>0) && 
			(last->type == type) &&
		 	((type == GL_LINES)	|| (type == GL_TRIANGLES) || (type == GL_POINTS)) &&
			((last->first + last->count) == first) )
		last->count += count;
	else if(primitive_buffer_reserve_extra(1) == 0)	{
		primitive_t * p_prim = &primitive_buffer.data[primitive_buffer.size++];
		p_prim->type  = type;
		p_prim->first = first;
		p_prim->count = count;
	}
}

static inline int 
primitive_buffer_last_type()
{
	return primitive_buffer.size > 0 ? primitive_buffer.data[primitive_buffer.size-1].type : GL_ZERO;
}

void
drawgl_set_colour(GLfloat r,GLfloat g,GLfloat b,GLfloat a)
{
	red		= r;
	green	= g;
	blue	= b;
	alpha = a;
}

PCB_INLINE void
drawgl_add_triangle(GLfloat x1,GLfloat y1,GLfloat x2,GLfloat y2,GLfloat x3,GLfloat y3)
{
	/* Debug Drawing 
	primitive_buffer_add(GL_LINES,vertex_buffer.size,6);
	vertex_buffer_reserve_extra(6);
	vertex_buffer_add(x1,y1);	vertex_buffer_add(x2,y2);
	vertex_buffer_add(x2,y2);	vertex_buffer_add(x3,y3);
	vertex_buffer_add(x3,y3);	vertex_buffer_add(x1,y1);
  */

	primitive_buffer_add(GL_TRIANGLES,vertex_buffer.size,3);
	vertex_buffer_reserve_extra(3);
	vertex_buffer_add(x1,y1);
	vertex_buffer_add(x2,y2);
	vertex_buffer_add(x3,y3);

}

PCB_INLINE void
drawgl_add_line(GLfloat x1,GLfloat y1,GLfloat x2,GLfloat y2)
{
	primitive_buffer_add(GL_LINES,vertex_buffer.size,2);
	vertex_buffer_reserve_extra(2);
	vertex_buffer_add(x1,y1);
	vertex_buffer_add(x2,y2);
}

PCB_INLINE void
drawgl_add_rectangle(GLfloat x1,GLfloat y1,GLfloat x2,GLfloat y2)
{
	primitive_buffer_add(GL_LINE_LOOP,vertex_buffer.size,4);
	vertex_buffer_reserve_extra(4);
	vertex_buffer_add(x1,y1);
	vertex_buffer_add(x2,y1);
	vertex_buffer_add(x2,y2);
	vertex_buffer_add(x1,y2);
}

PCB_INLINE void
drawgl_add_mask_create()
{
	primitive_buffer_add(PRIM_MASK_CREATE,0,0);
}

PCB_INLINE void
drawgl_add_mask_destroy()
{
	primitive_buffer_add(PRIM_MASK_DESTROY,0,0);
}

PCB_INLINE void
drawgl_add_mask_use()
{
	primitive_buffer_add(PRIM_MASK_USE,0,0);
}

PCB_INLINE void
drawgl_direct_draw_rectangle(GLfloat x1,GLfloat y1,GLfloat x2,GLfloat y2)
{
	glBegin(GL_LINE_LOOP);
		glColor4f(red,green,blue,alpha);
		glVertex2f(x1,y1);
		glVertex2f(x2,y1);
		glVertex2f(x2,y2);
		glVertex2f(x1,y2);
	glEnd();
}

PCB_INLINE void
drawgl_direct_draw_solid_rectangle(GLfloat x1,GLfloat y1,GLfloat x2,GLfloat y2)
{
	glBegin(GL_TRIANGLES);
		glColor4f(red,green,blue,alpha);
		glVertex2f(x1,y1);
		glVertex2f(x2,y1);
		glVertex2f(x1,y2);
		glVertex2f(x2,y1);
		glVertex2f(x2,y2);
		glVertex2f(x1,y2);
	glEnd();
}

PCB_INLINE void
drawgl_reserve_triangles(int count)
{
	vertex_buffer_reserve_extra(count * 3);
}
		
/* This function will draw the specified primitive but it may also modify the state of
 * the stencil buffer when MASK primtives exist.
 */
static inline void
drawgl_draw_primtive(primitive_t * prim)
{
	switch(prim->type) {
		case PRIM_MASK_CREATE :
			if(mask_stencil)
				stencilgl_return_stencil_bit(mask_stencil);
			mask_stencil = stencilgl_allocate_clear_stencil_bit();
			if(mask_stencil != 0) {
				glPushAttrib(GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
				glStencilMask(mask_stencil);
				glStencilFunc(GL_ALWAYS,mask_stencil,mask_stencil);
				glStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);
				glColorMask(0,0,0,0);
			}
			break;			

		case PRIM_MASK_USE :
			{
				GLint ref = 0;
				glPopAttrib();
				glPushAttrib(GL_STENCIL_BUFFER_BIT);
				glGetIntegerv(GL_STENCIL_REF,&ref);
				glStencilFunc(GL_GEQUAL,ref & ~mask_stencil,mask_stencil);
			}
			break;			

		case PRIM_MASK_DESTROY :
			glPopAttrib();
			stencilgl_return_stencil_bit(mask_stencil);
			mask_stencil = 0;
			break;			

		default :
	  	glDrawArrays(prim->type, prim->first, prim->count);
			break;
	}
}

void
drawgl_flush()
{
 	int index = primitive_buffer.dirty_index;
	int end = primitive_buffer.size;
	primitive_t * prim = &primitive_buffer.data[index];

	/* Setup the vertex buffer */
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
 	glVertexPointer(2, GL_FLOAT, sizeof(vertex_t), &vertex_buffer.data[0].x);
 	glColorPointer(4, GL_FLOAT, sizeof(vertex_t), &vertex_buffer.data[0].r);

	/* draw the primitives */
	while(index < end) {
		drawgl_draw_primtive(prim);
		++prim;
		++index;
	}

	/* disable the vertex buffer */
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
 
	primitive_buffer.dirty_index = end;
}

/* Draw all buffered primitives. The dirty index is ignored and will remain unchanged.
 * This function accepts stencil bits that can be used to mask the drawing.
 */
PCB_INLINE void
drawgl_draw_all(int stencil_bits)
{
	int index = primitive_buffer.size;
	primitive_t * prim;
	int mask = 0;

	if((index == 0) || (primitive_buffer.data == NULL))
		return;

	--index;
	prim = &primitive_buffer.data[index];

	/* Setup the vertex buffer */
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
 	glVertexPointer(2, GL_FLOAT, sizeof(vertex_t), &vertex_buffer.data[0].x);
 	glColorPointer(4, GL_FLOAT, sizeof(vertex_t), &vertex_buffer.data[0].r);

	/* draw the primitives */
	while(index >= 0)	{
		switch(prim->type) {
			case PRIM_MASK_DESTROY :
				/* The primitives are drawn in reverse order. The mask primitives are required
				 * to be processed in forward order so we must search for the matching 'mask create'
				 * primitive and then iterate through the primtives until we reach the 'mask destroy' 
				 * primitive.
				 */
				{
					primitive_t * next_prim = prim-1;;
					primitive_t * mask_prim = prim;
					int mask_index = index;
					int next_index = index-1;

					/* Find the 'mask create' primitive. */ 
					while((mask_index>=0) && (mask_prim->type != PRIM_MASK_CREATE))	{
						--mask_prim;
						--mask_index;
					}
						
					/* Process the primitives in forward order until we reach the 'mask destroy' primitive */
					if(mask_prim->type == PRIM_MASK_CREATE)	{
						next_index = mask_index;
						next_prim = mask_prim;

						while(mask_index <= index) {
							switch(mask_prim->type)	{
								case PRIM_MASK_CREATE :
									if(mask)
										stencilgl_return_stencil_bit(mask);
									mask = stencilgl_allocate_clear_stencil_bit();

									if(mask != 0)	{
										glPushAttrib(GL_STENCIL_BUFFER_BIT);
										
										glEnable(GL_STENCIL_TEST);
										stencilgl_mode_write_set(mask);
									}
									glPushAttrib(GL_COLOR_BUFFER_BIT);
									glColorMask(0,0,0,0);
									break;			

								case PRIM_MASK_USE :
									glPopAttrib();
									if(mask != 0)	{
										glEnable(GL_STENCIL_TEST);
										glStencilOp(GL_KEEP,GL_KEEP,GL_ZERO);
										glStencilFunc(GL_EQUAL,stencil_bits , stencil_bits | mask );
										glStencilMask(stencil_bits);
									}
									break;			

								case PRIM_MASK_DESTROY :
									if(mask != 0)	{
										glPopAttrib();
										stencilgl_return_stencil_bit(mask);
										mask = 0;
									}
									break;			

								default :
							  	glDrawArrays(mask_prim->type, mask_prim->first, mask_prim->count);
									break;
							}
							++mask_prim;
							++mask_index;
						}

						index = next_index;
						prim = next_prim;
					}
					else {
						index = mask_index;
						prim = mask_prim;
					}
				}
				break;

			default :
		  	glDrawArrays(prim->type, prim->first, prim->count);
				break;
		}

		--prim;
		--index;
	}

	if(mask)
		stencilgl_return_stencil_bit(mask);

	/* disable the vertex buffer */
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

}

void
drawgl_reset()
{
	vertex_buffer_clear();
	primitive_buffer_clear();
}

PCB_INLINE void
drawgl_set_marker()
{
	vertex_buffer_set_marker();
	primitive_buffer_set_marker();
}

PCB_INLINE void
drawgl_rewind_to_marker()
{
	vertex_buffer_rewind();
	primitive_buffer_rewind();
}

void
drawgl_uninit()
{
	vertex_buffer_destroy();
	primitive_buffer_destroy();
}


