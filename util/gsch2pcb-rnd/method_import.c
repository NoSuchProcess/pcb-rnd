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
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "gsch2pcb.h"
#include "gsch2pcb_rnd_conf.h"
#include "method_pkg.h"
#include "method_pcb.h"
#include "run.h"
#include "netlister.h"
#include "method.h"
#include "../src/conf.h"
#include "../src/conf_core.h"
#include "../src/compat_misc.h"
#include "../src/compat_fs.h"

char *cmd_file_name;
char *pcb_file_name;

static void method_import_init(void)
{
	cmd_file_name = str_concat(NULL, conf_g2pr.utils.gsch2pcb_rnd.sch_basename, ".cmd", NULL);
	pcb_file_name = str_concat(NULL, conf_g2pr.utils.gsch2pcb_rnd.sch_basename, ".pcb", NULL);
	local_project_pcb_name = pcb_file_name;
}

static void method_import_go()
{
	char *verbose_str = NULL;
	const char *gnetlist;

	gnetlist = gnetlist_name();
	if (!conf_g2pr.utils.gsch2pcb_rnd.verbose)
		verbose_str = "-q";

	if (!build_and_run_command("%s %s -L %s -g pcbrndfwd -o %s %L %L", gnetlist, verbose_str, PCBLIBDIR, cmd_file_name, &extra_gnetlist_arg_list, &schematics)) {
		fprintf(stderr, "Failed to run gnetlist\n");
		exit(1);
	}

	/* Tell user what to do next */
	printf("\nNext step:\n");
	printf("1.  Run pcb-rnd on your board file (or on an empty board if it's the first time).\n");
	printf("2.  From within pcb-rnd, enter\n\n");
	printf("    :ExecuteFile(%s)\n\n", cmd_file_name);
}

static int method_import_guess_out_name(void)
{
	int res;
	char *name;
	
	name = str_concat(NULL, conf_g2pr.utils.gsch2pcb_rnd.sch_basename, ".lht", NULL);
	res = pcb_file_readable(name);
	free(name);
	if (!res) {
		name = str_concat(NULL, conf_g2pr.utils.gsch2pcb_rnd.sch_basename, ".pcb.lht", NULL);
		res = pcb_file_readable(name);
		free(name);
	}
	return res;
}

static void method_import_uninit(void)
{
	if (pcb_file_name != NULL)
		free(pcb_file_name);
	if (cmd_file_name != NULL)
		free(cmd_file_name);
}

static method_t method_import;

void method_import_register(void)
{
	method_import.name = "import";
	method_import.desc = "import schematics (pure action script variant)";
	method_import.init = method_import_init;
	method_import.go = method_import_go;
	method_import.uninit = method_import_uninit;
	method_import.guess_out_name = method_import_guess_out_name;
	method_register(&method_import);
}
