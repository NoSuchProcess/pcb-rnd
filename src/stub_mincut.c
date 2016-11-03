/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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
 */

#include "config.h"
#include <stdlib.h>
#include "global_objs.h"
#include "stub_mincut.h"
#include "const.h"
#include "flag.h"

static void stub_rat_proc_shorts_dummy(void)
{
}

static void stub_rat_found_short_dummy(PinType * pin, PadType * pad, const char *with_net)
{
	/* original behavior: just warn at random pins/pads */
	if (pin != NULL)
		SET_FLAG(PCB_FLAG_WARN, pin);
	if (pad != NULL)
		SET_FLAG(PCB_FLAG_WARN, pad);

	stub_rat_proc_shorts_dummy();
}

void (*stub_rat_found_short)(PinType * pin, PadType * pad, const char *with_net) = stub_rat_found_short_dummy;
void (*stub_rat_proc_shorts)(void) = stub_rat_proc_shorts_dummy;
