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
static hid_cfg_mod_t parse_mods(const char *value, const char **last, unsigned int vlen)
{
	hid_cfg_mod_t m = 0;
	int press = 0;
	const char *next;

	while(isspace(*value)) value++;

	if (*value != '<') {
	for(;;) {
		if ((vlen >= 5) && (strncasecmp(value, "shift", 5) == 0))        m |= M_Shift;
		else if ((vlen >= 4) && (strncasecmp(value, "ctrl", 4) == 0))    m |= M_Ctrl;
		else if ((vlen >= 3) && (strncasecmp(value, "alt", 3) == 0))     m |= M_Alt;
		else if ((vlen >= 7) && (strncasecmp(value, "release", 7) == 0)) m |= M_Release;
		else if ((vlen >= 5) && (strncasecmp(value, "press", 5) == 0))   press = 1;
		else
			Message("Unkown modifier: %s\n", value);
		/* skip to next word */
		next = strpbrk(value, "<-");
		if (next == NULL)
			break;
		if (*next == '<')
			break;
		vlen -= (next - value);
		value = next+1;
	}
	}

	if (last != NULL)
		*last = value;

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

/************************** MOUSE ***************************/

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
			hid_cfg_mod_t mod_mask = parse_mods(m->name, NULL, -1);
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


/************************** KEYBOARD ***************************/
int hid_cfg_keys_init(hid_cfg_keys_t *km)
{
	htip_init(&km->keys, keyhash_int, keyeq_int);
}

int hid_cfg_keys_uninit(hid_cfg_keys_t *km)
{
#warning TODO: recursive free of nodes
	htip_uninit(&km->keys);
}

hid_cfg_keyseq_t *hid_cfg_keys_add_under(hid_cfg_keys_t *km, hid_cfg_keyseq_t *parent, hid_cfg_mod_t mods, unsigned short int key_char, int terminal)
{
	hid_cfg_keyseq_t *ns;
	hid_cfg_keyhash_t addr;
	htip_t *phash = (parent == NULL) ? &km->keys : &parent->seq_next;

	/* do not grow the tree under actions */
	if ((parent != NULL) && (parent->action_node != NULL))
		return NULL;


	addr.details.mods = mods;
	addr.details.key_char = key_char;

	/* already in the tree */
	ns = htip_get(phash, addr.hash);
	if (ns != NULL) {
		if (terminal)
			return NULL; /* full-path-match is collision */
		return ns;
	}

	/* new node on this level */
	ns = calloc(sizeof(hid_cfg_keyseq_t), 1);
	htip_set(phash, addr.hash, ns);
	return ns;
}

const hid_cfg_keytrans_t hid_cfg_key_default_trans[] = {
	{ "semicolon", ';'  },
	{ "return",    '\r' },
	{ "enter",     '\r' },
	{ "space",     ' '  },
/*	{ "backspace", '\b' },*/
/*	{ "tab",       '\t' },*/
	{ NULL,        0    },
};

static unsigned short int translate_key(hid_cfg_keys_t *km, const char *desc, int len)
{
	char tmp[256];

	if ((km->auto_chr) && (len == 1))
			return *desc;

	if (len > sizeof(tmp)-1) {
		Message("key sym name too long\n");
		return 0;
	}
	strncpy(tmp, desc, len);
	tmp[len] = '\0';

	if (km->auto_tr != NULL) {
		const hid_cfg_keytrans_t *t;
		for(t = km->auto_tr; t->name != NULL; t++) {
			if (strcasecmp(tmp, t->name) == 0) {
				tmp[0] = t->sym;
				tmp[1] = '\0';
				len = 1;
				break;
			}
		}
	}

	return km->translate_key(tmp, len);
}


int hid_cfg_keys_add_by_desc(hid_cfg_keys_t *km, const char *keydesc, lht_node_t *action_node, hid_cfg_keyseq_t **out_seq, int out_seq_len)
{
	const char *curr, *next, *last, *k;
	hid_cfg_mod_t mods[HIDCFG_MAX_KEYSEQ_LEN];
	unsigned short int key_chars[HIDCFG_MAX_KEYSEQ_LEN];
	int n, slen, len;
	hid_cfg_keyseq_t *lasts;

	slen = 0;
	curr = keydesc;
	do {
		if (slen >= HIDCFG_MAX_KEYSEQ_LEN)
			return -1;
		while(isspace(*curr)) curr++;
		if (*curr == '\0')
			break;
		next = strchr(curr, ';');
		if (next != NULL) {
			len = next - curr;
			while(*next == ';') next++;
		}
		else
			len = strlen(curr);

		mods[slen] = parse_mods(curr, &last, len);

		k = strchr(last, '<');
		if (k == NULL) {
			Message("Missing <key> in the key description: '%s'\n", keydesc);
			return -1;
		}
		len -= k-last;
		k++; len--;
		if ((strncmp(k, "key>", 4) != 0) && (strncmp(k, "Key>", 4) != 0)) {
			Message("Missing <key> in the key description");
			return -1;
		}
		k+=4; len-=4;
		key_chars[slen] = translate_key(km, k, len);

		if (key_chars[slen] == 0) {
			Message("Unrecognised key symbol in key description");
			return -1;
		}


		slen++;
		curr = next;
	} while(curr != NULL);

	if ((out_seq != NULL) && (slen >= out_seq_len))
		return -1;

	printf("KEY insert\n");

	lasts = NULL;
	for(n = 0; n < slen; n++) {
		hid_cfg_keyseq_t *s;
		int terminal = (n == slen-1);

		printf(" mods=%x sym=%x\n", mods[n], key_chars[n]);

		s = hid_cfg_keys_add_under(km, lasts, mods[n], key_chars[n], terminal);
		if (s == NULL) {
		printf("  ERROR\n");
#warning TODO: free stuff?
			return -1;
		}
		if (terminal)
			s->action_node = action_node;

		if (out_seq != NULL)
			out_seq[n] = s;
		lasts = s;
	}

	return slen;
}

int hid_cfg_keys_input(hid_cfg_keys_t *km, hid_cfg_mod_t mods, unsigned short int key_char, hid_cfg_keyseq_t **seq, int *seq_len)
{
	hid_cfg_keyseq_t *ns;
	hid_cfg_keyhash_t addr;
	htip_t *phash = (*seq_len == 0) ? &km->keys : &(seq[*seq_len])->seq_next;

	addr.details.mods = mods;
	addr.details.key_char = key_char;

	/* already in the tree */
	ns = htip_get(phash, addr.hash);
	if (ns == NULL)
		return -1;

	seq[*seq_len] = ns;
	(*seq_len)++;

	/* found a terminal node with an action */
	if (ns->action_node != NULL) {
		int len = *seq_len;
		*seq_len = 0;
		return len;
	}

	return 0;
}

int hid_cfg_keys_action(hid_cfg_keyseq_t **seq, int seq_len)
{
	if (seq_len < 1)
		return -1;

	hid_cfg_action(seq[seq_len-1]->action_node);
#warning TODO:
	return 0;
}
