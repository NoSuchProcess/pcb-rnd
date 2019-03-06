%{
/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,2006 Thomas Nau
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* lexical definitions to parse ASCII input of geda/pcb PCB and Element
   description */

#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <locale.h>

#if defined(_POSIX_SOURCE) || defined(_HPUX_SOURCE)
#include <unistd.h>
#endif

#include "flag_str.h"

#include "crosshair.h"
#include "data.h"
#include "error.h"
#include "file.h"
#include "flag_str.h"
#include "parse_common.h"
#include "parse_y.h"
#include "plug_footprint.h"
#include "attribs.h"
#include "compat_misc.h"
#include "compat_cc.h"
#include "obj_common.h"
#include "paths.h"
#include "safe_fs.h"
#include "macro.h"

#define YY_NO_INPUT

/* ---------------------------------------------------------------------------
 * some shared parser identifiers
 */
#ifdef FLEX_SCANNER

#define yyunput PCB_ATTRIBUTE_UNUSED yyunput
#endif

const char			*yyfilename;	/* in this file */
pcb_board_t *	yyPCB;			/* used by parser */
pcb_data_t *	yyData;
pcb_subc_t *yysubc;
pcb_coord_t yysubc_ox, yysubc_oy;
pcb_font_t *	yyFont;
conf_role_t yy_settings_dest;
pcb_flag_t yy_pcb_flags;
int *yyFontkitValid;
int yy_parse_tags;
pcb_bool yyFontReset;
int yyElemFixLayers = 0;

static int parse_number (void);
static void add_tag(char *line);

/* ---------------------------------------------------------------------------
 * an external prototypes
 */
int	yyparse(void);

static int Parse(FILE *Pipe, const char *Executable, const char *Path, const char *Filename);

%}

%option prefix="pcb_"


HEX				0x[0-9a-fA-F]+
INTEGER                 [+-]?([1-9][0-9]*|0)
FLOATING                {INTEGER}"."[0-9]*
STRINGCHAR		([^"\n\r\\]|\\.)

%option yylineno

%%

FileVersion	{ return(T_FILEVERSION); }
PCB			{ return(T_PCB); }
Grid		{ return(T_GRID); }
Cursor		{ return(T_CURSOR); }
Thermal		{ return(T_THERMAL); }
PolyArea	{ return(T_AREA); }
DRC		{ return(T_DRC); }
Flags		{ return(T_FLAGS); }
Layer		{ return(T_LAYER); }
Pin			{ return(T_PIN); }
Pad			{ return(T_PAD); }
Via			{ return(T_VIA); }
Line		{ return(T_LINE); }
Rat		{ return(T_RAT); }
Rectangle	{ return(T_RECTANGLE); }
Text		{ return(T_TEXT); }
ElementLine	{ return(T_ELEMENTLINE); }
ElementArc	{ return(T_ELEMENTARC); }
Element		{ return(T_ELEMENT); }
SymbolLine	{ return(T_SYMBOLLINE); }
Symbol		{ return(T_SYMBOL); }
Mark		{ return(T_MARK); }
Groups		{ return(T_GROUPS); }
Styles		{ return(T_STYLES); }
Polygon		{ return(T_POLYGON); }
Hole		{ return(T_POLYGON_HOLE); }
Arc		{ return(T_ARC); }
NetList		{ return(T_NETLIST); }
Net		{ return(T_NET); }
Connect		{ return(T_CONN); }
NetListPatch		{ return(T_NETLISTPATCH); }
add_conn		{ return(T_ADD_CONN); }
del_conn		{ return(T_DEL_CONN); }
change_attrib		{ return(T_CHANGE_ATTRIB); }
Attribute	{ return(T_ATTRIBUTE); }

nm	{ return T_NM; }
um	{ return T_UM; }
mm	{ return T_MM; }
m	{ return T_M; }
km	{ return T_KM; }
umil	{ return T_UMIL; }
cmil	{ return T_CMIL; }
mil	{ return T_MIL; }
in	{ return T_IN; }

\'.\'				{
						pcb_lval.integer = (unsigned) *(yytext+1);
						return(CHAR_CONST);
					}
{FLOATING}		{	return parse_number(); }
{INTEGER}		{	pcb_lval.integer = pcb_round (strtod (yytext, NULL)); return INTEGER; }

{HEX}			{	unsigned n;
				sscanf((char *) yytext, "%x", &n);
				pcb_lval.integer = n;
				return INTEGER;
					}
\"{STRINGCHAR}*\"	{
						char	*p1, *p2;

							/* return NULL on empty string */
						if (yyleng == 2)
						{
							pcb_lval.string = NULL;
							return(STRING);
						}

							/* allocate memory and copy string;
							 * stringlength is counted and copied without
							 * leading and trailing '"'
							 */
						yyleng -= 2;
						pcb_lval.string = (char *)calloc (yyleng+1, sizeof (char));
						p1 = (char *) (yytext +1);
						p2 = pcb_lval.string;
						while(yyleng--)
						{
								/* check for special character */
							if (*p1 == '\\')
							{
								yyleng--;
								p1++;

							}
							*p2++ = *p1++;
						}
						*p2 = '\0';
						return(STRING);
					}
##.*				{ if (yy_parse_tags) { add_tag(yytext); }}
#.*					{}
[ \t]+				{}
[\n]				{
#ifndef FLEX_SCANNER
						yylineno++;
#endif
					}
[\r]				{}
.					{ return(*yytext); }

%%

/* sets up the preprocessor command */
static int Parse(FILE *Pipe, const char *Executable, const char *Path, const char *Filename)
{
	static char *command = NULL;
	int returncode;
	int used_popen = 0;
	char *tmps;
	size_t l;
#ifdef FLEX_SCANNER
	static pcb_bool firsttime = pcb_true;
#endif
	pcb_io_pcb_usty_seen = 0;
	if (Pipe == NULL) {
		if (PCB_EMPTY_STRING_P(Executable)) {
			l = 2;
			if (Path != NULL)
				l += strlen(Path);

			l += strlen(Filename);

			if ((tmps = (char *)malloc(l * sizeof(char))) == NULL) {
				fprintf(stderr, "Parse():  malloc failed\n");
				exit(1);
			}

			if (Path != NULL && *Path != '\0')
				sprintf(tmps, "%s%s%s", Path, PCB_DIR_SEPARATOR_S, Filename);
			else
				sprintf(tmps, "%s", Filename);

			yyin = pcb_fopen(tmps, "r");
			if (!yyin) {
				free(tmps);
				return (1);
			}
			free(tmps);
		}
		else {
			pcb_build_argfn_t p;
			used_popen = 1;
			memset(&p, 0, sizeof(p));
			p.params['p' - 'a'] = Path;
			p.params['f' - 'a'] = Filename;
			command = pcb_build_argfn(Executable, &p);
			/* open pipe to stdout of command */
			if (*command == '\0' || (yyin = pcb_popen(command, "r")) == NULL) {
				pcb_popen_error_message(command);
				free(command);
				return (1);
			}
			free(command);
		}
	}
	else {
		yyin = Pipe;
	}

#ifdef FLEX_SCANNER
	/* reset parser if not called the first time */
	if (!firsttime)
		yyrestart(yyin);
	firsttime = pcb_false;
#endif

	/* init linenumber and filename for yyerror() */
	yylineno = 1;
	yyfilename = Filename;

	/* We need to save the data temporarily because lex-yacc are able
	 * to break the application if the input file has an illegal format.
	 * It's not necessary if the system supports the call of functions
	 * on termination.
	 */

	pcb_create_be_lenient(pcb_true);

#if !defined(HAS_ATEXIT)
	if (PCB && PCB->Data)
		pcb_tmp_data_save();
	returncode = pcb_parse();
	pcb_tmp_data_remove();
#else
	returncode = pcb_parse();
#endif
	/* clean up parse buffer */
	yy_delete_buffer(YY_CURRENT_BUFFER);

	pcb_create_be_lenient(pcb_false);

	if (Pipe != NULL)
		return returncode;

	if (used_popen)
		return (pcb_pclose(yyin) ? 1 : returncode);
	return (fclose(yyin) ? 1 : returncode);
}

/* ---------------------------------------------------------------------------
 * initializes LEX and calls parser for a single element file
 */
int io_pcb_ParseElement(pcb_plug_io_t *ctx, pcb_data_t *Ptr, const char *name)
{
	FILE *f;
	int ret;
	pcb_fp_fopen_ctx_t st;

	f = pcb_fp_fopen(pcb_fp_default_search_path(), name, &st, NULL);

	/* set up the yy globals only after the pcb_fp_fopen() because it can
	   also call Parse */
	yy_parse_tags = 0;
	yy_settings_dest = CFR_invalid;
	yyPCB = NULL;
	yyData = Ptr;
	pcb_data_set_layer_parents(Ptr);
	yyFont = pcb_font(PCB, 0, 1);
	yyFontReset = pcb_false;
	yyFontkitValid = NULL;
	yysubc = NULL;
	yyElemFixLayers = 1;


	if (f == NULL)
		return -1;

	ret = Parse(f, NULL,NULL,NULL);

	yyElemFixLayers = 0;

	pcb_fp_fclose(f, &st);

	return(ret);
}

/* ---------------------------------------------------------------------------
 * initializes LEX and calls parser for a complete board
 */
#define	TEST_FLAG_LOCAL(F,FLG)		(((FLG) & (F)) ? 1 : 0)
#define CONF_BOOL_FLAG(F,FLG)	(TEST_FLAG_LOCAL(F,FLG.f) ? "true" : "false")

/* Hack: set a no-save-attribute flag while loading some fields from the file;
   because we have all these flags set we won't need to list the paths that
   have hardwired flags again in a "don't save these in attributes" list. */
#define CONF_NO_ATTRIB(path) \
do { \
	conf_native_t *n = conf_get_field(path); \
	if (n != NULL) \
		n->random_flags.io_pcb_no_attrib = 1; \
} while(0) \

#define CONF_SET(target, path, arr_idx, new_val, pol) \
do { \
	CONF_NO_ATTRIB(path); \
	conf_set(target, path, arr_idx, new_val, pol); \
} while(0) \

int io_pcb_ParsePCB(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, conf_role_t settings_dest)
{
	int retval;
	const char *fcmd;
	yy_parse_tags = 0;
	yyPCB = Ptr;
	yyData = NULL;
	yyFont = NULL;
	yyFontReset = pcb_true;
	yyFontkitValid = NULL;
	yysubc = NULL;
	yy_settings_dest = settings_dest;

	if (settings_dest != CFR_invalid)
		conf_reset(settings_dest, Filename);

	io_pcb_preproc_board(PCB);

	pcb_data_clip_inhibit_inc(PCB->Data);
	fcmd = conf_core.rc.file_command;
	retval = Parse(NULL, fcmd, conf_core.rc.file_path, Filename);

	if ((settings_dest != CFR_invalid) && (retval == 0)) {
		/* overwrite settings from the flags, mark them not-to-save */
		CONF_SET(settings_dest, "plugins/mincut/enable", -1, CONF_BOOL_FLAG(PCB_ENABLEPCB_FLAG_MINCUT, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/show_number", -1, CONF_BOOL_FLAG(PCB_SHOWNUMBERFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/show_drc", -1, CONF_BOOL_FLAG(PCB_SHOWPCB_FLAG_DRC, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/rubber_band_mode", -1, CONF_BOOL_FLAG(PCB_RUBBERBANDFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/auto_drc", -1, CONF_BOOL_FLAG(PCB_AUTOPCB_FLAG_DRC, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/all_direction_lines", -1, CONF_BOOL_FLAG(PCB_ALLDIRECTIONFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/swap_start_direction", -1, CONF_BOOL_FLAG(PCB_SWAPSTARTDIRFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/unique_names", -1, CONF_BOOL_FLAG(PCB_UNIQUENAMEFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/clear_line", -1, CONF_BOOL_FLAG(PCB_CLEARNEWFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/full_poly", -1, CONF_BOOL_FLAG(PCB_NEWPCB_FLAG_FULLPOLY, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/snap_pin", -1, CONF_BOOL_FLAG(PCB_SNAPPCB_FLAG_PIN, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/orthogonal_moves", -1, CONF_BOOL_FLAG(PCB_ORTHOMOVEFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/live_routing", -1, CONF_BOOL_FLAG(PCB_LIVEROUTEFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/lock_names", -1, CONF_BOOL_FLAG(PCB_LOCKNAMESFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/only_names", -1, CONF_BOOL_FLAG(PCB_ONLYNAMESFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/hide_names", -1, CONF_BOOL_FLAG(PCB_HIDENAMESFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/thin_draw", -1, CONF_BOOL_FLAG(PCB_THINDRAWFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/thin_draw_poly", -1, CONF_BOOL_FLAG(PCB_THINDRAWPOLYFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/local_ref", -1, CONF_BOOL_FLAG(PCB_LOCALREFFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/check_planes", -1, CONF_BOOL_FLAG(PCB_CHECKPLANESFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/description", -1, CONF_BOOL_FLAG(PCB_DESCRIPTIONFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/name_on_pcb", -1, CONF_BOOL_FLAG(PCB_NAMEONPCBFLAG, yy_pcb_flags), POL_OVERWRITE);

		/* don't save this because it is saved manually as PCB::grid::unit */
		CONF_NO_ATTRIB("editor/grid_unit");

		/* don't save these to reduce noise - they are reset by the GUI anyway */
		CONF_NO_ATTRIB("editor/mode");

		/* it's saved in [styles] */
		CONF_NO_ATTRIB("design/routes");

		/* load config nodes not disabled above, from optional attributes */
		io_pcb_attrib_a2c(Ptr);

		conf_update(NULL, -1);
	}
	if (retval == 0) {
		/* restore loader so the next save will use the same units */
		const char *loader = pcb_attribute_get(&PCB->Attributes, "PCB::loader");
		if (loader != NULL) {
			pcb_find_io_t f;
			int len;

			len = pcb_find_io(&f, 1, PCB_IOT_PCB, 1, loader);
/*			printf("PCB::loader: %s -> %d\n", loader, len);*/
			if (len > 0) {
/*				printf("   ::loader: '%s'\n", f.plug->description);*/
				PCB->Data->loader = f.plug;
			}
		}
		else {
			/* Loaded a file that did not have any information about preferred format */
			switch((int)pcb_io_pcb_usty_seen) {
				case PCB_USTY_CMIL:
					pcb_message(PCB_MSG_INFO, "No preferred unit format info available for '%s' - will use '%s' on save (guessed from consistent unitless centimil input)\n", Filename, pcb_centimil_io_pcb->description);
					PCB->Data->loader = pcb_centimil_io_pcb;
					break;
				case PCB_USTY_NANOMETER:
					pcb_message(PCB_MSG_INFO, "No preferred unit format info available for '%s' - will use '%s' on save (guessed from consistent nanometer input)\n", Filename, pcb_nanometer_io_pcb->description);
					PCB->Data->loader = pcb_nanometer_io_pcb;
					break;
				case PCB_USTY_UNITS:
				case PCB_USTY_UNITS | PCB_USTY_NANOMETER:
					pcb_message(PCB_MSG_INFO, "No preferred unit format info available for '%s' - will use '%s' on save (guessed from non-nanometer explicit unit usage)\n", Filename, pcb_preferred_io_pcb->description);
					PCB->Data->loader = pcb_preferred_io_pcb;
					break;
				default:
					pcb_message(PCB_MSG_WARNING, "No preferred unit format info available and guessing failed for '%s' - will use '%s' on save\n", Filename, pcb_preferred_io_pcb->description);
					PCB->Data->loader = pcb_preferred_io_pcb;
			}
		}
	}
	pcb_layer_auto_fixup(PCB);
	io_pcb_postproc_board(PCB);
	pcb_data_clip_inhibit_dec(PCB->Data, 1);
	return retval;
}

/* ---------------------------------------------------------------------------
 * initializes LEX and calls parser for a font
 */
int io_pcb_ParseFont(pcb_plug_io_t *ctx, pcb_font_t *Ptr, const char *Filename)
{
	int r = 0, valid;
	const char *fcmd;
	yy_parse_tags = 1;
	yyPCB = NULL;
	yyFont = Ptr;
	yyFontkitValid = &valid;
	yysubc = NULL;
	yyFontReset = pcb_false;

	yy_settings_dest = CFR_invalid;
	fcmd = conf_core.rc.font_command;
	r = Parse(NULL, fcmd, NULL, Filename);
	if (r == 0) {
#ifdef DEBUG
		pcb_message(PCB_MSG_DEBUG, "Found %s in %s\n", Filename, conf_core.rc.font_command);
#endif
	}
	return r;
}

static int
parse_number ()
{
  pcb_lval.number = strtod ((char *) yytext, NULL);
  return FLOATING;
}

static void add_tag(char *line)
{
	char *key, *val;
	while((*line == '#') || isspace(*line)) line++;
	key = line;
	val = strchr(line, ':');
	if (val != NULL) {
		*val = '\0';
		val++;
	}
	if (yyFont != NULL) {
/*		printf("tag:%s=%s:\n", key,val); */
		if (strcmp(key, "name") == 0) {
			free(yyFont->name);
			yyFont->name = pcb_strdup(val);
		}
	}
}

