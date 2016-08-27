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

typedef enum {
	PCB_PROPT_invalid,
	PCB_PROPT_STRING,
	PCB_PROPT_COORD,
	PCB_PROPT_ANGLE,
	PCB_PROPT_INT
} pcb_obj_prop_type_t;

typedef struct {
	pcb_obj_prop_type_t type;
	union {
		const char *string;
		Coord coord;
		Angle angle;
		int i;
	} value;
} pcb_obj_prop_t;


typedef char *htprop_key_t;
typedef pcb_obj_prop_t htprop_value_t;
#define HT(x) htprop_ ## x
#include <genht/ht.h>
#undef HT
