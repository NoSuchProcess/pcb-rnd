%prefix pcb_pads_%

%struct
{
	long line, first_col, last_col;
}

%union
{
	double d;
	int i;
	char *s;
	rnd_coord_t c;
}


%{
/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  PADS IO plugin - PADS grammar
 *  pcb-rnd Copyright (C) 2020 Tibor 'Igor2' Palinkas
 *  (Supported by NLnet NGI0 PET Fund in 2020)
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
 */
#include <stdio.h>
#include "../src_plugins/io_pads/pads_gram.h"
#include "../src_plugins/io_pads/pads.h"

TODO("Can remove this once the coord unit is converted with getvalue")
#include <librnd/core/unit.h>

#define XCRD(c) pcb_pads_coord_x(c)
#define YCRD(c) pcb_pads_coord_y(c)

%}

/* Generic */
%token T_ID T_INTEGER T_REAL_ONLY T_QSTR
%token T_HDR_PCB T_UNITS T_USERGRID T_LINEWIDTH T_TEXTSIZE T_PLANEFLAGS

%type <s> T_QSTR
%type <s> T_ID
%type <i> T_INTEGER
%type <d> T_REAL_ONLY
%type <d> real
%type <c> coord
%type <i> yn

%%

full_file:
	file
	;

file:
	/* empty */
	| header_block nl file
	;

header_block:
	header_block_pcb
	;


header_block_pcb:
	T_HDR_PCB junk_nl block_pcb
	;

block_pcb:
	  /* empty */
	| block_pcb_ block_pcb
	;

block_pcb_:
	  T_UNITS T_INTEGER  junk_nl                { printf("UNIT!\n"); }
	| T_USERGRID T_INTEGER T_INTEGER  junk_nl   { printf("UGRID\n"); }
	| T_LINEWIDTH T_INTEGER junk_nl             { printf("LINEWIDTH\n"); }
	| T_TEXTSIZE T_INTEGER T_INTEGER  junk_nl   { printf("TEXTSIZE\n"); }
	| T_PLANEFLAGS T_ID T_ID
		yn yn yn yn yn yn yn yn yn yn yn yn yn yn yn yn yn yn
		junk_nl                                   { printf("PLANEFLAGS remove-isolated=%d\n", $4); }
	| T_ID  junk_nl                             { /*printf("unknown: %s\n", $1);*/ free($1); }
	;

/*** common and misc ***/

junk_nl:
	{ while(fgetc(ctx->f) != '\n') ; ungetc('\n', ctx->f); } nl
	;

yn:
	T_ID
		{
			char cmd = $1[0];
			$$ = -1;
			if ((cmd == '\0') || ($1[1] != '\0')) {
				pcb_pads_error(ctx, yyctx->lval, "expected Y or N (string too long)\n");
				free($1);
				goto yyerrlab;
			}
			else {
				free($1);
				if (cmd == 'Y') $$ = 1;
				else if (cmd == 'N') $$ = 0;
				else {
					pcb_pads_error(ctx, yyctx->lval, "expected Y or N (wrong char)\n");
					goto yyerrlab;
				}
			}
		}
		;

nl:
	  '\n'
	| '\n' nl
	;

maybe_nl:
	  /* empty */
	| nl
	;

real:
	  T_INTEGER            { $$ = $1; }
	| T_REAL_ONLY          { $$ = $1; }
	;

coord:
	real    { $$ = RND_MIL_TO_COORD($1); }
	;

