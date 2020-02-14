/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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

#include <librnd/config.h>

#include <stdlib.h>
#include <genht/hash.h>
#include <string.h>

#include <librnd/core/pixmap.h>

#include <librnd/core/error.h>

static unsigned int pixmap_hash_(const void *key_, int pixels)
{
	pcb_pixmap_t *key = (pcb_pixmap_t *)key_;
	unsigned int i;

	if (pixels && key->hash_valid)
		return key->hash;

	i = longhash(key->sx);
	i ^= longhash(key->sy);
	i ^= longhash((long)(key->tr_rot * 1000.0) + (((long)key->tr_xmirror) << 3) + (((long)key->tr_ymirror) << 4));
	i ^= longhash((long)(key->tr_xscale * 1000.0) + (long)(key->tr_yscale * 1000.0));
	if (pixels) {
		i ^= jenhash(key->p, key->size);
		key->hash = i;
		key->hash_valid = 1;
	}
	else
		i ^= longhash(key->neutral_oid);
	return i;
}

unsigned int pcb_pixmap_hash_meta(const void *key)
{
	return pixmap_hash_(key, 0);
}

unsigned int pcb_pixmap_hash_pixels(const void *key)
{
	return pixmap_hash_(key, 1);
}


static int pixmap_eq_(const void *keya_, const void *keyb_, int pixels)
{
	const pcb_pixmap_t *keya = keya_, *keyb = keyb_;
	if ((keya->transp_valid) || (keyb->transp_valid)) {
		if (keya->transp_valid != keyb->transp_valid)
			return 0;
		if ((keya->tr != keyb->tr) || (keya->tg != keyb->tg) || (keya->tb != keyb->tb))
			return 0;
	}
	if ((keya->tr_xmirror != keyb->tr_xmirror) || (keya->tr_ymirror != keyb->tr_ymirror))
		return 0;
	if ((keya->tr_rot != keyb->tr_rot) || (keya->tr_xscale != keyb->tr_xscale) || (keya->tr_yscale != keyb->tr_yscale))
		return 0;
	if ((keya->sx != keyb->sx) || (keya->sy != keyb->sy))
		return 0;
	if (pixels)
	 return (memcmp(keya->p, keyb->p, keya->size) == 0);
	else
		return (keya->neutral_oid == keyb->neutral_oid);
}

int pcb_pixmap_eq_meta(const void *keya, const void *keyb)
{
	return pixmap_eq_(keya, keyb, 0);
}

int pcb_pixmap_eq_pixels(const void *keya, const void *keyb)
{
	return pixmap_eq_(keya, keyb, 1);
}


static pcb_pixmap_import_t *pcb_pixmap_chain;

void pcb_pixmap_reg_import(const pcb_pixmap_import_t *imp, const char *cookie)
{
	pcb_pixmap_import_t *i = malloc(sizeof(pcb_pixmap_import_t));
	memcpy(i, imp, sizeof(pcb_pixmap_import_t));
	i->cookie = cookie;
	i->next = pcb_pixmap_chain;
	pcb_pixmap_chain = i;
}

static void pcb_pixmap_unreg_(pcb_pixmap_import_t *i, pcb_pixmap_import_t *prev)
{
	if (prev == NULL)
		pcb_pixmap_chain = i->next;
	else
		prev->next = i->next;
	free(i);
}

void pcb_pixmap_unreg_import_all(const char *cookie)
{
	pcb_pixmap_import_t *i, *prev = NULL, *next;
	for(i = pcb_pixmap_chain; i != NULL; i = next) {
		next = i->next;
		if (i->cookie == cookie)
			pcb_pixmap_unreg_(i, prev);
		else
			prev = i;
	}
}

void rnd_pixmap_uninit(void)
{
	pcb_pixmap_import_t *i;
	for(i = pcb_pixmap_chain; i != NULL; i = i->next)
		pcb_message(PCB_MSG_ERROR, "pcb_pixmap_chain is not empty: %s. Fix your plugins!\n", i->cookie);
}

int pcb_pixmap_load(pcb_hidlib_t *hidlib, pcb_pixmap_t *pxm, const char *fn)
{
	pcb_pixmap_import_t *i;
	for(i = pcb_pixmap_chain; i != NULL; i = i->next)
		if (i->load(hidlib, pxm, fn) == 0)
			return 0;
	return -1;
}

pcb_pixmap_t *rnd_pixmap_load(pcb_hidlib_t *hidlib, const char *fn)
{
	pcb_pixmap_t *p = calloc(sizeof(pcb_pixmap_t), 1);
	if (pcb_pixmap_load(hidlib, p, fn) == 0)
		return p;
	free(p);
	return NULL;
}

void rnd_pixmap_free_fields(pcb_pixmap_t *pxm)
{
	free(pxm->p);
}

void rnd_pixmap_free(pcb_pixmap_t *pxm)
{
	rnd_pixmap_free_fields(pxm);
	free(pxm);
}
