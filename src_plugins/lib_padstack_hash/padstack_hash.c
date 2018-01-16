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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <stdio.h>
#include <genht/htpp.h>

#include "padstack_hash.h"
#include "obj_elem.h"
#include "obj_pad.h"
#include "obj_pinvia.h"

typedef struct pcb_pshash_item_s {
	char name[16];
	int is_pad;
	union {
		const pcb_pin_t *pin;
		const pcb_pad_t *pad;
		const void *any;
	} ptr;
} pcb_pshash_item_t;

static unsigned int pcb_pshash_padhash(const pcb_pshash_item_t *key)
{
	return pcb_pad_hash_padstack(key->ptr.pad);
}

static unsigned int pcb_pshash_pinhash(const pcb_pshash_item_t *key)
{
	return pcb_pin_hash_padstack(key->ptr.pin);
}

static int pcb_pshash_padeq(const pcb_pshash_item_t *key1, const pcb_pshash_item_t *key2)
{
#warning padstack TODO: move these tests to  pcb_pad_eq_padstack
	return (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, key1->ptr.pad) == PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, key2->ptr.pad)) &&
		(PCB_FLAG_TEST(PCB_FLAG_SQUARE, key1->ptr.pad) == PCB_FLAG_TEST(PCB_FLAG_SQUARE, key2->ptr.pad)) &&
		(PCB_FLAG_TEST(PCB_FLAG_OCTAGON, key1->ptr.pad) == PCB_FLAG_TEST(PCB_FLAG_OCTAGON, key2->ptr.pad)) &&
		pcb_pad_eq_padstack(key1->ptr.pad, key2->ptr.pad);
}

static int pcb_pshash_pineq(const pcb_pshash_item_t *key1, const pcb_pshash_item_t *key2)
{
#warning padstack TODO: move these tests to  pcb_pin_eq_padstack
	return (PCB_FLAG_TEST(PCB_FLAG_SQUARE, key1->ptr.pin) == PCB_FLAG_TEST(PCB_FLAG_SQUARE, key2->ptr.pin)) &&
		(PCB_FLAG_TEST(PCB_FLAG_OCTAGON, key1->ptr.pin) == PCB_FLAG_TEST(PCB_FLAG_OCTAGON, key2->ptr.pin)) &&
		pcb_pin_eq_padstack(key1->ptr.pin, key2->ptr.pin);
}


static unsigned int pcb_pshash_keyhash(const void *key_)
{
	const pcb_pshash_item_t *key = key_;
	if (key->is_pad)
		return pcb_pshash_padhash(key);
	return pcb_pshash_pinhash(key);
}

static int pcb_pshash_keyeq(const void *key1_, const void *key2_)
{
	const pcb_pshash_item_t *key1 = key1_, *key2 = key2_;

	if (key1->is_pad != key2->is_pad)
		return 0;

	if (key1->ptr.any == key2->ptr.any)
		return 1;

	if (key1->is_pad)
		return pcb_pshash_padeq(key1, key2);
	return pcb_pshash_pineq(key1, key2);
}

void pcb_pshash_init(pcb_pshash_hash_t *psh)
{
	psh->cnt = 0;
	htpp_init(&psh->ht, pcb_pshash_keyhash, pcb_pshash_keyeq);
}

void pcb_pshash_uninit(pcb_pshash_hash_t *psh)
{
	htpp_entry_t *e;
	for (e = htpp_first(&psh->ht); e; e = htpp_next(&psh->ht, e))
		free(e->key);

	htpp_uninit(&psh->ht);
}

static const char *pcb_pshash_pp(pcb_pshash_hash_t *psh, pcb_pshash_item_t *key, int *new_item)
{
	const char *name = htpp_get(&psh->ht, key);
	pcb_pshash_item_t *nkey;

	if (new_item == NULL) {
		/* read-only lookup */
		return name;
	}

	if (name != NULL) {
		/* already registered */
		*new_item = 0;
		return name;
	}

	/* register new */
	*new_item = 1;

	nkey = malloc(sizeof(pcb_pshash_item_t));
	nkey->is_pad = key->is_pad;
	nkey->ptr.any = key->ptr.any;

	if (nkey->is_pad)
		sprintf(nkey->name, "pad_%d", psh->cnt);
	else
		sprintf(nkey->name, "pin_%d", psh->cnt);
	psh->cnt++;

	htpp_set(&psh->ht, nkey, nkey->name);
	return nkey->name;
}


const char *pcb_pshash_pad(pcb_pshash_hash_t *psh, const pcb_pad_t *pad, int *new_item)
{
	pcb_pshash_item_t key;
	key.is_pad = 1;
	key.ptr.pad = pad;
	return pcb_pshash_pp(psh, &key, new_item);
}

const char *pcb_pshash_pin(pcb_pshash_hash_t *psh, const pcb_pin_t *pin, int *new_item)
{
	pcb_pshash_item_t key;
	key.is_pad = 0;
	key.ptr.pin = pin;
	return pcb_pshash_pp(psh, &key, new_item);
}


int pplg_check_ver_lib_padstack_hash(int ver_needed) { return 0; }

void pplg_uninit_lib_padstack_hash(void)
{
}

int pplg_init_lib_padstack_hash(void)
{
	return 0;
}
