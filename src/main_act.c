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

#include "global.h"
#include "error.h"
#include "action_helper.h"
#include "hid_actions.h"
#include "hid_attrib.h"
#include "hid.h"
#include "data.h"


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

int ActionPrintUsage(int argc, char **argv, Coord x, Coord y)
{
	HID **hl = hid_enumerate();
	HID_AttrNode *ha;
	int i;
	int n_printer = 0, n_gui = 0, n_exporter = 0;

	for (i = 0; hl[i]; i++) {
		if (hl[i]->gui)
			n_gui++;
		if (hl[i]->printer)
			n_printer++;
		if (hl[i]->exporter)
			n_exporter++;
	}

	u("PCB Printed Circuit Board editing program, http://pcb.gpleda.org");
	u("%s [-h|-V|--copyright]\t\t\tHelp, version, copyright", Progname);
	u("%s [--gui GUI] [gui options] <pcb file>\t\tto edit", Progname);
	u("Available GUI hid%s:", n_gui == 1 ? "" : "s");
	for (i = 0; hl[i]; i++)
		if (hl[i]->gui)
			fprintf(stderr, "\t%-8s %s\n", hl[i]->name, hl[i]->description);
	u("%s -p [printing options] <pcb file>\tto print", Progname);
	u("Available printing hid%s:", n_printer == 1 ? "" : "s");
	for (i = 0; hl[i]; i++)
		if (hl[i]->printer)
			fprintf(stderr, "\t%-8s %s\n", hl[i]->name, hl[i]->description);
	u("%s -x hid [export options] <pcb file>\tto export", Progname);
	u("Available export hid%s:", n_exporter == 1 ? "" : "s");
	for (i = 0; hl[i]; i++)
		if (hl[i]->exporter)
			fprintf(stderr, "\t%-8s %s\n", hl[i]->name, hl[i]->description);

#warning TODO: two level help instead
	for (i = 0; hl[i]; i++)
		if (hl[i]->usage != NULL)
			hl[i]->usage();
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
};

REGISTER_ACTIONS(main_action_list, NULL)
