/*
 *
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */
#ifndef PCB_VIEW_H
#define PCB_VIEW_H

#include <genlist/gendlist.h>
#include "unit.h"
#include "idpath.h"
#include "box.h"

/* A saved view on the board (e.g. a drc violation); metadata includes two lists
   of objects to highlight on preview and some optional, application-specific
   data (e.g. drc size values) */

typedef enum pcb_view_type_e {
	PCB_VIEW_PLAIN,       /* has no data */
	PCB_VIEW_DRC
} pcb_view_type_t;

struct pcb_view_s {
	unsigned long int uid;        /* ID unique for each view (for GUI identification) - 0 means invalid */

	char *type;
	char *title;
	char *explanation;

	/* these indicate whether some of the following fields are valid */
	unsigned have_bbox:1;
	unsigned have_xy:1;

	pcb_box_t bbox;               /* bounding box of all error objects (in both groups) */

	pcb_coord_t x, y;             /* optional: a coord to mark on the preview  */
	pcb_idpath_list_t objs[2];    /* optional: two groups of objects to highlight on preview */

	pcb_view_type_t data_type;
	union {
		struct {
			unsigned have_measured:1;
			pcb_coord_t measured_value;
			pcb_coord_t required_value;
		} drc;
	} data;

	gdl_elem_t link;              /* always part of a list */
};

/* List of drc violations */
#define TDL(x)      pcb_view_list_ ## x
#define TDL_LIST_T  pcb_view_list_t
#define TDL_ITEM_T  pcb_view_t
#define TDL_FIELD   link
#define TDL_SIZE_T  size_t
#define TDL_FUNC

#define pcb_view_list_foreach(list, iterator, loop_elem) \
	gdl_foreach_((&((list)->lst)), (iterator), (loop_elem))

#include <genlist/gentdlist_impl.h>
#include <genlist/gentdlist_undef.h>

/* Free all fields and the view pointer too */
void pcb_view_free(pcb_view_t *item);

/* Free all items and empty the list but do not free the list pointer */
void pcb_view_list_free_fields(pcb_view_list_t *lst);

void pcb_view_list_free(pcb_view_list_t *lst);


/* Slow, linear search for an UID in a list; returns NULL if not found */
pcb_view_t *pcb_view_by_uid(const pcb_view_list_t *lst, unsigned long int uid);

/* Zoom the drawing area to the drc error */
void pcb_view_goto(pcb_view_t *item);

/* Allocate a new, floating (unlinked) view with no data or bbox */
pcb_view_t *pcb_view_new(const char *type, const char *title, const char *explanation);

/* Append obj to one of the object groups in view (resolving to idpath) */
void pcb_view_append_obj(pcb_view_t *view, int grp, pcb_any_obj_t *obj);

/* Calculate and set v->bbox from v->objs[] bboxes */
void pcb_view_set_bbox_by_objs(pcb_data_t *data, pcb_view_t *v);

/*** temporary, until moved out to a plugin ***/
int pcb_drc_all(pcb_view_list_t *lst);

#endif
