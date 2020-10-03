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
	PCB_OBJ_GFX        = 0x0000080,

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
	PCB_OBJ_FLOATER    = 0x2000000,
	PCB_OBJ_GFX_POINT  = 0x4000000
} pcb_vobjtype_t;


/* combinations, groups, masks of pcb_objtype_t */
typedef enum pcb_objmask_e {
	/* lists */
	PCB_OBJ_CLASS_PIN  = (PCB_OBJ_PSTK | PCB_OBJ_SUBC_PART),
	PCB_OBJ_CLASS_TERM = (PCB_OBJ_CLASS_PIN | PCB_OBJ_SUBC_PART | PCB_OBJ_LINE | PCB_OBJ_ARC | PCB_OBJ_POLY | PCB_OBJ_TEXT),
	PCB_OBJ_CLASS_LAYER = (PCB_OBJ_ARC | PCB_OBJ_LINE | PCB_OBJ_POLY | PCB_OBJ_TEXT | PCB_OBJ_GFX),
	PCB_OBJ_CLASS_GLOBAL = (PCB_OBJ_SUBC | PCB_OBJ_PSTK),

	/* masks */
	PCB_OBJ_CLASS_REAL = 0x0000FFF, /* global and on-layer objects (but not abstract things like layers) */
	PCB_OBJ_CLASS_HOST = 0x00FF000, /* host types: layers, boards, nets */
	PCB_OBJ_CLASS_MASK = 0xFF00000, /* for virtual searches */
	PCB_OBJ_CLASS_OBJ  = 0x0000000, /* anything with common object fields (pcb_any_obj_t) */
	PCB_OBJ_ANY        = 0xFFFFFFF
} pcb_objmask_t;


/* point and box type - they are so common everything depends on them */
struct rnd_point_s {    /* a line/polygon point */
	rnd_coord_t X, Y, X2, Y2;   /* so Point type can be cast as rnd_box_t */
	long int ID;
};

typedef double pcb_xform_mx_t[9];
#define PCB_XFORM_MX_IDENT {1,0,0,   0,1,0,   0,0,1}

struct rnd_xform_s {   /* generic object transformation; all-zero means no transformation */
	rnd_coord_t bloat;           /* if non-zero, bloat (positive) or shrink (negative) by this value */

	unsigned layer_faded:1;      /* draw layer colors faded */
	unsigned omit_overlay:1;     /* do not draw overlays (which are useful on screen but normally omitted on exports, except if --as-shown is specified */
	unsigned partial_export:1;   /* 1 if only objects with the EXPORTSEL flag should be drawn */
	unsigned no_slot_in_nonmech:1; /* if 1, do not draw slots on non-mechanical layers (e.g. "no slot in copper") */
	unsigned wireframe:1;        /* when 1, draw wireframe contours instead of solid objects */
	unsigned thin_draw:1;        /* when 1, draw thin centerline instead of solid objects (implies thin_draw_poly) */
	unsigned thin_draw_poly:1;   /* when 1, draw thin countour instead of solid polygons */
	unsigned check_planes:1;     /* when 1, draw polygons only */
	unsigned flag_color:1;       /* when zero, ignore colors that would be derived from object flags (i.e. selection, warn, found) */
	unsigned hide_floaters:1;    /* when 1 omit drawing floaters (typically refdes text on silk) */
	unsigned show_solder_side:1;    /* GUI */
	unsigned invis_other_groups:1;  /* GUI */
	unsigned black_current_group:1; /* GUI */
	/* WARNING: After adding new fields, make sure to update pcb_xform_add() and pcb_xform_is_nop() below */
};

#define pcb_xform_clear(dst)      memset(dst, 0, sizeof(rnd_xform_t))
#define pcb_xform_copy(dst, src)  memcpy(dst, src, sizeof(rnd_xform_t))
#define pcb_xform_add(dst, src) \
	do { \
		rnd_xform_t *__dst__ = dst; \
		const rnd_xform_t *__src__ = src; \
		__dst__->bloat += __src__->bloat; \
		__dst__->layer_faded |= __src__->layer_faded; \
		__dst__->omit_overlay |= __src__->omit_overlay; \
		__dst__->partial_export |= __src__->partial_export; \
		__dst__->no_slot_in_nonmech |= __src__->no_slot_in_nonmech; \
		__dst__->wireframe |= __src__->wireframe; \
		__dst__->thin_draw |= __src__->thin_draw; \
		__dst__->thin_draw_poly |= __src__->thin_draw_poly; \
		__dst__->check_planes |= __src__->check_planes; \
		__dst__->flag_color |= __src__->flag_color; \
		__dst__->hide_floaters |= __src__->hide_floaters; \
		__dst__->show_solder_side |= __src__->show_solder_side; \
		__dst__->invis_other_groups |= __src__->invis_other_groups; \
		__dst__->black_current_group |= __src__->black_current_group; \
	} while(0)
#define pcb_xform_is_nop(src) (\
	((src)->bloat == 0) && \
	((src)->layer_faded == 0) && \
	((src)->omit_overlay == 0) && ((src)->partial_export == 0) && \
	((src)->no_slot_in_nonmech == 0) && \
	((src)->wireframe == 0) && \
	((src)->thin_draw == 0) && \
	((src)->thin_draw_poly == 0) && \
	((src)->check_planes == 0) && \
	((src)->flag_color == 0) && \
	((src)->hide_floaters == 0) && \
	((src)->show_solder_side == 0) && \
	((src)->invis_other_groups == 0) && \
	((src)->black_current_group == 0) \
	)

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


int pcb_obj_get_bbox_naked(int Type, void *Ptr1, void *Ptr2, void *Ptr3, rnd_box_t *res);

/* Host transformations: typically the transformations an object of a subc
   inherits from the subc */
typedef struct pcb_host_trans_s {
	rnd_coord_t ox, oy;
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


/* rnd_true during file loads, for example to allow overlapping vias.
   rnd_false otherwise, to stop the user from doing normally dangerous
   things.  */
void pcb_create_be_lenient(rnd_bool);
extern rnd_bool pcb_create_being_lenient;

void pcb_create_ID_bump(int min_id);
void pcb_create_ID_reset(void);
long int pcb_create_ID_get(void);

/* Copy src attributes into obj's attributes, overwriting anything. If src_obj
   is not NULL, it is the source object - be smart about extobj and other
   attribute side effects (some of those will not be copied or will be
   changed); src_obj should be the objec src attributes are coming from */
void pcb_obj_add_attribs(pcb_any_obj_t *obj, const pcb_attribute_list_t *src, pcb_any_obj_t *src_obj);

/* ---------------------------------------------------------------------------
 * Do not change the following definitions even if they're not very
 * nice.  It allows us to have functions act on these "base types" and
 * not need to know what kind of actual object they're working on.
 */

/* Any object that ends up in common use as pcb_any_obj_t, MUST be defined
   using this as the first fieldsusing the following macros.  */
#define PCB_ANY_OBJ_FIELDS \
	rnd_box_t            BoundingBox; \
	long int             ID; \
	pcb_flag_t           Flags; \
	pcb_objtype_t        type; \
	pcb_parenttype_t     parent_type; \
	pcb_parent_t         parent; \
	rnd_box_t            bbox_naked; \
	unsigned             ind_onpoint:1; \
	unsigned             ind_editpoint:1; \
	pcb_attribute_list_t Attributes \

#define PCB_ANY_PRIMITIVE_FIELDS \
	PCB_ANY_OBJ_FIELDS; \
	const char           *term, *extobj_role; \
	void                 *ratconn; \
	unsigned char        thermal; \
	unsigned char        intconn, intnoconn; \
	unsigned int         noexport:1; \
	unsigned int         noexport_named:1; \
	unsigned int         extobj_editing:1; \
	rnd_color_t          *override_color

/* Lines, pads, and rats all use this so they can be cross-cast.  */
#define PCB_ANYLINEFIELDS \
	PCB_ANY_PRIMITIVE_FIELDS; \
	rnd_coord_t Thickness, Clearance; \
	rnd_point_t Point1, Point2

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
void pcb_obj_center(const pcb_any_obj_t *obj, rnd_coord_t *x, rnd_coord_t *y);

/* Return the clearance value of object on the specified layer (in
   case of padstack - in case of other objects layer is ignored) */
rnd_coord_t pcb_obj_clearance_at(pcb_board_t *pcb, const pcb_any_obj_t *o, pcb_layer_t *at);

unsigned char *pcb_obj_common_get_thermal(pcb_any_obj_t *obj, unsigned long lid, rnd_bool_t alloc);

/* Update cached attributes (->term) */
void pcb_obj_attrib_post_change(pcb_attribute_list_t *list, const char *name, const char *value);

/* Returns the first invalid character of an ID (terminal, refdes) or NULL */
const char *pcb_obj_id_invalid(const char *id);

/* Fix an ID in place (replace anything invalid with '_'); returns id */
char *pcb_obj_id_fix(char *id);

/* switch() on o->type and call the right implementation */
void pcb_obj_pre(pcb_any_obj_t *o);
void pcb_obj_post(pcb_any_obj_t *o);

/* change the ID of an object already registered in the ID hash */
void pcb_obj_change_id(pcb_any_obj_t *o, long int new_id);

/* sets the bounding box of a point */
void pcb_set_point_bounding_box(rnd_point_t *Pnt);

void pcb_obj_common_free(pcb_any_obj_t *o);

/* Determine the size class of a sub-kilobyte object: the largest 2^n that
   is smaller than the size; size must be at least 16. */
#define pcb_size_class(a) ((a) < 32 ? 16 : ((a) < 64 ? 32 : ((a) < 128 ? 64 : ((a) < 256 ? 128 : ((a) < 512 ? 256 : ((a) < 1024 ? 512 : 1024 ))))))

/* Return an object-instance-unique integer value */
RND_INLINE size_t pcb_obj_iid(pcb_any_obj_t *obj)
{
	return (size_t)obj / pcb_size_class(sizeof(pcb_any_obj_t));
}

/* Recalculate the bounding box of the object */
void pcb_obj_update_bbox(pcb_board_t *pcb, pcb_any_obj_t *obj);


/* Assuming clearance is happening (flags), clearance of an object
   in a polygon is the bigger of the obj's ->clearance and the polygon's
   ->enforce_clearance */
#define pcb_obj_clearance(obj, in_poly) \
	(RND_MAX((obj)->Clearance, (in_poly)->enforce_clearance))

#define pcb_obj_clearance_p2(obj, in_poly) \
	(RND_MAX((obj)->Clearance, ((in_poly)->enforce_clearance)*2))

#define pcb_obj_clearance_o05(obj, in_poly) \
	(RND_MAX((obj)->Clearance/2, ((in_poly)->enforce_clearance)))

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
	rnd_coord_t radius = RND_MM_TO_COORD(0.2); \
	int selected = PCB_FLAG_TEST(PCB_FLAG_SELECTED, obj); \
	rnd_render->set_color(pcb_draw_out.fgGC, selected ? &conf_core.appearance.color.selected : &conf_core.appearance.color.subc); \
	rnd_hid_set_line_cap(pcb_draw_out.fgGC, rnd_cap_round); \
	rnd_hid_set_line_width(pcb_draw_out.fgGC, -2); \
	rnd_render->draw_line(pcb_draw_out.fgGC, cx-radius, cy-radius, cx+radius, cy+radius); \
	rnd_render->draw_line(pcb_draw_out.fgGC, cx+radius, cy-radius, cx-radius, cy+radius); \
} while(0)

#define pcb_obj_editpont_setup() \
do { \
	rnd_render->set_color(pcb_draw_out.fgGC, &conf_core.appearance.color.subc); \
	rnd_hid_set_line_cap(pcb_draw_out.fgGC, rnd_cap_round); \
	rnd_hid_set_line_width(pcb_draw_out.fgGC, -2); \
} while(0)

#define pcb_obj_editpont_cross(x, y) \
do { \
	static const rnd_coord_t radius = RND_MM_TO_COORD(0.2); \
	rnd_render->draw_line(pcb_draw_out.fgGC, (x)-radius, (y), (x)+radius, (y)); \
	rnd_render->draw_line(pcb_draw_out.fgGC, (x), (y)-radius, (x), (y)+radius); \
} while(0)


#define pcb_hidden_floater(obj,info) ((info)->xform->hide_floaters &&  PCB_FLAG_TEST(PCB_FLAG_FLOATER, (obj)))
#define pcb_hidden_floater_gui(obj) (conf_core.editor.hide_names &&  PCB_FLAG_TEST(PCB_FLAG_FLOATER, (obj)))
#define pcb_partial_export(obj,info) (((info)->xform != NULL) && (info)->xform->partial_export && (!PCB_FLAG_TEST(PCB_FLAG_EXPORTSEL, (obj))))

/* Returns whether a subc part object is editable (not under the subc lock) */
#define pcb_subc_part_editable(pcb, obj) \
	(((pcb)->loose_subc) || ((obj)->term != NULL) || PCB_FLAG_TEST(PCB_FLAG_FLOATER, (obj)))

/* set const char *dst to a color, depending on the bound layer type:
   top silk and copper get the color of the first crresponding layer
   from current PCB, the rest get the far-side color;
   set the selected color if sel is true.
   NOTE: caller needs to make sure sel is 0 in case of xform->flag_colors == 0 */
#define PCB_OBJ_COLOR_ON_BOUND_LAYER(dst, layer, sel) \
do { \
	if (layer->meta.bound.type & PCB_LYT_TOP) { \
		rnd_layer_id_t lid = -1; \
		pcb_layergrp_t *g; \
		rnd_layergrp_id_t grp = -1; \
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
