/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  tedax IO plugin - netlist import
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include <genht/htsp.h>
#include <genht/hash.h>

#include "board.h"
#include "data.h"
#include "error.h"
#include "pcb-printf.h"
#include "compat_misc.h"
#include "actions.h"
#include "safe_fs.h"

#include "parse.h"

typedef struct {
	char *value;
	char *footprint;
} fp_t;

static void *htsp_get2(htsp_t *ht, const char *key, size_t size)
{
	void *res = htsp_get(ht, key);
	if (res == NULL) {
		res = calloc(size, 1);
		htsp_set(ht, pcb_strdup(key), res);
	}
	return res;
}

int tedax_net_fload(FILE *fn)
{
	char line[520];
	char *argv[16];
	int argc;
	htsp_t fps;
	htsp_entry_t *e;

	if (tedax_seek_hdr(fn, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0])) < 0)
		return -1;

	if (tedax_seek_block(fn, "netlist", "v1", 0, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0])) < 0)
		return -1;

	htsp_init(&fps, strhash, strkeyeq);

	pcb_actionl("Netlist", "Freeze", NULL);
	pcb_actionl("Netlist", "Clear", NULL);

	while((argc = tedax_getline(fn, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0]))) >= 0) {
		if ((argc == 3) && (strcmp(argv[0], "footprint") == 0)) {
			fp_t *fp = htsp_get2(&fps, argv[1], sizeof(fp_t));
			fp->footprint = pcb_strdup(argv[2]);
		}
		else if ((argc == 3) && (strcmp(argv[0], "value") == 0)) {
			fp_t *fp = htsp_get2(&fps, argv[1], sizeof(fp_t));
			fp->value = pcb_strdup(argv[2]);
		}
		else if ((argc == 4) && (strcmp(argv[0], "conn") == 0)) {
			char id[512];
			sprintf(id, "%s-%s", argv[2], argv[3]);
			pcb_actionl("Netlist", "Add",  argv[1], id, NULL);
		}
		else if ((argc == 2) && (strcmp(argv[0], "end") == 0) && (strcmp(argv[1], "netlist") == 0))
			break;
	}

	pcb_actionl("Netlist", "Sort", NULL);
	pcb_actionl("Netlist", "Thaw", NULL);

	pcb_actionl("ElementList", "start", NULL);
	for (e = htsp_first(&fps); e; e = htsp_next(&fps, e)) {
		fp_t *fp = e->value;

/*		pcb_trace("tedax fp: refdes=%s val=%s fp=%s\n", e->key, fp->value, fp->footprint);*/
		if (fp->footprint == NULL)
			pcb_message(PCB_MSG_ERROR, "tedax: not importing refdes=%s: no footprint specified\n", e->key);
		else
			pcb_actionl("ElementList", "Need", null_empty(e->key), null_empty(fp->footprint), null_empty(fp->value), NULL);

		free(e->key);
		free(fp->value);
		free(fp->footprint);
		free(fp);
	}

	pcb_actionl("ElementList", "Done", NULL);

	htsp_uninit(&fps);

	return 0;
}


int tedax_net_load(const char *fname_net)
{
	FILE *fn;
	int ret = 0;

	fn = pcb_fopen(fname_net, "r");
	if (fn == NULL) {
		pcb_message(PCB_MSG_ERROR, "can't open file '%s' for read\n", fname_net);
		return -1;
	}

	ret = tedax_net_fload(fn);

	fclose(fn);
	return ret;
}

