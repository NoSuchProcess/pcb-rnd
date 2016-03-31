/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <liblihata/lihata.h>
#include <liblihata/tree.h>
#include <genht/hash.h>

#include "global.h"
#include "hid.h"
#include "hid_cfg.h"
#include "hid_cfg_input.h"
#include "hid_actions.h"
#include "error.h"

/* split value into a list of '-' separated words; examine each word
   and set the bitmask of modifiers */
static hid_cfg_mod_t parse_mods(const char *value)
{
	hid_cfg_mod_t m = 0;
	int press = 0;

	for(;;) {
		if (strncmp(value, "shift", 5) == 0)        m |= M_Shift;
		else if (strncmp(value, "ctrl", 4) == 0)    m |= M_Ctrl;
		else if (strncmp(value, "alt", 3) == 0)     m |= M_Alt;
		else if (strncmp(value, "release", 7) == 0) m |= M_Release;
		else if (strncmp(value, "press", 5) == 0)   press = 1;
		else
			Message("Unkown modifier: %s\n", value);
		/* skip to next word */
		value = strchr(value, '-');
		if (value == NULL)
			break;
		value++;
	}

	if (press && (m & M_Release))
		Message("Bogus modifier: both press and release\n");

	return m;
}

static hid_cfg_mod_t button_name2mask(const char *name)
{
	/* All mouse-related resources must be named.  The name is the
	   mouse button number.  */
	if (!name)
		return 0;
	else if (strcasecmp(name, "left") == 0)   return MB_LEFT;
	else if (strcasecmp(name, "middle") == 0) return MB_MIDDLE;
	else if (strcasecmp(name, "right") == 0)  return MB_RIGHT;
	else if (strcasecmp(name, "up") == 0)     return MB_UP;
	else if (strcasecmp(name, "down") == 0)   return MB_DOWN;
	else {
		Message("Error: unknown mouse button: %s\n", name);
		return 0;
	}
}


static int keyeq_int(htip_key_t a, htip_key_t b)   { return a == b; }
static unsigned int keyhash_int(htip_key_t a)      { return murmurhash32(a & 0xFFFF); }

int hid_cfg_mouse_init(hid_cfg_t *hr, hid_cfg_mouse_t *mouse)
{
	lht_node_t *btn, *m;

	mouse->mouse = hid_cfg_get_menu(hr, "/mouse");

	if (mouse->mouse == NULL) {
		Message("Warning: no /mouse section in the resource file - mouse is disabled\n");
		return -1;
	}

	if (mouse->mouse->type != LHT_LIST) {
		hid_cfg_error(mouse->mouse, "Warning: should be a list - mouse is disabled\n");
		return -1;
	}

	if (mouse->mouse_mask == NULL)
		mouse->mouse_mask = htip_alloc(keyhash_int, keyeq_int);
	else
		htip_clear(mouse->mouse_mask);

	for(btn = mouse->mouse->data.list.first; btn != NULL; btn = btn->next) {
		hid_cfg_mod_t btn_mask = button_name2mask(btn->name);
		if (btn_mask == 0) {
			hid_cfg_error(btn, "unknown mouse button");
			continue;
		}
		if (btn->type != LHT_LIST) {
			hid_cfg_error(btn, "needs to be a list");
			continue;
		}
		for(m = btn->data.list.first; m != NULL; m = m->next) {
			hid_cfg_mod_t mod_mask = parse_mods(m->name);
			htip_set(mouse->mouse_mask, btn_mask|mod_mask, m);
		}
	}
	return 0;
}

static lht_node_t *find_best_action(hid_cfg_mouse_t *mouse, hid_cfg_mod_t button_and_mask)
{
	lht_node_t *n;

	if (mouse->mouse_mask == NULL)
		return NULL;

	/* look for exact mod match */
	n = htip_get(mouse->mouse_mask, button_and_mask);
	if (n != NULL)
		return n;

	if (button_and_mask & M_Release) {
		/* look for plain release for the given button */
		n = htip_get(mouse->mouse_mask, (button_and_mask & M_ANY) | M_Release);
		if (n != NULL)
			return n;
	}

	return NULL;
}

void hid_cfg_mouse_action(hid_cfg_mouse_t *mouse, hid_cfg_mod_t button_and_mask)
{
	hid_cfg_action(find_best_action(mouse, button_and_mask));
}
