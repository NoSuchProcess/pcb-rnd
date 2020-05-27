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
	rnd_coord_t cx, cy;            /* center coordinates */
	rnd_coord_t sx, sy;            /* size x and y on board (net box size before rotation) */
	rnd_angle_t rot;
	unsigned int xmirror:1, ymirror:1;

	long int pxm_id;               /* preferred id for the pixmap (to preserve it on save/load) */

	rnd_pixmap_t *pxm_neutral;     /* graphics is a pixmap, if not NULL - in neutral scale/rot */
	rnd_pixmap_t *pxm_xformed;     /* transformed version from the cache */

	/* calculated/cached fields */
	unsigned int render_under:1;   /* render under layer objects (default is 0=above) */
	rnd_point_t corner[4];         /* the 4 corners - has to be point so search.c can return it for the crosshair to edit */
	gdl_elem_t link;               /* an gfx is in a list on a layer */
};

/*** Memory ***/
pcb_gfx_t *pcb_gfx_alloc(pcb_layer_t *);
pcb_gfx_t *pcb_gfx_alloc_id(pcb_layer_t *layer, long int id);
void pcb_gfx_free(pcb_gfx_t *data);

pcb_gfx_t *pcb_gfx_new(pcb_layer_t *layer, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t sx, rnd_coord_t sy, rnd_angle_t rot, pcb_flag_t Flags);
pcb_gfx_t *pcb_gfx_dup(pcb_layer_t *dst, pcb_gfx_t *src);
pcb_gfx_t *pcb_gfx_dup_at(pcb_layer_t *dst, pcb_gfx_t *src, rnd_coord_t dx, rnd_coord_t dy);
void *pcb_gfx_destroy(pcb_layer_t *Layer, pcb_gfx_t *gfx);

void pcb_gfx_reg(pcb_layer_t *layer, pcb_gfx_t *gfx);
void pcb_gfx_unreg(pcb_gfx_t *gfx);

/* Add objects without creating them or making any "sanity modifications" to them */
void pcb_add_gfx_on_layer(pcb_layer_t *layer, pcb_gfx_t *gfx);


/*** Utility ***/
void pcb_gfx_update(pcb_gfx_t *gfx); /* update corner cache: call this after any geometry change */
void pcb_gfx_bbox(pcb_gfx_t *gfx);
void pcb_gfx_rotate90(pcb_gfx_t *gfx, rnd_coord_t X, rnd_coord_t Y, unsigned Number);
void pcb_gfx_rotate(pcb_layer_t *layer, pcb_gfx_t *gfx, rnd_coord_t X, rnd_coord_t Y, double cosa, double sina, rnd_angle_t angle);
void pcb_gfx_mirror(pcb_gfx_t *gfx, rnd_coord_t y_offs, rnd_bool undoable);
void pcb_gfx_flip_side(pcb_layer_t *layer, pcb_gfx_t *gfx);
void pcb_gfx_scale(pcb_gfx_t *gfx, double sx, double sy, double sth);
void pcb_gfx_chg_geo(pcb_gfx_t *gfx, rnd_coord_t cx, rnd_coord_t cy, rnd_coord_t sx, rnd_coord_t sy,  rnd_angle_t rot, rnd_bool undoable);

/* assings pxm to gfx and free pxm (if needed) */
void pcb_gfx_set_pixmap_free(pcb_gfx_t *gfx, rnd_pixmap_t *pxm, rnd_bool undoable);

/* same but dups pxm so it is never free'd */
void pcb_gfx_set_pixmap_dup(pcb_gfx_t *gfx, const rnd_pixmap_t *pxm, rnd_bool undoable);

/* Request a resize by moving one of the corners relative, by dx;dy */
void pcb_gfx_resize_move_corner(pcb_gfx_t *gfx, int corn_idx, rnd_coord_t dx, rnd_coord_t dy, int undoable);

/* Change the transparent pixel value on the underlying pixmap */
int gfx_set_transparent(pcb_gfx_t *gfx, unsigned char tr, unsigned char tg, unsigned char tb, int undoable);
int gfx_set_transparent_by_coord(pcb_gfx_t *gfx, long px, long py);
void gfx_set_transparent_gui(pcb_gfx_t *gfx);

/* Resize the image by a pair of screen coords and an expected size; if allow_x
   or allow_y is zero, ignore the component in that direction (intrepreted on
   the rotated image) */
int gfx_set_resize_by_pixel_dist(pcb_gfx_t *gfx, long pdx, long pdy, rnd_coord_t len, rnd_bool allow_x, rnd_bool allow_y, rnd_bool undoable);
void gfx_set_resize_gui(rnd_hidlib_t *hl, pcb_gfx_t *gfx, rnd_bool allow_x, rnd_bool allow_y);


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
	rnd_cardinal_t l; \
	pcb_layer_t *layer = (top)->Layer; \
	for (l =0; l < ((top)->LayerN > 0 ? (top)->LayerN : PCB->Data->LayerN); l++, layer++) \
	{ \
		PCB_GFX_LOOP(layer)

#define PCB_GFX_COPPER_LOOP(top) do { \
	rnd_cardinal_t l; \
	pcb_layer_t *layer = (top)->Layer; \
	for (l =0; l < ((top)->LayerN > 0 ? (top)->LayerN : PCB->Data->LayerN); l++, layer++) \
	{ \
		if (!(pcb_layer_flags(PCB, l) & PCB_LYT_COPPER)) continue; \
		PCB_GFX_LOOP(layer)

#define PCB_GFX_SILK_LOOP(top) do { \
	rnd_cardinal_t l; \
	pcb_layer_t *layer = (top)->Layer; \
	for (l = 0; l < ((top)->LayerN > 0 ? (top)->LayerN : PCB->Data->LayerN); l++, layer++) \
	{ \
		if (!(pcb_layer_flags(PCB, l) & PCB_LYT_SILK)) continue; \
		PCB_GFX_LOOP(layer)

#define PCB_GFX_VISIBLE_LOOP(top) do { \
	rnd_cardinal_t l; \
	pcb_layer_t *layer = (top)->Layer; \
	for (l = 0; l < ((top)->LayerN > 0 ? (top)->LayerN : PCB->Data->LayerN); l++, layer++) \
	{ \
		if (layer->meta.real.vis) \
			PCB_GFX_LOOP(layer)

#endif
