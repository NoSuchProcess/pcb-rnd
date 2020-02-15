/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

/* Drawing primitive: custom graphics (e.g. pixmap) */

#ifndef PCB_OBJ_GFX_H
#define PCB_OBJ_GFX_H

#include <genlist/gendlist.h>
#include <librnd/core/global_typedefs.h>
#include "obj_common.h"

struct pcb_gfx_s {       /* holds information about gfxs */
	PCB_ANY_PRIMITIVE_FIELDS;
	pcb_coord_t cx, cy;            /* center coordinates */
	pcb_coord_t sx, sy;            /* size x and y on board (net box size before rotation) */
	pcb_angle_t rot;
	unsigned int xmirror:1, ymirror:1;

	pcb_pixmap_t *pxm_neutral;     /* graphics is a pixmap, if not NULL - in neutral scale/rot */
	pcb_pixmap_t *pxm_xformed;     /* transformed version from the cache */

	/* calculated/cached fields */
	pcb_coord_t cox[4], coy[4];    /* the 4 corners */
	gdl_elem_t link;               /* an gfx is in a list on a layer */
};

/*** Memory ***/
pcb_gfx_t *pcb_gfx_alloc(pcb_layer_t *);
pcb_gfx_t *pcb_gfx_alloc_id(pcb_layer_t *layer, long int id);
void pcb_gfx_free(pcb_gfx_t *data);

pcb_gfx_t *pcb_gfx_new(pcb_layer_t *layer, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t sx, pcb_coord_t sy, pcb_angle_t rot, pcb_flag_t Flags);
pcb_gfx_t *pcb_gfx_dup(pcb_layer_t *dst, pcb_gfx_t *src);
pcb_gfx_t *pcb_gfx_dup_at(pcb_layer_t *dst, pcb_gfx_t *src, pcb_coord_t dx, pcb_coord_t dy);
void *pcb_gfx_destroy(pcb_layer_t *Layer, pcb_gfx_t *gfx);

void pcb_gfx_reg(pcb_layer_t *layer, pcb_gfx_t *gfx);
void pcb_gfx_unreg(pcb_gfx_t *gfx);

/* Add objects without creating them or making any "sanity modifications" to them */
void pcb_add_gfx_on_layer(pcb_layer_t *layer, pcb_gfx_t *gfx);


/*** Utility ***/
void pcb_gfx_update(pcb_gfx_t *gfx); /* update corner cache: call this after any geometry change */
void pcb_gfx_bbox(pcb_gfx_t *gfx);
void pcb_gfx_rotate90(pcb_gfx_t *gfx, pcb_coord_t X, pcb_coord_t Y, unsigned Number);
void pcb_gfx_rotate(pcb_layer_t *layer, pcb_gfx_t *gfx, pcb_coord_t X, pcb_coord_t Y, double cosa, double sina, pcb_angle_t angle);
void pcb_gfx_mirror(pcb_gfx_t *gfx, pcb_coord_t y_offs, pcb_bool undoable);
void pcb_gfx_flip_side(pcb_layer_t *layer, pcb_gfx_t *gfx);
void pcb_gfx_scale(pcb_gfx_t *gfx, double sx, double sy, double sth);
void pcb_gfx_chg_geo(pcb_gfx_t *gfx, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t sx, pcb_coord_t sy,  pcb_angle_t rot, pcb_bool undoable);

/* assings pxm to gfx and free pxm (if needed) */
void pcb_gfx_set_pixmap_free(pcb_gfx_t *gfx, pcb_pixmap_t *pxm, pcb_bool undoable);

/*** hash and eq ***/
int pcb_gfx_eq(const pcb_host_trans_t *tr1, const pcb_gfx_t *g1, const pcb_host_trans_t *tr2, const pcb_gfx_t *g2);
unsigned int pcb_gfx_hash(const pcb_host_trans_t *tr, const pcb_gfx_t *g);

/* un-administrate a gfx; call before changing geometry */
void pcb_gfx_pre(pcb_gfx_t *gfx);

/* re-administrate a gfx; call after changing geometry */
void pcb_gfx_post(pcb_gfx_t *gfx);

#define PCB_GFX_LOOP(element) do {                                      \
  pcb_gfx_t *gfx;                                                   \
  gdl_iterator_t __it__;                                            \
  linelist_foreach(&(element)->Gfx, &__it__, gfx) {

#define PCB_GFX_ALL_LOOP(top) do { \
	pcb_cardinal_t l; \
	pcb_layer_t *layer = (top)->Layer; \
	for (l =0; l < ((top)->LayerN > 0 ? (top)->LayerN : PCB->Data->LayerN); l++, layer++) \
	{ \
		PCB_GFX_LOOP(layer)

#define PCB_GFX_COPPER_LOOP(top) do { \
	pcb_cardinal_t l; \
	pcb_layer_t *layer = (top)->Layer; \
	for (l =0; l < ((top)->LayerN > 0 ? (top)->LayerN : PCB->Data->LayerN); l++, layer++) \
	{ \
		if (!(pcb_layer_flags(PCB, l) & PCB_LYT_COPPER)) continue; \
		PCB_GFX_LOOP(layer)

#define PCB_GFX_SILK_LOOP(top) do { \
	pcb_cardinal_t l; \
	pcb_layer_t *layer = (top)->Layer; \
	for (l = 0; l < ((top)->LayerN > 0 ? (top)->LayerN : PCB->Data->LayerN); l++, layer++) \
	{ \
		if (!(pcb_layer_flags(PCB, l) & PCB_LYT_SILK)) continue; \
		PCB_GFX_LOOP(layer)

#define PCB_GFX_VISIBLE_LOOP(top) do { \
	pcb_cardinal_t l; \
	pcb_layer_t *layer = (top)->Layer; \
	for (l = 0; l < ((top)->LayerN > 0 ? (top)->LayerN : PCB->Data->LayerN); l++, layer++) \
	{ \
		if (layer->meta.real.vis) \
			PCB_GFX_LOOP(layer)

#endif
