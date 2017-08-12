/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
 *
 *  This module, rats.c, was written and is Copyright (C) 1997 by harry eaton
 *  this module is also subject to the GNU GPL as described below
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

#include <ctype.h>
#include <genht/htsp.h>
#include <genvector/vtp0.h>

#include "compat_misc.h"
#include "obj_common.h"
#include "obj_term.h"

static int term_name_invalid(const char *tname)
{
	if ((tname == NULL) || (*tname == '\0'))
		return 1;
	for(;*tname != '\0'; tname++)
		if ((!isalnum(*tname)) && (*tname != '_') && (*tname != '-'))
			return 1;
	return 0;
}

pcb_term_err_t pcb_term_name_is_valid(const char *tname)
{
	if (term_name_invalid(tname))
		return PCB_TERM_ERR_INVALID_NAME;

	return PCB_TERM_ERR_SUCCESS;
}

pcb_term_err_t pcb_term_add(htsp_t *terminals, const char *tname, const pcb_any_obj_t *obj)
{
	htsp_entry_t *e;
	vtp0_t *v;

#warning TODO
/*
	if (obj->term != NULL)
		return PCB_TERM_ERR_ALREADY_TERMINAL;
*/

	if (term_name_invalid(tname))
		return PCB_TERM_ERR_INVALID_NAME;

	e = htsp_getentry(terminals, tname);
	if (e == NULL) {
		/* allocate new terminal */
		tname = pcb_strdup(tname);
		v = malloc(sizeof(vtp0_t));
		vtp0_init(v);
		htsp_set(terminals, tname, v);
	}
	else {
		/* need to use the ones from the hash to avoid extra allocation/leak */
		v = e->value;
		tname = e->key;
	}

#warning TODO
/*	obj->term = tname;*/
	vtp0_append(v, obj);

	return PCB_TERM_ERR_SUCCESS;
}

pcb_term_err_t pcb_term_del(htsp_t *terminals, const pcb_any_obj_t *obj)
{
	return PCB_TERM_ERR_SUCCESS;
}

pcb_term_err_t pcb_term_remove(htsp_t *terminals, const char *tname)
{
	return PCB_TERM_ERR_SUCCESS;
}

vtp0_t *pcb_term_get(htsp_t *terminals, const char *tname)
{
	return NULL;
}

