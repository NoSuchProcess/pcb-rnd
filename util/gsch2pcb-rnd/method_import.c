/* gsch2pcb-rnd
 *
 *  Original version: Bill Wilson    billw@wt.net
 *  rnd-version: (C) 2015..2016,2020 Tibor 'Igor2' Palinkas
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
#include "config.h"
#include "gsch2pcb.h"
#include "gsch2pcb_rnd_conf.h"
#include "method_import.h"
#include "run.h"
#include "netlister.h"
#include "method.h"
#include <librnd/core/conf.h>
#include "../src/conf_core.h"
#include <librnd/core/compat_misc.h>
#include <librnd/core/compat_fs.h>
#include <librnd/core/misc_util.h>
#include <librnd/core/hid_init.h>

char *cmd_file_name;
char *pcb_file_name;
char *net_file_name;

static void method_import_init(void)
{
	pcb_file_name = pcb_concat(conf_g2pr.utils.gsch2pcb_rnd.sch_basename, ".pcb", NULL);
	cmd_file_name = pcb_concat(conf_g2pr.utils.gsch2pcb_rnd.sch_basename, ".cmd", NULL);
	net_file_name = pcb_concat(conf_g2pr.utils.gsch2pcb_rnd.sch_basename, ".net", NULL);
	local_project_pcb_name = pcb_file_name;
}

static void import_go(int sep_net)
{
	char *verbose_str = NULL;
	const char *gnetlist, *backend;

	gnetlist = gnetlist_name();
	if (!conf_g2pr.utils.gsch2pcb_rnd.verbose)
		verbose_str = "-q";

	backend = sep_net ? "pcbrndfwd_elem" : "pcbrndfwd";
	require_gnetlist_backend(SCMDIR, backend);
	if (!build_and_run_command("%s %s -L %s -g %s -o %s %L %L", gnetlist, verbose_str, PCBLIBDIR, backend, cmd_file_name, &extra_gnetlist_arg_list, &schematics)) {
		fprintf(stderr, "Failed to run gnetlist with backend %s to generate the elements\n", backend);
		exit(1);
	}

	if (sep_net) {
		if (!build_and_run_command("%s %s -L %s -g PCB -o %s %L %L", gnetlist, verbose_str, PCBLIBDIR, net_file_name, &extra_gnetlist_arg_list, &schematics)) {
			fprintf(stderr, "Failed to run gnetlist net file\n");
			exit(1);
		}
	}

	if (conf_g2pr.utils.gsch2pcb_rnd.quiet_mode)
		return;

	/* Tell user what to do next */
	printf("\nNext step:\n");
	printf("1.  Run pcb-rnd on your board file (or on an empty board if it's the first time).\n");
	printf("2.  From within pcb-rnd, enter\n\n");
	printf("    :ExecuteFile(%s)\n\n", cmd_file_name);

	if (sep_net) {
		printf("    (this will update the elements)\n\n");
		printf("3.  From within pcb-rnd, select \"File -> Load netlist file\" and select \n");
		printf("    %s to load the updated netlist.\n\n", net_file_name);
	}
	else
		printf("    (this will update the elements and the netlist)\n\n");
}

static void method_import_go()
{
	import_go(0);
}

static void method_import_sep_go()
{
	import_go(1);
}


static int method_import_guess_out_name(void)
{
	int res;
	char *name;
	
	name = pcb_concat(conf_g2pr.utils.gsch2pcb_rnd.sch_basename, ".lht", NULL);
	res = rnd_file_readable(name);
	free(name);
	if (!res) {
		name = pcb_concat(conf_g2pr.utils.gsch2pcb_rnd.sch_basename, ".pcb.lht", NULL);
		res = rnd_file_readable(name);
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
	if (net_file_name != NULL)
		free(net_file_name);
}

static method_t method_import;
static method_t method_import_sep;

void method_import_register(void)
{
	method_import.name = "import";
	method_import.desc = "import schematics (pure action script)";
	method_import.init = method_import_init;
	method_import.go = method_import_go;
	method_import.uninit = method_import_uninit;
	method_import.guess_out_name = method_import_guess_out_name;
	method_import.not_by_guess = 0;
	method_register(&method_import);

	method_import_sep.name = "importsep";
	method_import_sep.desc = "import schematics (separate action script and netlist)";
	method_import_sep.init = method_import_init;
	method_import_sep.go = method_import_sep_go;
	method_import_sep.uninit = method_import_uninit;
	method_import_sep.guess_out_name = method_import_guess_out_name;
	method_import_sep.not_by_guess = 1;
	method_register(&method_import_sep);
}
