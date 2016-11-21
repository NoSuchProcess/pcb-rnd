/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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

#ifndef PCB_OBJ_COMMON_H
#define PCB_OBJ_COMMON_H

#include <genht/hash.h>
#include <genlist/gendlist.h>
#include <string.h>
#include "flag.h"
#include "attrib.h"
#include "global_typedefs.h"

/* point and box type - they are so common everything depends on them */
struct pcb_point_s {    /* a line/polygon point */
	pcb_coord_t X, Y, X2, Y2;   /* so Point type can be cast as pcb_box_t */
	long int ID;
};

struct pcb_box_s {  /* a bounding box */
	pcb_coord_t X1, Y1;     /* upper left */
	pcb_coord_t X2, Y2;     /* and lower right corner */
};

pcb_box_t *GetObjectBoundingBox(int Type, void *Ptr1, void *Ptr2, void *Ptr3);

/* memset object to 0, but keep the link field */
#define reset_obj_mem(type, obj) \
do { \
	gdl_elem_t __lnk__ = obj->link; \
	memset(obj, 0, sizeof(type)); \
	obj->link = __lnk__; \
} while(0) \


/* pcb_true during file loads, for example to allow overlapping vias.
   pcb_false otherwise, to stop the user from doing normally dangerous
   things.  */
void pcb_create_be_lenient(pcb_bool);
extern pcb_bool pcb_create_being_lenient;

void pcb_create_ID_bump(int min_id);
void pcb_create_ID_reset(void);
long int pcb_create_ID_get(void);

void pcb_obj_add_attribs(void *obj, const pcb_attribute_list_t *src);

/* ---------------------------------------------------------------------------
 * Do not change the following definitions even if they're not very
 * nice.  It allows us to have functions act on these "base types" and
 * not need to know what kind of actual object they're working on.
 */

/* Any object that uses the "object flags" defined in const.h, or
   exists as an object on the pcb, MUST be defined using this as the
   first fields, either directly or through PCB_ANYLINEFIELDS.  */
#define PCB_ANYOBJECTFIELDS			\
	pcb_box_t		BoundingBox;	\
	long int	ID;		\
	pcb_flag_t	Flags; \
	pcb_attribute_list_t Attributes

	/*  struct pcb_lib_entry_t *net */

/* Lines, pads, and rats all use this so they can be cross-cast.  */
#define	PCB_ANYLINEFIELDS			\
	PCB_ANYOBJECTFIELDS;		\
	pcb_coord_t		Thickness,      \
                        Clearance;      \
	pcb_point_t	Point1,		\
			Point2

/* All on-pcb objects (elements, lines, pads, vias, rats, etc) are
   based on this. */
typedef struct {
	PCB_ANYOBJECTFIELDS;
} pcb_any_obj_t;

/* Lines, rats, pads, etc.  */
typedef struct {
	PCB_ANYLINEFIELDS;
} pcb_any_line_t;

/*** Functions and macros used for hashing ***/

/* compare two strings and return 0 if they are equal. NULL == NULL means equal. */
static inline PCB_FUNC_UNUSED int pcb_neqs(const char *s1, const char *s2)
{
	if ((s1 == NULL) && (s2 == NULL)) return 0;
	if ((s1 == NULL) || (s2 == NULL)) return 1;
	return strcmp(s1, s2) != 0;
}

static inline PCB_FUNC_UNUSED unsigned pcb_hash_coord(pcb_coord_t c)
{
	return murmurhash(&(c), sizeof(pcb_coord_t));
}


/* compare two fields and return 0 if they are equal */
#define pcb_field_neq(s1, s2, f) ((s1)->f != (s2)->f)

/* hash relative x and y within an element */
#define pcb_hash_element_ox(e, c) ((e) == NULL ? pcb_hash_coord(c) : pcb_hash_coord(c - e->MarkX))
#define pcb_hash_element_oy(e, c) ((e) == NULL ? pcb_hash_coord(c) : pcb_hash_coord(c - e->MarkY))

#define pcb_hash_str(s) ((s) == NULL ? 0 : strhash(s))

#define pcb_element_offs(e,ef, s,sf) ((e == NULL) ? (s)->sf : ((s)->sf) - ((e)->ef))
#define pcb_element_neq_offsx(e1, x1, e2, x2, f) (pcb_element_offs(e1, MarkX, x1, f) != pcb_element_offs(e2, MarkX, x2, f))
#define pcb_element_neq_offsy(e1, y1, e2, y2, f) (pcb_element_offs(e1, MarkY, y1, f) != pcb_element_offs(e2, MarkY, y2, f))

#endif
