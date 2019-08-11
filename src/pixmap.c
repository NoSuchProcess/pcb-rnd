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

#include "config.h"

#include <genht/hash.h>
#include <string.h>

#include "pixmap.h"

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


