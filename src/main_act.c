/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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

#include "board.h"
#include "data.h"
#include "crosshair.h"
#include "actions_pcb.h"
#include <librnd/core/compat_misc.h>
#include "layer.h"
#include <librnd/core/actions.h>
#include "conf_core.h"
#include "build_run.h"
#include <librnd/core/safe_fs.h>
#include "flag_str.h"
#include "obj_common.h"
#include <librnd/core/hid_init.h>

#define PCB do_not_use_PCB

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
	rnd_hid_t **hl = rnd_hid_enumerate();
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
	for(cs = pcb_action_args; cs[2] != NULL; cs += RND_ACTION_ARGS_WIDTH) {
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
	rnd_hid_t **hl = rnd_hid_enumerate();
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
	u("-c conf/path=value        set the value of a configuration item (in RND_CFR_CLI)");
	u("-C conffile               load config file (as RND_CFR_CLI; after all -c's)");

	return 0;
}

fgw_error_t pcb_act_PrintUsage(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *topic = NULL, *subt = NULL;
	RND_ACT_MAY_CONVARG(1, FGW_STR, PrintUsage, topic = argv[1].val.str);
	RND_ACT_IRES(0);

	u("");
	if (topic != NULL) {
		rnd_hid_t **hl = rnd_hid_enumerate();
		int i;

		if (strcmp(topic, "invocation") == 0)  return help_invoc();
		if (strcmp(topic, "main") == 0)        return help_main();

		for (i = 0; hl[i]; i++) {
			if ((hl[i]->usage != NULL) && (strcmp(topic, hl[i]->name) == 0)) {
				RND_ACT_MAY_CONVARG(2, FGW_STR, PrintUsage, subt = argv[2].val.str);
				RND_ACT_IRES(hl[i]->usage(hl[i], subt));
				return 0;
			}
		}
		fprintf(stderr, "No help available for %s\n", topic);
		RND_ACT_IRES(-1);
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
	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_DumpVersion[] = "DumpVersion()";
static const char pcb_acth_DumpVersion[] = "Dump version in script readable format.";
fgw_error_t pcb_act_DumpVersion(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	printf("%s\n", PCB_VERSION);
	printf("%s\n", PCB_REVISION);
	RND_ACT_IRES(0);
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
	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_PrintPaths[] = "PrintPaths()";
static const char pcb_acth_PrintPaths[] = "Print full paths and search paths.";
static void print_list(const rnd_conflist_t *cl)
{
	int n;
	rnd_conf_listitem_t *ci;
	const char *p;

	printf(" ");
	rnd_conf_loop_list_str(cl, ci, p, n) {
		printf("%c%s", (n == 0) ? '"' : ':', p);
	}
	printf("\"\n");
}
fgw_error_t pcb_act_PrintPaths(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	htsp_entry_t *e;
	rnd_conf_fields_foreach(e) {
		rnd_conf_native_t *n = e->value;
		if ((strncmp(n->hash_path, "rc/path/", 8) == 0) && (n->type == RND_CFN_STRING) && (n->used == 1))
			printf("%-32s = %s\n", n->hash_path, n->val.string[0]);
	}
	printf("rc/default_font_file             ="); print_list(&conf_core.rc.default_font_file);
	printf("rc/library_search_paths          ="); print_list(&conf_core.rc.library_search_paths);
	printf("rc/library_shell                 = \"%s\"\n", conf_core.rc.library_shell);
	RND_ACT_IRES(0);
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

	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_System[] = "System(shell_cmd)";
static const char pcb_acth_System[] = "Run shell command";
fgw_error_t pcb_act_System(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char tmp[128];
	const char *cmd;

	RND_ACT_CONVARG(1, FGW_STR, System, cmd = argv[1].val.str);
	RND_ACT_IRES(0);

	rnd_setenv("PCB_RND_BOARD_FILE_NAME", RND_ACT_HIDLIB->filename == NULL ? "" : RND_ACT_HIDLIB->filename, 1);
	rnd_snprintf(tmp, sizeof(tmp), "%mm", pcb_crosshair.X);
	rnd_setenv("PCB_RND_CROSSHAIR_X_MM", tmp, 1);
	rnd_snprintf(tmp, sizeof(tmp), "%mm", pcb_crosshair.Y);
	rnd_setenv("PCB_RND_CROSSHAIR_Y_MM", tmp, 1);
	rnd_setenv("PCB_RND_CURRENT_LAYER_NAME", PCB_CURRLAYER(PCB_ACT_BOARD)->name, 1);
	RND_ACT_IRES(rnd_system(RND_ACT_HIDLIB, cmd));
	return 0;
}

static const char pcb_acts_ExecuteFile[] = "ExecuteFile(filename)";
static const char pcb_acth_ExecuteFile[] = "Run actions from the given file.";
/* DOC: executefile.html */
fgw_error_t pcb_act_ExecuteFile(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fname;

	RND_ACT_CONVARG(1, FGW_STR, ExecuteFile, fname = argv[1].val.str);
	RND_ACT_IRES(pcb_act_execute_file(RND_ACT_HIDLIB, fname));
	return 0;
}

static rnd_action_t main_action_list[] = {
	{"PrintUsage", pcb_act_PrintUsage, pcb_acth_PrintUsage, pcb_acts_PrintUsage},
	{"PrintVersion", pcb_act_PrintVersion, pcb_acth_PrintVersion, pcb_acts_PrintVersion},
	{"DumpVersion", pcb_act_DumpVersion, pcb_acth_DumpVersion, pcb_acts_DumpVersion},
	{"PrintCopyright", pcb_act_PrintCopyright, pcb_acth_PrintCopyright, pcb_acts_PrintCopyright},
	{"PrintPaths", pcb_act_PrintPaths, pcb_acth_PrintPaths, pcb_acts_PrintPaths},
	{"DumpObjFlags", pcb_act_DumpObjFlags, pcb_acth_DumpObjFlags, pcb_acts_DumpObjFlags},
	{"System", pcb_act_System, pcb_acth_System, pcb_acts_System},
	{"ExecCommand", pcb_act_System, pcb_acth_System, pcb_acts_System},
	{"ExecuteFile", pcb_act_ExecuteFile, pcb_acth_ExecuteFile, pcb_acts_ExecuteFile}
};

void pcb_main_act_init2(void)
{
	RND_REGISTER_ACTIONS(main_action_list, NULL);
}
