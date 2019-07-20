/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 *
 *  Old contact info:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */

#define Progname "pcb-rnd"

#include "config.h"

#include <stdio.h>
#include "config.h"

#include "undo.h"
#include "change.h"
#include "board.h"
#include "data.h"
#include "crosshair.h"
#include "compat_misc.h"
#include "layer.h"
#include "actions.h"
#include "hid_init.h"
#include "conf_core.h"
#include "plugins.h"
#include "build_run.h"
#include "file_loaded.h"
#include "safe_fs.h"
#include "flag_str.h"
#include "obj_common.h"
#include "hid_init.h"

static const char pcb_acts_PrintActions[] = "PrintActions()";
static const char pcb_acth_PrintActions[] = "Print all actions available.";
fgw_error_t pcb_act_PrintActions(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_print_actions();
	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_DumpActions[] = "DumpActions()";
static const char pcb_acth_DumpActions[] = "Dump all actions available.";
fgw_error_t pcb_act_DumpActions(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_dump_actions();
	PCB_ACT_IRES(0);
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
	u("For more information, please read the topic help pages:");
	u("  %s --help topic", Progname);
	u("Topics are:");
	u("  invocation           how to run pcb-rnd");
	u("  main                 main/misc flags (affecting none or all plugins)");
	for (i = 0; hl[i]; i++)
		if (hl[i]->usage != NULL)
			u("  %-20s %s", hl[i]->name, hl[i]->description);
	return 0;
}

extern const char *pcb_action_args[];
extern const int PCB_ACTION_ARGS_WIDTH;
static int help_main(void) {
	const char **cs;
	for(cs = pcb_action_args; cs[2] != NULL; cs += PCBHL_ACTION_ARGS_WIDTH) {
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

fgw_error_t pcb_act_PrintUsage(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *topic = NULL, *subt = NULL;
	PCB_ACT_MAY_CONVARG(1, FGW_STR, PrintUsage, topic = argv[1].val.str);
	PCB_ACT_IRES(0);

	u("");
	if (topic != NULL) {
		pcb_hid_t **hl = pcb_hid_enumerate();
		int i;

		if (strcmp(topic, "invocation") == 0)  return help_invoc();
		if (strcmp(topic, "main") == 0)        return help_main();

		for (i = 0; hl[i]; i++) {
			if ((hl[i]->usage != NULL) && (strcmp(topic, hl[i]->name) == 0)) {
				PCB_ACT_MAY_CONVARG(2, FGW_STR, PrintUsage, subt = argv[2].val.str);
				PCB_ACT_IRES(hl[i]->usage(hl[i], subt));
				return 0;
			}
		}
		fprintf(stderr, "No help available for %s\n", topic);
		PCB_ACT_IRES(-1);
		return 0;
	}
	else
		help0();
	return 0;
}


static const char pcb_acts_PrintVersion[] = "PrintVersion()";
static const char pcb_acth_PrintVersion[] = "Print version.";
fgw_error_t pcb_act_PrintVersion(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	printf("%s\n", pcb_get_info_program());
	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_DumpVersion[] = "DumpVersion()";
static const char pcb_acth_DumpVersion[] = "Dump version in script readable format.";
fgw_error_t pcb_act_DumpVersion(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	printf("%s\n", PCB_VERSION);
	printf("%s\n", PCB_REVISION);
	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_PrintCopyright[] = "PrintCopyright()";
static const char pcb_acth_PrintCopyright[] = "Print copyright notice.";
fgw_error_t pcb_act_PrintCopyright(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	printf("%s\n", pcb_get_info_copyright());
	printf("%s\n", pcb_get_info_license());

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
				 "    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.\n\n");
	PCB_ACT_IRES(0);
	return 0;
}

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
fgw_error_t pcb_act_PrintPaths(fgw_arg_t *res, int argc, fgw_arg_t *argv)
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
	PCB_ACT_IRES(0);
	return 0;
}


static const char pcb_acts_PrintFiles[] = "PrintFiles()";
static const char pcb_acth_PrintFiles[] = "Print files currently loaded.";
static void print_cat(pcb_file_loaded_t *cat)
{
	htsp_entry_t *e;
	printf("%s\n", cat->name);
	for (e = htsp_first(&cat->data.category.children); e; e = htsp_next(&cat->data.category.children, e)) {
		pcb_file_loaded_t *file = e->value;
		printf(" %s\t%s\t%s\n", file->name, file->data.file.path, file->data.file.desc);
	}
}
fgw_error_t pcb_act_PrintFiles(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	htsp_entry_t *e;
	printf("# Data files loaded\n");
	for (e = htsp_first(&pcb_file_loaded); e; e = htsp_next(&pcb_file_loaded, e))
		print_cat(e->value);
	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_DumpPlugins[] = "DumpPlugins()";
static const char pcb_acth_DumpPlugins[] = "Print plugins loaded in a format digestable by scripts.";
fgw_error_t pcb_act_DumpPlugins(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pup_plugin_t *p;
	const pup_buildin_t **bu;
	int n;

	printf("#state\tname\tbuildin\tautoload\trefco\tloaded_from\n");

	for(p = pcb_pup.plugins; p != NULL; p = p->next)
		printf("loaded\t%s\t%d\t%d\t%d\t%s\n",
			p->name,
			!!(p->flags & PUP_FLG_STATIC), !!(p->flags & PUP_FLG_AUTOLOAD), p->references,
			(p->path == NULL ? "<builtin>" : p->path));

	for(n = 0, bu = pcb_pup.bu; n < pcb_pup.bu_used; n++, bu++)
		if (pup_lookup(&pcb_pup, (*bu)->name) == NULL)
			printf("unloaded buildin\t%s\t1\t0\t0\t<builtin>\n", (*bu)->name);

	PCB_ACT_IRES(0);
	return 0;
}


static const char pcb_acts_DumpPluginDirs[] = "DumpPluginDirs()";
static const char pcb_acth_DumpPluginDirs[] = "Print plugins directories in a format digestable by scripts.";
fgw_error_t pcb_act_DumpPluginDirs(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char **p;
	for(p = pcb_pup_paths; *p != NULL; p++)
		printf("%s\n", *p);

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_DumpObjFlags[] = "DumpObjFlags()";
static const char pcb_acth_DumpObjFlags[] = "Print a script processable digest of all flags, per object type";
static void dumpoflg(void *ctx, unsigned long flg, const pcb_flag_bits_t *fb)
{
	printf("	%lx	%s	%s\n", flg, fb->name, fb->help);
}
fgw_error_t pcb_act_DumpObjFlags(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	unsigned long ot, max = PCB_OBJ_CLASS_REAL + 1;

	for(ot = 1; ot < max; ot <<= 1) {
		const char *name = pcb_obj_type_name(ot);

		if (*name == '<')
			continue;
		printf("%s\n", name);
		pcb_strflg_map(0x7fffffff, ot, NULL, dumpoflg);
	}

	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_System[] = "System(shell_cmd)";
static const char pcb_acth_System[] = "Run shell command";
fgw_error_t pcb_act_System(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char tmp[128];
	const char *cmd;

	PCB_ACT_CONVARG(1, FGW_STR, System, cmd = argv[1].val.str);
	PCB_ACT_IRES(0);

	pcb_setenv("PCB_RND_BOARD_FILE_NAME", PCB->hidlib.filename == NULL ? "" : PCB->hidlib.filename, 1);
	pcb_snprintf(tmp, sizeof(tmp), "%mm", pcb_crosshair.X);
	pcb_setenv("PCB_RND_CROSSHAIR_X_MM", tmp, 1);
	pcb_snprintf(tmp, sizeof(tmp), "%mm", pcb_crosshair.Y);
	pcb_setenv("PCB_RND_CROSSHAIR_Y_MM", tmp, 1);
	pcb_setenv("PCB_RND_CURRENT_LAYER_NAME", CURRENT->name, 1);
	PCB_ACT_IRES(pcb_system(&PCB->hidlib, cmd));
	return 0;
}

static const char pcb_acts_ExecuteFile[] = "ExecuteFile(filename)";
static const char pcb_acth_ExecuteFile[] = "Run actions from the given file.";
/* DOC: executefile.html */
fgw_error_t pcb_act_ExecuteFile(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	FILE *fp;
	const char *fname;
	char line[256];
	int n = 0;
	char *sp;

	PCB_ACT_CONVARG(1, FGW_STR, ExecuteFile, fname = argv[1].val.str);

	if ((fp = pcb_fopen(&PCB->hidlib, fname, "r")) == NULL) {
		fprintf(stderr, "Could not open actions file \"%s\".\n", fname);
		return 1;
	}

	defer_updates = 1;
	defer_needs_update = 0;
	while (fgets(line, sizeof(line), fp) != NULL) {
		n++;
		sp = line;

		/* eat the trailing newline */
		while (*sp && *sp != '\r' && *sp != '\n')
			sp++;
		*sp = '\0';

		/* eat leading spaces and tabs */
		sp = line;
		while (*sp && (*sp == ' ' || *sp == '\t'))
			sp++;

		/*
		 * if we have anything left and its not a comment line
		 * then execute it
		 */

		if (*sp && *sp != '#') {
			/*pcb_message("%s : line %-3d : \"%s\"\n", fname, n, sp); */
			pcb_parse_actions(sp);
		}
	}

	defer_updates = 0;
	if (defer_needs_update) {
		pcb_undo_inc_serial();
		pcb_gui->invalidate_all(pcb_gui, &PCB->hidlib);
	}
	fclose(fp);
	PCB_ACT_IRES(0);
	return 0;
}

pcb_action_t main_action_list[] = {
	{"PrintActions", pcb_act_PrintActions, pcb_acth_PrintActions, pcb_acts_PrintActions},
	{"DumpActions", pcb_act_DumpActions, pcb_acth_DumpActions, pcb_acts_DumpActions},
	{"PrintUsage", pcb_act_PrintUsage, pcb_acth_PrintUsage, pcb_acts_PrintUsage},
	{"PrintVersion", pcb_act_PrintVersion, pcb_acth_PrintVersion, pcb_acts_PrintVersion},
	{"DumpVersion", pcb_act_DumpVersion, pcb_acth_DumpVersion, pcb_acts_DumpVersion},
	{"PrintCopyright", pcb_act_PrintCopyright, pcb_acth_PrintCopyright, pcb_acts_PrintCopyright},
	{"PrintPaths", pcb_act_PrintPaths, pcb_acth_PrintPaths, pcb_acts_PrintPaths},
	{"PrintFiles", pcb_act_PrintFiles, pcb_acth_PrintFiles, pcb_acts_PrintFiles},
	{"DumpPlugins", pcb_act_DumpPlugins, pcb_acth_DumpPlugins, pcb_acts_DumpPlugins},
	{"DumpPluginDirs", pcb_act_DumpPluginDirs, pcb_acth_DumpPluginDirs, pcb_acts_DumpPluginDirs},
	{"DumpObjFlags", pcb_act_DumpObjFlags, pcb_acth_DumpObjFlags, pcb_acts_DumpObjFlags},
	{"System", pcb_act_System, pcb_acth_System, pcb_acts_System},
	{"ExecCommand", pcb_act_System, pcb_acth_System, pcb_acts_System},
	{"ExecuteFile", pcb_act_ExecuteFile, pcb_acth_ExecuteFile, pcb_acts_ExecuteFile}
};

PCB_REGISTER_ACTIONS(main_action_list, NULL)
