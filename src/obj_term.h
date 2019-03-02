/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
#include "layer.h"

typedef enum pcb_term_err_e {
	PCB_TERM_ERR_SUCCESS = 0,
	PCB_TERM_ERR_ALREADY_TERMINAL, /* object is already in a terminal, can not be added in another */
	PCB_TERM_ERR_NOT_IN_TERMINAL,  /* object is not part of any terminal */
	PCB_TERM_ERR_TERM_NOT_FOUND,
	PCB_TERM_ERR_NOT_IN_SUBC,
	PCB_TERM_ERR_NO_CHANGE,
	PCB_TERM_ERR_INVALID_NAME
} pcb_term_err_t;

/* any type that can be a temrinal */
#define PCB_TERM_OBJ_TYPES \
	(PCB_OBJ_LINE | PCB_OBJ_TEXT | PCB_OBJ_POLY | PCB_OBJ_ARC | PCB_OBJ_PSTK | PCB_OBJ_SUBC)

/* Initialize a clean hash of terminals for a subcircuit */
pcb_term_err_t pcb_term_init(htsp_t *terminals);

/* Remove all objects from all terminals and destroy the hash */
pcb_term_err_t pcb_term_uninit(htsp_t *terminals);

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

/* Rename an object's ->term field in an undoable way */
pcb_term_err_t pcb_term_undoable_rename(pcb_board_t *pcb, pcb_any_obj_t *obj, const char *new_name);

/* Look up subc_name/term_name on layers matching lyt. Returns the object
   or NULL if not found. If the *out parameters are non-NULL, load them.
   Ignores subcircuits marked as nonetlist even if explicitly named. */
pcb_any_obj_t *pcb_term_find_name(const pcb_board_t *pcb, pcb_data_t *data, pcb_layer_type_t lyt, const char *subc_name, const char *term_name, pcb_subc_t **parent_out, pcb_layergrp_id_t *gid_out);

#endif
