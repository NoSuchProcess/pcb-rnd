/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  tedax IO plugin - stackup import/export
 *  pcb-rnd Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include "board.h"
#include "parse.h"
#include "safe_fs.h"
#include "error.h"
#include "tdrc.h"

typedef struct {
	const char *conf;    /* full path in the conf */
	const char *ttype;   /* tEDAx layer type */
	const char *tkind;   /* tEDAx layer kind */
} drc_rule_t;

static const drc_rule_t rules[] = {
	{"design/bloat",     "copper", "gap"},
	{"design/shrink",    "copper", "overlap"},
	{"design/min_wid",   "copper", "min_size"},
	{"design/min_slk",   "silk",   "min_size"},
	{"design/min_drill", "mech",   "min_size"},
};

#define NUM_RULES (sizeof(rules) / sizeof(rules[0]))

int tedax_drc_fsave(pcb_board_t *pcb, const char *drcid, FILE *f)
{
	const drc_rule_t *r;
	int n;

	fprintf(f, "begin drc v1 ");
	tedax_fprint_escape(f, drcid);
	fputc('\n', f);

	for(n = 0, r = rules; n < NUM_RULES; r++,n++) {
		conf_native_t *nat = pcb_conf_get_field(r->conf);
		if ((nat == NULL) || (nat->prop->src == NULL))
			continue;
		pcb_fprintf(f, " rule all %s %s %.06mm pcb_rnd_old_drc_from_conf\n", r->ttype, r->tkind, nat->val.coord[0]);
	}

	fprintf(f, "end drc\n");
	return 0;
}

int tedax_drc_save(pcb_board_t *pcb, const char *drcid, const char *fn)
{
	int res;
	FILE *f;

	f = pcb_fopen_askovr(&PCB->hidlib, fn, "w", NULL);
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "tedax_drc_save(): can't open %s for writing\n", fn);
		return -1;
	}
	fprintf(f, "tEDAx v1\n");
	res = tedax_drc_fsave(pcb, drcid, f);
	fclose(f);
	return res;
}

int tedax_drc_fload(pcb_board_t *pcb, FILE *f, const char *blk_id, int silent)
{
	const drc_rule_t *r;
	char line[520], *argv[16];
	int argc, n;
	pcb_coord_t val[NUM_RULES] = {0};

	if (tedax_seek_hdr(f, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0])) < 0)
		return -1;

	if ((argc = tedax_seek_block(f, "drc", "v1", blk_id, silent, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0]))) < 1)
		return -1;

	while((argc = tedax_getline(f, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0]))) >= 0) {
		if (strcmp(argv[0], "rule") == 0) {
			double d;
			for(n = 0, r = rules; n < NUM_RULES; r++,n++) {
				pcb_bool succ;
				if ((strcmp(argv[2], r->ttype) != 0) || (strcmp(argv[3], r->tkind) != 0))
					continue;
				d = pcb_get_value(argv[4], "mm", NULL, &succ);
				if (succ) {
					if (d > val[n])
						val[n] = d;
				}
				else
					pcb_message(PCB_MSG_ERROR, "ignoring invalid numeric value '%s'\n", argv[4]);
			}
		}
		else if ((argc == 2) && (strcmp(argv[0], "end") == 0) && (strcmp(argv[1], "drc") == 0))
			break;
		else
			pcb_message(PCB_MSG_ERROR, "ignoring invalid command in drc %s\n", argv[0]);
	}

	for(n = 0, r = rules; n < NUM_RULES; r++,n++)
		pcb_conf_setf(CFR_DESIGN, r->conf, -1, "%$mm", val[n]);
	return 0;
}

int tedax_drc_load(pcb_board_t *pcb, const char *fn, const char *blk_id, int silent)
{
	int res;
	FILE *f;

	f = pcb_fopen(&PCB->hidlib, fn, "r");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "tedax_drc_load(): can't open %s for reading\n", fn);
		return -1;
	}
	res = tedax_drc_fload(pcb, f, blk_id, silent);
	fclose(f);
	return res;
}


