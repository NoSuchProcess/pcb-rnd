/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* Layer visibility logics (affects the GUI and some exporters) */

#ifndef PCB_LAYER_VIS_H
#define PCB_LAYER_VIS_H

#include "layer.h"

/* Given a string description of a layer visibility stack, adjust the layer
   visibility to correspond */
void LayerStringToLayerStack(const char *layer_string);

/* changes the visibility of all layers in a group; returns the number of
   changed layers */
int ChangeGroupVisibility(int Layer, pcb_bool On, pcb_bool ChangeStackOrder);

/* resets the layer visibility stack setting */
void ResetStackAndVisibility(void);

/* saves the layerstack setting */
void SaveStackAndVisibility(void);

/* restores the layerstack setting */
void RestoreStackAndVisibility(void);

#endif
