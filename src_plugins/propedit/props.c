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
#include "propsel.h"
#include "compat_misc.h"
#include "pcb-printf.h"
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

/* Retrieve values for a prop */
pcb_props_t *pcb_props_get(htsp_t *props, const char *propname)
{
	return htsp_get(props, propname);
}

/* Store a new value */
pcb_props_t *pcb_props_add(htsp_t *props, const char *propname, pcb_prop_type_t type, pcb_propval_t val)
{
	pcb_props_t *p;
	htprop_entry_t *e;

	if ((type <= PCB_PROPT_invalid) || (type >= PCB_PROPT_max))
		return NULL;

	/* look up or create the value list (p) associated with the property name */
	p = htsp_get(props, propname);
	if (p == NULL) {
		p = malloc(sizeof(pcb_props_t));
		p->type = type;
		htprop_init(&p->values, prophash[type], propkeyeq[type]);
		htsp_set(props, pcb_strdup(propname), p);
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

pcb_props_t *pcb_props_stat(htsp_t *props, const char *propname, pcb_propval_t *most_common, pcb_propval_t *min, pcb_propval_t *max, pcb_propval_t *avg)
{
	pcb_props_t *p;
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

	p = htsp_get(props, propname);
	if (p == NULL)
		return NULL;

	if ((p->type == PCB_PROPT_STRING) && ((min != NULL) || (max != NULL) || (avg != NULL)))
		return NULL;

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

	return p;
}

#undef STAT

static char buff[8][128];
static int buff_idx = 0;
const char *propedit_sprint_val(pcb_prop_type_t type, pcb_propval_t val)
{
	char *b;
	buff_idx++;
	if (buff_idx > 7)
		buff_idx = 0;
	b = buff[buff_idx];
	switch(type) {
		case PCB_PROPT_STRING: return val.string; break;
		case PCB_PROPT_COORD:  pcb_snprintf(b, 128, "%.06$mm\n%.06$ml", val.coord, val.coord); break;
		case PCB_PROPT_ANGLE:  sprintf(b, "%.2f", val.angle); break;
		case PCB_PROPT_INT:    sprintf(b, "%d", val.i); break;
		default: strcpy(b, "<unknown type>");
	}
	return b;
}

/*****************************************************************************/

void propedit_ins_prop(pe_ctx_t *ctx, htsp_entry_t *pe)
{
	htprop_entry_t *e;
	void *rowid;
	pcb_props_t *p = pe->value;
	pcb_propval_t common, min, max, avg;

	if (gui->propedit_add_prop != NULL)
		rowid = gui->propedit_add_prop(ctx, pe->key, 1, p->values.fill);

	if (gui->propedit_add_stat != NULL) {
		if (p->type == PCB_PROPT_STRING) {
			pcb_props_stat(ctx->core_props, pe->key, &common, NULL, NULL, NULL);
			gui->propedit_add_stat(ctx, pe->key, rowid, propedit_sprint_val(p->type, common), NULL, NULL, NULL);
		}
		else {
			pcb_props_stat(ctx->core_props, pe->key, &common, &min, &max, &avg);
			gui->propedit_add_stat(ctx, pe->key, rowid, propedit_sprint_val(p->type, common), propedit_sprint_val(p->type, min), propedit_sprint_val(p->type, max), propedit_sprint_val(p->type, avg));
		}
	}

	if (gui->propedit_add_value != NULL)
		for (e = htprop_first(&p->values); e; e = htprop_next(&p->values, e))
			gui->propedit_add_value(ctx, pe->key, rowid, propedit_sprint_val(p->type, e->key), e->value);
}


const char *propedit_query(void *pe, const char *cmd, const char *key, const char *val, int idx)
{
	pe_ctx_t *ctx = pe;
	const char *s;
	static const char *ok = "ok";

	if (memcmp(cmd, "v1st", 4) == 0) { /* get the first value */
		ctx->qprop = htsp_get(ctx->core_props, key);
		if (ctx->qprop == NULL)
			return NULL;

		ctx->qprope = htprop_first(&ctx->qprop->values);
		goto vnxt;
	}

	else if (memcmp(cmd, "vnxt", 4) == 0) { /* get the next value */
		vnxt:;
		if (ctx->qprope == NULL)
			return NULL;
		s = propedit_sprint_val(ctx->qprop->type, ctx->qprope->key);
		ctx->qprope = htprop_next(&ctx->qprop->values, ctx->qprope);
		return s;
	}


	else if (memcmp(cmd, "vset", 4) == 0) { /* set value */
		if (pcb_propsel_set(key, val) > 0) {
			htsp_entry_t *pe;
			pcb_props_uninit(ctx->core_props);
			ctx->core_props = pcb_props_init();
			pcb_propsel_map_core(ctx->core_props);
			pe = htsp_getentry(ctx->core_props, key);
			if (pe != NULL) {
				propedit_ins_prop(ctx, pe);
				return ok;
			}
		}
		return NULL;
	}

	return NULL;
}

