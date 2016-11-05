

%{
/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* lexical definitions to parse ASCII input of PCB and Element description
 */

/* for popen() */
#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <locale.h>

#if defined(_POSIX_SOURCE) || defined(_HPUX_SOURCE)
#include <unistd.h>
#endif

#include "flags.h"

#include "crosshair.h"
#include "data.h"
#include "error.h"
#include "file.h"
#include "misc.h"
#include "flag_str.h"
#include "parse_common.h"
#include "parse_y.h"
#include "plug_footprint.h"
#include "attribs.h"
#include "compat_misc.h"
#include "compat_cc.h"
#include "obj_common.h"

#define YY_NO_INPUT

/* ---------------------------------------------------------------------------
 * some shared parser identifiers
 */
#ifdef FLEX_SCANNER

#define yyunput ATTRIBUTE_UNUSED yyunput
#endif

const char			*yyfilename;	/* in this file */
PCBTypePtr		yyPCB;			/* used by parser */
DataTypePtr		yyData;
ElementTypePtr		yyElement;
FontTypePtr		yyFont;
conf_role_t yy_settings_dest;
FlagType yy_pcb_flags;

static int parse_number (void);

/* ---------------------------------------------------------------------------
 * an external prototypes
 */
int	yyparse(void);

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
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

/* ---------------------------------------------------------------------------
 * sets up the preprocessor command
 */
static int Parse(FILE *Pipe, const char *Executable, const char *Path, const char *Filename)
{
	static	char	*command = NULL;
	int		returncode;
	int		used_popen = 0;
	char *tmps;
	size_t l;
#ifdef FLEX_SCANNER
	static	pcb_bool	firsttime = pcb_true;
#endif

	if (Pipe == NULL) {
	if (EMPTY_STRING_P (Executable))
	  {
	    l = 2;
	    if ( Path != NULL )
              l += strlen (Path);

            l += strlen (Filename);

	    if ( (tmps = (char *) malloc ( l * sizeof (char))) == NULL)
              {
                fprintf (stderr, "Parse():  malloc failed\n");
                exit (1);
              }

	    if ( Path != NULL && *Path != '\0')
              sprintf (tmps, "%s%s%s", Path, PCB_DIR_SEPARATOR_S, Filename);
            else
              sprintf (tmps, "%s", Filename);

	    yyin = fopen (tmps, "r");
	    if (!yyin)
	      {
		return(1);
	      }
            free (tmps);
	  }
	else
	  {
	    used_popen = 1;

	    command = EvaluateFilename(Executable, Path, Filename);

	    /* open pipe to stdout of command */
	    if (*command == '\0' || (yyin = popen(command, "r")) == NULL)
	      {
		PopenErrorMessage(command);
		free(command);
		return(1);
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

	CreateBeLenient (pcb_true);

#if !defined(HAS_ATEXIT)
	if (PCB && PCB->Data)
	  SaveTMPData();
	returncode = pcb_parse();
	RemoveTMPData();
#else
	returncode = pcb_parse();
#endif
	/* clean up parse buffer */
	yy_delete_buffer(YY_CURRENT_BUFFER);

	CreateBeLenient (pcb_false);

	if (Pipe != NULL)
		return returncode;

	if (used_popen)
	  return(pclose(yyin) ? 1 : returncode);
	return(fclose(yyin) ? 1 : returncode);
}

/* ---------------------------------------------------------------------------
 * initializes LEX and calls parser for a single element file
 */
int io_pcb_ParseElement(plug_io_t *ctx, DataTypePtr Ptr, const char *name)
{
	FILE *f;
	int ret;
	fp_fopen_ctx_t st;

	yy_settings_dest = CFR_invalid;
	yyPCB = NULL;
	yyData = Ptr;
	yyFont = &PCB->Font;
	yyElement = NULL;

	f = fp_fopen(fp_default_search_path(), name, &st);

	if (f == NULL)
		return -1;

	ret = Parse(f, NULL,NULL,NULL);

	fp_fclose(f, &st);

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

int io_pcb_ParsePCB(plug_io_t *ctx, PCBTypePtr Ptr, const char *Filename, conf_role_t settings_dest)
{
	int retval;
	yyPCB = Ptr;
	yyData = NULL;
	yyFont = NULL;
	yyElement = NULL;
	yy_settings_dest = settings_dest;
	if (settings_dest != CFR_invalid)
		conf_reset(settings_dest, Filename);
	setlocale(LC_ALL, "C"); /* make sure numerics are read predictably */
	retval = Parse(NULL, conf_core.rc.file_command, conf_core.rc.file_path, Filename);
	setlocale(LC_ALL, "");
	if ((settings_dest != CFR_invalid) && (retval == 0)) {
		/* overwrite settings from the flags, mark them not-to-save */
		CONF_SET(settings_dest, "plugins/mincut/enable", -1, CONF_BOOL_FLAG(ENABLEPCB_FLAG_MINCUT, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/show_number", -1, CONF_BOOL_FLAG(SHOWNUMBERFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/show_drc", -1, CONF_BOOL_FLAG(SHOWPCB_FLAG_DRC, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/rubber_band_mode", -1, CONF_BOOL_FLAG(RUBBERBANDFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/auto_drc", -1, CONF_BOOL_FLAG(AUTOPCB_FLAG_DRC, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/all_direction_lines", -1, CONF_BOOL_FLAG(ALLDIRECTIONFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/swap_start_direction", -1, CONF_BOOL_FLAG(SWAPSTARTDIRFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/unique_names", -1, CONF_BOOL_FLAG(UNIQUENAMEFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/clear_line", -1, CONF_BOOL_FLAG(CLEARNEWFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/full_poly", -1, CONF_BOOL_FLAG(NEWPCB_FLAG_FULLPOLY, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/snap_pin", -1, CONF_BOOL_FLAG(SNAPPCB_FLAG_PIN, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/orthogonal_moves", -1, CONF_BOOL_FLAG(ORTHOMOVEFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/live_routing", -1, CONF_BOOL_FLAG(LIVEROUTEFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/lock_names", -1, CONF_BOOL_FLAG(LOCKNAMESFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/only_names", -1, CONF_BOOL_FLAG(ONLYNAMESFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/hide_names", -1, CONF_BOOL_FLAG(HIDENAMESFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/thin_draw", -1, CONF_BOOL_FLAG(THINDRAWFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/thin_draw_poly", -1, CONF_BOOL_FLAG(THINDRAWPOLYFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/local_ref", -1, CONF_BOOL_FLAG(LOCALREFFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/check_planes", -1, CONF_BOOL_FLAG(CHECKPLANESFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/description", -1, CONF_BOOL_FLAG(DESCRIPTIONFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/name_on_pcb", -1, CONF_BOOL_FLAG(NAMEONPCBFLAG, yy_pcb_flags), POL_OVERWRITE);
		CONF_SET(settings_dest, "editor/show_mask", -1, CONF_BOOL_FLAG(SHOWMASKFLAG, yy_pcb_flags), POL_OVERWRITE);

		/* don't save this because it is saved manually as PCB::grid::unit */
		CONF_NO_ATTRIB("editor/grid_unit");

		/* don't save these to reduce noise - they are reset by the GUI anyway */
		CONF_NO_ATTRIB("editor/mode");

		/* it's saved in [styles] */
		CONF_NO_ATTRIB("design/routes");

		/* load config nodes not disabled above, from optional attributes */
		io_pcb_attrib_a2c(Ptr);

		conf_update(NULL);
	}
	if (retval == 0) {
		/* restore loader so the next save will use the same units */
		const char *loader = AttributeGetFromList(&PCB->Attributes, "PCB::loader");
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
	}
	return retval;
}

/* ---------------------------------------------------------------------------
 * initializes LEX and calls parser for a font
 */
int io_pcb_ParseFont(plug_io_t *ctx, FontTypePtr Ptr, const char *Filename)
{
	int r = 0;
	yyPCB = NULL;
	yyFont = Ptr;
	yyElement = NULL;

	yy_settings_dest = CFR_invalid;
	r = Parse(NULL, conf_core.rc.font_command, NULL, Filename);
	if (r == 0) {
#ifdef DEBUG
		Message (PCB_MSG_DEBUG, "Found %s in %s\n", Filename, conf_core.rc.font_command);
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
