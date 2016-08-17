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
  - use popen() instead of glib's spawn (stderr is always printed to stderr)
 */
#include "config.h"

#include <stdlib.h>
#include <stdio.h>


extern char *pcb_file_name, *pcb_new_file_name, *bak_file_name, *pins_file_name, *net_file_name;

static char *usage_string0 =
	"usage: gsch2pcb [options] {project | foo.sch [foo1.sch ...]}\n"
	"\n"
	"Generate a PCB layout file from a set of gschem schematics.\n"
	"   gnetlist -g PCB is run to generate foo.net from the schematics.\n" "\n"
/* TODO */
/*  "   gnetlist -g gsch2pcb is run to get PCB elements which\n"
  "   match schematic footprints.  For schematic footprints which don't match\n"
  "   any PCB layout elements, search a set of file element directories in\n"
  "   an attempt to find matching PCB file elements.\n"*/
	"   Output to foo.pcb if it doesn't exist.  If there is a current foo.pcb,\n"
	"   output only new elements to foo.new.pcb.\n"
	"   If any elements with a non-empty element name in the current foo.pcb\n"
	"   have no matching schematic component, then remove those elements from\n"
	"   foo.pcb and rename foo.pcb to a foo.pcb.bak sequence.\n"
	"\n"
	"   gnetlist -g pcbpins is run to get a PCB actions file which will rename all\n"
	"   of the pins in a .pcb file to match pin names from the schematic.\n"
	"\n"
	"   \"project\" is a file (not ending in .sch) containing a list of\n"
	"   schematics to process and some options.  A schematics line is like:\n"
	"       schematics foo1.sch foo2.sch ...\n"
	"   Options in a project file are like command line args without the \"-\":\n"
	"       output-name myproject\n"
	"\n"
	"options (may be included in a project file):\n"
	"   -d, --elements-dir D    Search D for PCB file elements.  These defaults\n"
	"                           are searched if they exist. See the default\n"
	"                           search paths at the end of this text."
	"   -c, --elements-dir-clr  Clear the elements dir. Useful before a series\n"
	"                           if -d's to flush defaults.\n"
	"   -s, --elements-shell S  Use S as a prefix for running parametric footrint\n"
	"                           generators. It is useful on systems where popen()\n"
	"                           doesn't do the right thing or the process should\n"
	"                           be wrapped. Example -s \"/bin/sh -c\"\n"
	"   -P, --default-pcb       Change the default PCB file's name; this file is\n"
	"                           inserted on top of the *.new.pcb generated, for\n"
	"                           PCB default settings\n"
	"   -o, --output-name N     Use output file names N.net, N.pcb, and N.new.pcb\n"
	"                           instead of foo.net, ... where foo is the basename\n"
	"                           of the first command line .sch file.\n"
	"   -r, --remove-unfound    Don't include references to unfound elements in\n"
	"                           the generated .pcb files.  Use if you want PCB to\n"
	"                           be able to load the (incomplete) .pcb file.\n"
	"                           This is the default behavior.\n"
	"   -k, --keep-unfound      Keep include references to unfound elements in\n"
	"                           the generated .pcb files.  Use if you want to hand\n"
	"                           edit or otherwise preprocess the generated .pcb file\n"
	"                           before running pcb.\n"
	"   -p, --preserve          Preserve elements in PCB files which are not found\n"
	"                           in the schematics.  Note that elements with an empty\n"
	"                           element name (schematic refdes) are never deleted,\n"
	"                           so you really shouldn't need this option.\n"
	"   -q, --quiet             Don't tell the user what to do next after running\n"
	"                           gsch2pcb-rnd.\n" "\n";

static char *usage_string1 =
	"   --gnetlist backend    A convenience run of extra gnetlist -g commands.\n"
	"                         Example:  gnetlist partslist3\n"
	"                         Creates:  myproject.partslist3\n"
	" --empty-footprint name  See the project.sample file.\n"
	"\n"
	"options (not recognized in a project file):\n"
	"   --gnetlist-arg arg    Allows additional arguments to be passed to gnetlist.\n"
	"       --fix-elements    If a schematic component footprint is not equal\n"
	"                         to its PCB element Description, update the\n"
	"                         Description instead of replacing the element.\n"
	"                         Do this the first time gsch2pcb is used with\n"
	"                         PCB files originally created with gschem2pcb.\n"
	"   -v, --verbose         Use -v -v for additional file element debugging.\n"
	"   -V, --version\n\n"
	"environment variables:\n"
	"   GNETLIST              If set, this specifies the name of the gnetlist program\n"
	"                         to execute.\n"
	"\n"
	"Additional Resources:\n"
	"\n"
	"  gnetlist user guide:  http://wiki.geda-project.org/geda:gnetlist_ug\n"
	"  gEDA homepage:        http://www.geda-project.org\n"
	"  PCB homepage:         http://pcb.geda-project.org\n" "  pcb-rnd homepage:     http://repo.hu/projects/pcb-rnd\n" "\n";


void usage(void)
{
	puts(usage_string0);
	puts(usage_string1);
	exit(0);
}

void next_steps(int initial_pcb, int quiet_mode)
{
	if (initial_pcb) {
		printf("\nNext step:\n");
		printf("1.  Run pcb on your file %s.\n", pcb_file_name);
		printf("    You will find all your footprints in a bundle ready for you to place\n");
		printf("    or disperse with \"Select -> Disperse all elements\" in PCB.\n\n");
		printf("2.  From within PCB, select \"File -> Load netlist file\" and select \n");
		printf("    %s to load the netlist.\n\n", net_file_name);
		printf("3.  From within PCB, enter\n\n");
		printf("           :ExecuteFile(%s)\n\n", pins_file_name);
		printf("    to propagate the pin names of all footprints to the layout.\n\n");
	}
	else if (!quiet_mode) {
		printf("\nNext steps:\n");
		printf("1.  Run pcb on your file %s.\n", pcb_file_name);
		printf("2.  From within PCB, select \"File -> Load layout data to paste buffer\"\n");
		printf("    and select %s to load the new footprints into your existing layout.\n", pcb_new_file_name);
		printf("3.  From within PCB, select \"File -> Load netlist file\" and select \n");
		printf("    %s to load the updated netlist.\n\n", net_file_name);
		printf("4.  From within PCB, enter\n\n");
		printf("           :ExecuteFile(%s)\n\n", pins_file_name);
		printf("    to update the pin names of all footprints.\n\n");
	}
}
