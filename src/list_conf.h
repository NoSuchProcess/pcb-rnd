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

#ifndef LIST_CONF_H
#define LIST_CONF_H

#include "conf.h"

#include <genlist/gentdlist_undef.h>

#ifdef FROM_CONF_C
#define TDL_DONT_UNDEF
#endif

#define TDL(x)      conflist_ ## x
#define TDL_LIST_T  conflist_t
#define TDL_LIST_U  conflist_u
#define TDL_ITEM_T  conf_listitem_t
#define TDL_FIELD   link
#define TDL_SIZE_T  size_t
#define TDL_FUNC

#define conflist_foreach(list, iterator, loop_elem) \
	gdl_foreach_((&((list)->lst)), (iterator), (loop_elem))


#include <genlist/gentdlist_impl.h>
#include <genlist/gentdlist_undef.h>

#endif
