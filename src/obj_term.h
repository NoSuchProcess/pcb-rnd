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

/* Terminals: within a subcircuit, a terminal is a point of netlist connection.
   Subcircuit objects can be tagged to belong to a terminal, by terminal ID
   (similar tothe old pin number concept).

   A terminals is a str->ptr hash, keyed by terminal name. Each item is
   a vtp0_t vector cotaining one or more pcb_any_obj_t * pointers to the
   objects making up that terminal.
*/

#ifndef PCB_OBJ_TERM_H
#define PCB_OBJ_TERM_H

#include <genht/htsp.h>
#include <genvector/vtp0.h>
#include "obj_common.h"

typedef enum pcb_term_err_e {
	PCB_TERM_ERR_SUCCESS = 0,
	PCB_TERM_ERR_ALREADY_TERMINAL, /* object is already in a terminal, can not be added in another */
	PCB_TERM_ERR_NOT_IN_TERMINAL,  /* object is not part of any terminal */
	PCB_TERM_ERR_TERM_NOT_FOUND,
	PCB_TERM_ERR_INVALID_NAME
} pcb_term_err_t;

/* Determines if tname is a valid terminal name */
pcb_term_err_t pcb_term_name_is_valid(const char *tname);

/* Add obj to a list of terminals named tname. Sets the term field of obj if
   it is unset (else fails). */
pcb_term_err_t pcb_term_add(htsp_t *terminals, const char *tname, pcb_any_obj_t *obj);

/* Remove obj from terminal tname. Sets obj's term to NULL. Removes
   terminal if it becomes empty. */
pcb_term_err_t pcb_term_del(htsp_t *terminals, pcb_any_obj_t *obj);

/* Remove a terminal from, calling pcb_term_del() on all objects in it. */
pcb_term_err_t pcb_term_remove(htsp_t *terminals, const char *tname);

/* Returns a vector of (pcb_any_obj_t *) containing all objects for the named
   termina. Returns NULL if tname doesn't exist in terminals. */
#define pcb_term_get(terminals, tname) \
	(vtp0_t *)htsp_get(terminals, (char *)tname)


#endif
