/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* prototypes for file routines
 */

#ifndef	PCB_FILE_H
#define	PCB_FILE_H

#include <stdio.h>							/* needed to define 'FILE *' */
#include "global.h"
#include "plug_io.h"

/* This is used by the lexer/parser */
typedef struct {
	int ival;
	Coord bval;
	double dval;
	char has_units;
} PLMeasure;

typedef struct {
	const char *write_coord_fmt;
} io_pcb_ctx_t;

int io_pcb_WriteBuffer(plug_io_t *ctx, FILE *f, BufferType *buff);
int io_pcb_WriteElementData(plug_io_t *ctx, FILE *f, DataTypePtr);
int io_pcb_WritePCB(plug_io_t *ctx, FILE *f, const char *old_filename, const char *new_filename, pcb_bool emergency);

void PreLoadElementPCB(void);
void PostLoadElementPCB(void);

/* 
 * Whenever the pcb file format is modified, this version number
 * should be updated to the date when the new code is committed.
 * It will be written out to the file and also used by pcb to give
 * guidance to the user as to what the minimum version of pcb required
 * is.
 */

/* This is the version needed by the file we're saving.  */
int PCBFileVersionNeeded(void);

/* This is the version we support.  */
#define PCB_FILE_VERSION 20110603

#endif
