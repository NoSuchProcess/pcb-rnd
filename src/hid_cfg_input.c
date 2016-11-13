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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <liblihata/lihata.h>
#include <liblihata/tree.h>
#include <genht/hash.h>
#include <genvector/gds_char.h>

#include "config.h"
#include "hid_cfg_input.h"
#include "hid_cfg_action.h"
#include "error.h"

/* split value into a list of '-' separated words; examine each word
   and set the bitmask of modifiers */
static pcb_hid_cfg_mod_t parse_mods(const char *value, const char **last, unsigned int vlen)
{
	pcb_hid_cfg_mod_t m = 0;
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
			pcb_message(PCB_MSG_DEFAULT, "Unknown modifier: %s\n", value);
		/* skip to next word */
		next = strpbrk(value, "<- \t");
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
		pcb_message(PCB_MSG_DEFAULT, "Bogus modifier: both press and release\n");

	return m;
}

static pcb_hid_cfg_mod_t button_name2mask(const char *name)
{
	/* All mouse-related resources must be named.  The name is the
	   mouse button number.  */
	if (!name)
		return 0;
	else if (strcasecmp(name, "left") == 0)   return MB_LEFT;
	else if (strcasecmp(name, "middle") == 0) return MB_MIDDLE;
	else if (strcasecmp(name, "right") == 0)  return MB_RIGHT;

	else if (strcasecmp(name, "scroll-up") == 0)     return MB_SCROLL_UP;
	else if (strcasecmp(name, "scroll-down") == 0)   return MB_SCROLL_DOWN;
	else if (strcasecmp(name, "scroll-left") == 0)   return MB_SCROLL_UP;
	else if (strcasecmp(name, "scroll-right") == 0)  return MB_SCROLL_DOWN;
	else {
		pcb_message(PCB_MSG_DEFAULT, "Error: unknown mouse button: %s\n", name);
		return 0;
	}
}

static unsigned int keyhash_int(htip_key_t a)      { return murmurhash32(a & 0xFFFF); }

/************************** MOUSE ***************************/

int hid_cfg_mouse_init(pcb_hid_cfg_t *hr, pcb_hid_cfg_mouse_t *mouse)
{
	lht_node_t *btn, *m;

	mouse->mouse = pcb_hid_cfg_get_menu(hr, "/mouse");

	if (mouse->mouse == NULL) {
		pcb_message(PCB_MSG_DEFAULT, "Warning: no /mouse section in the resource file - mouse is disabled\n");
		return -1;
	}

	if (mouse->mouse->type != LHT_LIST) {
		hid_cfg_error(mouse->mouse, "Warning: should be a list - mouse is disabled\n");
		return -1;
	}

	if (mouse->mouse_mask == NULL)
		mouse->mouse_mask = htip_alloc(keyhash_int, htip_keyeq);
	else
		htip_clear(mouse->mouse_mask);

	for(btn = mouse->mouse->data.list.first; btn != NULL; btn = btn->next) {
		pcb_hid_cfg_mod_t btn_mask = button_name2mask(btn->name);
		if (btn_mask == 0) {
			hid_cfg_error(btn, "unknown mouse button");
			continue;
		}
		if (btn->type != LHT_LIST) {
			hid_cfg_error(btn, "needs to be a list");
			continue;
		}
		for(m = btn->data.list.first; m != NULL; m = m->next) {
			pcb_hid_cfg_mod_t mod_mask = parse_mods(m->name, NULL, -1);
			htip_set(mouse->mouse_mask, btn_mask|mod_mask, m);
		}
	}
	return 0;
}

static lht_node_t *find_best_action(pcb_hid_cfg_mouse_t *mouse, pcb_hid_cfg_mod_t button_and_mask)
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

void hid_cfg_mouse_action(pcb_hid_cfg_mouse_t *mouse, pcb_hid_cfg_mod_t button_and_mask)
{
	hid_cfg_action(find_best_action(mouse, button_and_mask));
}


/************************** KEYBOARD ***************************/
int hid_cfg_keys_init(pcb_hid_cfg_keys_t *km)
{
	htip_init(&km->keys, keyhash_int, htip_keyeq);
	return 0;
}

int hid_cfg_keys_uninit(pcb_hid_cfg_keys_t *km)
{
#warning TODO: recursive free of nodes
	htip_uninit(&km->keys);
	return 0;
}

pcb_hid_cfg_keyseq_t *hid_cfg_keys_add_under(pcb_hid_cfg_keys_t *km, pcb_hid_cfg_keyseq_t *parent, pcb_hid_cfg_mod_t mods, unsigned short int key_char, int terminal)
{
	pcb_hid_cfg_keyseq_t *ns;
	hid_cfg_keyhash_t addr;
	htip_t *phash = (parent == NULL) ? &km->keys : &parent->seq_next;

	/* do not grow the tree under actions */
	if ((parent != NULL) && (parent->action_node != NULL))
		return NULL;


	addr.hash = 0;
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
	ns = calloc(sizeof(pcb_hid_cfg_keyseq_t), 1);
	if (!terminal)
		htip_init(&ns->seq_next, keyhash_int, htip_keyeq);
	htip_set(phash, addr.hash, ns);
	return ns;
}

const pcb_hid_cfg_keytrans_t hid_cfg_key_default_trans[] = {
	{ "semicolon", ';'  },
	{ NULL,        0    },
};

static unsigned short int translate_key(pcb_hid_cfg_keys_t *km, const char *desc, int len)
{
	char tmp[256];

	if ((km->auto_chr) && (len == 1))
			return *desc;

	if (len > sizeof(tmp)-1) {
		pcb_message(PCB_MSG_DEFAULT, "key sym name too long\n");
		return 0;
	}
	strncpy(tmp, desc, len);
	tmp[len] = '\0';

	if (km->auto_tr != NULL) {
		const pcb_hid_cfg_keytrans_t *t;
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

static int parse_keydesc(pcb_hid_cfg_keys_t *km, const char *keydesc, pcb_hid_cfg_mod_t *mods, unsigned short int *key_chars, int arr_len)
{
	const char *curr, *next, *last, *k;
	int slen, len;

	slen = 0;
	curr = keydesc;
	do {
		if (slen >= arr_len)
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
			pcb_message(PCB_MSG_DEFAULT, "Missing <key> in the key description: '%s'\n", keydesc);
			return -1;
		}
		len -= k-last;
		k++; len--;
		if ((strncmp(k, "key>", 4) != 0) && (strncmp(k, "Key>", 4) != 0)) {
			pcb_message(PCB_MSG_DEFAULT, "Missing <key> in the key description\n");
			return -1;
		}
		k+=4; len-=4;
		key_chars[slen] = translate_key(km, k, len);

		if (key_chars[slen] == 0) {
			char *s;
			s = malloc(len+1);
			memcpy(s, k, len);
			s[len] = '\0';
			pcb_message(PCB_MSG_DEFAULT, "Unrecognised key symbol in key description: %s\n", s);
			free(s);
			return -1;
		}

		slen++;
		curr = next;
	} while(curr != NULL);
	return slen;
}

int hid_cfg_keys_add_by_strdesc(pcb_hid_cfg_keys_t *km, const char *keydesc, const lht_node_t *action_node, pcb_hid_cfg_keyseq_t **out_seq, int out_seq_len)
{
	pcb_hid_cfg_mod_t mods[HIDCFG_MAX_KEYSEQ_LEN];
	unsigned short int key_chars[HIDCFG_MAX_KEYSEQ_LEN];
	pcb_hid_cfg_keyseq_t *lasts;
	int slen, n;

	slen = parse_keydesc(km, keydesc, mods, key_chars, HIDCFG_MAX_KEYSEQ_LEN);
	if (slen <= 0)
		return slen;

	if ((out_seq != NULL) && (slen >= out_seq_len))
		return -1;

/*	printf("KEY insert\n");*/

	lasts = NULL;
	for(n = 0; n < slen; n++) {
		pcb_hid_cfg_keyseq_t *s;
		int terminal = (n == slen-1);

/*		printf(" mods=%x sym=%x\n", mods[n], key_chars[n]);*/

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

int hid_cfg_keys_add_by_desc(pcb_hid_cfg_keys_t *km, const lht_node_t *keydescn, const lht_node_t *action_node, pcb_hid_cfg_keyseq_t **out_seq, int out_seq_len)
{
	switch(keydescn->type) {
		case LHT_TEXT: return hid_cfg_keys_add_by_strdesc(km, keydescn->data.text.value, action_node, out_seq, out_seq_len);
		case LHT_LIST:
		{
			int ret = -1, cnt;
			lht_node_t *n;
			for(n = keydescn->data.list.first, cnt = 0; n != NULL; n = n->next, cnt++) {
				if (n->type != LHT_TEXT)
					break;
				if (cnt == 0)
					ret = hid_cfg_keys_add_by_strdesc(km, n->data.text.value, action_node, out_seq, out_seq_len);
				else
					hid_cfg_keys_add_by_strdesc(km, n->data.text.value, action_node, NULL, 0);
			}
			return ret;
		}
		default:;
	}
	return -1;
}

static void gen_accel(gds_t *s, pcb_hid_cfg_keys_t *km, const char *keydesc, int *cnt, const char *sep)
{
	pcb_hid_cfg_mod_t mods[HIDCFG_MAX_KEYSEQ_LEN];
	unsigned short int key_chars[HIDCFG_MAX_KEYSEQ_LEN];
	int slen, n;

	slen = parse_keydesc(km, keydesc, mods, key_chars, HIDCFG_MAX_KEYSEQ_LEN);
	if (slen <= 0)
		return;

	if (*cnt > 0)
		gds_append_str(s, sep);

	for(n = 0; n < slen; n++) {
		char buff[64];

		if (n > 0)
			gds_append(s, ' ');

		if (km->key_name(key_chars[n], buff, sizeof(buff)) != 0)
			strcpy(buff, "<unknown>");

		if (mods[n] & M_Alt)   gds_append_str(s, "Alt-");
		if (mods[n] & M_Ctrl)  gds_append_str(s, "Ctrl-");
		if (mods[n] & M_Shift) gds_append_str(s, "Shift-");
		gds_append_str(s, buff);
	}
}

char *hid_cfg_keys_gen_accel(pcb_hid_cfg_keys_t *km, const lht_node_t *keydescn, unsigned long mask, const char *sep)
{
	gds_t s;
	int cnt = 0;

	memset(&s, 0, sizeof(s));

	switch(keydescn->type) {
		case LHT_TEXT:
			if (mask & 1)
				gen_accel(&s, km, keydescn->data.text.value, &cnt, sep);
			break;
		case LHT_LIST:
		{
			int cnt;
			lht_node_t *n;
			for(n = keydescn->data.list.first, cnt = 0; n != NULL; n = n->next, cnt++, mask >>= 1) {
				if (n->type != LHT_TEXT)
					break;
				if (!(mask & 1))
					continue;
				gen_accel(&s, km, n->data.text.value, &cnt, sep);
			}
		}
		default:;
	}
	return s.array;
}


int hid_cfg_keys_input(pcb_hid_cfg_keys_t *km, pcb_hid_cfg_mod_t mods, unsigned short int key_char, pcb_hid_cfg_keyseq_t **seq, int *seq_len)
{
	pcb_hid_cfg_keyseq_t *ns;
	hid_cfg_keyhash_t addr;
	htip_t *phash = (*seq_len == 0) ? &km->keys : &((seq[(*seq_len)-1])->seq_next);

	addr.hash = 0;
	addr.details.mods = mods;
	addr.details.key_char = key_char;

	/* already in the tree */
	ns = htip_get(phash, addr.hash);
	if (ns == NULL) {
		(*seq_len) = 0;
		return -1;
	}

	seq[*seq_len] = ns;
	(*seq_len)++;

	/* found a terminal node with an action */
	if (ns->action_node != NULL) {
		int len = *seq_len;
		(*seq_len) = 0;
		return len;
	}

	return 0;
}

int hid_cfg_keys_action(pcb_hid_cfg_keyseq_t **seq, int seq_len)
{
	if (seq_len < 1)
		return -1;

	return hid_cfg_action(seq[seq_len-1]->action_node);
}
