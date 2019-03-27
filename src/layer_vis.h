/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,2004,2006 Thomas Nau
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas (pcb-rnd extensions)
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

/* Layer visibility logics (affects the GUI and some exporters) */

#ifndef PCB_LAYER_VIS_H
#define PCB_LAYER_VIS_H

#include "layer.h"

/* changes the visibility of all layers in a group; returns the number of
   changed layers; on should be 0 or 1 for setting the state or -1 for toggling it */
int pcb_layervis_change_group_vis(pcb_layer_id_t Layer, int On, pcb_bool ChangeStackOrder);

/* resets the layer visibility stack setting */
void pcb_layervis_reset_stack(void);

/* saves the layerstack setting */
void pcb_layervis_save_stack(void);

/* restores the layerstack setting */
void pcb_layervis_restore_stack(void);

/* Return the last used layer (or if none, any layer) that matches target type */
pcb_layer_id_t pcb_layer_vis_last_lyt(pcb_layer_type_t target);

/* Use this to temporarily enable all layers, so that they can be
   exported even if they're not currently visible.  save_array must be
   PCB_MAX_LAYER+2 big. */
void pcb_hid_save_and_show_layer_ons(int *save_array);

/* Use this to restore them.  */
void pcb_hid_restore_layer_ons(int *save_array);

/* (un)init config watches and events to keep layers in sync */
void pcb_layer_vis_init(void);
void pcb_layer_vis_uninit(void);

/* Open/close, on/off all layers and groups; does NOT generate an event */
void pcb_layer_vis_change_all(pcb_board_t *pcb, pcb_bool_op_t open, pcb_bool_op_t vis);

#endif
