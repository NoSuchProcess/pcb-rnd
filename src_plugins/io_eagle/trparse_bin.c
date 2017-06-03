/*
 *														COPYRIGHT
 *
 *	pcb-rnd, interactive printed circuit board design
 *	Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *	Contact addresses for paper mail and Email:
 *	Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *	Thomas.Nau@rz.uni-ulm.de
 *
 */

#include "config.h"

#include <stdlib.h>
#include <assert.h>

#include "error.h"

#include "trparse.h"
#include "trparse_bin.h"

static int eagle_bin_load(trparse_t *pst, const char *fn)
{

/*
	pst->doc = doc;
	pst->root = root;
*/

	return -1;
}

static int eagle_bin_unload(trparse_t *pst)
{
	return -1;
}

static trnode_t *eagle_bin_children(trparse_t *pst, trnode_t *node)
{
	return NULL;
}

trparse_calls_t trparse_bin_calls = {
	eagle_bin_load,
	eagle_bin_unload,
	eagle_bin_children
};
