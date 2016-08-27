/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design - pcb-rnd
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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

#include "global.h"
#include <genht/htsp.h>

typedef enum {
	PCB_PROPT_invalid,
	PCB_PROPT_STRING,
	PCB_PROPT_COORD,
	PCB_PROPT_ANGLE,
	PCB_PROPT_INT,
	PCB_PROPT_max
} pcb_prop_type_t;

typedef union {
	const char *string;
	Coord coord;
	Angle angle;
	int i;
} pcb_propval_t;

typedef pcb_propval_t htprop_key_t;
typedef unsigned long int htprop_value_t;
#define HT(x) htprop_ ## x
#include <genht/ht.h>
#undef HT

typedef struct {
	pcb_prop_type_t type;
	htprop_t values;
	unsigned core:1;  /* 1 if it is a core property */
} pcb_props_t;

/* A property list (props) is a string->pcb_props_t. Each entry is a named
   property with a value that's a type and a value hash (vhash). vhash's
   key is each value that the property ever took, and vhash's value is an
   integer value of how many times the given property is taken */
htsp_t *pcb_props_init(void);
void pcb_props_uninit(htsp_t *props);

/* Add a value of a named property; if the value is already known, its counter
   is increased. If propname didn't exist, create it. Returns NULL on error.
   Error conditions:
    - invalid type
    - mismatching type for the property (all values of a given property must be the same)
*/
pcb_props_t *pcb_props_add(htsp_t *props, const char *propname, pcb_prop_type_t type, pcb_propval_t val);

/* Return the type name of a property type or NULL on error. */
const char *pcb_props_type_name(pcb_prop_type_t type);

