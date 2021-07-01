/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  PADS IO plugin - read: decode, parse, interpret
 *  pcb-rnd Copyright (C) 2020,2021 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 PET Fund in 2020 and 2021)
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

#include <stdio.h>
#include <assert.h>
#include <math.h>

#include <genht/htsp.h>
#include <genht/hash.h>
#include <genht/ht_utils.h>
#include <librnd/core/error.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/rotate.h>
#include "board.h"
#include "extobj.h"
#include "thermal.h"
#include "netlist.h"
#include "obj_term.h"

#include "delay_create.h"
#include "delay_clearance.h"
#include "delay_postproc.h"
#include "read.h"
#include "io_pads_conf.h"

extern conf_io_pads_t conf_io_pads;

/* Parser return value convention:
	1      = succesfully read a block or *section* or line
	0      = eof
	-1..-3 = error
	-4     = bumped into the next *section*
*/

/* virtual layer ID for the non-existent board outline layer */
#define PADS_LID_BOARD 257


typedef struct pads_read_decal_s {
	TODO("do we need swaps?")
	int decal_names_len; /* in bytes, double \0 at the end included */
	char decal_names[1]; /* really as long as it needs to be */
} pads_read_part_t;

typedef struct pads_read_ctx_s {
	pcb_board_t *pcb;
	FILE *f;
	double coord_scale; /* multiply input integer coord values to get pcb-rnd nanometer */
	double ver;
	pcb_dlcr_t dlcr;
	pcb_dlcr_layer_t *layer;
	htsp_t parts;       /* translate part to partdecal and swaps; key=partname value=(pads_read_part_t *) */
	const char *signal_netname; /* netname of the signal currently being read */

	/* location */
	const char *fn;
	long line, col, start_line, start_col;

	/* cache */
	const pcb_extobj_t *teardrop_eo;
	rnd_coord_t clr[PCB_DLCL_max];
	unsigned teardrop_warned:1;
	unsigned got_design_rules:1;

} pads_read_ctx_t;

typedef struct {
	char *assoc_silk, *assoc_mask, *assoc_paste, *assoc_assy;
} pads_layer_t;

#define PADS_ERROR(args) \
do { \
	rnd_message(RND_MSG_ERROR, "io_pads read: syntax error at %s:%ld.%ld: ", rctx->fn, rctx->line, rctx->col); \
	rnd_message args; \
} while(0)

static void pads_update_loc(pads_read_ctx_t *rctx, int c)
{
	if (c == '\n') {
		rctx->line++;
		rctx->col = 1;
	}
	else
		rctx->col++;
}

static void pads_start_loc(pads_read_ctx_t *rctx)
{
	rctx->start_line = rctx->line;
	rctx->start_col = rctx->col;
}

pcb_dlcr_layer_t *pads_layer_alloc(long lid)
{
	pcb_dlcr_layer_t *layer = calloc(sizeof(pcb_dlcr_layer_t), 1);
	layer->id = lid;
	layer->user_data = &layer->local_user_data;
	return layer;
}


#include "read_low.c"
#include "read_high.c"
#include "read_high_misc.c"

static int pads_parse_header(pads_read_ctx_t *rctx)
{
	char *s, *end, tmp[256];

	if (fgets(tmp, sizeof(tmp), rctx->f) == NULL) {
		rnd_message(RND_MSG_ERROR, "io_pads: missing header\n");
		return -1;
	}
	s = tmp+15;
	if ((*s == 'V') || (*s == 'v'))
		s++;
	rctx->ver = strtod(s, &end);
	if (*end != '-') {
		rnd_message(RND_MSG_ERROR, "io_pads: invalid header (version: invalid numeric)\n");
		return -1;
	}
	s = end+1;
	if (strncmp(s, "BASIC", 5) == 0)
		rctx->coord_scale = 2.0/3.0;
	else if (strncmp(s, "MILS", 4) == 0)
		rctx->coord_scale = RND_MIL_TO_COORD(1);
	else if (strncmp(s, "METRIC", 6) == 0)
		rctx->coord_scale = RND_MM_TO_COORD(1.0);
	else if (strncmp(s, "INCHES", 6) == 0)
		rctx->coord_scale = RND_INCH_TO_COORD(1.0/100000.0);
	else {
		rnd_message(RND_MSG_ERROR, "io_pads: invalid header (unknown unit '%s')\n", s);
		return -1;
	}
	return 0;
}

static void pads_assign_layer_assoc(pads_read_ctx_t *rctx, pcb_dlcr_layer_t *src, const char *assoc_name, pcb_layer_type_t loc)
{
	pcb_dlcr_layer_t *l;

	if (assoc_name == NULL)
		return;

	l = htsp_get(&rctx->dlcr.name2layer, assoc_name);
	if (l == NULL) {
		rnd_message(RND_MSG_ERROR, "io_pads: non-existent associated layer '%s' in layer %d (%s)\n", assoc_name, src->id, src->name);
		return;
	}
	if ((l->lyt & PCB_LYT_ANYWHERE) != 0) {
		rnd_message(RND_MSG_ERROR, "io_pads: doubly associated layer '%s' in layer %d (%s)\n", assoc_name, src->id, src->name);
		return;
	}
	l->lyt |= loc;
/*	rnd_trace("assoc layer '%s' %lx!\n", l->name, l->lyt);*/
}

static void pads_assign_layers(pads_read_ctx_t *rctx)
{
	pcb_dlcr_layer_t *lastcop = NULL;
	int seen_copper = 0;
	long n;

	/* assign location to copper layers, assuming they are in order */
	for(n = 0; n < rctx->dlcr.id2layer.used; n++) {
		pcb_dlcr_layer_t *l = rctx->dlcr.id2layer.array[n];

		if ((l != NULL) && (l->lyt & PCB_LYT_COPPER)) {
			if (!seen_copper)
				l->lyt |= PCB_LYT_TOP;
			else
				l->lyt |= PCB_LYT_INTERN;
			lastcop = l;
			seen_copper = 1;
		}
	}
	if (lastcop != NULL) {
		lastcop->lyt &= ~PCB_LYT_INTERN;
		lastcop->lyt |= PCB_LYT_BOTTOM;
	}

	/* assign location to associated top/bottom layers */
	for(n = 0; n < rctx->dlcr.id2layer.used; n++) {
		pads_layer_t *pl;
		pcb_dlcr_layer_t *l = rctx->dlcr.id2layer.array[n];

		if (l == NULL) continue;

		pl = l->user_data;

		/* locate associated top and bottom layers */
		if (l->lyt & PCB_LYT_COPPER) {
			pcb_layer_type_t loc = l->lyt & PCB_LYT_ANYWHERE;
			if ((loc == PCB_LYT_TOP) || (loc == PCB_LYT_BOTTOM)) {
				pads_assign_layer_assoc(rctx, l, pl->assoc_silk, loc);
				pads_assign_layer_assoc(rctx, l, pl->assoc_paste, loc);
				pads_assign_layer_assoc(rctx, l, pl->assoc_mask, loc);
				pads_assign_layer_assoc(rctx, l, pl->assoc_assy, loc);
			}
		}

		free(pl->assoc_silk); pl->assoc_silk = NULL;
		free(pl->assoc_paste); pl->assoc_paste = NULL;
		free(pl->assoc_mask); pl->assoc_mask = NULL;
		free(pl->assoc_assy); pl->assoc_assy = NULL;
	}

	/* register special, hardwired/implicit layers */
	{
		pcb_dlcr_layer_t *ly;
	
		ly = pads_layer_alloc(PADS_LID_BOARD);
		ly->name = rnd_strdup("outline");
		ly->lyt = PCB_LYT_BOUNDARY;
		ly->purpose = "uroute";
		pcb_dlcr_layer_reg(&rctx->dlcr, ly);

		ly = pads_layer_alloc(1024);
		ly->name = rnd_strdup("unassigned");
		ly->lyt = PCB_LYT_DOC;
		pcb_dlcr_layer_reg(&rctx->dlcr, ly);


		ly = pads_layer_alloc(0);
		ly->name = rnd_strdup("all-layer");
		ly->lyt = PCB_LYT_DOC;
		ly->purpose = "unimplemented";
		pcb_dlcr_layer_reg(&rctx->dlcr, ly);
	}
}


static int pads_parse_block(pads_read_ctx_t *rctx)
{
	while(!feof(rctx->f)) {
		char word[256];
		int res = pads_read_word(rctx, word, sizeof(word), 1);
/*rnd_trace("WORD='%s'/%d\n", word, res);*/
		if (res <= 0)
			return res;

		if (*word == '\0') res = 1; /* ignore empty lines between blocks */
		else if (strcmp(word, "*PCB*") == 0) res = pads_parse_pcb(rctx);
		else if (strcmp(word, "*REUSE*") == 0) { TODO("What to do with this?"); res = pads_parse_ignore_sect(rctx); }
		else if (strcmp(word, "*TEXT*") == 0) res = pads_parse_texts(rctx);
		else if (strcmp(word, "*LINES*") == 0) res = pads_parse_lines(rctx);
		else if (strcmp(word, "*VIA*") == 0) res = pads_parse_vias(rctx);
		else if (strcmp(word, "*PARTDECAL*") == 0) res = pads_parse_partdecals(rctx);
		else if (strcmp(word, "*PARTTYPE*") == 0) res = pads_parse_parttypes(rctx);
		else if (strcmp(word, "*PARTT") == 0) res = pads_parse_parttypes(rctx);
		else if (strcmp(word, "*PART*") == 0) res = pads_parse_parts(rctx);
		else if (strcmp(word, "*SIGNAL*") == 0) res = pads_parse_signal(rctx);
		else if (strcmp(word, "*POUR*") == 0) res = pads_parse_pours(rctx);
		else if (strcmp(word, "*MISC*") == 0) res = pads_parse_misc(rctx);
		else if (strcmp(word, "*CLUSTER*") == 0) res = pads_parse_ignore_sect(rctx);
		else if (strcmp(word, "*ROUTE*") == 0) res = pads_parse_ignore_sect(rctx);
		else if (strcmp(word, "*TESTPOINT*") == 0) res = pads_parse_ignore_sect(rctx);
		else if (strcmp(word, "*END*") == 0) {
			rnd_trace("*END* - legit end of file\n");
			return 1;
		}
		else {
			PADS_ERROR((RND_MSG_ERROR, "unknown block: '%s'\n", word));
			return -1;
		}

		if (res == -4) continue; /* bumped into the next section, just go on reading */

		/* exit the loop on error */
		if (res <= 0)
			return res;
	}
	return -1;
}

static int pads_proto_layer_lookup(pcb_dlcr_t *dlcr, pcb_pstk_shape_t *shp)
{
	int level = shp->dlcr_psh_layer_id;
	switch(level) { /* set layer type on PADS-specific cases */
		case -2: shp->layer_mask = PCB_LYT_TOP | PCB_LYT_COPPER; break;
		case -1: shp->layer_mask = PCB_LYT_INTERN | PCB_LYT_COPPER; break;
		case  0: shp->layer_mask = PCB_LYT_BOTTOM | PCB_LYT_COPPER; break;
		default: return 1; /* run the default layer lookup */
	}

	return 0; /* handled here */
}


static const char *postproc_thermal_lookup(void *uctx, pcb_any_obj_t *obj)
{
	htpp_t *ht = uctx;
	return htpp_get(ht, obj);
}

static void postproc_thermal(pads_read_ctx_t *rctx)
{
	long n;
	htpp_t obj2net;
	htsp_entry_t *e;

	htpp_init(&obj2net, ptrhash, ptrkeyeq);

	/* add known objects */
	for(n = 0; n < rctx->dlcr.netname_objs.used; n += 2)
		htpp_insert(&obj2net, rctx->dlcr.netname_objs.array[n], rctx->dlcr.netname_objs.array[n+1]);

	/* add netlisted terminals */
	for(e = htsp_first(&rctx->pcb->netlist[PCB_NETLIST_INPUT]); e != NULL; e = htsp_next(&rctx->pcb->netlist[PCB_NETLIST_INPUT], e)) {
		pcb_net_term_t *t;
		pcb_net_t *net = e->value;
		for(t = pcb_termlist_first(&net->conns); t != NULL; t = pcb_termlist_next(t)) {
			pcb_any_obj_t *o = pcb_term_find_name(rctx->pcb, rctx->pcb->Data, PCB_LYT_COPPER, t->refdes, t->term, NULL, NULL);
			if (o != NULL)
				htpp_insert(&obj2net, o, net->name);
		}
	}


	for(n = 0; n < rctx->dlcr.netname_objs.used; n += 2) {
		pcb_any_obj_t *o = rctx->dlcr.netname_objs.array[n];
		const char *netname = rctx->dlcr.netname_objs.array[n+1];
		if (o->type == PCB_OBJ_POLY)
			pcb_dlcr_post_poly_thermal_netname(rctx->pcb, (pcb_poly_t *)o, netname, PCB_THERMAL_ROUND | PCB_THERMAL_DIAGONAL | PCB_THERMAL_ON, postproc_thermal_lookup, &obj2net);
	}

	htpp_uninit(&obj2net);
}

int io_pads_parse_pcb(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *filename, rnd_conf_role_t settings_dest)
{
	rnd_hidlib_t *hl = &PCB->hidlib;
	FILE *f;
	int ret = 0;
	pads_read_ctx_t rctx = {0};

	f = rnd_fopen(hl, filename, "r");
	if (f == NULL)
		return -1;

	rctx.col = rctx.start_col = 1;
	rctx.line = rctx.start_line = 2; /* compensate for the header we read with fgets() */
	rctx.pcb = pcb;
	rctx.fn = filename;
	rctx.f = f;

	pcb_dlcr_init(&rctx.dlcr);
	rctx.dlcr.flip_y = 1;
	rctx.dlcr.save_netname_objs = 1;
	rctx.dlcr.proto_layer_lookup = pads_proto_layer_lookup;

	/* read the header */
	if (pads_parse_header(&rctx) != 0) {
		fclose(f);
		pcb_dlcr_uninit(&rctx.dlcr);
		return -1;
	}

	htsp_init(&rctx.parts, strhash, strkeyeq);

	pcb_layergrp_inhibit_inc();
	pcb_data_clip_inhibit_inc(pcb->Data);

	ret = (pads_parse_block(&rctx) == 1) ? 0 : -1;
	if (ret != 0) 
		rnd_message(RND_MSG_INFO ,"io_pads: last line parsed: %ld\n", rctx.line);

	pads_assign_layers(&rctx);
	pcb_dlcr_create(pcb, &rctx.dlcr);
	pcb_dlcl_apply(pcb, rctx.clr);
	postproc_thermal(&rctx);
	pcb_dlcr_uninit(&rctx.dlcr);

	pcb_data_clip_inhibit_dec(pcb->Data, 1);
	pcb_layergrp_inhibit_dec();


	genht_uninit_deep(htsp, &rctx.parts, {
		free(htent->key);
		free(htent->value);
	});


	fclose(f);
	return ret;
}

int io_pads_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f)
{
	char tmp[256];
	if (fgets(tmp, sizeof(tmp), f) == NULL)
		return 0;
	return (strncmp(tmp, "!PADS-POWERPCB", 14) == 0);
}
