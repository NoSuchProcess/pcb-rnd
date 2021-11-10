/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2021 Tibor 'Igor2' Palinkas
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

/* The preferences dialog, application specific tabs */

#include "config.h"

#include <librnd/plugins/lib_hid_common/dlg_pref.h>

/* application tabs */
#undef  PREF_TAB
#define PREF_TAB 0
#include "dlg_pref_general.c"

#undef  PREF_TAB
#define PREF_TAB 1
#include "dlg_pref_board.c"

#undef  PREF_TAB
#define PREF_TAB 2
#include "dlg_pref_sizes.c"

#undef  PREF_TAB
#define PREF_TAB 3
#include "dlg_pref_lib.c"

#undef  PREF_TAB
#define PREF_TAB 4
#include "dlg_pref_layer.c"

#undef  PREF_TAB
#define PREF_TAB 5
#include "dlg_pref_color.c"

int pcb_dlg_pref_tab = PREF_TAB;
void (*pcb_dlg_pref_first_init)(pref_ctx_t *ctx, int tab) = PREF_INIT_FUNC;

