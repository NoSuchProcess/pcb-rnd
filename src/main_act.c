/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact addresses for paper mail and Email:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */

#define Progname "pcb-rnd"

#include "config.h"

#include "hid_actions.h"
#include "hid_init.h"
#include "conf_core.h"


/* --------------------------------------------------------------------------- */

static const char pcb_acts_PrintActions[] = "PrintActions()";

static const char pcb_acth_PrintActions[] = "Print all actions available.";

int pcb_act_PrintActions(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_print_actions();
	return 0;
}
/* --------------------------------------------------------------------------- */

static const char pcb_acts_DumpActions[] = "DumpActions()";

static const char pcb_acth_DumpActions[] = "Dump all actions available.";

int pcb_act_DumpActions(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_dump_actions();
	return 0;
}

/* print usage lines */
static inline void u(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
	va_end(ap);
}

static const char pcb_acts_PrintUsage[] =
	"PrintUsage()\n"
	"PrintUsage(plugin)";

static const char pcb_acth_PrintUsage[] = "Print command line arguments of pcb-rnd or a plugin loaded.";

static int help0(void)
{
	pcb_hid_t **hl = pcb_hid_enumerate();
	int i;

	u("pcb-rnd Printed Circuit Board editing program, http://repo.hu/projects/pcb-rnd");
	u("For more information, please read the topic help pages:", Progname);
	u("  %s --help topic");
	u("Topics are:");
	u("  invocation           how to run pcb-rnd");
	u("  main                 main/misc flags (affecting none or all plugins)");
	for (i = 0; hl[i]; i++)
		if (hl[i]->usage != NULL)
			u("  %-20s %s", hl[i]->name, hl[i]->description);
	return 0;
}

extern const char *pcb_action_args[];
static int help_main(void) {
	const char **cs;
	for(cs = pcb_action_args; cs[2] != NULL; cs += 4) {
		fprintf(stderr, "%s [", Progname);
		if (cs[0] != NULL)
			fprintf(stderr, "-%s", cs[0]);
		if ((cs[0] != NULL) && (cs[1] != NULL))
			fprintf(stderr, "|");
		if (cs[1] != NULL)
			fprintf(stderr, "-%s", cs[1]);
		fprintf(stderr, "]    %s\n", cs[3]);
	}
	return 0;
}

static int help_invoc(void)
{
	pcb_hid_t **hl = pcb_hid_enumerate();
	int i;
	int n_printer = 0, n_gui = 0, n_exporter = 0;

	u("pcb-rnd invocation:");
	u("");
	u("%s [main options]                                    See --help main", Progname);
	u("");
	u("%s [generics] [--gui GUI] [gui options] <pcb file>   interactive GUI", Progname);

	u("Available GUI hid%s:", n_gui == 1 ? "" : "s");
	for (i = 0; hl[i]; i++)
		if (hl[i]->gui)
			fprintf(stderr, "\t%-8s %s\n", hl[i]->name, hl[i]->description);

	u("\n%s [generics] -p [printing options] <pcb file>\tto print", Progname);
	u("Available printing hid%s:", n_printer == 1 ? "" : "s");
	for (i = 0; hl[i]; i++)
		if (hl[i]->printer)
			fprintf(stderr, "\t%-8s %s\n", hl[i]->name, hl[i]->description);

	u("\n%s [generics] -x hid [export options] <pcb file>\tto export", Progname);
	u("Available export hid%s:", n_exporter == 1 ? "" : "s");
	for (i = 0; hl[i]; i++)
		if (hl[i]->exporter)
			fprintf(stderr, "\t%-8s %s\n", hl[i]->name, hl[i]->description);


	u("\nGenerics:");
	u("-c conf/path=value        set the value of a configuration item (in CFR_CLI)");

	return 0;
}

int pcb_act_PrintUsage(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	u("");
	if (argc > 0) {
		pcb_hid_t **hl = pcb_hid_enumerate();
		int i;

		if (strcmp(argv[0], "invocation") == 0)  return help_invoc();
		if (strcmp(argv[0], "main") == 0)        return help_main();

		for (i = 0; hl[i]; i++) {
			if ((hl[i]->usage != NULL) && (strcmp(argv[0], hl[i]->name) == 0)) {
				if (argc > 1)
					return hl[i]->usage(argv[1]);
				else
					return hl[i]->usage(NULL);
			}
		}
		fprintf(stderr, "No help available for %s\n", argv[0]);
		return -1;
	}
	else
		help0();
	return 0;
}


/* --------------------------------------------------------------------------- */
static const char pcb_acts_PrintVersion[] = "PrintVersion()";

static const char pcb_acth_PrintVersion[] = "Print version.";

int pcb_act_PrintVersion(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	printf("PCB version %s\n", VERSION);
	return 0;
}

/* --------------------------------------------------------------------------- */
static const char pcb_acts_PrintCopyright[] = "PrintCopyright()";

static const char pcb_acth_PrintCopyright[] = "Print copyright notice.";

int pcb_act_PrintCopyright(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	printf("\n"
				 "                COPYRIGHT for the original pcb program:\n\n"
				 "    PCB, interactive printed circuit board design\n"
				 "    Copyright (C) 1994,1995,1996,1997 Thomas Nau\n"
				 "    Copyright (C) 1998, 1999, 2000 Harry Eaton\n\n");
	printf("                COPYRIGHT for %s (pcb-rnd) version %s:\n"
				 "    pcb-rnd, a fork of PCB with random improvements\n"
				 "    Copyright (C) 2013, 2014, 2015, 2016 Tibor 'Igor2' Palinkas\n\n", Progname, VERSION);
	printf("    This program is free software; you can redistribute it and/or modify\n"
				 "    it under the terms of the GNU General Public License as published by\n"
				 "    the Free Software Foundation; either version 2 of the License, or\n"
				 "    (at your option) any later version.\n\n");
	printf("    This program is distributed in the hope that it will be useful,\n"
				 "    but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
				 "    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
				 "    GNU General Public License for more details.\n\n");
	printf("    You should have received a copy of the GNU General Public License\n"
				 "    along with this program; if not, write to the Free Software\n"
				 "    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n\n");
	return 0;
}

/* --------------------------------------------------------------------------- */
static const char pcb_acts_PrintPaths[] = "PrintPaths()";

static const char pcb_acth_PrintPaths[] = "Print full paths and search paths.";

static void print_list(const conflist_t *cl)
{
	int n;
	conf_listitem_t *ci;
	const char *p;

	printf(" ");
	conf_loop_list_str(cl, ci, p, n) {
		printf("%c%s", (n == 0) ? '"' : ':', p);
	}
	printf("\"\n");
}

int pcb_act_PrintPaths(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	htsp_entry_t *e;
	conf_fields_foreach(e) {
		conf_native_t *n = e->value;
		if ((strncmp(n->hash_path, "rc/path/", 8) == 0) && (n->type == CFN_STRING) && (n->used == 1))
			printf("%-32s = %s\n", n->hash_path, n->val.string[0]);
	}
	printf("rc/default_font_file             ="); print_list(&conf_core.rc.default_font_file);
	printf("rc/library_search_paths          ="); print_list(&conf_core.rc.library_search_paths);
	printf("rc/library_shell                 = \"%s\"\n", conf_core.rc.library_shell);
	return 0;
}


/* --------------------------------------------------------------------------- */

pcb_hid_action_t main_action_list[] = {
	{"PrintActions", 0, pcb_act_PrintActions,
	 pcb_acth_PrintActions, pcb_acts_PrintActions}
	,
	{"DumpActions", 0, pcb_act_DumpActions,
	 pcb_acth_DumpActions, pcb_acts_DumpActions}
	,
	{"PrintUsage", 0, pcb_act_PrintUsage,
	 pcb_acth_PrintUsage, pcb_acts_PrintUsage}
	,
	{"PrintVersion", 0, pcb_act_PrintVersion,
	 pcb_acth_PrintVersion, pcb_acts_PrintVersion}
	,
	{"PrintCopyright", 0, pcb_act_PrintCopyright,
	 pcb_acth_PrintCopyright, pcb_acts_PrintCopyright}
	,
	{"PrintPaths", 0, pcb_act_PrintPaths,
	 pcb_acth_PrintPaths, pcb_acts_PrintPaths}
};

PCB_REGISTER_ACTIONS(main_action_list, NULL)
