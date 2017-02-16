/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  tedax import plugin
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <genht/htsp.h>
#include <genht/hash.h>

#include "board.h"
#include "data.h"
#include "error.h"
#include "pcb-printf.h"
#include "compat_misc.h"

#include "action_helper.h"
#include "hid_actions.h"
#include "plugins.h"
#include "hid.h"


static const char *tedax_cookie = "tedax importer";

/* remove leading whitespace */
#define ltrim(s) while(isspace(*s)) s++

/* remove trailing newline;; trailing backslash is an error */
#define rtrim(s) \
	do { \
		char *end; \
		for(end = s + strlen(s) - 1; (end >= s) && ((*end == '\r') || (*end == '\n')); end--) \
			*end = '\0'; \
		if (*end == '\\') \
			return -1; \
	} while(0)

static int tedax_getline(FILE *f, char *buff, int buff_size, char *argv[], int argv_size)
{
	int argc;

	for(;;) {
		char *s, *o;

		if (fgets(buff, buff_size, f) == NULL)
			return -1;

		s = buff;
		if (*s == '#') /* comment */
			continue;
		ltrim(s);
		rtrim(s);
		if (*s == '\0') /* empty line */
			continue;

		/* argv split */
		for(argc = 0, o = argv[0] = s; *s != '\0';) {
			if (*s == '\\') {
				s++;
				*o = *s;
				o++;
				s++;
				continue;
			}
			if ((argc+1 < argv_size) && ((*s == ' ') || (*s == '\t'))) {
				*s = '\0';
				s++;
				o++;
				while((*s == ' ') || (*s == '\t'))
					s++;
				argc++;
				argv[argc] = o;
			}
			else {
				*o = *s;
				s++;
				o++;
			}
		}
		return argc+1; /* valid line, split up */
	}

	return -1; /* can't get here */
}

#define null_empty(s) ((s) == NULL ? "" : (s))

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

static int tedax_parse_net(FILE *fn)
{
	char line[520];
	char *argv[16];
	int argc;
	htsp_t fps;
	htsp_entry_t *e;

	/* look for the header */
	if (tedax_getline(fn, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0])) < 2)
		return -1;
	if ((argv[1] == NULL) || (strcmp(argv[0], "tEDAx") != 0) || (strcmp(argv[1], "v1") != 0))
		return -1;

	htsp_init(&fps, strhash, strkeyeq);

	pcb_hid_actionl("Netlist", "Freeze", NULL);
	pcb_hid_actionl("Netlist", "Clear", NULL);

	while((argc = tedax_getline(fn, line, sizeof(line), argv, sizeof(argv)/sizeof(argv[0]))) >= 0) {
		if (strcmp(argv[0], "footprint") == 0) {
			fp_t *fp = htsp_get2(&fps, argv[1], sizeof(fp_t));
			fp->footprint = pcb_strdup(argv[2]);
		}
		else if (strcmp(argv[0], "value") == 0) {
			fp_t *fp = htsp_get2(&fps, argv[1], sizeof(fp_t));
			fp->value = pcb_strdup(argv[2]);
		}
		else if (strcmp(argv[0], "conn") == 0) {
			char id[512];
			sprintf(id, "%s-%s", argv[2], argv[3]);
			pcb_hid_actionl("Netlist", "Add",  argv[1], id, NULL);
		}
	}

	pcb_hid_actionl("Netlist", "Sort", NULL);
	pcb_hid_actionl("Netlist", "Thaw", NULL);

	pcb_hid_actionl("ElementList", "start", NULL);
	for (e = htsp_first(&fps); e; e = htsp_next(&fps, e)) {
		fp_t *fp = e->value;

		pcb_trace("tedax fp: refdes=%s val=%s fp=%s\n", e->key, fp->value, fp->footprint);
		if (fp->footprint == NULL)
			pcb_message(PCB_MSG_ERROR, "tedax: not importing refdes=%s: no footprint specified\n", e->key);
		else
			pcb_hid_actionl("ElementList", "Need", null_empty(e->key), null_empty(fp->footprint), null_empty(fp->value), NULL);

		free(e->key);
		free(fp->value);
		free(fp->footprint);
		free(fp);
	}

	pcb_hid_actionl("ElementList", "Done", NULL);

	htsp_uninit(&fps);

	return 0;
}


static int tedax_load_net(const char *fname_net)
{
	FILE *fn;
	int ret = 0;

	fn = fopen(fname_net, "r");
	if (fn == NULL) {
		pcb_message(PCB_MSG_ERROR, "can't open file '%s' for read\n", fname_net);
		return -1;
	}

	ret = tedax_parse_net(fn);

	fclose(fn);
	return ret;
}

static const char pcb_acts_LoadtedaxFrom[] = "LoadTedaxFrom(filename)";
static const char pcb_acth_LoadtedaxFrom[] = "Loads the specified tedax netlist file.";
int pcb_act_LoadtedaxFrom(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *fname = NULL;
	static char *default_file = NULL;

	fname = argc ? argv[0] : 0;

	if (!fname || !*fname) {
		fname = pcb_gui->fileselect("Load tedax netlist file...",
																"Picks a tedax netlist file to load.\n",
																default_file, ".net", "tedax", HID_FILESELECT_READ);
		if (fname == NULL)
			PCB_ACT_FAIL(LoadtedaxFrom);
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	return tedax_load_net(fname);
}

pcb_hid_action_t tedax_action_list[] = {
	{"LoadTedaxFrom", 0, pcb_act_LoadtedaxFrom, pcb_acth_LoadtedaxFrom, pcb_acts_LoadtedaxFrom}
};

PCB_REGISTER_ACTIONS(tedax_action_list, tedax_cookie)

static void hid_tedax_uninit()
{
	pcb_hid_remove_actions_by_cookie(tedax_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_import_tedax_init()
{
	PCB_REGISTER_ACTIONS(tedax_action_list, tedax_cookie)
	return hid_tedax_uninit;
}
