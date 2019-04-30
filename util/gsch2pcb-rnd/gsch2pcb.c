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
  - use pcb_popen() instead of glib's spawn (stderr is always printed to stderr)
 */

#include "config.h"

#define LOCAL_PROJECT_FILE "project.lht"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "../src/plug_footprint.h"
#include "../src/conf.h"
#include "../src/conf_core.h"
#include "../src/hidlib_conf.h"
#include "../src_3rd/qparse/qparse.h"
#include "../config.h"
#include "../src/error.h"
#include "../src/plugins.h"
#include "../src/compat_misc.h"
#include "../src/compat_fs.h"
#include "../src/misc_util.h"
#include "../src/safe_fs.h"
#include "method.h"
#include "help.h"
#include "gsch2pcb_rnd_conf.h"
#include "gsch2pcb.h"
#include "method_import.h"

static const char *want_method_default = "import";

gdl_list_t pcb_element_list; /* initialized to 0 */
gadl_list_t schematics, extra_gnetlist_arg_list, extra_gnetlist_list;

int n_deleted, n_added_ef, n_fixed, n_PKG_removed_new,
    n_PKG_removed_old, n_preserved, n_changed_value, n_not_found,
    n_unknown, n_none, n_empty;

int bak_done, need_PKG_purge;

conf_gsch2pcb_rnd_t conf_g2pr;

method_t *methods = NULL, *current_method;

void method_register(method_t *method)
{
	method->next = methods;
	methods = method;
}

method_t *method_find(const char *name)
{
	method_t *m;
	for(m = methods; m != NULL; m = m->next)
		if (strcmp(m->name, name) == 0)
			return m;
	return NULL;
}

/* Return a pointer to the suffix if inp ends in that suffix */
static const char *loc_str_has_suffix(const char *inp, const char *suffix, int suff_len)
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

static void add_schematic(const char *sch)
{
	char **n;
	n = gadl_new(&schematics);
	*n = pcb_strdup(sch);
	gadl_append(&schematics, n);
	if (!conf_g2pr.utils.gsch2pcb_rnd.sch_basename) {
		const char *suff = loc_str_has_suffix(sch, ".sch", 4);
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
			pcb_message(PCB_MSG_WARNING, "WARNING: using elements-dir from %s - this overrides the normal pcb-rnd configured library search paths\n", config);
			warned = 1;
		}
		conf_set(CFR_CLI, "rc/library_search_paths", -1, arg, POL_PREPEND);
	}
	else if (!strcmp(config, "output-name") || !strcmp(config, "o"))
		conf_set(CFR_CLI, "utils/gsch2pcb_rnd/sch_basename", -1, arg, POL_OVERWRITE);
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
	int n;

	f = pcb_fopen(path, "r");
	if (!f)
		return;

	/* check if we have a lihata project file or config file by looking at the first 16 non-comments */
	for(n = 0; n < 16;) {
		if (fgets(buf, sizeof(buf), f) == NULL)
			break;
		s = buf;
		while(isspace(*s)) s++;
		if ((*s == '#') || (*s == '\r') || (*s == '\n') || (*s == '\0'))
			continue;
		/* look for known lihata roots */
		if (strncmp(s, "ha:geda-project-v1", 18) == 0)
			goto lihata_prj;
		if (strncmp(s, "li:pcb-rnd-conf-v1", 18) == 0)
			goto lihata_prj;
		n++;
	}

	rewind(f);

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
	return;

lihata_prj:;
	fclose(f);
	if (conf_load_as(CFR_PROJECT, path, 0) != 0) {
		pcb_message(PCB_MSG_ERROR, "Failed to parse project file %s.\n", path);
		exit(1);
	}
	conf_update(NULL, -1); /* because of our new project file */
}

static void load_extra_project_files(void)
{
	char *path;
	static int done = FALSE;

	if (done)
		return;

	load_project("/etc/gsch2pcb");
	load_project("/usr/local/etc/gsch2pcb");

	path = pcb_concat(conf_core.rc.path.home, PCB_DIR_SEPARATOR_S, ".gEDA", PCB_DIR_SEPARATOR_S, "gsch2pcb", NULL);
	load_project(path);
	free(path);

	done = TRUE;
}

int have_cli_project_file = 0;
int have_cli_schematics = 0;
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
				char tmp[64];
				sprintf(tmp, "%ld", conf_g2pr.utils.gsch2pcb_rnd.verbose + 1);
				conf_set(CFR_CLI, "utils/gsch2pcb_rnd/verbose", -1, tmp, POL_OVERWRITE);
				continue;
			}
			else if (!strcmp(opt, "m") || !strcmp(opt, "method")) {
				if (method_find(arg) == NULL) {
					pcb_message(PCB_MSG_ERROR, "Error: can't use unknown method '%s'; try --help\n", arg);
					exit(1);
				}
				conf_set(CFR_CLI, "utils/gsch2pcb_rnd/method", -1, arg, POL_OVERWRITE);
				i++;
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
				if (have_cli_project_file) {
					pcb_message(PCB_MSG_ERROR, "ERROR: multiple project files specified on the command line (last one: %s). Either use multiple schematics or a single project file. Try %s --help\n", argv[i], argv[0]);
					exit(1);
				}
				load_extra_project_files();
				load_project(argv[i]);
				have_cli_project_file = 1;
			}
			else {
				add_schematic(argv[i]);
				have_cli_schematics = 1;
			}
		}
	}
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

void require_gnetlist_backend(const char *dir, const char *backend)
{
	char *path = pcb_strdup_printf("%s/gnet-%s.scm", dir, backend);
	if (!pcb_file_readable(path))
		pcb_message(PCB_MSG_WARNING, "WARNING: %s is not found, gnetlist will probably fail; please check your pcb-rnd installation!\n", path);
	free(path);
}


const char *local_project_pcb_name = NULL;

/************************ main ***********************/
int main(int argc, char ** argv)
{
	const char *want_method;

	method_import_register();

	conf_init();
	conf_core_init();
	conf_core_postproc(); /* to get all the paths initialized */
	pcb_hidlib_conf_init();

	gadl_list_init(&schematics, sizeof(char *), NULL, NULL);
	gadl_list_init(&extra_gnetlist_arg_list, sizeof(char *), NULL, NULL);
	gadl_list_init(&extra_gnetlist_list, sizeof(char *), NULL, NULL);

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(conf_g2pr, field,isarray,type_name,cpath,cname,desc,flags);
#include "gsch2pcb_rnd_conf_fields.h"

	get_args(argc, argv);

	conf_load_all(NULL, NULL);
	conf_update(NULL, -1);

	load_extra_project_files();
	conf_update(NULL, -1); /* because of CLI changes */

	if (!have_cli_project_file && !have_cli_schematics) {
		if (!pcb_file_readable(LOCAL_PROJECT_FILE)) {
			pcb_message(PCB_MSG_ERROR, "Don't know what to do: no project or schematics given, no local project file %s found. Try %s --help\n", LOCAL_PROJECT_FILE, argv[0]);
			exit(1);
		}
		if (conf_load_as(CFR_PROJECT, LOCAL_PROJECT_FILE, 0) != 0) {
			pcb_message(PCB_MSG_ERROR, "Failed to load project file %s. Try %s --help\n", LOCAL_PROJECT_FILE, argv[0]);
			exit(1);
		}
		conf_update(NULL, -1); /* because of our new project file */
	}
	else if ((local_project_pcb_name != NULL) && (!have_cli_project_file))
		conf_load_project(NULL, local_project_pcb_name);

	if (!have_cli_schematics) {  /* load all schematics from the project file unless we have schematics from the cli */
		conf_native_t *nat = conf_get_field("utils/gsch2pcb_rnd/schematics");
		if (nat != NULL) {
			conf_listitem_t *ci;
			for (ci = conflist_first(nat->val.list); ci != NULL; ci = conflist_next(ci)) {
				const char *p = ci->val.string[0];
				if (ci->type != CFN_STRING)
					continue;
				add_schematic(p);
			}
		}
		else {
			pcb_message(PCB_MSG_ERROR, "No schematics specified on the cli or in project files. Try %s --help\n", argv[0]);
			exit(1);
		}
	}

	if (gadl_length(&schematics) == 0) {
		pcb_message(PCB_MSG_ERROR, "No schematics specified on the cli; can't find any in the project files/configs either. Try %s --help\n", argv[0]);
		exit(1);
	}

	conf_update(NULL, -1); /* because of the project file */

	want_method = conf_g2pr.utils.gsch2pcb_rnd.method;
	if (want_method == NULL) {
		method_t *m;
		for(m = methods; m != NULL; m = m->next) {
			if (m->not_by_guess)
				continue;
			if (m->guess_out_name()) {
				current_method = m;
				break;
			}
		}
		if (current_method == NULL) {
			want_method = want_method_default;
			if (!conf_g2pr.utils.gsch2pcb_rnd.quiet_mode)
				pcb_message(PCB_MSG_WARNING, "Warning: method not specified for a project (that has no board or project file yet); defaulting to -m %s. This warning is harmless if you are running gsch2pcb-rnd for the first time on this project and you are fine with this method.", want_method);
		}
	}

	if (current_method == NULL) {
		current_method = method_find(want_method);
		if (current_method == NULL) {
			pcb_message(PCB_MSG_ERROR, "Error: can't find method %s\n", want_method);
			exit(1);
		}
	}

	current_method->init();
	conf_update(NULL, -1);


	current_method->go(); /* the traditional, "parse element and edit the pcb file" approach */

	current_method->uninit();

	conf_uninit();

	free_strlist(&schematics);
	free_strlist(&extra_gnetlist_arg_list);
	free_strlist(&extra_gnetlist_list);

	return 0;
}
