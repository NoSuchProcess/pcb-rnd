%{
/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2017, 2018 Tibor 'Igor2' Palinkas
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

/* grammar to parse ASCII input of geda/PCB description (alien format) */

#include "config.h"
#include "flag.h"
#include "board.h"
#include "conf_core.h"
#include "layer.h"
#include "data.h"
#include "error.h"
#include "file.h"
#include "parse_l.h"
#include "polygon.h"
#include "remove.h"
#include "rtree.h"
#include "flag_str.h"
#include "obj_pinvia_therm.h"
#include "rats_patch.h"
#include "route_style.h"
#include "compat_misc.h"
#include "src_plugins/lib_compat_help/pstk_compat.h"
#include "netlist2.h"

/* frame between the groundplane and the copper or mask - noone seems
   to remember what these two are for; changing them may have unforeseen
   side effects. */
#define PCB_GROUNDPLANEFRAME        PCB_MIL_TO_COORD(15)
#define PCB_MASKFRAME               PCB_MIL_TO_COORD(3)

static	pcb_layer_t *Layer;
static	pcb_poly_t *Polygon;
static	pcb_symbol_t *Symbol;
static	int		pin_num;
static pcb_net_t *currnet;
static	pcb_bool			LayerFlag[PCB_MAX_LAYER + 2];
static	int	old_fmt; /* 1 if we are reading a PCB(), 0 if PCB[] */
static	unsigned char yy_intconn;

extern	char			*yytext;		/* defined by LEX */
extern	pcb_board_t *	yyPCB;
extern	pcb_data_t *	yyData;
extern	pcb_subc_t *yysubc;
extern	pcb_coord_t yysubc_ox, yysubc_oy;
extern	pcb_font_t *	yyFont;
extern	pcb_bool	yyFontReset;
extern	int				pcb_lineno;		/* linenumber */
extern	char			*yyfilename;	/* in this file */
extern	conf_role_t yy_settings_dest;
extern pcb_flag_t yy_pcb_flags;
extern int *yyFontkitValid;
extern int yyElemFixLayers;

static char *layer_group_string;

static pcb_attribute_list_t *attr_list;

int yyerror(const char *s);
int yylex();
static int check_file_version (int);

static void do_measure (PLMeasure *m, pcb_coord_t i, double d, int u);
#define M(r,f,d) do_measure (&(r), f, d, 1)

/* Macros for interpreting what "measure" means - integer value only,
   old units (mil), or new units (cmil).  */
#define IV(m) integer_value (m)
#define OU(m) old_units (m)
#define NU(m) new_units (m)

static int integer_value (PLMeasure m);
static pcb_coord_t old_units (PLMeasure m);
static pcb_coord_t new_units (PLMeasure m);
static pcb_flag_t pcb_flag_old(unsigned int flags);
static void load_meta_coord(const char *path, pcb_coord_t crd);
static void load_meta_float(const char *path, double val);

#define YYDEBUG 1
#define YYERROR_VERBOSE 1

#include "parse_y.h"

%}

%name-prefix "pcb_"

%verbose

%union									/* define YYSTACK type */
{
	int		integer;
	double		number;
	char		*string;
	pcb_flag_t	flagtype;
	PLMeasure	measure;
}

%token	<number>	FLOATING		/* line thickness, coordinates ... */
%token	<integer>	INTEGER	CHAR_CONST	/* flags ... */
%token	<string>	STRING			/* element names ... */

%token	T_FILEVERSION T_PCB T_LAYER T_VIA T_RAT T_LINE T_ARC T_RECTANGLE T_TEXT T_ELEMENTLINE
%token	T_ELEMENT T_PIN T_PAD T_GRID T_FLAGS T_SYMBOL T_SYMBOLLINE T_CURSOR
%token	T_ELEMENTARC T_MARK T_GROUPS T_STYLES T_POLYGON T_POLYGON_HOLE T_NETLIST T_NET T_CONN
%token	T_NETLISTPATCH T_ADD_CONN T_DEL_CONN T_CHANGE_ATTRIB
%token	T_AREA T_THERMAL T_DRC T_ATTRIBUTE
%token	T_UMIL T_CMIL T_MIL T_IN T_NM T_UM T_MM T_M T_KM
%type	<integer>	symbolid
%type	<string>	opt_string
%type	<flagtype>	flags
%type	<number>	number
%type	<measure>	measure

%%

parse
		: parsepcb
		| parsedata
		| parsefont
		| error { YYABORT; }
		;

parsepcb
		:	{
					/* reset flags for 'used layers';
					 * init font and data pointers
					 */
				int	i;

				if (!yyPCB)
				{
					pcb_message(PCB_MSG_ERROR, "illegal fileformat\n");
					YYABORT;
				}
				for (i = 0; i < PCB_MAX_LAYER + 2; i++)
					LayerFlag[i] = pcb_false;
				yyFont = &yyPCB->fontkit.dflt;
				yyFontkitValid = &yyPCB->fontkit.valid;
				yyData = yyPCB->Data;
				PCB_SET_PARENT(yyData, board, yyPCB);
				yyData->LayerN = 0;
				yyPCB->NetlistPatches = yyPCB->NetlistPatchLast = NULL;
				layer_group_string = NULL;
				old_fmt = 0;
			}
		  pcbfileversion
		  pcbname
		  pcbgrid
		  pcbcursor
		  polyarea
		  pcbthermal
		  pcbdrc
		  pcbflags
		  pcbgroups
		  pcbstyles
		  pcbfont
		  pcbdata
		  pcbnetlist
		  pcbnetlistpatch
			{
			  pcb_board_t *pcb_save = PCB;
			  if ((yy_settings_dest != CFR_invalid) && (layer_group_string != NULL))
					conf_set(yy_settings_dest, "design/groups", -1, layer_group_string, POL_OVERWRITE);
			  pcb_board_new_postproc(yyPCB, 0);
			  if (layer_group_string == NULL) {
			     if (pcb_layer_improvise(yyPCB, pcb_true) != 0) {
			        pcb_message(PCB_MSG_ERROR, "missing layer-group string, failed to improvise the groups\n");
			        YYABORT;
			     }
			     pcb_message(PCB_MSG_ERROR, "missing layer-group string: invalid input file, had to improvise, the layer stack is most probably broken\n");
			  }
			  else {
			    if (pcb_layer_parse_group_string(yyPCB, layer_group_string, yyData->LayerN, old_fmt))
			    {
			      pcb_message(PCB_MSG_ERROR, "illegal layer-group string\n");
			      YYABORT;
			    }
			    else {
			     if (pcb_layer_improvise(yyPCB, pcb_false) != 0) {
			        pcb_message(PCB_MSG_ERROR, "failed to extend-improvise the groups\n");
			        YYABORT;
			     }
			    }
			  }
			/* initialize the polygon clipping now since
			 * we didn't know the layer grouping before.
			 */
			free(layer_group_string);
			PCB = yyPCB;
			PCB_POLY_ALL_LOOP(yyData);
			{
			  pcb_poly_init_clip(yyData, layer, polygon);
			}
			PCB_ENDALL_LOOP;
			PCB = pcb_save;
			}

		| { PreLoadElementPCB ();
		    layer_group_string = NULL; }
		  element
		  { LayerFlag[0] = pcb_true;
		    LayerFlag[1] = pcb_true;
		    if (yyElemFixLayers) {
		    	yyData->LayerN = 2;
		    	yyData->Layer[0].parent_type = PCB_PARENT_DATA;
		    	yyData->Layer[0].parent.data = yyData;
		    	yyData->Layer[0].is_bound = 1;
		    	yyData->Layer[0].meta.bound.type = PCB_LYT_SILK | PCB_LYT_TOP;
		    	yyData->Layer[1].parent_type = PCB_PARENT_DATA;
		    	yyData->Layer[1].parent.data = yyData;
		    	yyData->Layer[1].is_bound = 1;
		    	yyData->Layer[1].meta.bound.type = PCB_LYT_SILK | PCB_LYT_BOTTOM;
		    }
		    PostLoadElementPCB ();
		  }
		;

parsedata
		:	{
					/* reset flags for 'used layers';
					 * init font and data pointers
					 */
				int	i;

				if (!yyData || !yyFont)
				{
					pcb_message(PCB_MSG_ERROR, "illegal fileformat\n");
					YYABORT;
				}
				for (i = 0; i < PCB_MAX_LAYER + 2; i++)
					LayerFlag[i] = pcb_false;
				yyData->LayerN = 0;
			}
		 pcbdata
		;

pcbfont
		: parsefont
		|
		;

parsefont
		:
			{
					/* mark all symbols invalid */
				int	i;

				if (!yyFont)
				{
					pcb_message(PCB_MSG_ERROR, "illegal fileformat\n");
					YYABORT;
				}
				if (yyFontReset) {
					pcb_font_free (yyFont);
					yyFont->id = 0;
				}
				*yyFontkitValid = pcb_false;
			}
		  symbols
			{
				*yyFontkitValid = pcb_true;
		  		pcb_font_set_info(yyFont);
			}
		;

pcbfileversion
: |
T_FILEVERSION '[' INTEGER ']'
{
  if (check_file_version ($3) != 0)
    {
      YYABORT;
    }
}
;

pcbname
		: T_PCB '(' STRING ')'
			{
				yyPCB->Name = $3;
				yyPCB->MaxWidth = PCB_MAX_COORD;
				yyPCB->MaxHeight = PCB_MAX_COORD;
				old_fmt = 1;
			}
		| T_PCB '(' STRING measure measure ')'
			{
				yyPCB->Name = $3;
				yyPCB->MaxWidth = OU ($4);
				yyPCB->MaxHeight = OU ($5);
				old_fmt = 1;
			}
		| T_PCB '[' STRING measure measure ']'
			{
				yyPCB->Name = $3;
				yyPCB->MaxWidth = NU ($4);
				yyPCB->MaxHeight = NU ($5);
				old_fmt = 0;
			}
		;

pcbgrid
		: pcbgridold
		| pcbgridnew
		| pcbhigrid
		;
pcbgridold
		: T_GRID '(' measure measure measure ')'
			{
				yyPCB->Grid = OU ($3);
				yyPCB->GridOffsetX = OU ($4);
				yyPCB->GridOffsetY = OU ($5);
			}
		;
pcbgridnew
		: T_GRID '(' measure measure measure INTEGER ')'
			{
				yyPCB->Grid = OU ($3);
				yyPCB->GridOffsetX = OU ($4);
				yyPCB->GridOffsetY = OU ($5);
				if (yy_settings_dest != CFR_invalid) {
					if ($6)
						conf_set(yy_settings_dest, "editor/draw_grid", -1, "true", POL_OVERWRITE);
					else
						conf_set(yy_settings_dest, "editor/draw_grid", -1, "false", POL_OVERWRITE);
				}
			}
		;

pcbhigrid
		: T_GRID '[' measure measure measure INTEGER ']'
			{
				yyPCB->Grid = NU ($3);
				yyPCB->GridOffsetX = NU ($4);
				yyPCB->GridOffsetY = NU ($5);
				if (yy_settings_dest != CFR_invalid) {
					if ($6)
						conf_set(yy_settings_dest, "editor/draw_grid", -1, "true", POL_OVERWRITE);
					else
						conf_set(yy_settings_dest, "editor/draw_grid", -1, "false", POL_OVERWRITE);
				}
			}
		;

pcbcursor
		: T_CURSOR '(' measure measure number ')'
			{
/* Not loading cursor position and zoom anymore */
			}
		| T_CURSOR '[' measure measure number ']'
			{
/* Not loading cursor position and zoom anymore */
			}
		|
		;

polyarea
		:
		| T_AREA '[' number ']'
			{
				/* Read in cmil^2 for now; in future this should be a noop. */
				load_meta_float("design/poly_isle_area", PCB_MIL_TO_COORD(PCB_MIL_TO_COORD ($3) / 100.0) / 100.0);
			}
		;

pcbthermal
		:
		| T_THERMAL '[' number ']'
			{
				yyPCB->ThermScale = $3;
			}
		;

pcbdrc
		:
		| pcbdrc1
		| pcbdrc2
		| pcbdrc3
		;

pcbdrc1
                : T_DRC '[' measure measure measure ']'
		        {
				load_meta_coord("design/bloat", NU($3));
				load_meta_coord("design/shrink", NU($4));
				load_meta_coord("design/min_wid", NU($5));
				load_meta_coord("design/min_ring", NU($5));
			}
		;

pcbdrc2
                : T_DRC '[' measure measure measure measure ']'
		        {
				load_meta_coord("design/bloat", NU($3));
				load_meta_coord("design/shrink", NU($4));
				load_meta_coord("design/min_wid", NU($5));
				load_meta_coord("design/min_slk", NU($6));
				load_meta_coord("design/min_ring", NU($5));
			}
		;

pcbdrc3
                : T_DRC '[' measure measure measure measure measure measure ']'
		        {
				load_meta_coord("design/bloat", NU($3));
				load_meta_coord("design/shrink", NU($4));
				load_meta_coord("design/min_wid", NU($5));
				load_meta_coord("design/min_slk", NU($6));
				load_meta_coord("design/min_drill", NU($7));
				load_meta_coord("design/min_ring", NU($8));
			}
		;

pcbflags
		: T_FLAGS '(' INTEGER ')'
			{
				yy_pcb_flags = pcb_flag_make($3 & PCB_FLAGS);
			}
		| T_FLAGS '(' STRING ')'
			{
			  yy_pcb_flags = pcb_strflg_board_s2f($3, yyerror);
			  free($3);
			}
		|
		;

pcbgroups
		: T_GROUPS '(' STRING ')'
			{
			  layer_group_string = $3;
			}
		|
		;

pcbstyles
		: T_STYLES '(' STRING ')'
			{
				if (pcb_route_string_parse($3, &yyPCB->RouteStyle, "mil"))
				{
					pcb_message(PCB_MSG_ERROR, "illegal route-style string\n");
					YYABORT;
				}
				free($3);
			}
		| T_STYLES '[' STRING ']'
			{
				if (pcb_route_string_parse(($3 == NULL ? "" : $3), &yyPCB->RouteStyle, "cmil"))
				{
					pcb_message(PCB_MSG_ERROR, "illegal route-style string\n");
					YYABORT;
				}
				free($3);
			}
		|
		;

pcbdata
		: pcbdefinitions
		|
		;

pcbdefinitions
		: pcbdefinition
		| pcbdefinitions pcbdefinition
		;

pcbdefinition
		: via
		| { attr_list = & yyPCB->Attributes; } attribute
		| rats
		| layer
		|
			{
					/* clear pointer to force memory allocation by
					 * the appropriate subroutine
					 */
				yysubc = NULL;
			}
		  element
		| error { YYABORT; }
		;

via
		: via_hi_format
		| via_2.0_format
		| via_1.7_format
		| via_newformat
		| via_oldformat
		;

via_hi_format
			/* x, y, thickness, clearance, mask, drilling-hole, name, flags */
		: T_VIA '[' measure measure measure measure measure measure STRING flags ']'
			{
				pcb_old_via_new(yyData, -1, NU ($3), NU ($4), NU ($5), NU ($6), NU ($7),
				                     NU ($8), $9, $10);
				free ($9);
			}
		;

via_2.0_format
			/* x, y, thickness, clearance, mask, drilling-hole, name, flags */
		: T_VIA '(' measure measure measure measure measure measure STRING INTEGER ')'
			{
				pcb_old_via_new(yyData, -1, OU ($3), OU ($4), OU ($5), OU ($6), OU ($7), OU ($8), $9,
					pcb_flag_old($10));
				free ($9);
			}
		;


via_1.7_format
			/* x, y, thickness, clearance, drilling-hole, name, flags */
		: T_VIA '(' measure measure measure measure measure STRING INTEGER ')'
			{
				pcb_old_via_new(yyData, -1, OU ($3), OU ($4), OU ($5), OU ($6),
					     OU ($5) + OU($6), OU ($7), $8, pcb_flag_old($9));
				free ($8);
			}
		;

via_newformat
			/* x, y, thickness, drilling-hole, name, flags */
		: T_VIA '(' measure measure measure measure STRING INTEGER ')'
			{
				pcb_old_via_new(yyData, -1, OU ($3), OU ($4), OU ($5), 2*PCB_GROUNDPLANEFRAME,
					OU($5) + 2*PCB_MASKFRAME,  OU ($6), $7, pcb_flag_old($8));
				free ($7);
			}
		;

via_oldformat
			/* old format: x, y, thickness, name, flags */
		: T_VIA '(' measure measure measure STRING INTEGER ')'
			{
				pcb_coord_t	hole = (OU($5) * PCB_DEFAULT_DRILLINGHOLE);

					/* make sure that there's enough copper left */
				if (OU($5) - hole < PCB_MIN_PINORVIACOPPER &&
					OU($5) > PCB_MIN_PINORVIACOPPER)
					hole = OU($5) - PCB_MIN_PINORVIACOPPER;

				pcb_old_via_new(yyData, -1, OU ($3), OU ($4), OU ($5), 2*PCB_GROUNDPLANEFRAME,
					OU($5) + 2*PCB_MASKFRAME, hole, $6, pcb_flag_old($7));
				free ($6);
			}
		;

rats
		: T_RAT '[' measure measure INTEGER measure measure INTEGER flags ']'
			{
				pcb_rat_new(yyData, -1, NU ($3), NU ($4), NU ($6), NU ($7), $5, $8,
					conf_core.appearance.rat_thickness, $9);
			}
		| T_RAT '(' measure measure INTEGER measure measure INTEGER INTEGER ')'
			{
				pcb_rat_new(yyData, -1, OU ($3), OU ($4), OU ($6), OU ($7), $5, $8,
					conf_core.appearance.rat_thickness, pcb_flag_old($9));
			}
		;

layer
			/* name */
		: T_LAYER '(' INTEGER STRING opt_string ')' '('
			{
				if ($3 <= 0 || $3 > PCB_MAX_LAYER)
				{
					yyerror("Layernumber out of range");
					YYABORT;
				}
				if (LayerFlag[$3-1])
				{
					yyerror("Layernumber used twice");
					YYABORT;
				}
				Layer = &yyData->Layer[$3-1];
				Layer->parent.data = yyData;
				Layer->parent_type = PCB_PARENT_DATA;
				Layer->type = PCB_OBJ_LAYER;

					/* memory for name is already allocated */
				if (Layer->name != NULL)
					free((char*)Layer->name);
				Layer->name = $4;   /* shouldn't this be strdup()'ed ? */
				LayerFlag[$3-1] = pcb_true;
				if (yyData->LayerN < $3)
				  yyData->LayerN = $3;
				if ($5 != NULL)
					free($5);
			}
		  layerdata ')'
		;

layerdata
		: layerdefinitions
		|
		;

layerdefinitions
		: layerdefinition
		| layerdefinitions layerdefinition
		;

layerdefinition
		: line_hi_format
		| line_1.7_format
		| line_oldformat
		| arc_hi_format
		| arc_1.7_format
		| arc_oldformat
			/* x1, y1, x2, y2, flags */
		| T_RECTANGLE '(' measure measure measure measure INTEGER ')'
			{
				pcb_poly_new_from_rectangle(Layer,
					OU ($3), OU ($4), OU ($3) + OU ($5), OU ($4) + OU ($6), 0, pcb_flag_old($7));
			}
		| text_hi_format
		| text_newformat
		| text_oldformat
		| { attr_list = & Layer->Attributes; } attribute
		| polygon_format

line_hi_format
			/* x1, y1, x2, y2, thickness, clearance, flags */
		: T_LINE '[' measure measure measure measure measure measure flags ']'
			{
				pcb_line_new(Layer, NU ($3), NU ($4), NU ($5), NU ($6),
				                            NU ($7), NU ($8), $9);
			}
		;

line_1.7_format
			/* x1, y1, x2, y2, thickness, clearance, flags */
		: T_LINE '(' measure measure measure measure measure measure INTEGER ')'
			{
				pcb_line_new(Layer, OU ($3), OU ($4), OU ($5), OU ($6),
						     OU ($7), OU ($8), pcb_flag_old($9));
			}
		;

line_oldformat
			/* x1, y1, x2, y2, thickness, flags */
		: T_LINE '(' measure measure measure measure measure measure ')'
			{
				/* eliminate old-style rat-lines */
			if ((IV ($8) & PCB_FLAG_RAT) == 0)
				pcb_line_new(Layer, OU ($3), OU ($4), OU ($5), OU ($6), OU ($7),
					200*PCB_GROUNDPLANEFRAME, pcb_flag_old(IV ($8)));
			}
		;

arc_hi_format
			/* x, y, width, height, thickness, clearance, startangle, delta, flags */
		: T_ARC '[' measure measure measure measure measure measure number number flags ']'
			{
			  pcb_arc_new(Layer, NU ($3), NU ($4), NU ($5), NU ($6), $9, $10,
			                             NU ($7), NU ($8), $11, pcb_true);
			}
		;

arc_1.7_format
			/* x, y, width, height, thickness, clearance, startangle, delta, flags */
		: T_ARC '(' measure measure measure measure measure measure number number INTEGER ')'
			{
				pcb_arc_new(Layer, OU ($3), OU ($4), OU ($5), OU ($6), $9, $10,
						    OU ($7), OU ($8), pcb_flag_old($11), pcb_true);
			}
		;

arc_oldformat
			/* x, y, width, height, thickness, startangle, delta, flags */
		: T_ARC '(' measure measure measure measure measure measure number INTEGER ')'
			{
				pcb_arc_new(Layer, OU ($3), OU ($4), OU ($5), OU ($5), IV ($8), $9,
					OU ($7), 200*PCB_GROUNDPLANEFRAME, pcb_flag_old($10), pcb_true);
			}
		;

text_oldformat
			/* x, y, direction, text, flags */
		: T_TEXT '(' measure measure number STRING INTEGER ')'
			{
					/* use a default scale of 100% */
				pcb_text_new(Layer,yyFont,OU ($3), OU ($4), $5 * 90.0, 100, 0, $6, pcb_flag_old($7));
				free ($6);
			}
		;

text_newformat
			/* x, y, direction, scale, text, flags */
		: T_TEXT '(' measure measure number number STRING INTEGER ')'
			{
				if ($8 & PCB_FLAG_ONSILK)
				{
					pcb_layer_t *lay = &yyData->Layer[yyData->LayerN +
						(($8 & PCB_FLAG_ONSOLDER) ? PCB_SOLDER_SIDE : PCB_COMPONENT_SIDE) - 2];

					pcb_text_new(lay ,yyFont, OU ($3), OU ($4), $5 * 90.0, $6, 0, $7,
						      pcb_flag_old($8));
				}
				else
					pcb_text_new(Layer, yyFont, OU ($3), OU ($4), $5 * 90.0, $6, 0, $7,
						      pcb_flag_old($8));
				free ($7);
			}
		;
text_hi_format
			/* x, y, direction, scale, text, flags */
		: T_TEXT '[' measure measure number number STRING flags ']'
			{
				/* FIXME: shouldn't know about .f */
				/* I don't think this matters because anything with hi_format
				 * will have the silk on its own layer in the file rather
				 * than using the PCB_FLAG_ONSILK and having it in a copper layer.
				 * Thus there is no need for anything besides the 'else'
				 * part of this code.
				 */
				if ($8.f & PCB_FLAG_ONSILK)
				{
					pcb_layer_t *lay = &yyData->Layer[yyData->LayerN +
						(($8.f & PCB_FLAG_ONSOLDER) ? PCB_SOLDER_SIDE : PCB_COMPONENT_SIDE) - 2];

					pcb_text_new(lay, yyFont, NU ($3), NU ($4), $5 * 90.0, $6, 0, $7, $8);
				}
				else
					pcb_text_new(Layer, yyFont, NU ($3), NU ($4), $5 * 90.0, $6, 0, $7, $8);
				free ($7);
			}
		;

polygon_format
		: /* flags are passed in */
		T_POLYGON '(' flags ')' '('
			{
				Polygon = pcb_poly_new(Layer, 0, $3);
			}
		  polygonpoints
		  polygonholes ')'
			{
				pcb_cardinal_t contour, contour_start, contour_end;
				pcb_bool bad_contour_found = pcb_false;
				/* ignore junk */
				for (contour = 0; contour <= Polygon->HoleIndexN; contour++)
				  {
				    contour_start = (contour == 0) ?
						      0 : Polygon->HoleIndex[contour - 1];
				    contour_end = (contour == Polygon->HoleIndexN) ?
						 Polygon->PointN :
						 Polygon->HoleIndex[contour];
				    if (contour_end - contour_start < 3)
				      bad_contour_found = pcb_true;
				  }

				if (bad_contour_found)
				  {
				    pcb_message(PCB_MSG_WARNING, "WARNING parsing file '%s'\n"
					    "    line:        %i\n"
					    "    description: 'ignored polygon (< 3 points in a contour)'\n",
					    yyfilename, pcb_lineno);
				    pcb_destroy_object(yyData, PCB_OBJ_POLY, Layer, Polygon, Polygon);
				  }
				else
				  {
				    pcb_poly_bbox(Polygon);
				    if (!Layer->polygon_tree)
				      Layer->polygon_tree = pcb_r_create_tree();
				    pcb_r_insert_entry(Layer->polygon_tree, (pcb_box_t *) Polygon);
				  }
			}
		;

polygonholes
		: /* empty */
		| polygonholes polygonhole
		;

polygonhole
		: T_POLYGON_HOLE '('
			{
				pcb_poly_hole_new(Polygon);
			}
		  polygonpoints ')'
		;

polygonpoints
		: /* empty */
		| polygonpoint polygonpoints
		;

polygonpoint
			/* xcoord ycoord */
		: '(' measure measure ')'
			{
				pcb_poly_point_new(Polygon, OU ($2), OU ($3));
			}
		| '[' measure measure ']'
			{
				pcb_poly_point_new(Polygon, NU ($2), NU ($3));
			}
		;

element
		: element_oldformat
		| element_1.3.4_format
		| element_newformat
		| element_1.7_format
		| element_hi_format
		;

element_oldformat
			/* element_flags, description, pcb-name,
			 * text_x, text_y, text_direction, text_scale, text_flags
			 */
		: T_ELEMENT '(' STRING STRING measure measure INTEGER ')' '('
			{
				yysubc = io_pcb_element_new(yyData, yysubc, yyFont, pcb_no_flags(),
					$3, $4, NULL, OU ($5), OU ($6), $7, 100, pcb_no_flags(), pcb_false);
				free ($3);
				free ($4);
				pin_num = 1;
			}
		  elementdefinitions ')'
			{
				io_pcb_element_fin(yyData);
			}
		;

element_1.3.4_format
			/* element_flags, description, pcb-name,
			 * text_x, text_y, text_direction, text_scale, text_flags
			 */
		: T_ELEMENT '(' INTEGER STRING STRING measure measure measure measure INTEGER ')' '('
			{
				yysubc = io_pcb_element_new(yyData, yysubc, yyFont, pcb_flag_old($3),
					$4, $5, NULL, OU ($6), OU ($7), IV ($8), IV ($9), pcb_flag_old($10), pcb_false);
				free ($4);
				free ($5);
				pin_num = 1;
			}
		  elementdefinitions ')'
			{
				io_pcb_element_fin(yyData);
			}
		;

element_newformat
			/* element_flags, description, pcb-name, value,
			 * text_x, text_y, text_direction, text_scale, text_flags
			 */
		: T_ELEMENT '(' INTEGER STRING STRING STRING measure measure measure measure INTEGER ')' '('
			{
				yysubc = io_pcb_element_new(yyData, yysubc, yyFont, pcb_flag_old($3),
					$4, $5, $6, OU ($7), OU ($8), IV ($9), IV ($10), pcb_flag_old($11), pcb_false);
				free ($4);
				free ($5);
				free ($6);
				pin_num = 1;
			}
		  elementdefinitions ')'
			{
				io_pcb_element_fin(yyData);
			}
		;

element_1.7_format
			/* element_flags, description, pcb-name, value, mark_x, mark_y,
			 * text_x, text_y, text_direction, text_scale, text_flags
			 */
		: T_ELEMENT '(' INTEGER STRING STRING STRING measure measure
			measure measure number number INTEGER ')' '('
			{
				yysubc = io_pcb_element_new(yyData, yysubc, yyFont, pcb_flag_old($3),
					$4, $5, $6, OU ($7) + OU ($9), OU ($8) + OU ($10),
					$11, $12, pcb_flag_old($13), pcb_false);
				yysubc_ox = OU ($7);
				yysubc_oy = OU ($8);
				free ($4);
				free ($5);
				free ($6);
			}
		  relementdefs ')'
			{
				io_pcb_element_fin(yyData);
			}
		;

element_hi_format
			/* element_flags, description, pcb-name, value, mark_x, mark_y,
			 * text_x, text_y, text_direction, text_scale, text_flags
			 */
		: T_ELEMENT '[' flags STRING STRING STRING measure measure
			measure measure number number flags ']' '('
			{
				yysubc = io_pcb_element_new(yyData, yysubc, yyFont, $3,
					$4, $5, $6, NU ($7) + NU ($9), NU ($8) + NU ($10),
					$11, $12, $13, pcb_false);
				yysubc_ox = NU ($7);
				yysubc_oy = NU ($8);
				free ($4);
				free ($5);
				free ($6);
			}
		  relementdefs ')'
			{
				if (pcb_subc_is_empty(yysubc)) {
					pcb_subc_free(yysubc);
					yysubc = NULL;
				}
				else {
					io_pcb_element_fin(yyData);
				}
			}
		;

elementdefinitions
		: elementdefinition
		| elementdefinitions elementdefinition
		;

elementdefinition
		: pin_1.6.3_format
		| pin_newformat
		| pin_oldformat
		| pad_newformat
		| pad
			/* x1, y1, x2, y2, thickness */
		| T_ELEMENTLINE '[' measure measure measure measure measure ']'
			{
				io_pcb_element_line_new(yysubc, NU ($3), NU ($4), NU ($5), NU ($6), NU ($7));
			}
			/* x1, y1, x2, y2, thickness */
		| T_ELEMENTLINE '(' measure measure measure measure measure ')'
			{
				io_pcb_element_line_new(yysubc, OU ($3), OU ($4), OU ($5), OU ($6), OU ($7));
			}
			/* x, y, width, height, startangle, anglediff, thickness */
		| T_ELEMENTARC '[' measure measure measure measure number number measure ']'
			{
				io_pcb_element_arc_new(yysubc, NU ($3), NU ($4), NU ($5), NU ($6), $7, $8, NU ($9));
			}
			/* x, y, width, height, startangle, anglediff, thickness */
		| T_ELEMENTARC '(' measure measure measure measure number number measure ')'
			{
				io_pcb_element_arc_new(yysubc, OU ($3), OU ($4), OU ($5), OU ($6), $7, $8, OU ($9));
			}
			/* x, y position */
		| T_MARK '[' measure measure ']'
			{
				yysubc_ox = NU ($3);
				yysubc_oy = NU ($4);
			}
		| T_MARK '(' measure measure ')'
			{
				yysubc_ox = OU ($3);
				yysubc_oy = OU ($4);
			}
		| { attr_list = & yysubc->Attributes; } attribute
		;

relementdefs
		: relementdef
		| relementdefs relementdef
		;

relementdef
		: pin_1.7_format
		| pin_hi_format
		| pad_1.7_format
		| pad_hi_format
			/* x1, y1, x2, y2, thickness */
		| T_ELEMENTLINE '[' measure measure measure measure measure ']'
			{
				io_pcb_element_line_new(yysubc, NU ($3) + yysubc_ox,
					NU ($4) + yysubc_oy, NU ($5) + yysubc_ox,
					NU ($6) + yysubc_oy, NU ($7));
			}
		| T_ELEMENTLINE '(' measure measure measure measure measure ')'
			{
				io_pcb_element_line_new(yysubc, OU ($3) + yysubc_ox,
					OU ($4) + yysubc_oy, OU ($5) + yysubc_ox,
					OU ($6) + yysubc_oy, OU ($7));
			}
			/* x, y, width, height, startangle, anglediff, thickness */
		| T_ELEMENTARC '[' measure measure measure measure number number measure ']'
			{
				io_pcb_element_arc_new(yysubc, NU ($3) + yysubc_ox,
					NU ($4) + yysubc_oy, NU ($5), NU ($6), $7, $8, NU ($9));
			}
		| T_ELEMENTARC '(' measure measure measure measure number number measure ')'
			{
				io_pcb_element_arc_new(yysubc, OU ($3) + yysubc_ox,
					OU ($4) + yysubc_oy, OU ($5), OU ($6), $7, $8, OU ($9));
			}
		| { attr_list = & yysubc->Attributes; } attribute
		;

pin_hi_format
			/* x, y, thickness, clearance, mask, drilling hole, name,
			   number, flags */
		: T_PIN '[' measure measure measure measure measure measure STRING STRING flags ']'
			{
				pcb_pstk_t *pin = io_pcb_element_pin_new(yysubc, NU ($3) + yysubc_ox,
					NU ($4) + yysubc_oy, NU ($5), NU ($6), NU ($7), NU ($8), $9,
					$10, $11);
				pcb_attrib_compat_set_intconn(&pin->Attributes, yy_intconn);
				free ($9);
				free ($10);
			}
		;
pin_1.7_format
			/* x, y, thickness, clearance, mask, drilling hole, name,
			   number, flags */
		: T_PIN '(' measure measure measure measure measure measure STRING STRING INTEGER ')'
			{
				io_pcb_element_pin_new(yysubc, OU ($3) + yysubc_ox,
					OU ($4) + yysubc_oy, OU ($5), OU ($6), OU ($7), OU ($8), $9,
					$10, pcb_flag_old($11));
				free ($9);
				free ($10);
			}
		;

pin_1.6.3_format
			/* x, y, thickness, drilling hole, name, number, flags */
		: T_PIN '(' measure measure measure measure STRING STRING INTEGER ')'
			{
				io_pcb_element_pin_new(yysubc, OU ($3), OU ($4), OU ($5), 2*PCB_GROUNDPLANEFRAME,
					OU ($5) + 2*PCB_MASKFRAME, OU ($6), $7, $8, pcb_flag_old($9));
				free ($7);
				free ($8);
			}
		;

pin_newformat
			/* x, y, thickness, drilling hole, name, flags */
		: T_PIN '(' measure measure measure measure STRING INTEGER ')'
			{
				char	p_number[8];

				sprintf(p_number, "%d", pin_num++);
				io_pcb_element_pin_new(yysubc, OU ($3), OU ($4), OU ($5), 2*PCB_GROUNDPLANEFRAME,
					OU ($5) + 2*PCB_MASKFRAME, OU ($6), $7, p_number, pcb_flag_old($8));

				free ($7);
			}
		;

pin_oldformat
			/* old format: x, y, thickness, name, flags
			 * drilling hole is 40% of the diameter
			 */
		: T_PIN '(' measure measure measure STRING INTEGER ')'
			{
				pcb_coord_t	hole = OU ($5) * PCB_DEFAULT_DRILLINGHOLE;
				char	p_number[8];

					/* make sure that there's enough copper left */
				if (OU ($5) - hole < PCB_MIN_PINORVIACOPPER &&
					OU ($5) > PCB_MIN_PINORVIACOPPER)
					hole = OU ($5) - PCB_MIN_PINORVIACOPPER;

				sprintf(p_number, "%d", pin_num++);
				io_pcb_element_pin_new(yysubc, OU ($3), OU ($4), OU ($5), 2*PCB_GROUNDPLANEFRAME,
					OU ($5) + 2*PCB_MASKFRAME, hole, $6, p_number, pcb_flag_old($7));
				free ($6);
			}
		;

pad_hi_format
			/* x1, y1, x2, y2, thickness, clearance, mask, name , pad number, flags */
		: T_PAD '[' measure measure measure measure measure measure measure STRING STRING flags ']'
			{
				pcb_pstk_t *pad = io_pcb_element_pad_new(yysubc, NU ($3) + yysubc_ox,
					NU ($4) + yysubc_oy,
					NU ($5) + yysubc_ox,
					NU ($6) + yysubc_oy, NU ($7), NU ($8), NU ($9),
					$10, $11, $12);
				pcb_attrib_compat_set_intconn(&pad->Attributes, yy_intconn);
				free ($10);
				free ($11);
			}
		;

pad_1.7_format
			/* x1, y1, x2, y2, thickness, clearance, mask, name , pad number, flags */
		: T_PAD '(' measure measure measure measure measure measure measure STRING STRING INTEGER ')'
			{
				io_pcb_element_pad_new(yysubc,OU ($3) + yysubc_ox,
					OU ($4) + yysubc_oy, OU ($5) + yysubc_ox,
					OU ($6) + yysubc_oy, OU ($7), OU ($8), OU ($9),
					$10, $11, pcb_flag_old($12));
				free ($10);
				free ($11);
			}
		;

pad_newformat
			/* x1, y1, x2, y2, thickness, name , pad number, flags */
		: T_PAD '(' measure measure measure measure measure STRING STRING INTEGER ')'
			{
				io_pcb_element_pad_new(yysubc,OU ($3),OU ($4),OU ($5),OU ($6),OU ($7), 2*PCB_GROUNDPLANEFRAME,
					OU ($7) + 2*PCB_MASKFRAME, $8, $9, pcb_flag_old($10));
				free ($8);
				free ($9);
			}
		;

pad
			/* x1, y1, x2, y2, thickness, name and flags */
		: T_PAD '(' measure measure measure measure measure STRING INTEGER ')'
			{
				char		p_number[8];

				sprintf(p_number, "%d", pin_num++);
				io_pcb_element_pad_new(yysubc,OU ($3),OU ($4),OU ($5),OU ($6),OU ($7), 2*PCB_GROUNDPLANEFRAME,
					OU ($7) + 2*PCB_MASKFRAME, $8,p_number, pcb_flag_old($9));
				free ($8);
			}
		;

flags		: INTEGER	{ $$ = pcb_flag_old($1); }
		| STRING	{ $$ = pcb_strflg_s2f($1, yyerror, &yy_intconn, 1); free($1); }
		;

symbols
		: symbol
		| symbols symbol
		;

symbol : symbolhead symboldata ')'

symbolhead	: T_SYMBOL '[' symbolid measure ']' '('
			{
				if ($3 <= 0 || $3 > PCB_MAX_FONTPOSITION)
				{
					yyerror("fontposition out of range");
					YYABORT;
				}
				Symbol = &yyFont->Symbol[$3];
				if (Symbol->Valid)
				{
					yyerror("symbol ID used twice");
					YYABORT;
				}
				Symbol->Valid = pcb_true;
				Symbol->Delta = NU ($4);
			}
		| T_SYMBOL '(' symbolid measure ')' '('
			{
				if ($3 <= 0 || $3 > PCB_MAX_FONTPOSITION)
				{
					yyerror("fontposition out of range");
					YYABORT;
				}
				Symbol = &yyFont->Symbol[$3];
				if (Symbol->Valid)
				{
					yyerror("symbol ID used twice");
					YYABORT;
				}
				Symbol->Valid = pcb_true;
				Symbol->Delta = OU ($4);
			}
		;

symbolid
		: INTEGER
		| CHAR_CONST
		;

symboldata
		: /* empty */
		| symboldata symboldefinition
		| symboldata hiressymbol
		;

symboldefinition
			/* x1, y1, x2, y2, thickness */
		: T_SYMBOLLINE '(' measure measure measure measure measure ')'
			{
				pcb_font_new_line_in_sym(Symbol, OU ($3), OU ($4), OU ($5), OU ($6), OU ($7));
			}
		;
hiressymbol
			/* x1, y1, x2, y2, thickness */
		: T_SYMBOLLINE '[' measure measure measure measure measure ']'
			{
				pcb_font_new_line_in_sym(Symbol, NU ($3), NU ($4), NU ($5), NU ($6), NU ($7));
			}
		;

pcbnetlist	: pcbnetdef
		|
		;
pcbnetdef
			/* net(...) net(...) ... */
		: T_NETLIST '(' ')' '('
		  nets ')'
		;

nets
		: netdefs
		|
		;

netdefs
		: net
		| netdefs net
		;

net
			/* name style pin pin ... */
		: T_NET '(' STRING STRING ')' '('
			{
				currnet = pcb_net_get(yyPCB, &yyPCB->netlist[PCB_NETLIST_INPUT], $3, 1);
				if (($4 != NULL) && (*$4 != '\0'))
					pcb_attribute_put(&currnet->Attributes, "style", $4);
				free ($3);
				free ($4);
			}
		 connections ')'
		;

connections
		: conndefs
		|
		;

conndefs
		: conn
		| conndefs conn
		;

conn
		: T_CONN '(' STRING ')'
			{
				pcb_net_term_get_by_pinname(currnet, $3, 1);
				free ($3);
			}
		;

/* %start-doc pcbfile Netlistpatch

@syntax
NetListPatch ( ) (
@ @ @ @dots{} netpatch @dots{}
)
@end syntax

%end-doc */

pcbnetlistpatch	: pcbnetpatchdef
		|
		;
pcbnetpatchdef
			/* net(...) net(...) ... */
		: T_NETLISTPATCH '(' ')' '('
		  netpatches ')'
		;

netpatches
		: netpatchdefs
		|
		;

netpatchdefs
		: netpatch
		| netpatchdefs netpatch
		;

/* %start-doc pcbfile NetPatch

@syntax
op (arg1 arg2 ...) (
@ @ @ @dots{} netlist patch directive @dots{}
)
@end syntax

%end-doc */

netpatch
			/* name style pin pin ... */
		: T_ADD_CONN      '(' STRING STRING ')'         { pcb_ratspatch_append(yyPCB, RATP_ADD_CONN, $3, $4, NULL); free($3); free($4); }
		| T_DEL_CONN      '(' STRING STRING ')'         { pcb_ratspatch_append(yyPCB, RATP_DEL_CONN, $3, $4, NULL); free($3); free($4); }
		| T_CHANGE_ATTRIB '(' STRING STRING STRING ')'  { pcb_ratspatch_append(yyPCB, RATP_CHANGE_ATTRIB, $3, $4, $5); free($3); free($4); free($5); }
		;

attribute
		: T_ATTRIBUTE '(' STRING STRING ')'
			{
				char *old_val, *key = $3, *val = $4 ? $4 : (char *)"";
				old_val = pcb_attribute_get(attr_list, key);
				if (old_val != NULL)
					pcb_message(PCB_MSG_ERROR, "mutliple values for attribute %s: '%s' and '%s' - ignoring '%s'\n", key, old_val, val, val);
				else
					pcb_attribute_put(attr_list, key, val);
				free(key);
				free(val);
			}
		;

opt_string	: STRING { $$ = $1; }
		| /* empty */ { $$ = 0; }
		;

number
		: FLOATING	{ $$ = $1; }
		| INTEGER	{ $$ = $1; }
		;

measure
		/* Default unit (no suffix) is cmil */
		: number	{ do_measure(&$$, $1, PCB_MIL_TO_COORD ($1) / 100.0, 0); }
		| number T_UMIL	{ M ($$, $1, PCB_MIL_TO_COORD ($1) / 100000.0); pcb_io_pcb_usty_seen |= PCB_USTY_UNITS; }
		| number T_CMIL	{ M ($$, $1, PCB_MIL_TO_COORD ($1) / 100.0); pcb_io_pcb_usty_seen |= PCB_USTY_UNITS; }
		| number T_MIL	{ M ($$, $1, PCB_MIL_TO_COORD ($1)); pcb_io_pcb_usty_seen |= PCB_USTY_UNITS; }
		| number T_IN	{ M ($$, $1, PCB_INCH_TO_COORD ($1)); pcb_io_pcb_usty_seen |= PCB_USTY_UNITS; }
		| number T_NM	{ M ($$, $1, PCB_MM_TO_COORD ($1) / 1000000.0); pcb_io_pcb_usty_seen |= PCB_USTY_NANOMETER; }
		| number T_UM	{ M ($$, $1, PCB_MM_TO_COORD ($1) / 1000.0); pcb_io_pcb_usty_seen |= PCB_USTY_UNITS; }
		| number T_MM	{ M ($$, $1, PCB_MM_TO_COORD ($1)); pcb_io_pcb_usty_seen |= PCB_USTY_UNITS; }
		| number T_M	{ M ($$, $1, PCB_MM_TO_COORD ($1) * 1000.0); pcb_io_pcb_usty_seen |= PCB_USTY_UNITS; }
		| number T_KM	{ M ($$, $1, PCB_MM_TO_COORD ($1) * 1000000.0); pcb_io_pcb_usty_seen |= PCB_USTY_UNITS; }
		;

%%

/* ---------------------------------------------------------------------------
 * error routine called by parser library
 */
int yyerror(const char * s)
{
	pcb_message(PCB_MSG_ERROR, "ERROR parsing file '%s'\n"
		"    line:        %i\n"
		"    description: '%s'\n",
		yyfilename, pcb_lineno, s);
	return(0);
}

int pcb_wrap()
{
  return 1;
}

static int
check_file_version (int ver)
{
  if ( ver > PCB_FILE_VERSION ) {
    pcb_message(PCB_MSG_ERROR, "ERROR:  The file you are attempting to load is in a format\n"
	     "which is too new for this version of pcb.  To load this file\n"
	     "you need a version of pcb which is >= %d.  If you are\n"
	     "using a version built from git source, the source date\n"
	     "must be >= %d.  This copy of pcb can only read files\n"
	     "up to file version %d.\n", ver, ver, PCB_FILE_VERSION);
    return 1;
  }

  return 0;
}

static void
do_measure (PLMeasure *m, pcb_coord_t i, double d, int u)
{
  m->ival = i;
  m->bval = pcb_round (d);
  m->dval = d;
  m->has_units = u;
}

static int
integer_value (PLMeasure m)
{
  if (m.has_units)
    yyerror("units ignored here");
  return m.ival;
}

static pcb_coord_t
old_units (PLMeasure m)
{
  if (m.has_units)
    return m.bval;
  if (m.ival != 0)
    pcb_io_pcb_usty_seen |= PCB_USTY_CMIL; /* ... because we can't save in mil */
  return pcb_round (PCB_MIL_TO_COORD (m.ival));
}

static pcb_coord_t
new_units (PLMeasure m)
{
  if (m.has_units)
    return m.bval;
  if (m.dval != 0)
    pcb_io_pcb_usty_seen |= PCB_USTY_CMIL;
  /* if there's no unit m.dval already contains the converted value */
  return pcb_round (m.dval);
}

/* This converts old flag bits (from saved PCB files) to new format.  */
static pcb_flag_t pcb_flag_old(unsigned int flags)
{
	pcb_flag_t rv;
	int i, f;
	memset(&rv, 0, sizeof(rv));
	/* If we move flag bits around, this is where we map old bits to them.  */
	rv.f = flags & 0xffff;
	f = 0x10000;
	for (i = 0; i < 8; i++) {
		/* use the closest thing to the old thermal style */
		if (flags & f)
			rv.t[i / 2] |= (1 << (4 * (i % 2)));
		f <<= 1;
	}
	return rv;
}

/* load a board metadata into conf_core */
static void load_meta_coord(const char *path, pcb_coord_t crd)
{
	char tmp[128];
	pcb_sprintf(tmp, "%$mm", crd);
	conf_set(CFR_DESIGN, path, -1, tmp, POL_OVERWRITE);
}

static void load_meta_float(const char *path, double val)
{
	char tmp[128];
	pcb_sprintf(tmp, "%f", val);
	conf_set(CFR_DESIGN, path, -1, tmp, POL_OVERWRITE);
}
