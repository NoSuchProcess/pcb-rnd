/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2018 Tibor 'Igor2' Palinkas
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
 */
#include "config.h"
#include "props.h"
#include "propsel.h"
#include "compat_misc.h"
#include "pcb-printf.h"
#include "hid.h"
#include <genht/hash.h>
/*#define HT_INVALID_VALUE ((pcb_propval_t){PCB_PROPT_invalid, {0}})*/
#define HT(x) htprop_ ## x
#include <genht/ht.c>
#undef HT

/* Type names in text */
static const char *type_names[] = { "invalid", "string", "coord", "angle", "int" };

/* A hash function for each known type */
static unsigned int prophash_coord(pcb_propval_t key)  { return longhash(key.coord); }
static unsigned int prophash_angle(pcb_propval_t key)  { return longhash(key.angle); }
static unsigned int prophash_int(pcb_propval_t key)    { return longhash(key.i); }
static unsigned int prophash_string(pcb_propval_t key) { return key.string == NULL ? 0 : strhash(key.string); }

typedef unsigned int (*prophash_ft)(pcb_propval_t key);
static prophash_ft prophash[PCB_PROPT_max] = {
	NULL, prophash_string, prophash_coord, prophash_angle, prophash_int
};

/* A keyeq function for each known type */
static int propkeyeq_coord(pcb_propval_t a, pcb_propval_t b)  { return a.coord == b.coord; }
static int propkeyeq_angle(pcb_propval_t a, pcb_propval_t b)  { return a.angle == b.angle; }
static int propkeyeq_int(pcb_propval_t a, pcb_propval_t b)    { return a.i == b.i; }
static int propkeyeq_string(pcb_propval_t a, pcb_propval_t b)
{
	if ((b.string == NULL) && (a.string == NULL))
		return 1;
	if ((b.string == NULL) || (a.string == NULL))
		return 0;
	return (strcmp(a.string, b.string) == 0);
}


typedef int (*propkeyeq_ft)(pcb_propval_t a, pcb_propval_t b);
static propkeyeq_ft propkeyeq[PCB_PROPT_max] = {
	NULL, propkeyeq_string, propkeyeq_coord, propkeyeq_angle, propkeyeq_int
};


/* Init & uninit */
void pcb_props_init(pcb_propedit_t *ctx, pcb_board_t *pcb)
{
	memset(ctx, 0, sizeof(pcb_propedit_t));
	htsp_init(&ctx->props, strhash, strkeyeq);
	ctx->pcb = pcb;
}

void pcb_props_reset(pcb_propedit_t *ctx)
{
	htsp_entry_t *e;
	for(e = htsp_first(&ctx->props); e != NULL; e = htsp_next(&ctx->props, e)) {
		pcb_props_t *p = e->value;
		htprop_uninit(&p->values);
		free(p);
		free(e->key);
	}
	htsp_clear(&ctx->props);
}


void pcb_props_uninit(pcb_propedit_t *ctx)
{
	pcb_props_reset(ctx);
	TODO("clear the vectors")
	memset(ctx, 0, sizeof(pcb_propedit_t));
}

/* Retrieve values for a prop */
pcb_props_t *pcb_props_get(pcb_propedit_t *ctx, const char *propname)
{
	if (propname == NULL)
		return NULL;
	return htsp_get(&ctx->props, propname);
}

/* Store a new value */
pcb_props_t *pcb_props_add(pcb_propedit_t *ctx, const char *propname, pcb_prop_type_t type, pcb_propval_t val)
{
	pcb_props_t *p;
	htprop_entry_t *e;

	if ((type <= PCB_PROPT_invalid) || (type >= PCB_PROPT_max))
		return NULL;

	/* look up or create the value list (p) associated with the property name */
	p = htsp_get(&ctx->props, propname);
	if (p == NULL) {
		p = malloc(sizeof(pcb_props_t));
		p->type = type;
		htprop_init(&p->values, prophash[type], propkeyeq[type]);
		htsp_set(&ctx->props, pcb_strdup(propname), p);
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

#define STAT(val, field, cnt) \
do { \
	if (val.field < minp.field) minp = val; \
	if (val.field > maxp.field) maxp = val; \
	avgp.field += val.field * cnt; \
} while(0)

int pcb_props_stat(pcb_propedit_t *ctx, pcb_props_t *p, pcb_propval_t *most_common, pcb_propval_t *min, pcb_propval_t *max, pcb_propval_t *avg)
{
	htprop_entry_t *e;
	pcb_propval_t bestp, minp, maxp, avgp;
	unsigned long best = 0, num_vals = 0;

	if (most_common != NULL)
		most_common->string = NULL;
	if (min != NULL)
		min->string = NULL;
	if (max != NULL)
		max->string = NULL;
	if (avg != NULL)
		avg->string = NULL;

	if ((p->type == PCB_PROPT_STRING) && ((min != NULL) || (max != NULL) || (avg != NULL)))
		return -1;

	/* set up internal avg, min, max */
	memset(&avgp, 0, sizeof(avgp));
	switch(p->type) {
		case PCB_PROPT_invalid: break;
		case PCB_PROPT_max: break;
		case PCB_PROPT_STRING: break;
		case PCB_PROPT_COORD:  minp.coord = COORD_MAX; maxp.coord = -minp.coord;  break;
		case PCB_PROPT_ANGLE:  minp.angle = 100000;    maxp.angle = -minp.angle; break;
		case PCB_PROPT_INT:    minp.i     = INT_MAX;   maxp.i = -minp.i; break;
	}

	/* walk through all known values */
	for (e = htprop_first(&p->values); e; e = htprop_next(&p->values, e)) {
		if (e->value > best) {
			best = e->value;
			bestp = e->key;
		}
		num_vals += e->value;
		switch(p->type) {
			case PCB_PROPT_invalid: break;
			case PCB_PROPT_max: break;
			case PCB_PROPT_STRING: break;
			case PCB_PROPT_COORD:  STAT(e->key, coord, e->value); break;
			case PCB_PROPT_ANGLE:  STAT(e->key, angle, e->value); break;
			case PCB_PROPT_INT:    STAT(e->key, i,     e->value); break;
		}
	}

	/* generate the result */
	if (num_vals != 0) {
		switch(p->type) {
			case PCB_PROPT_invalid: break;
			case PCB_PROPT_max: break;
			case PCB_PROPT_STRING: break;
			case PCB_PROPT_COORD:  avgp.coord = avgp.coord/num_vals;  break;
			case PCB_PROPT_ANGLE:  avgp.angle = avgp.angle/num_vals;  break;
			case PCB_PROPT_INT:    avgp.i     = avgp.i/num_vals;  break;
		}
		if (avg != NULL) *avg = avgp;
		if (min != NULL) *min = minp;
		if (max != NULL) *max = maxp;
		if (most_common != NULL) *most_common = bestp;
	}
	return 0;
}

int prop_cmp(const void *e1_, const void *e2_)
{
	const htsp_entry_t *e1 = e1_, *e2 = e2_;
	if (e1->key[0] != e2->key[0]) /* special exception: list p/ first then a/ */
		return e1->key[0] < e2->key[0];
	return strcmp(e1->key, e2->key);
}

htsp_entry_t *pcb_props_sort(pcb_propedit_t *ctx)
{
	htsp_entry_t *e, *arr = malloc(sizeof(htsp_entry_t) * (ctx->props.used + 1));
	int n;

	for(e = htsp_first(&ctx->props), n = 0; e != NULL; e = htsp_next(&ctx->props, e), n++)
		arr[n] = *e;

	qsort(arr, n, sizeof(htsp_entry_t), prop_cmp);

	arr[ctx->props.used].key = NULL;
	return arr;
}


#undef STAT
