/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#ifndef PCB_OBJ_COMMON_H
#define PCB_OBJ_COMMON_H

#include <math.h>
#include <string.h>
#include "flag.h"
#include "attrib.h"
#include "global_typedefs.h"
#include "data_parent.h"

/* Real objects that have actual struct; can be used as a bitfield */
typedef enum pcb_objtype_e {
	PCB_OBJ_VOID       = 0x0000000,

	PCB_OBJ_ARC        = 0x0000001,
	PCB_OBJ_LINE       = 0x0000002,
	PCB_OBJ_POLY       = 0x0000004,
	PCB_OBJ_TEXT       = 0x0000008,
	PCB_OBJ_SUBC       = 0x0000010,
	PCB_OBJ_PSTK       = 0x0000020,
	PCB_OBJ_RAT        = 0x0000040,


	/* more abstract objects */
	PCB_OBJ_NET        = 0x0001000,
	PCB_OBJ_NET_TERM   = 0x0002000,
	PCB_OBJ_LAYER      = 0x0004000,
	PCB_OBJ_LAYERGRP   = 0x0008000
} pcb_objtype_t;

/* Virtual objects for configuring searches; can be used as a bitfield with the above */
typedef enum pcb_vobjtype_e {
	PCB_OBJ_ARC_POINT  = 0x0100000,
	PCB_OBJ_LINE_POINT = 0x0200000,
	PCB_OBJ_POLY_POINT = 0x0400000,
	PCB_OBJ_SUBC_PART  = 0x0800000,
	PCB_OBJ_LOCKED     = 0x1000000,
	PCB_OBJ_FLOATER    = 0x2000000
} pcb_vobjtype_t;


/* combinations, groups, masks of pcb_objtype_t */
typedef enum pcb_objmask_e {
	/* lists */
	PCB_OBJ_CLASS_PIN  = (PCB_OBJ_PSTK | PCB_OBJ_SUBC_PART),
	PCB_OBJ_CLASS_TERM = (PCB_OBJ_CLASS_PIN | PCB_OBJ_SUBC_PART | PCB_OBJ_LINE | PCB_OBJ_ARC | PCB_OBJ_POLY | PCB_OBJ_TEXT),

	/* masks */
	PCB_OBJ_CLASS_REAL = 0x0000FFF, /* global and on-layer objects (but not abstract things like layers) */
	PCB_OBJ_CLASS_HOST = 0x00FF000, /* host types: layers, boards, nets */
	PCB_OBJ_CLASS_MASK = 0xFF00000, /* for virtual searches */
	PCB_OBJ_CLASS_OBJ  = 0x0000000, /* anything with common object fields (pcb_any_obj_t) */
	PCB_OBJ_ANY        = 0xFFFFFFF
} pcb_objmask_t;


/* point and box type - they are so common everything depends on them */
struct pcb_point_s {    /* a line/polygon point */
	pcb_coord_t X, Y, X2, Y2;   /* so Point type can be cast as pcb_box_t */
	long int ID;
};

typedef double pcb_xform_mx_t[9];
#define PCB_XFORM_MX_IDENT {1,0,0,   0,1,0,   0,0,1}

struct pcb_xform_s {   /* generic object transformation; all-zero means no transformation */
	pcb_coord_t bloat;           /* if non-zero, bloat (positive) or shrink (negative) by this value */

	unsigned layer_faded:1;      /* draw layer colors faded */
	unsigned omit_overlay:1;     /* do not draw overlays (which are useful on screen but normally omitted on exports, except if --as-shown is specified */
	/* WARNING: After adding new fields, make sure to update pcb_xform_add() and pcb_xform_is_nop() below */
};

#define pcb_xform_clear(dst)      memset(dst, 0, sizeof(pcb_xform_t))
#define pcb_xform_copy(dst, src)  memcpy(dst, src, sizeof(pcb_xform_t))
#define pcb_xform_add(dst, src) \
	do { \
		pcb_xform_t *__dst__ = dst; \
		const pcb_xform_t *__src__ = src; \
		__dst__->bloat += __src__->bloat; \
		__dst__->layer_faded |= __src__->layer_faded; \
		__dst__->omit_overlay |= __src__->omit_overlay; \
	} while(0)
#define pcb_xform_is_nop(src) (((src)->bloat == 0) && ((src)->layer_faded == 0) && ((src)->omit_overlay == 0))

/* Returns true if overlay drawing should be omitted */
#define pcb_xform_omit_overlay(info) \
	((info != NULL) && (info->xform != NULL) && (info->xform->omit_overlay != 0))

/* Standard 2d matrix transformation using mx as pcb_xform_mx_t */
#define pcb_xform_x(mx, x_in, y_in) ((double)(x_in) * mx[0] + (double)(y_in) * mx[1] + mx[2])
#define pcb_xform_y(mx, x_in, y_in) ((double)(x_in) * mx[3] + (double)(y_in) * mx[4] + mx[5])

void pcb_xform_mx_rotate(pcb_xform_mx_t mx, double deg);
void pcb_xform_mx_translate(pcb_xform_mx_t mx, double xt, double yt);
void pcb_xform_mx_scale(pcb_xform_mx_t mx, double st, double sy);
void pcb_xform_mx_shear(pcb_xform_mx_t mx, double sx, double sy);
void pcb_xform_mx_mirrorx(pcb_xform_mx_t mx); /* mirror over the x axis (flip y coords) */

/* Return the user readable name of an object type in a string; never NULL */
const char *pcb_obj_type_name(pcb_objtype_t type);

/* returns a flag mask of all valid flags for an (old) object type */
pcb_flag_values_t pcb_obj_valid_flags(unsigned long int objtype);


int pcb_obj_get_bbox_naked(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_box_t *res);

/* Host transformations: typically the transformations an object of a subc
   inherits from the subc */
typedef struct pcb_host_trans_s {
	pcb_coord_t ox, oy;
	int on_bottom;
	double rot;
	double cosa, sina; /* rot angle cache */
} pcb_host_trans_t;

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

/* Any object that ends up in common use as pcb_any_obj_t, MUST be defined
   using this as the first fieldsusing the following macros.  */
#define PCB_ANY_OBJ_FIELDS \
	pcb_box_t            BoundingBox; \
	long int             ID; \
	pcb_flag_t           Flags; \
	pcb_objtype_t        type; \
	pcb_parenttype_t     parent_type; \
	pcb_parent_t         parent; \
	pcb_box_t            bbox_naked; \
	pcb_attribute_list_t Attributes \

#define PCB_ANY_PRIMITIVE_FIELDS \
	PCB_ANY_OBJ_FIELDS; \
	const char           *term; \
	void                 *ratconn; \
	unsigned char        thermal; \
	unsigned char        intconn, intnoconn; \
	unsigned int         noexport:1; \
	unsigned int         noexport_named:1; \
	const pcb_color_t    *override_color

/* Lines, pads, and rats all use this so they can be cross-cast.  */
#define PCB_ANYLINEFIELDS \
	PCB_ANY_PRIMITIVE_FIELDS; \
	pcb_coord_t Thickness, Clearance; \
	pcb_point_t Point1, Point2

/* All on-pcb objects (elements, lines, pads, vias, rats, etc) are
   based on this. */
struct pcb_any_obj_s {
	PCB_ANY_PRIMITIVE_FIELDS;
};

/* Lines, rats, pads, etc.  */
struct pcb_any_line_s {
	PCB_ANYLINEFIELDS;
};

/* Return the geometric center of an object, as shown (center of bbox usually,
   but not for an arc) */
void pcb_obj_center(const pcb_any_obj_t *obj, pcb_coord_t *x, pcb_coord_t *y);

/* Return the clearance value of object on the specified layer (in
   case of padstack - in case of other objects layer is ignored) */
pcb_coord_t pcb_obj_clearance_at(pcb_board_t *pcb, const pcb_any_obj_t *o, pcb_layer_t *at);


/* Update cached attributes (->term) */
void pcb_obj_attrib_post_change(pcb_attribute_list_t *list, const char *name, const char *value);

/* Returns the first invalid character of an ID (terminal, refdes) or NULL */
const char *pcb_obj_id_invalid(const char *id);

/* Fix an ID in place (replace anything invalid with '_'); returns id */
char *pcb_obj_id_fix(char *id);

/* switch() on o->type and call the right implementation */
void pcb_obj_pre(pcb_any_obj_t *o);
void pcb_obj_post(pcb_any_obj_t *o);


#define pcb_obj_id_reg(data, obj) \
	do { \
		pcb_any_obj_t *__obj__ = (pcb_any_obj_t *)(obj); \
		htip_set(&(data)->id2obj, __obj__->ID, __obj__); \
	} while(0)

#define pcb_obj_id_del(data, obj) \
	htip_pop(&(data)->id2obj, (obj)->ID)

/* Figure object's noexport attribute vs. the current exporter and run
   inhibit if object should not be exported. On GUI, draw the no-export mark
   at x;y */
#define pcb_obj_noexport(info, obj, inhibit) \
do { \
	if (obj->noexport) { \
		if (info->exporting) { \
			if (!obj->noexport_named || (pcb_attribute_get(&obj->Attributes, info->noexport_name) != NULL)) { \
				inhibit; \
			} \
		} \
		else \
			pcb_draw_delay_label_add((pcb_any_obj_t *)obj); \
	} \
} while(0)

#define pcb_obj_noexport_mark(obj, cx, cy) \
do { \
	pcb_coord_t radius = PCB_MM_TO_COORD(0.2); \
	int selected = PCB_FLAG_TEST(PCB_FLAG_SELECTED, obj); \
	pcb_render->set_color(pcb_draw_out.fgGC, selected ? &conf_core.appearance.color.selected : &conf_core.appearance.color.subc); \
	pcb_hid_set_line_cap(pcb_draw_out.fgGC, pcb_cap_round); \
	pcb_hid_set_line_width(pcb_draw_out.fgGC, -2); \
	pcb_render->draw_line(pcb_draw_out.fgGC, cx-radius, cy-radius, cx+radius, cy+radius); \
	pcb_render->draw_line(pcb_draw_out.fgGC, cx+radius, cy-radius, cx-radius, cy+radius); \
} while(0)

#define pcb_hidden_floater(obj) (conf_core.editor.hide_names && PCB_FLAG_TEST(PCB_FLAG_FLOATER, (obj)))

/* Returns whether a subc part object is editable (not under the subc lock) */
#define pcb_subc_part_editable(pcb, obj) \
	(((pcb)->loose_subc) || ((obj)->term != NULL) || PCB_FLAG_TEST(PCB_FLAG_FLOATER, (obj)))

/* set const char *dst to a color, depending on the bound layer type:
   top silk and copper get the color of the first crresponding layer
   from current PCB, the rest get the far-side color;
   set the selected color if sel is true */
#define PCB_OBJ_COLOR_ON_BOUND_LAYER(dst, layer, sel) \
do { \
	if (layer->meta.bound.type & PCB_LYT_TOP) { \
		pcb_layer_id_t lid = -1; \
		pcb_layergrp_t *g; \
		pcb_layergrp_id_t grp = -1; \
		if (layer->meta.bound.type & PCB_LYT_SILK) { \
			if (sel) {\
				dst = &conf_core.appearance.color.selected; \
			} \
			else {\
				if (layer != NULL) { \
					pcb_layer_t *ly = pcb_layer_get_real(layer); \
					if (ly != NULL) \
						dst = &ly->meta.real.color; \
					else \
						dst = &conf_core.appearance.color.element; \
				} \
				else \
					dst = &conf_core.appearance.color.element; \
			} \
			break; \
		} \
		else if (layer->meta.bound.type & PCB_LYT_COPPER) \
			grp = pcb_layergrp_get_top_copper(); \
		g = pcb_get_layergrp(PCB, grp); \
		if ((g != NULL) && (g->len > 0)) \
			lid = g->lid[0]; \
		if ((lid >= 0) && (lid <= PCB_MAX_LAYER)) { \
			if (sel) \
				dst = &conf_core.appearance.color.selected; \
			else {\
				if (layer != NULL) { \
					pcb_layer_t *ly = pcb_layer_get_real(layer); \
					if (ly != NULL) \
						dst = &ly->meta.real.color; \
					else \
						dst = &conf_core.appearance.color.layer[lid]; \
				} \
				else \
					dst = &conf_core.appearance.color.layer[lid]; \
			} \
			break; \
		} \
	} \
	if (sel) \
		dst = &conf_core.appearance.color.selected; \
	else \
		dst = &conf_core.appearance.color.invisible_objects; \
} while(0)

/* check if an object has clearance to polygon */
#define PCB_POLY_HAS_CLEARANCE(ply) \
	(PCB_FLAG_TEST(PCB_FLAG_CLEARPOLYPOLY, (ply)) && ((ply)->Clearance != 0))

#define PCB_NONPOLY_HAS_CLEARANCE(obj) \
	(PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, (obj)) && ((obj)->Clearance != 0))

#define PCB_OBJ_HAS_CLEARANCE(obj) \
	( \
		((obj)->type == PCB_OBJ_POLY) ? \
		PCB_POLY_HAS_CLEARANCE(obj) : PCB_NONPOLY_HAS_CLEARANCE(obj) \
	)

#define PCB_HAS_COLOROVERRIDE(obj) ((obj)->override_color != NULL)

/* a pointer is created from index addressing because the base pointer
 * may change when new memory is allocated;
 *
 * all data is relative to an objects name 'top' which can be either
 * PCB or PasteBuffer */
#define PCB_END_LOOP  }} while (0)
#define PCB_ENDALL_LOOP }} while (0); }} while(0)

#endif
