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
 *  RCS: $Id$
 */

/* just defines common parser identifiers
 */

#ifndef	PCB_LEX_H
#define	PCB_LEX_H

#include "global.h"
#include "plug_io.h"

int io_pcb_ParsePCB(plug_io_t *ctx, PCBTypePtr Ptr, const char *Filename, conf_role_t settings_dest);
int io_pcb_ParseElement(plug_io_t *ctx, DataTypePtr, const char *);
int io_pcb_ParseFont(plug_io_t *ctx, FontTypePtr, const char *);

#endif
