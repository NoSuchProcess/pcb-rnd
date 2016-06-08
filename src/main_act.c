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

#include "config.h"

#include "action_helper.h"
#include "hid_actions.h"
#include "hid_attrib.h"
#include "hid_init.h"
#include "hid.h"
#include "data.h"
#include "conf_core.h"


/* --------------------------------------------------------------------------- */

static const char printactions_syntax[] = "PrintActions()";

static const char printactions_help[] = "Print all actions available.";

int ActionPrintActions(int argc, char **argv, Coord x, Coord y)
{
	print_actions();
}
/* --------------------------------------------------------------------------- */

static const char dumpactions_syntax[] = "DumpActions()";

static const char dumpactions_help[] = "Dump all actions available.";

int ActionDumpActions(int argc, char **argv, Coord x, Coord y)
{
	dump_actions();
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

static const char printusage_syntax[] = 
	"PrintUsage()\n"
	"PrintUsage(plugin)";

static const char printusage_help[] = "Print command line aruments of pcb-rnd or a plugin loaded.";

static int help0(void)
{
	HID **hl = hid_enumerate();
	int i;

	u("pcb-rnd Printed Circuit Board editing program, http://repo.hu/projects/pcb-rnd");
	u("For more information, please read the topic help pages:", Progname);
	u("  %s --help topic");
	u("Topics are:");
	u("  invocation           how to run pcb-rnd");
	u("  main                 main/misc flags (affecting none or all plugins)");
	for (i = 0; hl[i]; i++)
		if (hl[i]->usage != NULL)
			u("  %-20s %s\n", hl[i]->name, hl[i]->description);
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
	HID **hl = hid_enumerate();
	HID_AttrNode *ha;
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

int ActionPrintUsage(int argc, char **argv, Coord x, Coord y)
{
	u("");
	if (argc > 0) {
		HID **hl = hid_enumerate();
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
	}
	else
		help0();
}


/* --------------------------------------------------------------------------- */
static const char printversion_syntax[] = "PrintVersion()";

static const char printversion_help[] = "Print version.";

int ActionPrintVersion(int argc, char **argv, Coord x, Coord y)
{
	printf("PCB version %s\n", VERSION);
}

/* --------------------------------------------------------------------------- */
static const char printcopyright_syntax[] = "PrintCopyright()";

static const char printcopyright_help[] = "Print copyright notice.";

int ActionPrintCopyright(int argc, char **argv, Coord x, Coord y)
{
	printf("\n"
				 "                COPYRIGHT for the original pcb program:\n\n"
				 "    PCB, interactive printed circuit board design\n"
				 "    Copyright (C) 1994,1995,1996,1997 Thomas Nau\n"
				 "    Copyright (C) 1998, 1999, 2000 Harry Eaton\n\n"
				 "                COPYRIGHT for %s (pcb-rnd) version %s:\n"
				 "    pcb-rnd, a fork of PCB with random improvements\n"
				 "    Copyright (C) 2013, 2014, 2015, 2016 Tibor 'Igor2' Palinkas\n\n"
				 "    This program is free software; you can redistribute it and/or modify\n"
				 "    it under the terms of the GNU General Public License as published by\n"
				 "    the Free Software Foundation; either version 2 of the License, or\n"
				 "    (at your option) any later version.\n\n"
				 "    This program is distributed in the hope that it will be useful,\n"
				 "    but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
				 "    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
				 "    GNU General Public License for more details.\n\n"
				 "    You should have received a copy of the GNU General Public License\n"
				 "    along with this program; if not, write to the Free Software\n"
				 "    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n\n", Progname, VERSION);
}

/* --------------------------------------------------------------------------- */
static const char printpaths_syntax[] = "PrintPaths()";

static const char printpaths_help[] = "Print full paths and search paths.";

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

int ActionPrintPaths(int argc, char **argv, Coord x, Coord y)
{
	printf("bindir                   = \"%s\"\n", bindir);
	printf("pcblibdir                = \"%s\"\n", pcblibdir);
	printf("rc.default_font_file     ="); print_list(&conf_core.rc.default_font_file);
	printf("rc.library_search_paths  ="); print_list(&conf_core.rc.library_search_paths);
	printf("rc.library_shell         = \"%s\"\n", conf_core.rc.library_shell);
}


/* --------------------------------------------------------------------------- */

HID_Action main_action_list[] = {
	{"PrintActions", 0, ActionPrintActions,
	 printactions_help, printactions_syntax}
	,
	{"DumpActions", 0, ActionDumpActions,
	 dumpactions_help, dumpactions_syntax}
	,
	{"PrintUsage", 0, ActionPrintUsage,
	 printusage_help, printusage_syntax}
	,
	{"PrintVersion", 0, ActionPrintVersion,
	 printversion_help, printversion_syntax}
	,
	{"PrintCopyright", 0, ActionPrintCopyright,
	 printcopyright_help, printcopyright_syntax}
	,
	{"PrintPaths", 0, ActionPrintPaths,
	 printpaths_help, printpaths_syntax}
};

REGISTER_ACTIONS(main_action_list, NULL)
