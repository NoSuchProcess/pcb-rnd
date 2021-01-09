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
#include "board.h"

#include "delay_create.h"
#include "read.h"

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

	/* location */
	const char *fn;
	long line, col, start_line, start_col;

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
		rctx->coord_scale = RND_MM_TO_COORD(1.0/10000.0);
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

int io_pads_parse_pcb(pcb_plug_io_t *ctx, pcb_board_t *pcb, const char *filename, rnd_conf_role_t settings_dest)
{
	rnd_hidlib_t *hl = &PCB->hidlib;
	FILE *f;
	int ret = 0;
	pads_read_ctx_t rctx;

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
	htsp_init(&rctx.parts, strhash, strkeyeq);

	/* read the header */
	if (pads_parse_header(&rctx) != 0) {
		fclose(f);
		pcb_dlcr_uninit(&rctx.dlcr);
		return -1;
	}

	
	ret = (pads_parse_block(&rctx) == 1) ? 0 : -1;

	pads_assign_layers(&rctx);
	pcb_dlcr_create(pcb, &rctx.dlcr);
	pcb_dlcr_uninit(&rctx.dlcr);

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
