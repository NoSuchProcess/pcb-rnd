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
		conf_native_t *nat = conf_get_field(r->conf);
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

	f = pcb_fopen(fn, "w");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "tedax_drc_save(): can't open %s for writing\n", fn);
		return -1;
	}
	fprintf(f, "tEDAx v1\n");
	res = tedax_drc_fsave(pcb, drcid, f);
	fclose(f);
	return res;
}


