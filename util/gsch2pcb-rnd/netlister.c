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

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*#include <ctype.h>*/
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../src/plug_footprint.h"
#include <librnd/core/paths.h>
#include <librnd/core/conf.h>
#include "../src/conf_core.h"
#include "../config.h"
#include <librnd/core/compat_misc.h>
#include "gsch2pcb_rnd_conf.h"
#include "gsch2pcb.h"
#include <librnd/core/misc_util.h>
#include "run.h"

const char *gnetlist_name(void)
{
	const char *gnetlist;
	/* Allow the user to specify a full path or a different name for
	 * the gnetlist command.  Especially useful if multiple copies
	 * are installed at once.
	 */
	gnetlist = getenv("GNETLIST");
	if (gnetlist == NULL)
		gnetlist = "gnetlist";
	return gnetlist;
}

int run_gnetlist(const char *pins_file, const char *net_file, const char *pcb_file, const char * basename, const gadl_list_t *largs)
{
	struct stat st;
	time_t mtime;
	const char *gnetlist;
	gadl_iterator_t it;
	char **sp;
	char *verbose_str = NULL;

	gnetlist = gnetlist_name();

	if (!conf_g2pr.utils.gsch2pcb_rnd.verbose)
		verbose_str = "-q";


	if (!build_and_run_command("%s %s -g pcbpins -o %s %L %L", gnetlist, verbose_str, pins_file, &extra_gnetlist_arg_list, largs))
		return FALSE;

	if (!build_and_run_command("%s %s -g PCB -o %s %L %L", gnetlist, verbose_str, net_file, &extra_gnetlist_arg_list, largs))
		return FALSE;

	mtime = (stat(pcb_file, &st) == 0) ? st.st_mtime : 0;

	require_gnetlist_backend(SCMDIR, "gsch2pcb-rnd");

	if (!build_and_run_command("%s %s -L " SCMDIR " -g gsch2pcb-rnd -o %s %L %L",
														 gnetlist, verbose_str, pcb_file, &extra_gnetlist_arg_list, largs)) {
		if (stat(pcb_file, &st) != 0 || mtime == st.st_mtime) {
			fprintf(stderr, "gsch2pcb: gnetlist command failed, `%s' not updated\n", pcb_file);
			return FALSE;
		}
		return FALSE;
	}

	gadl_foreach(&extra_gnetlist_list, &it, sp) {
		const char *s = *sp;
		const char *s2 = strstr(s, " -o ");
		char *out_file;
		char *backend;
		if (!s2) {
			out_file = rnd_concat(basename, ".", s, NULL);
			backend = rnd_strdup(s);
		}
		else {
			out_file = rnd_strdup(s2 + 4);
			backend = rnd_strndup(s, s2 - s);
		}

		if (!build_and_run_command("%s %s -g %s -o %s %L %L",
															 gnetlist, verbose_str, backend, out_file, &extra_gnetlist_arg_list, largs))
			return FALSE;
		free(out_file);
		free(backend);
	}

	return TRUE;
}
