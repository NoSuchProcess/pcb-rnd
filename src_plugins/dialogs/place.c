/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

typedef struct {
	int x, y, w, h;
} wingeo_t;

wingeo_t wingeo_invalid = {0, 0, 0, 0};

typedef const char *htsw_key_t;
typedef wingeo_t htsw_value_t;
#define HT(x) htsw_ ## x
#define HT_INVALID_VALUE wingeo_invalid;
#include <genht/ht.h>
#include <genht/ht.c>
#undef HT
#include <genht/hash.h>

static htsw_t wingeo;

static void pcb_dialog_store(const char *id, int x, int y, int w, int h)
{
	htsw_entry_t *e;
	wingeo_t wg;

/*	pcb_trace("dialog place set: '%s' %d;%d  %d*%d\n", id, x, y, w, h);*/

	e = htsw_getentry(&wingeo, (char *)id);
	if (e != NULL) {
		e->value.x = x;
		e->value.y = y;
		e->value.w = w;
		e->value.h = h;
		return;
	}

	wg.x = x;
	wg.y = y;
	wg.w = w;
	wg.h = h;
	htsw_set(&wingeo, pcb_strdup(id), wg);
}


static void pcb_dialog_place(void *user_data, int argc, pcb_event_arg_t argv[])
{
	void *hid_ctx;
	const char *id;

	if ((argc < 3) || (argv[1].type != PCB_EVARG_PTR) || (argv[2].type != PCB_EVARG_STR))
		return;

	hid_ctx = argv[1].d.p;
	id = argv[2].d.s;
/*	pcb_trace("dialog place: %p '%s'\n", hid_ctx, id);*/
}

static void pcb_dialog_resize(void *user_data, int argc, pcb_event_arg_t argv[])
{
	if ((argc < 7) || (argv[1].type != PCB_EVARG_PTR) || (argv[2].type != PCB_EVARG_STR))
		return;

/*	hid_ctx = argv[1].d.p;*/
	pcb_dialog_store(argv[2].d.s, argv[3].d.i, argv[4].d.i, argv[5].d.i, argv[6].d.i);
}

static void pcb_dialog_place_init(void)
{
	htsw_init(&wingeo, strhash, strkeyeq);
}

static void pcb_dialog_place_uninit(void)
{
	htsw_entry_t *e;
	for(e = htsw_first(&wingeo); e != NULL; e = htsw_next(&wingeo, e))
		free((char *)e->key);
	htsw_uninit(&wingeo);
}
