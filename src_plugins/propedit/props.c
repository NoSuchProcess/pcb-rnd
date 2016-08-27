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
#include "props.h"
/*#define HT_INVALID_VALUE ((pcb_propval_t){PCB_PROPT_invalid, {0}})*/
#define HT(x) htprop_ ## x
#include <genht/ht.c>
#undef HT

/* Type names in text */
static const char *type_names[] = { "invalid", "string", "coord", "angle", "int" };

/* A hash function for each known type */
static unsigned int prophash_string(pcb_propval_t key) { return strhash((char *)key.string); }
static unsigned int prophash_coord(pcb_propval_t key)  { return longhash(key.coord); }
static unsigned int prophash_angle(pcb_propval_t key)  { return longhash(key.angle); }
static unsigned int prophash_int(pcb_propval_t key)    { return longhash(key.i); }

typedef unsigned int (*prophash_ft)(pcb_propval_t key);
static prophash_ft prophash[PCB_PROPT_max] = {
	NULL, prophash_string, prophash_coord, prophash_angle, prophash_int
};

/* A keyeq function for each known type */
static int propkeyeq_string(pcb_propval_t a, pcb_propval_t b) { return (strcmp(a.string, b.string) == 0); }
static int propkeyeq_coord(pcb_propval_t a, pcb_propval_t b)  { return a.coord == b.coord; }
static int propkeyeq_angle(pcb_propval_t a, pcb_propval_t b)  { return a.angle == b.angle; }
static int propkeyeq_int(pcb_propval_t a, pcb_propval_t b)    { return a.i == b.i; }

typedef int (*propkeyeq_ft)(pcb_propval_t a, pcb_propval_t b);
static propkeyeq_ft propkeyeq[PCB_PROPT_max] = {
	NULL, propkeyeq_string, propkeyeq_coord, propkeyeq_angle, propkeyeq_int
};


/* Init & uninit */
htsp_t *pcb_props_init(void)
{
	return htsp_alloc(strhash, strkeyeq);
}

void pcb_props_uninit(htsp_t *props)
{
#warning TODO
}

/* Store a new value */
pcb_props_t *pcb_props_add(htsp_t *props, const char *propname, pcb_prop_type_t type, pcb_propval_t val)
{
	pcb_props_t *p;
	htprop_entry_t *e;

	if ((type <= PCB_PROPT_invalid) || (type >= PCB_PROPT_max))
		return NULL;

	/* look up or create the value list (p) associated with the property name */
	p = htsp_get(props, (char *)propname);
	if (p == NULL) {
		p = malloc(sizeof(pcb_props_t));
		p->type = type;
		htprop_init(&p->values, prophash[type], propkeyeq[type]);
		htsp_set(props, (char *)propname, p);
	}
	else {
		if (type != p->type)
			return NULL;
	}

	/* Check if we already have this value */
	e = htprop_getentry(&p->values, val);
	if (e == NULL)
		htprop_set(&p->values, val, 1);
	else
		e->value++;

	return p;
}


const char *pcb_props_type_name(pcb_prop_type_t type)
{
	if (((int)type < 0) || (type >= PCB_PROPT_max))
		return NULL;

	return type_names[type];
}
