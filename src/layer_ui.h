/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
 *
 *  This module, layer_ui.c, was written and is Copyright (C) 2016 by
 *  Tibor 'Igor2' Palinkas.
 *  this module is also subject to the GNU GPL as described below
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

#ifndef PCB_LAYER_UI_H
#define PCB_LAYER_UI_H

/* Virtual layers for UI and debug */
#include "layer.h"

#include "genvector/vtp0.h"
extern vtp0_t pcb_uilayers;


/* layer vector:  Elem=pcb_layer_t *; init=0 */
#define GVT(x) vtlayer_ ## x
#define GVT_ELEM_TYPE pcb_layer_t
#define GVT_SIZE_TYPE size_t
#define GVT_DOUBLING_THRS 64
#define GVT_START_SIZE 64
#define GVT_FUNC
#define GVT_SET_NEW_BYTES_TO 0
#include <genvector/genvector_impl.h>
#define GVT_REALLOC(vect, ptr, size)  realloc(ptr, size)
#define GVT_FREE(vect, ptr)           free(ptr)
#include <genvector/genvector_undef.h>

/* list of all UI layers */
extern vtlayer_t pcb_uilayer;


pcb_layer_t *pcb_uilayer_alloc(const char *cookie, const char *name, const char *color);
void pcb_uilayer_free(pcb_layer_t *l);
void pcb_uilayer_free_all_cookie(const char *cookie);
void pcb_uilayer_uninit(void);

pcb_layer_t *pcb_uilayer_get(long ui_ly_id);
long pcb_uilayer_get_id(pcb_layer_t *ly);

#endif
