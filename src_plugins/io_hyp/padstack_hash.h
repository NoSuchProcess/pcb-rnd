/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 */

#include <genht/htpp.h>
#include "obj_pad.h"
#include "obj_pinvia.h"

typedef struct pcb_pshash_hash_s {
	int cnt;
	htpp_t ht;
} pcb_pshash_hash_t;


void pcb_pshash_init(pcb_pshash_hash_t *psh);
void pcb_pshash_uninit(pcb_pshash_hash_t *psh);


/* If new_item != NULL: add a pin or pad in the hash, set *new_item to 1 or 0
   depending on whether it's a new pin/pad in the hash
   If new_item == NULL: look up existing pin or pad.
   return unique name or NULL if not found (or can't be added)
   */
const char *pcb_pshash_pad(pcb_pshash_hash_t *psh, pcb_pad_t *pad, int *new_item);
const char *pcb_pshash_pin(pcb_pshash_hash_t *psh, pcb_pin_t *pin, int *new_item);
