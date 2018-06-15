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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 *
 *  Old contact info:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */
#include "config.h"
#include "conf_core.h"

#include "board.h"
#include "data.h"
#include "action_helper.h"
#include "change.h"
#include "error.h"
#include "undo.h"
#include "plugins.h"
#include "paths.h"

#include "compat_fs.h"
#include "pcb-printf.h"
#include "remove.h"
#include "rats.h"
#include "hid_actions.h"
#include "import_sch_conf.h"
#include "misc_util.h"
#include "compat_nls.h"
#include "compat_misc.h"
#include "obj_rat.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

conf_import_sch_t conf_import_sch;

extern int pcb_act_ExecuteFile(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y);

/* ---------------------------------------------------------------- */
static const char pcb_acts_Import[] =
	"Import()\n"
	"Import([gnetlist|make[,source,source,...]])\n" "Import(setnewpoint[,(mark|center|X,Y)])\n" "Import(setdisperse,D,units)\n";

static const char pcb_acth_Import[] = "Import schematics.";

/* %start-doc actions Import

Imports subcircuits and netlist data from the schematics (or some other
source).  The first parameter, which is optional, is the mode.  If not
specified, the @code{import::mode} attribute in the PCB is used.
@code{gnetlist} means gnetlist is used to obtain the information from
the schematics.  @code{make} invokes @code{make}, assuming the user
has a @code{Makefile} in the current directory.  The @code{Makefile}
will be invoked with the following variables set:

@table @code

@item PCB
The name of the .pcb file

@item SRCLIST
A space-separated list of source files

@item OUT
The name of the file in which to put the command script, which may
contain any @pcb{} actions.  By default, this is a temporary file
selected by @pcb{}, but if you specify an @code{import::outfile}
attribute, that file name is used instead (and not automatically
deleted afterwards).

@end table

The target specified to be built is the first of these that apply:

@itemize @bullet

@item
The target specified by an @code{import::target} attribute.

@item
The output file specified by an @code{import::outfile} attribute.

@item
If nothing else is specified, the target is @code{pcb_import}.

@end itemize

If you specify an @code{import::makefile} attribute, then "-f <that
file>" will be added to the command line.

If you specify the mode, you may also specify the source files
(schematics).  If you do not specify any, the list of schematics is
obtained by reading the @code{import::src@var{N}} attributes (like
@code{import::src0}, @code{import::src1}, etc).

For compatibility with future extensions to the import file format,
the generated file @emph{must not} start with the two characters
@code{#%}.

If a temporary file is needed the @code{TMPDIR} environment variable
is used to select its location.
*/

/*
Note that the programs @code{gnetlist} and @code{make} must be
configured.

If @pcb{} cannot determine which schematic(s) to import from, the GUI
is called to let user choose (see @code{ImportGUI()}).

Note that Import() doesn't delete anything - after an Import, subcircuits
which shouldn't be on the board are selected and may be removed once
it's determined that the deletion is appropriate.

If @code{Import()} is called with @code{setnewpoint}, then the location
of new components can be specified.  This is where parts show up when
they're added to the board.  The default is the center of the board.

@table @code

@item Import(setnewpoint)

Prompts the user to click on the board somewhere, uses that point.  If
called by a hotkey, uses the current location of the crosshair.

@item Import(setnewpoint,mark)

Uses the location of the mark.  If no mark is present, the point is
not changed.

@item Import(setnewpoint,center)

Resets the point to the center of the board.

@item Import(setnewpoint,X,Y,units)

Sets the point to the specific coordinates given.  Example:
@code{Import(setnewpoint,50,25,mm)}

@end table

Note that the X and Y locations are stored in attributes named
@code{import::newX} and @code{import::newY} so you could change them
manually if you wished.

Calling @code{Import(setdisperse,D,units)} sets how much the newly
placed subcircuits are dispersed relative to the set point.  For example,
@code{Import(setdisperse,10,mm)} will offset each part randomly up to
10mm away from the point.  The default dispersion is 1/10th of the
smallest board dimension.  Dispersion is saved in the
@code{import::disperse} attribute.

%end-doc */

static int pcb_act_Import(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *mode;
	const char **sources = NULL;
	int nsources = 0;

	if (conf_import_sch.plugins.import_sch.verbose)
		pcb_message(PCB_MSG_DEBUG, "pcb_act_Import:  ===  Entering pcb_act_Import  ===\n");


	mode = PCB_ACTION_ARG(0);

	if (mode && pcb_strcasecmp(mode, "setdisperse") == 0) {
		const char *ds, *units;
		char buf[50];

		ds = PCB_ACTION_ARG(1);
		units = PCB_ACTION_ARG(2);
		if (!ds) {
			const char *as = pcb_attrib_get(PCB, "import::disperse");
			ds = pcb_gui->prompt_for(_("Enter dispersion:"), as ? as : "0");
		}
		if (units) {
			sprintf(buf, "%s%s", ds, units);
			pcb_attrib_put(PCB, "import::disperse", buf);
		}
		else
			pcb_attrib_put(PCB, "import::disperse", ds);
		if (PCB_ACTION_ARG(1) == NULL)
			free((char*)ds);
		return 0;
	}

	if (mode && pcb_strcasecmp(mode, "setnewpoint") == 0) {
		const char *xs, *ys, *units;
		pcb_coord_t x, y;
		char buf[50];

		xs = PCB_ACTION_ARG(1);
		ys = PCB_ACTION_ARG(2);
		units = PCB_ACTION_ARG(3);

		if (!xs) {
			pcb_hid_get_coords(_("Click on a location"), &x, &y);
		}
		else if (pcb_strcasecmp(xs, "center") == 0) {
			pcb_attrib_remove(PCB, "import::newX");
			pcb_attrib_remove(PCB, "import::newY");
			return 0;
		}
		else if (pcb_strcasecmp(xs, "mark") == 0) {
			if (pcb_marked.status) {
				x = pcb_marked.X;
				y = pcb_marked.Y;
			}
		}
		else if (ys) {
			x = pcb_get_value(xs, units, NULL, NULL);
			y = pcb_get_value(ys, units, NULL, NULL);
		}
		else {
			pcb_message(PCB_MSG_ERROR, _("Bad syntax for Import(setnewpoint)"));
			return 1;
		}

		pcb_sprintf(buf, "%$ms", x);
		pcb_attrib_put(PCB, "import::newX", buf);
		pcb_sprintf(buf, "%$ms", y);
		pcb_attrib_put(PCB, "import::newY", buf);
		return 0;
	}

	if (!mode)
		mode = pcb_attrib_get(PCB, "import::mode");
	if (!mode)
		mode = "gnetlist";

	if (argc > 1) {
		sources = argv + 1;
		nsources = argc - 1;
	}

	if (!sources) {
		char sname[40];
		char *src;

		nsources = -1;
		do {
			nsources++;
			sprintf(sname, "import::src%d", nsources);
			src = pcb_attrib_get(PCB, sname);
		} while (src);

		if (nsources > 0) {
			sources = (const char **) malloc((nsources + 1) * sizeof(char *));
			nsources = -1;
			do {
				nsources++;
				sprintf(sname, "import::src%d", nsources);
				src = pcb_attrib_get(PCB, sname);
				sources[nsources] = src;
			} while (src);
		}
	}

	if (!sources) {
		/* Replace .pcb with .sch and hope for the best.  */
		char *pcbname = PCB->Filename;
		char *schname;
		char *dot, *slash, *bslash;

		if (!pcbname)
			return pcb_hid_action("ImportGUI");

		schname = (char *) malloc(strlen(pcbname) + 5);
		strcpy(schname, pcbname);
		dot = strchr(schname, '.');
		slash = strchr(schname, '/');
		bslash = strchr(schname, '\\');
		if (dot && slash && dot < slash)
			dot = NULL;
		if (dot && bslash && dot < bslash)
			dot = NULL;
		if (dot)
			*dot = 0;
		strcat(schname, ".sch");

		if (access(schname, F_OK))
			return pcb_hid_action("ImportGUI");

		sources = (const char **) malloc(2 * sizeof(char *));
		sources[0] = schname;
		sources[1] = NULL;
		nsources = 1;
	}

	if (pcb_strcasecmp(mode, "gnetlist") == 0) {
		char *tmpfile = pcb_tempfile_name_new("gnetlist_output");
		const char **cmd;
		int i;

		if (tmpfile == NULL) {
			pcb_message(PCB_MSG_ERROR, _("Could not create temp file"));
			return 1;
		}

		if ((conf_import_sch.plugins.import_sch.gnetlist_program == NULL) || (*conf_import_sch.plugins.import_sch.gnetlist_program == '\0')) {
			pcb_message(PCB_MSG_ERROR, _("No gnetlist program configured, can not import. Please fill in configuration setting plugins/import_sch/gnetlist_program\n"));
			return 1;
		}

		cmd = (const char **) malloc((9 + nsources) * sizeof(char *));
		cmd[0] = conf_import_sch.plugins.import_sch.gnetlist_program;
		cmd[1] = "-L";
		cmd[2] = PCBLIBDIR;
		cmd[3] = "-g";
		cmd[4] = "pcbrndfwd";
		cmd[5] = "-o";
		cmd[6] = tmpfile;
		cmd[7] = "--";
		for (i = 0; i < nsources; i++)
			cmd[8 + i] = pcb_build_fn(sources[i]);
		cmd[8 + nsources] = NULL;

		if (conf_import_sch.plugins.import_sch.verbose) {
			pcb_message(PCB_MSG_DEBUG, "pcb_act_Import:  running gnetlist:\n");
			for (i = 0; i < 8+nsources; i++)
				pcb_message(PCB_MSG_DEBUG, " %s", cmd[i]);
			pcb_message(PCB_MSG_DEBUG, "\n");
		}

		if (pcb_spawnvp(cmd)) {
			for(i = 0; i < nsources; i++)
				free((char *) cmd[8 + i]);
			unlink(tmpfile);
			return 1;
		}

		if (conf_import_sch.plugins.import_sch.verbose)
			pcb_message(PCB_MSG_DEBUG, "pcb_act_Import:  about to run pcb_act_ExecuteFile, file = %s\n", tmpfile);

		cmd[0] = tmpfile;
		cmd[1] = NULL;
		pcb_act_ExecuteFile(1, cmd, 0, 0);

		for(i = 0; i < nsources; i++)
			free((char *) cmd[8 + i]);
		free(cmd);
		pcb_tempfile_unlink(tmpfile);
	}
	else if (pcb_strcasecmp(mode, "make") == 0) {
		int must_free_tmpfile = 0;
		char *tmpfile;
		const char *cmd[10];
		int i;
		char *srclist;
		int srclen;
		char *user_outfile = NULL;
		char *user_makefile = NULL;
		char *user_target = NULL;


		user_outfile = pcb_attrib_get(PCB, "import::outfile");
		user_makefile = pcb_attrib_get(PCB, "import::makefile");
		user_target = pcb_attrib_get(PCB, "import::target");
		if (user_outfile && !user_target)
			user_target = user_outfile;

		if ((conf_import_sch.plugins.import_sch.make_program == NULL) || (*conf_import_sch.plugins.import_sch.make_program == '\0')) {
			pcb_message(PCB_MSG_ERROR, _("No make program configured, can not import. Please fill in configuration setting plugins/import_sch/make_program\n"));
			return 1;
		}

		if (user_outfile)
			tmpfile = user_outfile;
		else {
			tmpfile = pcb_tempfile_name_new("gnetlist_output");
			if (tmpfile == NULL) {
				pcb_message(PCB_MSG_ERROR, _("Could not create temp file"));
				return 1;
			}
			must_free_tmpfile = 1;
		}

		srclen = sizeof("SRCLIST=") + 2;
		for (i = 0; i < nsources; i++)
			srclen += strlen(sources[i]) + 2;
		srclist = (char *) malloc(srclen);
		strcpy(srclist, "SRCLIST=");
		for (i = 0; i < nsources; i++) {
			if (i)
				strcat(srclist, " ");
			strcat(srclist, sources[i]);
		}

		cmd[0] = conf_import_sch.plugins.import_sch.make_program;
		cmd[1] = "-s";
		cmd[2] = pcb_concat("PCB=", PCB->Filename, NULL);
		cmd[3] = srclist;
		cmd[4] = pcb_concat("OUT=", tmpfile, NULL);
		i = 5;
		if (user_makefile) {
			cmd[i++] = "-f";
			cmd[i++] = user_makefile;
		}
		cmd[i++] = user_target ? user_target : (char *) "pcb_import";
		cmd[i++] = NULL;

		if (pcb_spawnvp(cmd)) {
			if (must_free_tmpfile)
				unlink(tmpfile);
			free((char*)cmd[2]);
			free((char*)cmd[3]);
			free((char*)cmd[4]);
			return 1;
		}

		cmd[0] = tmpfile;
		cmd[1] = NULL;
		pcb_act_ExecuteFile(1, cmd, 0, 0);

		free((char*)cmd[2]);
		free((char*)cmd[3]);
		free((char*)cmd[4]);
		if (must_free_tmpfile)
			pcb_tempfile_unlink(tmpfile);
	}
	else {
		pcb_message(PCB_MSG_ERROR, _("Unknown import mode: %s\n"), mode);
		return 1;
	}

	pcb_rats_destroy(pcb_false);
	pcb_rat_add_all(pcb_false, NULL);

	if (conf_import_sch.plugins.import_sch.verbose)
		pcb_message(PCB_MSG_DEBUG, "pcb_act_Import:  ===  Leaving pcb_act_Import  ===\n");

	return 0;
}

static const char *import_sch_cookie = "import_sch plugin";

pcb_hid_action_t import_sch_action_list[] = {
	{"Import", 0, pcb_act_Import,
	 pcb_acth_Import, pcb_acts_Import}
};

PCB_REGISTER_ACTIONS(import_sch_action_list, import_sch_cookie)

int pplg_check_ver_import_sch(int ver_needed) { return 0; }

void pplg_uninit_import_sch(void)
{
	pcb_hid_remove_actions_by_cookie(import_sch_cookie);
	conf_unreg_fields("plugins/import_sch/");
}

#include "dolists.h"
int pplg_init_import_sch(void)
{
	char *tmp;

	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(import_sch_action_list, import_sch_cookie)
#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	conf_reg_field(conf_import_sch, field,isarray,type_name,cpath,cname,desc,flags);
#include "import_sch_conf_fields.h"

	/* Compatibility: get some settings from the env */
	tmp = getenv ("PCB_MAKE_PROGRAM");
	if (tmp != NULL)
		conf_set(CFR_ENV, "plugins/import_sch/make_program", -1, tmp, POL_OVERWRITE);

	tmp = getenv ("PCB_GNETLIST");
	if (tmp != NULL)
		conf_set(CFR_ENV, "plugins/import_sch/gnetlist_program", -1, tmp, POL_OVERWRITE);

	return 0;
}
