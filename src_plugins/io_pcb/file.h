/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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

#ifndef	PCB_FILE_H
#define	PCB_FILE_H

#include <stdio.h>							/* needed to define 'FILE *' */
#include "config.h"
#include "board.h"
#include "plug_io.h"

/* This is used by the lexer/parser */
typedef struct {
	int ival;
	rnd_coord_t bval;
	double dval;
	char has_units;
} PLMeasure;

typedef enum {
	PCB_USTY_CMIL = 1,       /* unitless centimil */
	PCB_USTY_NANOMETER = 2,
	PCB_USTY_UNITS = 4       /* using various units */
} pcb_unit_style_t;

extern pcb_unit_style_t pcb_io_pcb_usty_seen;

typedef struct {
	const char *write_coord_fmt;
} io_pcb_ctx_t;

extern pcb_plug_io_t *pcb_preferred_io_pcb, *pcb_nanometer_io_pcb, *pcb_centimil_io_pcb;

int io_pcb_WritePCB(pcb_plug_io_t *ctx, FILE *f, const char *old_filename, const char *new_filename, rnd_bool emergency);
int io_pcb_write_subcs_head(pcb_plug_io_t *ctx, void **udata, FILE *f, int lib, long num_subcs);
int io_pcb_write_subcs_subc(pcb_plug_io_t *ctx, void **udata, FILE *f, pcb_subc_t *subc);
int io_pcb_write_subcs_tail(pcb_plug_io_t *ctx, void **udata, FILE *f);


void PreLoadElementPCB(void);
void PostLoadElementPCB(void);

int io_pcb_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f);

/*
 * Whenever the pcb file format is modified, this version number
 * should be updated to the date when the new code is committed.
 * It will be written out to the file and also used by pcb to give
 * guidance to the user as to what the minimum version of pcb required
 * is.
 */

/* This is the version needed by the file we're saving.  */
int PCBFileVersionNeeded(void);

/* Improvise layers and groups for a partial input file that lacks layer groups (and maybe even some layers) */
int pcb_layer_improvise(pcb_board_t *pcb, rnd_bool setup);

pcb_subc_t *io_pcb_element_new(pcb_data_t *Data, pcb_subc_t *Element, pcb_font_t *PCBFont, pcb_flag_t Flags, char *Description, char *NameOnPCB, char *Value, rnd_coord_t TextX, rnd_coord_t TextY, unsigned int Direction, int TextScale, pcb_flag_t TextFlags, rnd_bool uniqueName);
void io_pcb_element_fin(pcb_data_t *Data);
pcb_line_t *io_pcb_element_line_new(pcb_subc_t *subc, rnd_coord_t X1, rnd_coord_t Y1, rnd_coord_t X2, rnd_coord_t Y2, rnd_coord_t Thickness);
pcb_arc_t *io_pcb_element_arc_new(pcb_subc_t *subc, rnd_coord_t X, rnd_coord_t Y, rnd_coord_t Width, rnd_coord_t Height, rnd_angle_t angle, rnd_angle_t delta, rnd_coord_t Thickness);
pcb_pstk_t *io_pcb_element_pin_new(pcb_subc_t *subc, rnd_coord_t X, rnd_coord_t Y, rnd_coord_t Thickness, rnd_coord_t Clearance, rnd_coord_t Mask, rnd_coord_t DrillingHole, const char *Name, const char *Number, pcb_flag_t Flags);
pcb_pstk_t *io_pcb_element_pad_new(pcb_subc_t *subc, rnd_coord_t X1, rnd_coord_t Y1, rnd_coord_t X2, rnd_coord_t Y2, rnd_coord_t Thickness, rnd_coord_t Clearance, rnd_coord_t Mask, const char *Name, const char *Number, pcb_flag_t Flags);
void io_pcb_preproc_board(pcb_board_t *pcb);
void io_pcb_postproc_board(pcb_board_t *pcb);

#ifndef RND_HAS_ATEXIT
void pcb_tmp_data_save(void);
void pcb_tmp_data_remove(void);
#endif

/* This is the version we support.  */
#define PCB_FILE_VERSION 20110603

pcb_plug_fp_map_t *io_pcb_map_footprint(pcb_plug_io_t *ctx, FILE *f, const char *fn, pcb_plug_fp_map_t *head, int need_tags);

/* parses the group definition string which is a colon separated list of
   comma separated layer numbers (1,2,b:4,6,8,t); oldfmt is 0 or 1
   depending on PCB() or PCB[] in the file header. */
int pcb_layer_parse_group_string(pcb_board_t *pcb, const char *s, int LayerN, int oldfmt);


#endif
