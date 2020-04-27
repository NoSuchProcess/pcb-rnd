/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  LTSpice import HID
 *  pcb-rnd Copyright (C) 2017,2020 Tibor 'Igor2' Palinkas
 *  Copyright (C) 2017 Erich Heinzle (non passive footprint parsing)
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
#include <stdlib.h>
#include <string.h>
#include <qparse/qparse.h>

#include "board.h"
#include "data.h"
#include "plug_import.h"
#include <librnd/core/error.h>
#include <librnd/core/pcb-printf.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/compat_fs.h>

#include <librnd/core/actions.h>
#include <librnd/core/plugins.h>
#include <librnd/core/hid.h>

static const char *ltspice_cookie = "ltspice importer";

static int ltspice_hdr_asc(FILE *f)
{
	char s[1024];
	while(fgets(s, sizeof(s), f) != NULL)
		if (strncmp(s, "Version 4", 9) == 0)
			return 0;
	return -1;
}

/* remove leading whitespace */
#define ltrim(s) while(isspace(*s)) s++

/* remove trailing newline */
#define rtrim(s) \
	do { \
		char *end; \
		for(end = s + strlen(s) - 1; (end >= s) && ((*end == '\r') || (*end == '\n')); end--) \
			*end = '\0'; \
	} while(0)

typedef struct {
	char *refdes;
	char *value;
	char *footprint;
} symattr_t;


#define null_empty(s) ((s) == NULL ? "" : (s))

static void sym_flush(symattr_t *sattr)
{
/*	pcb_trace("ltspice sym: refdes=%s val=%s fp=%s\n", sattr->refdes, sattr->value, sattr->footprint);*/

	if (sattr->refdes != NULL) {
		if (sattr->footprint == NULL)
			rnd_message(PCB_MSG_ERROR, "ltspice: not importing refdes=%s: no footprint specified\n", sattr->refdes);
		else
			rnd_actionva(&PCB->hidlib, "ElementList", "Need", null_empty(sattr->refdes), null_empty(sattr->footprint), null_empty(sattr->value), NULL);
	}
	free(sattr->refdes); sattr->refdes = NULL;
	free(sattr->value); sattr->value = NULL;
	free(sattr->footprint); sattr->footprint = NULL;
}

static int ltspice_parse_asc(FILE *fa)
{
	symattr_t sattr;
	char line[1024];

	memset(&sattr, 0, sizeof(sattr));

	rnd_actionva(&PCB->hidlib, "ElementList", "start", NULL);

	while(fgets(line, sizeof(line), fa) != NULL) {
		char *s;
		int isPassive = 0;
		s = line;
		rtrim(s);

		if (strncmp(s, "SYMBOL", 6) == 0)
			sym_flush(&sattr);
		else if (strncmp(s, "SYMATTR", 7) == 0) {
			s+=8;
			ltrim(s);
			if (strncmp(s, "InstName", 8) == 0) {
				s+=9;
				ltrim(s);
				free(sattr.refdes);
				sattr.refdes = pcb_strdup(s);
				/* figure out if device is passive or not, as this affects
				   subsequent parsing of the "SYMATTR VALUE .... " line */
				if (strncmp(s, "R", 1) != 0 && strncmp(s, "L", 1) != 0 &&
					strncmp(s, "C", 1) != 0) {
					isPassive = 0;
				}
				else {
					isPassive = 1;
				}
			}
			else {
				if (strncmp(s, "Value", 5) == 0) {
					/* we get around non passives having a device quoted with no 
					mfg= field in the .net file by parsing the device name for
					an appended .pcb-rnd-TO92 etc...
					i.e. parse the following: SYMATTR Value 2N2222.pcb-rnd-TO92 */
					s+=6;
					ltrim(s);
					free(sattr.value);
					if (isPassive) {
						sattr.value = pcb_strdup(s);
					}
					else {
						char *fp;
						/*s+=6;*/
						fp = strstr(s, ".pcb-rnd-");
						if (fp != NULL) {
							sattr.value = pcb_strdup(fp);
							s = fp;							
							fp += 9;
							if (*fp == '"') {
								char *end;
								fp++;
								end = strchr(fp, '"');
								if (end != NULL)
									*end = '\0';
							}
							free(sattr.footprint);
							sattr.footprint = pcb_strdup(fp);
						}
					}
				}
				if (strncmp(s, "SpiceLine", 9) == 0) {
					/* for passives, the SpiceLine include the "mfg=" field */
					char *fp;
					s+=6;
					fp = strstr(s, "mfg=");
					if (fp != NULL) {
						fp += 4;
						if (*fp == '"') {
							char *end;
							fp++;
							end = strchr(fp, '"');
							if (end != NULL)
								*end = '\0';
						}
						if (strncmp(fp, ".pcb-rnd-", 9) == 0)
							fp += 9;
						if (strncmp(fp, "pcb-rnd-", 8) == 0)
							fp += 8;
						free(sattr.footprint);
						sattr.footprint = pcb_strdup(fp);
					}
				}
				/* nothing stops a user inserting 
				   "SYMATTR Footprint TO92" if keen in the .asc file */
				if (strncmp(s, "Footprint", 9) == 0) {
					s+=10;
					ltrim(s);
					free(sattr.footprint);
					sattr.footprint = pcb_strdup(s);
				}
			}
		}
	}
	sym_flush(&sattr);
	rnd_actionva(&PCB->hidlib, "ElementList", "Done", NULL);
	return 0;
}

static int ltspice_parse_net(FILE *fn)
{
	char line[1024];

	rnd_actionva(&PCB->hidlib, "Netlist", "Freeze", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Clear", NULL);

	while(fgets(line, sizeof(line), fn) != NULL) {
		int argc;
		char **argv, *s;

		s = line;
		ltrim(s);
		rtrim(s);
		argc = qparse2(s, &argv, QPARSE_DOUBLE_QUOTE | QPARSE_SINGLE_QUOTE);
		if ((argc > 1) && (strcmp(argv[0], "NET") == 0)) {
			int n;
			for(n = 2; n < argc; n++) {
/*				pcb_trace("net-add '%s' '%s'\n", argv[1], argv[n]);*/
				rnd_actionva(&PCB->hidlib, "Netlist", "Add",  argv[1], argv[n], NULL);
			}
		}
	}

	rnd_actionva(&PCB->hidlib, "Netlist", "Sort", NULL);
	rnd_actionva(&PCB->hidlib, "Netlist", "Thaw", NULL);

	return 0;
}


static int ltspice_load(const char *fname_net, const char *fname_asc)
{
	FILE *fn, *fa;
	int ret = 0;

	fn = pcb_fopen(&PCB->hidlib, fname_net, "r");
	if (fn == NULL) {
		rnd_message(PCB_MSG_ERROR, "can't open file '%s' for read\n", fname_net);
		return -1;
	}
	fa = pcb_fopen(&PCB->hidlib, fname_asc, "r");
	if (fa == NULL) {
		rnd_message(PCB_MSG_ERROR, "can't open file '%s' for read\n", fname_asc);
		fclose(fn);
		return -1;
	}

	if (ltspice_hdr_asc(fa)) {
		rnd_message(PCB_MSG_ERROR, "file '%s' doesn't look like a verison 4 asc file\n", fname_asc);
		goto error;
	}


	if (ltspice_parse_asc(fa) != 0) goto error;
	if (ltspice_parse_net(fn) != 0) goto error;

	quit:;
	fclose(fa);
	fclose(fn);
	return ret;

	error:
	ret = -1;
	goto quit;
}

static void gen_filenames(const char *fname, char **fname_net, char **fname_asc)
{
	const char *end;
	char *fname_base = NULL;

	end = strrchr(fname, '.');
	if (end != NULL) {
		if (strcmp(end, ".net") == 0)
			fname_base = pcb_strndup(fname, end - fname);
		else if (strcmp(end, ".asc") == 0)
			fname_base = pcb_strndup(fname, end - fname);
	}
	if (fname_base == NULL)
		fname_base = pcb_strdup(fname);

	*fname_net = pcb_strdup_printf("%s.net", fname_base);
	*fname_asc = pcb_strdup_printf("%s.asc", fname_base);
	free(fname_base);
}

static const char pcb_acts_LoadLtspiceFrom[] = "LoadLtspiceFrom(filename)";
static const char pcb_acth_LoadLtspiceFrom[] = "Loads the specified ltspice .net and .asc file - the netlist must be mentor netlist.";
fgw_error_t pcb_act_LoadLtspiceFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fname = NULL;
	char *fname_asc, *fname_net;
	static char *default_file = NULL;
	int rs;

	rnd_PCB_ACT_MAY_CONVARG(1, FGW_STR, LoadLtspiceFrom, fname = argv[1].val.str);

	if (!fname || !*fname) {
		fname = pcb_gui->fileselect(pcb_gui, "Load ltspice net+asc file pair...",
																"Picks a ltspice mentor net or asc file to load.\n",
																default_file, ".asc", NULL, "ltspice", PCB_HID_FSD_READ, NULL);
		if (fname == NULL)
			return 1;
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	gen_filenames(fname, &fname_net, &fname_asc);

	rs = ltspice_load(fname_net, fname_asc);

	free(fname_asc);
	free(fname_net);

	RND_ACT_IRES(rs);
	return 0;
}

static int ltspice_support_prio(pcb_plug_import_t *ctx, unsigned int aspects, const char **args, int numargs)
{
	int res = 0;
	FILE *f = NULL;
	unsigned int good = 0;
	char *fname_asc, *fname_net;

	if ((aspects != IMPORT_ASPECT_NETLIST) || (numargs != 1))
		return 0; /* only pure netlist import is supported */

	gen_filenames(args[0], &fname_net, &fname_asc);
	if (!pcb_file_readable(fname_net))
		goto quit;

	f = pcb_fopen(&PCB->hidlib, fname_asc, "r");
	if (f == NULL)
		goto quit;

	for(;;) {
		char *s, line[1024];
		s = fgets(line, sizeof(line), f);
		if (s == NULL)
			break;
		while(isspace(*s)) s++;
		if (strncmp(s, "SHEET", 5) == 0)
			good |= 1;
		else if (strncmp(s, "WIRE", 4) == 0)
			good |= 2;
		if (good == (1|2)) {
			res = 100;
			goto quit;
		}
	}


	quit:;
	if (f != NULL)
		fclose(f);
	free(fname_asc);
	free(fname_net);
	return res;
}


static int ltspice_import(pcb_plug_import_t *ctx, unsigned int aspects, const char **fns, int numfns)
{
	if (numfns != 1) {
		rnd_message(PCB_MSG_ERROR, "import_ltspice: requires exactly 1 input file name\n");
		return -1;
	}
	return rnd_actionva(&PCB->hidlib, "LoadLtspiceFrom", fns[0], NULL);
}

static pcb_plug_import_t import_ltspice;

rnd_action_t ltspice_action_list[] = {
	{"LoadLtspiceFrom", pcb_act_LoadLtspiceFrom, pcb_acth_LoadLtspiceFrom, pcb_acts_LoadLtspiceFrom}
};

int pplg_check_ver_import_ltspice(int ver_needed) { return 0; }

void pplg_uninit_import_ltspice(void)
{
	rnd_remove_actions_by_cookie(ltspice_cookie);
	PCB_HOOK_UNREGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_ltspice);
}

int pplg_init_import_ltspice(void)
{
	PCB_API_CHK_VER;

	/* register the IO hook */
	import_ltspice.plugin_data = NULL;

	import_ltspice.fmt_support_prio = ltspice_support_prio;
	import_ltspice.import           = ltspice_import;
	import_ltspice.name             = "LTSpice";
	import_ltspice.desc             = "schamtics from LTSpice";
	import_ltspice.ui_prio          = 50;
	import_ltspice.single_arg       = 1;
	import_ltspice.all_filenames    = 1;
	import_ltspice.ext_exec         = 0;

	PCB_HOOK_REGISTER(pcb_plug_import_t, pcb_plug_import_chain, &import_ltspice);

	RND_REGISTER_ACTIONS(ltspice_action_list, ltspice_cookie)
	return 0;
}
