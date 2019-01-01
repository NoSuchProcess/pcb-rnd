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
	const char *tedax;   /* "lloc ltype kind" for tEDAx */
} drc_out_rule_t;

static const drc_out_rule_t out_rules[] = {
	{"design/bloat",     "all copper gap"},
	{"design/shrink",    "all copper overlap"},
	{"design/min_wid",   "all copper min_size"},
	{"design/min_slk",   "all silk min_size"},
	{"design/min_drill", "all mech min_size"},
	{NULL, NULL}
};


int tedax_drc_fsave(pcb_board_t *pcb, const char *drcid, FILE *f)
{
	const drc_out_rule_t *r;

	fprintf(f, "begin drc v1 ");
	tedax_fprint_escape(f, drcid);
	fputc('\n', f);

	for(r = out_rules; r->conf != NULL; r++) {
		conf_native_t *nat = conf_get_field(r->conf);
		if ((nat == NULL) || (nat->prop->src == NULL))
			continue;
		pcb_fprintf(f, " rule %s %.06mm pcb_rnd_old_drc_from_conf\n", r->tedax, nat->val.coord[0]);
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


