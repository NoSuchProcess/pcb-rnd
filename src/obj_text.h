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

/* Drawing primitive: text */

#ifndef PCB_OBJ_TEXT_H
#define PCB_OBJ_TEXT_H

#include "obj_common.h"
#include "font.h"
#include <genvector/gds_char.h>

struct pcb_text_s {
	PCB_ANY_PRIMITIVE_FIELDS;
	int Scale;                    /* text scaling in percent */
	double scale_x, scale_y;      /* optional; x and y direction scale (1 means original size); if 0, use Scale/100 */
	rnd_coord_t X, Y;             /* origin */
	pcb_font_id_t fid;
	char *TextString;             /* string */
	rnd_coord_t thickness;        /* if non-zero, thickness of line/arc within the font */
	rnd_coord_t clearance;        /* clearance around the object in polygon if flags have PCB_FLAG_CLEARLINE */
	double rot;                   /* used when Direction is PCB_TEXT_FREEROT */
	unsigned tight_clearance:1;   /* CACHED from attribute: when true, clearance is calculated to follow the true contour of the text; when false, the old, pre-v7 bbox based clearance is applied */
	unsigned mirror_x:1;          /* CACHED from attribute: when true, mirror X coords (mirror over the Y axis) */
	gdl_elem_t link;              /* a text is in a list of a layer */
};

typedef enum { /* bitfield - order matters for backward compatibility */
	PCB_TXT_MIRROR_NO = 0,
	PCB_TXT_MIRROR_Y = 1, /* change Y coords (mirror over the X axis) */
	PCB_TXT_MIRROR_X = 2  /* change X coords (mirror over the Y axis) */
} pcb_text_mirror_t;

/* These need to be carefully written to avoid overflows, and return
   a Coord type.  */
#define PCB_SCALE_TEXT(COORD,TEXTSCALE) ((rnd_coord_t)rnd_round((COORD) * ((double)(TEXTSCALE) / 100.0)))
#define PCB_UNPCB_SCALE_TEXT(COORD,TEXTSCALE) ((rnd_coord_t)rnd_round((COORD) * (100.0 / (double)(TEXTSCALE))))

/* Determine what the Scale value would have been in the old, Scale only
   setup and store it in scale_out. Useful for compatibility saves. Returns
   0 on succes, -1 on error (with a best effort estimation left in scale_out) */
int pcb_text_old_scale(pcb_text_t *text, int *scale_out);

pcb_text_t *pcb_text_alloc(pcb_layer_t * layer);
pcb_text_t *pcb_text_alloc_id(pcb_layer_t *layer, long int id);
void pcb_text_free(pcb_text_t * data);
pcb_text_t *pcb_text_new(pcb_layer_t *Layer, pcb_font_t *PCBFont, rnd_coord_t X, rnd_coord_t Y, double rot, int Scale, rnd_coord_t thickness, const char *TextString, pcb_flag_t Flags);
pcb_text_t *pcb_text_dup(pcb_layer_t *dst, pcb_text_t *src);
pcb_text_t *pcb_text_dup_at(pcb_layer_t *dst, pcb_text_t *src, rnd_coord_t dx, rnd_coord_t dy);
void *pcb_text_destroy(pcb_layer_t *Layer, pcb_text_t *Text);

/* creates the text object on the layer without rtree or poly clipping administration */
pcb_text_t *pcb_text_new_(pcb_layer_t *Layer, pcb_font_t *PCBFont, rnd_coord_t X, rnd_coord_t Y, double rot, pcb_text_mirror_t mirror, int Scale, double scx, double scy, rnd_coord_t thickness, const char *TextString, pcb_flag_t Flags);

/* Create new text by bounding box: bbw and bbw are the expected bounding box
   width and height, scxy is the expected text width/height ratio; place the
   text by grabbing at anchor point anchx;anchy on the original bbox. The resulting
   text will typically be smaller than the input bounding box, with wither
   bbox width or bbox height matching the original, but text aspect ratio kept
   and anchor point placed at X;Y. */
pcb_text_t *pcb_text_new_by_bbox(pcb_layer_t *Layer, pcb_font_t *PCBFont, rnd_coord_t X, rnd_coord_t Y, rnd_coord_t bbw, rnd_coord_t bbh, rnd_coord_t anchx, rnd_coord_t anchy, double scxy, pcb_text_mirror_t mirror, double rot, rnd_coord_t thickness, const char *TextString, pcb_flag_t Flags);


/* Add objects without creating them or making any "sanity modifications" to them */
void pcb_add_text_on_layer(pcb_layer_t *Layer, pcb_text_t *text, pcb_font_t *PCBFont);

void pcb_text_bbox(pcb_font_t *FontPtr, pcb_text_t *Text);
void pcb_text_rotate90(pcb_text_t *Text, rnd_coord_t X, rnd_coord_t Y, unsigned Number);
void pcb_text_rotate(pcb_text_t *Text, rnd_coord_t X, rnd_coord_t Y, double cosa, double sina, double rotdeg);
void pcb_text_scale(pcb_text_t *text, double sx, double sy, double sth);
void pcb_text_flip_side(pcb_layer_t *layer, pcb_text_t *text, rnd_coord_t y_offs, rnd_bool undoable);
void pcb_text_mirror_coords(pcb_text_t *text, rnd_coord_t y_offs, rnd_bool undoable); /* mirror the coords but do not mirror the text itself (no on-solder) */
void pcb_text_set_font(pcb_text_t *text, pcb_font_id_t fid);
void pcb_text_update(pcb_layer_t *layer, pcb_text_t *text);

void pcb_text_reg(pcb_layer_t *layer, pcb_text_t *text);
void pcb_text_unreg(pcb_text_t *text);

void pcb_text_pre(pcb_text_t *text);
void pcb_text_post(pcb_text_t *text);

/* Before and after a flag change on text, call these; flagbits should
   be the new bits we are changing to; save should be the
   address of a local (void *) cache. The calls make sure bbox and rtree
   administration are done properly */
void pcb_text_flagchg_pre(pcb_text_t *Text, unsigned long flagbits, void **save);
void pcb_text_flagchg_post(pcb_text_t *Text, unsigned long oldflagbits, void **save);

/* Low level draw call for direct rendering on preview */
void pcb_text_draw_string_simple(pcb_font_t *font, const char *string, rnd_coord_t x0, rnd_coord_t y0, double scx, double scy, double rotdeg, pcb_text_mirror_t mirror, rnd_coord_t thickness, int xordraw, rnd_coord_t xordx, rnd_coord_t xordy);

/* Recalculate the bounding box of all dynamic text objects that are
   directly under data - useful e.g. on parent attr change */
void pcb_text_dyn_bbox_update(pcb_data_t *data);

/* Return the old direction value (n*90 deg rotation) for text rotation value.
   Returns false if has a rounding error greater than 0.5 deg */
rnd_bool pcb_text_old_direction(int *dir_out, double rot);

/* hash and eq */
int pcb_text_eq(const pcb_host_trans_t *tr1, const pcb_text_t *t1, const pcb_host_trans_t *tr2, const pcb_text_t *t2);
unsigned int pcb_text_hash(const pcb_host_trans_t *tr, const pcb_text_t *t);

/* Append dyntext fmt rendered from the perspective of obj */
int pcb_append_dyntext(gds_t *dst, const pcb_any_obj_t *obj, const char *fmt);

void pcb_text_init(void);
void pcb_text_uninit(void);

#define pcb_text_move(t,dx,dy)                                       \
	do {                                                               \
		rnd_coord_t __dx__ = (dx), __dy__ = (dy);                        \
		pcb_text_t *__t__ = (t);                                         \
		RND_BOX_MOVE_LOWLEVEL(&((__t__)->BoundingBox), __dx__, __dy__);  \
		RND_BOX_MOVE_LOWLEVEL(&((__t__)->bbox_naked), __dx__, __dy__);  \
		RND_MOVE_POINT((__t__)->X, (__t__)->Y, __dx__, __dy__);                \
	} while(0)

/* Determines if text is actually visible */
#define pcb_text_is_visible(b, l, t)      ((l)->meta.real.vis)

#define PCB_TEXT_LOOP(layer) do {                                       \
  pcb_text_t *text;                                                   \
  gdl_iterator_t __it__;                                            \
  textlist_foreach(&(layer)->Text, &__it__, text) {

#define	PCB_TEXT_ALL_LOOP(top) do {                        \
	rnd_cardinal_t l;                                   \
	pcb_layer_t *layer = (top)->Layer;                  \
	for (l = 0; l < ((top)->LayerN > 0 ? (top)->LayerN : PCB->Data->LayerN); l++, layer++) \
	{                                                   \
		PCB_TEXT_LOOP(layer)

#define PCB_TEXT_COPPER_LOOP(top) do	{		\
	rnd_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	for (l = 0; l < ((top)->LayerN > 0 ? (top)->LayerN : PCB->Data->LayerN); l++, layer++)	\
	{ \
		if (!(pcb_layer_flags(PCB, l) & PCB_LYT_COPPER)) continue; \
		PCB_TEXT_LOOP(layer)

#define PCB_SILK_COPPER_LOOP(top) do	{		\
	rnd_cardinal_t		l;			\
	pcb_layer_t *layer = (top)->Layer;		\
	for (l = 0; l < ((top)->LayerN > 0 ? (top)->LayerN : PCB->Data->LayerN); l++, layer++)	\
	{ \
		if (!(pcb_layer_flags(PCB, l) & PCB_LYT_SILK)) continue; \
		PCB_TEXT_LOOP(layer)

#define PCB_TEXT_VISIBLE_LOOP(board) do {                       \
	rnd_cardinal_t l;                                        \
	pcb_layer_t *layer = (board)->Data->Layer;               \
	for (l = 0; l < ((board)->Data->LayerN > 0 ? (board)->Data->LayerN : PCB->Data->LayerN); l++, layer++)      \
	{                                                        \
		PCB_TEXT_LOOP(layer);                                      \
		if (pcb_text_is_visible((board), layer, text))

#endif
