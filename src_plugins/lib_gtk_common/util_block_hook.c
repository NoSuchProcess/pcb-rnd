/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
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

#include "config.h"
#include <glib.h>
#include "util_block_hook.h"

typedef struct {
	GSource source;
	void (*func) (pcb_hidval_t user_data);
	pcb_hidval_t user_data;
} BlockHookSource;

static GSourceFuncs ghid_block_hook_funcs = {
	ghid_block_hook_prepare,
	ghid_block_hook_check,
	ghid_block_hook_dispatch,
	NULL													/* No destroy notification */
};

gboolean ghid_block_hook_prepare(GSource * source, gint * timeout)
{
	pcb_hidval_t data = ((BlockHookSource *) source)->user_data;
	((BlockHookSource *) source)->func(data);
	return FALSE;
}

gboolean ghid_block_hook_check(GSource * source)
{
	return FALSE;
}

gboolean ghid_block_hook_dispatch(GSource * source, GSourceFunc callback, gpointer user_data)
{
	return FALSE;
}

pcb_hidval_t ghid_add_block_hook(void (*func) (pcb_hidval_t data), pcb_hidval_t user_data)
{
	pcb_hidval_t ret;
	BlockHookSource *source;

	source = (BlockHookSource *) g_source_new(&ghid_block_hook_funcs, sizeof(BlockHookSource));

	source->func = func;
	source->user_data = user_data;

	g_source_attach((GSource *) source, NULL);

	ret.ptr = (void *) source;
	return ret;
}

void ghid_stop_block_hook(pcb_hidval_t mlpoll)
{
	GSource *source = (GSource *) mlpoll.ptr;
	g_source_destroy(source);
}
