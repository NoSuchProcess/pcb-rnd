/* $Id$ */

/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,1997,1998,2005,2006 Thomas Nau
 *  Copyright (C) 2015 Tibor 'Igor2' Palinkas
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

#include "config.h"
#include "conf_core.h"

#include <locale.h>

#include "global.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "change.h"
#include "create.h"
#include "crosshair.h"
#include "data.h"
#include "error.h"
#include "file.h"
#include "plug_io.h"
#include "hid.h"
#include "misc.h"
#include "move.h"
#include "mymem.h"
#include "parse_l.h"
#include "pcb-printf.h"
#include "polygon.h"
#include "rats.h"
#include "remove.h"
#include "set.h"
#include "strflags.h"
#include "compat_fs.h"
#include "paths.h"
#include "rats_patch.h"
#include "stub_edif.h"
#include "hid_actions.h"
#include "hid_flags.h"
#include "flags.h"
#include "attribs.h"
#include "route_style.h"


RCSID("$Id$");

/* writes element data */
int io_kicad_legacy_write_element(plug_io_t *ctx, FILE * FP, DataTypePtr Data)
{
	fputs("io_kicad_legacy_write_element()", FP);
	return 0;
}

/* writes the buffer to file */
int io_kicad_legacy_write_buffer(plug_io_t *ctx, FILE * FP, BufferType *buff)
{
	fputs("io_kicad_legacy_write_buffer()", FP);
	return 0;
}

/* ---------------------------------------------------------------------------
 * writes PCB to file
 */
int io_kicad_legacy_write_pcb(plug_io_t *ctx, FILE * FP)
{
	fputs("io_kicad_legacy_write_pcb()", FP);
	return 0;
}
