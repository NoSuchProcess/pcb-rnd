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
  - use pcb-rnd's conf system
  - use popen() instead of glib's spawn (stderr is always printed to stderr)
 */
#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../src/plug_footprint.h"
#include "../src/paths.h"
#include "../src/conf.h"
#include "../src/conf_core.h"
#include "../src_3rd/genvector/vts0.h"
#include "../src_3rd/genlist/gendlist.h"
#include "../src_3rd/genlist/genadlist.h"
#include "../src_3rd/qparse/qparse.h"
#include "../config.h"
#include "../src/error.h"
#include "../src/plugins.h"
#include "../src/compat_misc.h"
#include "../src/compat_fs.h"
#include "help.h"
#include "gsch2pcb_rnd_conf.h"
#include "gsch2pcb.h"
#include "netlister.h"
#include "run.h"
#include "fmt_pcb.h"
#include "fmt_pkg.h"


gdl_list_t pcb_element_list; /* initialized to 0 */
gadl_list_t schematics, extra_gnetlist_arg_list, extra_gnetlist_list;

int n_deleted, n_added_ef, n_fixed, n_PKG_removed_new,
    n_PKG_removed_old, n_preserved, n_changed_value, n_not_found,
    n_unknown, n_none, n_empty;

int bak_done, need_PKG_purge;

conf_gsch2pcb_rnd_t conf_g2pr;

/* Return a pointer to the suffix if inp ends in that suffix */
static char *loc_str_has_suffix(char *inp, const char *suffix, int suff_len)
{
	int len = strlen(inp);
	if ((len >= suff_len) && (strcmp(inp + len - suff_len, suffix) == 0))
		return inp + len - suff_len;
	return NULL;
}

char *fix_spaces(char * str)
{
	char *s;

	if (!str)
		return NULL;
	for (s = str; *s; ++s)
		if (*s == ' ' || *s == '\t')
			*s = '_';
	return str;
}

static void add_schematic(char * sch)
{
	char **n;
	n = gadl_new(&schematics);
	*n = pcb_strdup(sch);
	gadl_append(&schematics, n);
	if (!conf_g2pr.utils.gsch2pcb_rnd.sch_basename) {
		char *suff = loc_str_has_suffix(sch, ".sch", 4);
		if (suff != NULL) {
			char *tmp = pcb_strndup(sch, suff - sch);
			conf_set(CFR_CLI, "utils/gsch2pcb_rnd/sch_basename", -1, tmp, POL_OVERWRITE);
			free(tmp);
		}
	}
}

static void add_multiple_schematics(const char * sch)
{
	/* parse the string using shell semantics */
	int count;
	char **args;
	char *tmp = pcb_strdup(sch);

	count = qparse(tmp, &args);
	free(tmp);
	if (count > 0) {
		int i;
		for (i = 0; i < count; ++i)
			if (*args[i] != '\0')
				add_schematic(args[i]);
		qparse_free(count, &args);
	}
	else {
		fprintf(stderr, "invalid `schematics' option: %s\n", sch);
	}
}

static int parse_config(char * config, char * arg)
{
	char *s;

	/* remove trailing white space otherwise strange things can happen */
	if ((arg != NULL) && (strlen(arg) >= 1)) {
		s = arg + strlen(arg) - 1;
		while ((*s == ' ' || *s == '\t') && (s != arg))
			s--;
		s++;
		*s = '\0';
	}
	if (conf_g2pr.utils.gsch2pcb_rnd.verbose)
		printf("    %s \"%s\"\n", config, arg ? arg : "");

	if (!strcmp(config, "remove-unfound") || !strcmp(config, "r")) {
		/* This is default behavior set in header section */
		conf_set(CFR_CLI, "utils/gsch2pcb_rnd/remove_unfound_elements", -1, "1", POL_OVERWRITE);
		return 0;
	}
	if (!strcmp(config, "keep-unfound") || !strcmp(config, "k")) {
		conf_set(CFR_CLI, "utils/gsch2pcb_rnd/remove_unfound_elements", -1, "0", POL_OVERWRITE);
		return 0;
	}
	if (!strcmp(config, "quiet") || !strcmp(config, "q")) {
		conf_set(CFR_CLI, "utils/gsch2pcb_rnd/quiet_mode", -1, "1", POL_OVERWRITE);
		return 0;
	}
	if (!strcmp(config, "preserve") || !strcmp(config, "p")) {
		conf_set(CFR_CLI, "utils/gsch2pcb_rnd/preserve", -1, "1", POL_OVERWRITE);
		return 0;
	}
	if (!strcmp(config, "elements-shell") || !strcmp(config, "s"))
		conf_set(CFR_CLI, "rc/library_shell", -1, arg, POL_OVERWRITE);
	else if (!strcmp(config, "elements-dir") || !strcmp(config, "d")) {
		static int warned = 0;
		if (!warned) {
			fprintf(stderr, "WARNING: using elements-dir from %s - this overrides the normal pcb-rnd configured library search paths\n", config);
			warned = 1;
		}
		conf_set(CFR_CLI, "rc/library_search_paths", -1, arg, POL_PREPEND);
	}
	else if (!strcmp(config, "output-name") || !strcmp(config, "o"))
		conf_set(CFR_CLI, "utils/gsch2pcb_rnd/sch_base", -1, arg, POL_OVERWRITE);
	else if (!strcmp(config, "default-pcb") || !strcmp(config, "P"))
		conf_set(CFR_CLI, "utils/gsch2pcb_rnd/default_pcb", -1, arg, POL_OVERWRITE);
	else if (!strcmp(config, "schematics"))
		add_multiple_schematics(arg);
	else if (!strcmp(config, "gnetlist")) {
		char **n;
		n = gadl_new(&extra_gnetlist_list);
		*n = pcb_strdup(arg);
		gadl_append(&extra_gnetlist_list, n);
	}
	else if (!strcmp(config, "empty-footprint"))
		conf_set(CFR_CLI, "utils/gsch2pcb_rnd/empty_footprint_name", -1, arg, POL_OVERWRITE);
	else
		return -1;

	return 1;
}

static void load_project(char * path)
{
	FILE *f;
	char *s, buf[1024], config[32], arg[768];

	f = fopen(path, "r");
	if (!f)
		return;
	if (conf_g2pr.utils.gsch2pcb_rnd.verbose)
		printf("Reading project file: %s\n", path);
	while (fgets(buf, sizeof(buf), f)) {
		for (s = buf; *s == ' ' || *s == '\t' || *s == '\n'; ++s);
		if (!*s || *s == '#' || *s == '/' || *s == ';')
			continue;
		arg[0] = '\0';
		sscanf(s, "%31s %767[^\n]", config, arg);
		parse_config(config, arg);
	}
	fclose(f);
}

static void load_extra_project_files(void)
{
	char *path;
	static int done = FALSE;

	if (done)
		return;

	load_project("/etc/gsch2pcb");
	load_project("/usr/local/etc/gsch2pcb");

	path = str_concat(PCB_DIR_SEPARATOR_S, conf_core.rc.path.home, ".gEDA", "gsch2pcb", NULL);
	load_project(path);
	free(path);

	done = TRUE;
}

static void get_args(int argc, char ** argv)
{
	char *opt, *arg;
	int i, r;

	for (i = 1; i < argc; ++i) {
		opt = argv[i];
		arg = argv[i + 1];
		if (*opt == '-') {
			++opt;
			if (*opt == '-')
				++opt;
			if (!strcmp(opt, "version") || !strcmp(opt, "V")) {
				printf("gsch2pcb-rnd %s\n", GSCH2PCB_RND_VERSION);
				exit(0);
			}
			else if (!strcmp(opt, "verbose") || !strcmp(opt, "v")) {
				char tmp[16];
				sprintf(tmp, "%ld", conf_g2pr.utils.gsch2pcb_rnd.verbose + 1);
				conf_set(CFR_CLI, "utils/gsch2pcb_rnd/verbose", -1, tmp, POL_OVERWRITE);
				continue;
			}
			else if (!strcmp(opt, "c") || !strcmp(opt, "conf")) {
				const char *stmp;
				if (conf_set_from_cli(NULL, arg, NULL, &stmp) != 0) {
					fprintf(stderr, "Error: failed to set config %s: %s\n", arg, stmp);
					exit(1);
				}
				i++;
				continue;
			}
			else if (!strcmp(opt, "fix-elements")) {
				conf_set(CFR_CLI, "utils/gsch2pcb_rnd/fix_elements", -1, "1", POL_OVERWRITE);
				continue;
			}
			else if (!strcmp(opt, "gnetlist-arg")) {
				char **n;
				n = gadl_new(&extra_gnetlist_arg_list);
				*n = pcb_strdup(arg);
				gadl_append(&extra_gnetlist_arg_list, n);
				i++;
				continue;
			}
			else if (!strcmp(opt, "help") || !strcmp(opt, "h"))
				usage();
			else if (i < argc && ((r = parse_config(opt, (i < argc - 1) ? arg : NULL))
														>= 0)
				) {
				i += r;
				continue;
			}
			printf("gsch2pcb: bad or incomplete arg: %s\n", argv[i]);
			usage();
		}
		else {
			if (loc_str_has_suffix(argv[i], ".sch", 4) == NULL) {
				load_extra_project_files();
				load_project(argv[i]);
			}
			else
				add_schematic(argv[i]);
		}
	}
}

void pcb_plugin_register(const char *name, const char *path, void *handle, int dynamic_loaded, void (*uninit)(void))
{
	if (conf_g2pr.utils.gsch2pcb_rnd.verbose)
		printf("Plugin loaded: %s (%s)\n", name, path);
}

void free_strlist(gadl_list_t *lst)
{
	char **s;

	while((s = gadl_first(lst)) != NULL) {
		char *str = *s;
		gadl_free(s);
		free(str);
	}
}

#include "fp_init.h"

/************************ main ***********************/
char *pcb_file_name, *pcb_new_file_name, *bak_file_name, *pins_file_name, *net_file_name;
int main(int argc, char ** argv)
{
	int i;
	int initial_pcb = TRUE;
	int created_pcb_file = TRUE;

	if (argc < 2)
		usage();


	conf_init();
	conf_core_init();


	gadl_list_init(&schematics, sizeof(char *), NULL, NULL);
	gadl_list_init(&extra_gnetlist_arg_list, sizeof(char *), NULL, NULL);
	gadl_list_init(&extra_gnetlist_list, sizeof(char *), NULL, NULL);

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(conf_g2pr, field,isarray,type_name,cpath,cname,desc,flags);
#include "gsch2pcb_rnd_conf_fields.h"

	get_args(argc, argv);

	conf_load_all(NULL, NULL);
	conf_update(NULL);

	{
		pcb_uninit_t uninit_func;
#		include "fp_init.c"
	}

	load_extra_project_files();

	conf_update(NULL); /* because of CLI changes */

	pcb_fp_init();

	if (gadl_length(&schematics) == 0)
		usage();

	pins_file_name = str_concat(NULL, conf_g2pr.utils.gsch2pcb_rnd.sch_basename, ".cmd", NULL);
	net_file_name = str_concat(NULL, conf_g2pr.utils.gsch2pcb_rnd.sch_basename, ".net", NULL);
	pcb_file_name = str_concat(NULL, conf_g2pr.utils.gsch2pcb_rnd.sch_basename, ".pcb", NULL);

	conf_load_project(NULL, pcb_file_name);
	fmt_pcb_init();
	conf_update(NULL); /* because of the project file */

	{ /* set bak_file_name, finding the first number that results in a non-existing bak */
		int len;
		char *end;

		len = strlen(conf_g2pr.utils.gsch2pcb_rnd.sch_basename);
		bak_file_name = malloc(len+8+64); /* make room for ".pcb.bak" and an integer */
		memcpy(bak_file_name, conf_g2pr.utils.gsch2pcb_rnd.sch_basename, len);
		end = bak_file_name + len;
		strcpy(end, ".pcb.bak");
		end += 8;

		for (i = 0; pcb_file_readable(bak_file_name); ++i)
			sprintf(end, "%d", i);
	}

	if (pcb_file_readable(pcb_file_name)) {
		initial_pcb = FALSE;
		pcb_new_file_name = str_concat(NULL, conf_g2pr.utils.gsch2pcb_rnd.sch_basename, ".new.pcb", NULL);
		get_pcb_element_list(pcb_file_name);
	}
	else
		pcb_new_file_name = pcb_strdup(pcb_file_name);

	if (!run_gnetlist(pins_file_name, net_file_name, pcb_new_file_name, conf_g2pr.utils.gsch2pcb_rnd.sch_basename, &schematics)) {
		fprintf(stderr, "Failed to run gnetlist\n");
		exit(1);
	}

	if (add_elements(pcb_new_file_name) == 0) {
		build_and_run_command("rm %s", pcb_new_file_name);
		if (initial_pcb) {
			printf("No elements found, so nothing to do.\n");
			exit(0);
		}
	}

	if (conf_g2pr.utils.gsch2pcb_rnd.fix_elements)
		update_element_descriptions(pcb_file_name, bak_file_name);
	prune_elements(pcb_file_name, bak_file_name);

	/* Report work done during processing */
	if (conf_g2pr.utils.gsch2pcb_rnd.verbose)
		printf("\n");
	printf("\n----------------------------------\n");
	printf("Done processing.  Work performed:\n");
	if (n_deleted > 0 || n_fixed > 0 || need_PKG_purge || n_changed_value > 0)
		printf("%s is backed up as %s.\n", pcb_file_name, bak_file_name);
	if (pcb_element_list.length && n_deleted > 0)
		printf("%d elements deleted from %s.\n", n_deleted, pcb_file_name);

	if (n_added_ef > 0)
		printf("%d file elements added to %s.\n", n_added_ef, pcb_new_file_name);
	else if (n_not_found == 0) {
		printf("No elements to add so not creating %s\n", pcb_new_file_name);
		created_pcb_file = FALSE;
	}

	if (n_not_found > 0) {
		printf("%d not found elements added to %s.\n", n_not_found, pcb_new_file_name);
	}
	if (n_unknown > 0)
		printf("%d components had no footprint attribute and are omitted.\n", n_unknown);
	if (n_none > 0)
		printf("%d components with footprint \"none\" omitted from %s.\n", n_none, pcb_new_file_name);
	if (n_empty > 0)
		printf("%d components with empty footprint \"%s\" omitted from %s.\n", n_empty, conf_g2pr.utils.gsch2pcb_rnd.empty_footprint_name, pcb_new_file_name);
	if (n_changed_value > 0)
		printf("%d elements had a value change in %s.\n", n_changed_value, pcb_file_name);
	if (n_fixed > 0)
		printf("%d elements fixed in %s.\n", n_fixed, pcb_file_name);
	if (n_PKG_removed_old > 0) {
		printf("%d elements could not be found.", n_PKG_removed_old);
		if (created_pcb_file)
			printf("  So %s is incomplete.\n", pcb_file_name);
		else
			printf("\n");
	}
	if (n_PKG_removed_new > 0) {
		printf("%d elements could not be found.", n_PKG_removed_new);
		if (created_pcb_file)
			printf("  So %s is incomplete.\n", pcb_new_file_name);
		else
			printf("\n");
	}
	if (n_preserved > 0)
		printf("%d elements not in the schematic preserved in %s.\n", n_preserved, pcb_file_name);

	/* Tell user what to do next */
	if (conf_g2pr.utils.gsch2pcb_rnd.verbose)
		printf("\n");

	if (n_added_ef > 0)
		next_steps(initial_pcb, conf_g2pr.utils.gsch2pcb_rnd.quiet_mode);

	free(net_file_name);
	free(pins_file_name);
	free(pcb_file_name);
	free(bak_file_name);

	conf_uninit();

	free_strlist(&schematics);
	free_strlist(&extra_gnetlist_arg_list);
	free_strlist(&extra_gnetlist_list);

	if (pcb_new_file_name != NULL)
		free(pcb_new_file_name);

	return 0;
}
