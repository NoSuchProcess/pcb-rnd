/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  tedax IO plugin - netlist import
 *  pcb-rnd Copyright (C) 2017,2020 Tibor 'Igor2' Palinkas
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
#include "plug_import.h"
#include "plug_io.h"
#include <librnd/core/error.h>
#include <librnd/core/plugins.h>
#include <librnd/core/pcb-printf.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/actions.h>
#include <librnd/core/safe_fs.h>
#include "obj_subc.h"
#include "netlist.h"

#include "tnetlist.h"
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

int tedax_net_fload(FILE *fn, int import_fp, const char *blk_id, int silent)
{
	char line[520];
	char *argv[16];
	int argc;
	htsp_t fps, pinnames;
	htsp_entry_t *e;

	if (tedax_seek_hdr(fn, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0])) < 0)
		return -1;

	if (tedax_seek_block(fn, "netlist", "v1", blk_id, silent, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0])) < 0)
		return -1;

	htsp_init(&fps, strhash, strkeyeq);
	htsp_init(&pinnames, strhash, strkeyeq);

	pcb_actionva(&PCB->hidlib, "Netlist", "Freeze", NULL);
	pcb_actionva(&PCB->hidlib, "Netlist", "Clear", NULL);

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
			pcb_actionva(&PCB->hidlib, "Netlist", "Add",  argv[1], id, NULL);
		}
		else if ((argc == 4) && (strcmp(argv[0], "pinname") == 0)) {
			char id[512];
			sprintf(id, "%s-%s", argv[1], argv[2]);
			e = htsp_popentry(&pinnames, id);
			if (e != NULL) {
				free(e->key);
				free(e->value);
			}
			htsp_set(&pinnames, pcb_strdup(id), pcb_strdup(argv[3]));
		}
		else if ((argc == 2) && (strcmp(argv[0], "end") == 0) && (strcmp(argv[1], "netlist") == 0))
			break;
	}

	pcb_actionva(&PCB->hidlib, "Netlist", "Sort", NULL);
	pcb_actionva(&PCB->hidlib, "Netlist", "Thaw", NULL);

	if (import_fp) {
		pcb_actionva(&PCB->hidlib, "ElementList", "start", NULL);
		for (e = htsp_first(&fps); e; e = htsp_next(&fps, e)) {
			fp_t *fp = e->value;

/*			pcb_trace("tedax fp: refdes=%s val=%s fp=%s\n", e->key, fp->value, fp->footprint);*/
			if (fp->footprint == NULL)
				pcb_message(PCB_MSG_ERROR, "tedax: not importing refdes=%s: no footprint specified\n", e->key);
			else
				pcb_actionva(&PCB->hidlib, "ElementList", "Need", null_empty(e->key), null_empty(fp->footprint), null_empty(fp->value), NULL);

			free(e->key);
			free(fp->value);
			free(fp->footprint);
			free(fp);
		}
		pcb_actionva(&PCB->hidlib, "ElementList", "Done", NULL);
	}

	for (e = htsp_first(&pinnames); e; e = htsp_next(&pinnames, e)) {
		char *refdes = e->key, *name = e->value, *pin;
		pin = strchr(refdes, '-');
		if (pin != NULL) {
			*pin = '\0';
			pin++;
			pcb_actionva(&PCB->hidlib, "ChangePinName", refdes, pin, name, NULL);
		}
		free(e->key);
		free(e->value);
	}

	htsp_uninit(&fps);
	htsp_uninit(&pinnames);

	return 0;
}


int tedax_net_load(const char *fname_net, int import_fp, const char *blk_id, int silent)
{
	FILE *fn;
	int ret = 0;

	fn = pcb_fopen(&PCB->hidlib, fname_net, "r");
	if (fn == NULL) {
		pcb_message(PCB_MSG_ERROR, "can't open file '%s' for read\n", fname_net);
		return -1;
	}

	ret = tedax_net_fload(fn, import_fp, blk_id, silent);

	fclose(fn);
	return ret;
}

int tedax_net_fsave(pcb_board_t *pcb, const char *netlistid, FILE *f)
{
	pcb_cardinal_t n;
	htsp_entry_t *e;
	pcb_netlist_t *nl = &pcb->netlist[PCB_NETLIST_EDITED];

	fprintf(f, "begin netlist v1 ");
	tedax_fprint_escape(f, netlistid);
	fputc('\n', f);


	for(e = htsp_first(nl); e != NULL; e = htsp_next(nl, e)) {
		pcb_net_term_t *t;
		pcb_net_t *net = e->value;

		for(t = pcb_termlist_first(&net->conns); t != NULL; t = pcb_termlist_next(t))
			fprintf(f, " conn %s %s %s\n", net->name, t->refdes, t->term);
	}

	PCB_SUBC_LOOP(pcb->Data) {
		pcb_attribute_t *a;

		if ((subc->refdes == NULL) || (*subc->refdes == '\0') || PCB_FLAG_TEST(PCB_FLAG_NONETLIST, subc))
			continue;

		for(n = 0, a = subc->Attributes.List; n < subc->Attributes.Number; n++,a++) {
			if (strcmp(a->name, "refdes") == 0)
				continue;
			if (strcmp(a->name, "footprint") == 0) {
				fprintf(f, " footprint %s ", subc->refdes);
				tedax_fprint_escape(f, a->value);
				fputc('\n', f);
				continue;
			}
			if (strcmp(a->name, "value") == 0) {
				fprintf(f, " value %s ", subc->refdes);
				tedax_fprint_escape(f, a->value);
				fputc('\n', f);
				continue;
			}
			if (strcmp(a->name, "device") == 0) {
				fprintf(f, " device %s ", subc->refdes);
				tedax_fprint_escape(f, a->value);
				fputc('\n', f);
				continue;
			}
			pcb_fprintf(f, " comptag %s ", subc->refdes);
			tedax_fprint_escape(f, a->name);
			fputc(' ', f);
			tedax_fprint_escape(f, a->value);
			fputc('\n', f);
		}
	} PCB_END_LOOP;

	fprintf(f, "end netlist\n");
	return 0;
}


int tedax_net_save(pcb_board_t *pcb, const char *netlistid, const char *fn)
{
	int res;
	FILE *f;

	f = pcb_fopen_askovr(&PCB->hidlib, fn, "w", NULL);
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "tedax_net_save(): can't open %s for writing\n", fn);
		return -1;
	}
	fprintf(f, "tEDAx v1\n");
	res = tedax_net_fsave(pcb, netlistid, f);
	fclose(f);
	return res;
}


extern int pcb_io_tedax_test_parse(pcb_plug_io_t *plug_ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f);

static int tedaxnet_support_prio(pcb_plug_import_t *ctx, unsigned int aspects, const char **args, int numargs)
{
	FILE *f;
	int res;

	if ((aspects != IMPORT_ASPECT_NETLIST) || (numargs != 1))
		return 0; /* only pure netlist import is supported from a single file*/


	f = pcb_fopen(&PCB->hidlib, args[0], "r");
	if (f == NULL)
		return 0;

	res = pcb_io_tedax_test_parse(NULL, 0, args[0], f);
	fclose(f);
	return res ? 100 : 0;
}


static int tedaxnet_import(pcb_plug_import_t *ctx, unsigned int aspects, const char **args, int numargs)
{
	if (numargs != 1) {
		pcb_message(PCB_MSG_ERROR, "import_tedaxnet: requires exactly 1 input file name\n");
		return -1;
	}
	return tedax_net_load(args[0], 1, NULL, 0);
}

static pcb_plug_import_t import_tedaxnet;

void pcb_tedax_net_uninit(void)
{
	PCB_HOOK_UNREGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_tedaxnet);
}

void pcb_tedax_net_init(void)
{
	/* register the IO hook */
	import_tedaxnet.plugin_data = NULL;

	import_tedaxnet.fmt_support_prio = tedaxnet_support_prio;
	import_tedaxnet.import           = tedaxnet_import;
	import_tedaxnet.name             = "tEDAx netlist";
	import_tedaxnet.desc             = "netlist+footprints from tEDAx";
	import_tedaxnet.single_arg       = 1;
	import_tedaxnet.all_filenames    = 1;
	import_tedaxnet.ext_exec         = 0;

	PCB_HOOK_REGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_tedaxnet);
}
