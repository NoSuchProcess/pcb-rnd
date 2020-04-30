/* gsch2pcb-rnd
 *
 *  Original version: Bill Wilson    billw@wt.net
 *  rnd-version: (C) 2015..2016, Tibor 'Igor2' Palinkas
 *
 *  This program is free software which I release under the GNU General Public
 *  License. You may redistribute and/or modify this program under the terms
 *  of that license as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.  Version 2 is in the
 *  COPYRIGHT file in the top level directory of this distribution.
 *
 *  To get a copy of the GNU General Puplic License, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

 * Retrieved from the official git (2015.07.15)

 Behavior different from the original:
  - use getenv() instead of g_getenv(): on windows this won't do recursive variable expansion
  - use rnd-specific .scm
  - use rnd_popen() instead of glib's spawn (stderr is always printed to stderr)
 */
#include "config.h"
#include "method.h"
#include <stdlib.h>
#include <stdio.h>


extern char *pcb_file_name, *pcb_new_file_name, *bak_file_name, *pins_file_name, *net_file_name;

static char *usage_string0a =
	"usage: gsch2pcb [options] {project | foo.sch [foo1.sch ...]}\n"
	"\n"
	"Generate a PCB annotation file from a set of gschem schematics.\n"
	"   gnetlist -g PCB is run to generate foo.net from the schematics.\n" "\n";
/* TODO */
/*  "   gnetlist -g gsch2pcb is run to get PCB elements which\n"
  "   match schematic footprints.  For schematic footprints which don't match\n"
  "   any PCB layout elements, search a set of file element directories in\n"
  "   an attempt to find matching PCB file elements.\n"*/
static char *usage_string0b =
	"   Output to foo.cmd if it doesn't exist.\n"
	"   The annotation command file (to be run by the user from pcb-rnd) contains:.\n"
	"   If any elements with a non-empty element name on the board\n"
	"   have no matching schematic component, then remove those elements from\n"
	"   the board unless it's marked nonetlist. Load and palce the footprints\n"
	"   for new elements; replace elements in-place if their footprint changed;"
	"   update attributes, value and other metadata in-place."
	"\n";
static char *usage_string0c =
	"   gnetlist -g pcbpins is run to get a PCB actions file which will rename all\n"
	"   of the pins in a .pcb file to match pin names from the schematic.\n"
	"\n"
	"   \"project\" is a file (not ending in .sch) containing a list of\n"
	"   schematics to process and some options.  A schematics line is like:\n"
	"       schematics foo1.sch foo2.sch ...\n"
	"   Options in a project file are like command line args without the \"-\":\n"
	"       output-name myproject\n"
	"\n"
	"options (may be included in a project file):\n";
static char *usage_string0d =
	"   -d, --elements-dir D    Search D for PCB file elements.  These defaults\n"
	"                           are searched if they exist. See the default\n"
	"                           search paths at the end of this text."
	"   -c, --elements-dir-clr  Clear the elements dir. Useful before a series\n"
	"                           if -d's to flush defaults.\n"
	"   -m, --method M          Use method M for the import. See available\n"
	"                           methods below.\n";
static char *usage_string0e =
	"   -s, --elements-shell S  Use S as a prefix for running parametric footrint\n"
	"                           generators. It is useful on systems where popen()\n"
	"                           doesn't do the right thing or the process should\n"
	"                           be wrapped. Example -s \"/bin/sh -c\"\n"
	"   -P, --default-pcb       Change the default PCB file's name; this file is\n"
	"                           inserted on top of the *.new.pcb generated, for\n"
	"                           PCB default settings\n";
static char *usage_string0f =
	"   -o, --output-name N     Use output file names N.net, N.pcb, and N.new.pcb\n"
	"                           instead of foo.net, ... where foo is the basename\n"
	"                           of the first command line .sch file.\n"
	"   -r, --remove-unfound    Don't include references to unfound elements in\n"
	"                           the generated .pcb files.  Use if you want PCB to\n"
	"                           be able to load the (incomplete) .pcb file.\n"
	"                           This is the default behavior.\n";
static char *usage_string0g =
	"   -k, --keep-unfound      Keep include references to unfound elements in\n"
	"                           the generated .pcb files.  Use if you want to hand\n"
	"                           edit or otherwise preprocess the generated .pcb file\n"
	"                           before running pcb.\n";
static char *usage_string0h =
	"   -p, --preserve          Preserve elements in PCB files which are not found\n"
	"                           in the schematics.  Note that elements with an empty\n"
	"                           element name (schematic refdes) are never deleted,\n"
	"                           so you really shouldn't need this option.\n";
static char *usage_string0i =
	"   -q, --quiet             Don't tell the user what to do next after running\n"
	"                           gsch2pcb-rnd.\n" "\n";

static char *usage_string1a =
	"   --gnetlist backend    A convenience run of extra gnetlist -g commands.\n"
	"                         Example:  gnetlist partslist3\n"
	"                         Creates:  myproject.partslist3\n"
	" --empty-footprint name  See the project.sample file.\n"
	"\n";
static char *usage_string1b =
	"options (not recognized in a project file):\n"
	"   --gnetlist-arg arg    Allows additional arguments to be passed to gnetlist.\n"
	"       --fix-elements    If a schematic component footprint is not equal\n"
	"                         to its PCB element Description, update the\n"
	"                         Description instead of replacing the element.\n"
	"                         Do this the first time gsch2pcb is used with\n"
	"                         PCB files originally created with gschem2pcb.\n";
static char *usage_string1c =
	"   -v, --verbose         Use -v -v for additional file element debugging.\n"
	"   -V, --version\n\n"
	"environment variables:\n"
	"   GNETLIST              If set, this specifies the name of the gnetlist program\n"
	"                         to execute.\n"
	"\n";

static char *usage_string_foot =
	"Additional Resources:\n"
	"\n"
	"  pcb-rnd homepage:     http://repo.hu/projects/pcb-rnd\n"
	"  gnetlist user guide:  http://wiki.geda-project.org/geda:gnetlist_ug\n"
	"  gEDA homepage:        http://www.geda-project.org\n"
	"\n";


void usage(void)
{
	method_t *m;
	printf("%s", usage_string0a);
	printf("%s", usage_string0b);
	printf("%s", usage_string0c);
	printf("%s", usage_string0d);
	printf("%s", usage_string0e);
	printf("%s", usage_string0f);
	printf("%s", usage_string0g);
	printf("%s", usage_string0h);
	printf("%s", usage_string0i);
	printf("%s", usage_string1a);
	printf("%s", usage_string1b);
	printf("%s", usage_string1c);

	printf("\nMethods available:\n");
	for(m = methods; m != NULL; m = m->next)
		printf("  %-12s %s\n", m->name, m->desc);
	printf("\n");
	printf("%s", usage_string_foot);
	exit(0);
}
