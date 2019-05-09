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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <liblihata/lihata.h>
#include <liblihata/tree.h>
#include <genht/hash.h>
#include <genvector/gds_char.h>

#include "config.h"
#include "hid_cfg_input.h"
#include "hid_cfg_action.h"
#include "hidlib_conf.h"
#include "error.h"
#include "compat_misc.h"
#include "event.h"
#include "crosshair.h"

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
		if ((vlen >= 5) && (pcb_strncasecmp(value, "shift", 5) == 0))        m |= PCB_M_Shift;
		else if ((vlen >= 4) && (pcb_strncasecmp(value, "ctrl", 4) == 0))    m |= PCB_M_Ctrl;
		else if ((vlen >= 3) && (pcb_strncasecmp(value, "alt", 3) == 0))     m |= PCB_M_Alt;
		else if ((vlen >= 7) && (pcb_strncasecmp(value, "release", 7) == 0)) m |= PCB_M_Release;
		else if ((vlen >= 5) && (pcb_strncasecmp(value, "press", 5) == 0))   press = 1;
		else
			pcb_message(PCB_MSG_ERROR, "Unknown modifier: %s\n", value);
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

	if (press && (m & PCB_M_Release))
		pcb_message(PCB_MSG_ERROR, "Bogus modifier: both press and release\n");

	return m;
}

static pcb_hid_cfg_mod_t button_name2mask(const char *name)
{
	/* All mouse-related resources must be named.  The name is the
	   mouse button number.  */
	if (!name)
		return 0;
	else if (pcb_strcasecmp(name, "left") == 0)   return PCB_MB_LEFT;
	else if (pcb_strcasecmp(name, "middle") == 0) return PCB_MB_MIDDLE;
	else if (pcb_strcasecmp(name, "right") == 0)  return PCB_MB_RIGHT;

	else if (pcb_strcasecmp(name, "scroll-up") == 0)     return PCB_MB_SCROLL_UP;
	else if (pcb_strcasecmp(name, "scroll-down") == 0)   return PCB_MB_SCROLL_DOWN;
	else if (pcb_strcasecmp(name, "scroll-left") == 0)   return PCB_MB_SCROLL_UP;
	else if (pcb_strcasecmp(name, "scroll-right") == 0)  return PCB_MB_SCROLL_DOWN;
	else {
		pcb_message(PCB_MSG_ERROR, "Error: unknown mouse button: %s\n", name);
		return 0;
	}
}

static unsigned int keyhash_int(htip_key_t a)      { return murmurhash32(a & 0xFFFF); }

static unsigned int keyb_hash(const void *key_)
{
	const hid_cfg_keyhash_t *key = key_;
	unsigned int i = 0;
	i += key->key_raw; i <<= 8;
	i += ((unsigned int)key->key_tr) << 4;
	i += key->mods;
	return murmurhash32(i);
}

static int keyb_eq(const void *keya_, const void *keyb_)
{
	const hid_cfg_keyhash_t *keya = keya_, *keyb = keyb_;
	return (keya->key_raw == keyb->key_raw) && (keya->key_tr == keyb->key_tr) && (keya->mods == keyb->mods);
}

/************************** MOUSE ***************************/

int hid_cfg_mouse_init(pcb_hid_cfg_t *hr, pcb_hid_cfg_mouse_t *mouse)
{
	lht_node_t *btn, *m;

	mouse->mouse = pcb_hid_cfg_get_menu(hr, "/mouse");

	if (mouse->mouse == NULL) {
		pcb_message(PCB_MSG_ERROR, "Warning: no /mouse section in the resource file - mouse is disabled\n");
		return -1;
	}

	if (mouse->mouse->type != LHT_LIST) {
		pcb_hid_cfg_error(mouse->mouse, "Warning: should be a list - mouse is disabled\n");
		return -1;
	}

	if (mouse->mouse_mask == NULL)
		mouse->mouse_mask = htip_alloc(keyhash_int, htip_keyeq);
	else
		htip_clear(mouse->mouse_mask);

	for(btn = mouse->mouse->data.list.first; btn != NULL; btn = btn->next) {
		pcb_hid_cfg_mod_t btn_mask = button_name2mask(btn->name);
		if (btn_mask == 0) {
			pcb_hid_cfg_error(btn, "unknown mouse button");
			continue;
		}
		if (btn->type != LHT_LIST) {
			pcb_hid_cfg_error(btn, "needs to be a list");
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

	if (button_and_mask & PCB_M_Release) {
		/* look for plain release for the given button */
		n = htip_get(mouse->mouse_mask, (button_and_mask & PCB_M_ANY) | PCB_M_Release);
		if (n != NULL)
			return n;
	}

	return NULL;
}

void hid_cfg_mouse_action(pcb_hid_cfg_mouse_t *mouse, pcb_hid_cfg_mod_t button_and_mask, pcb_bool cmd_entry_active)
{
	pcbhl_conf.temp.click_cmd_entry_active = cmd_entry_active;
	pcb_hid_cfg_action(find_best_action(mouse, button_and_mask));
	pcb_event(NULL, PCB_EVENT_USER_INPUT_POST, NULL);
	pcbhl_conf.temp.click_cmd_entry_active = 0;
}


/************************** KEYBOARD ***************************/
int pcb_hid_cfg_keys_init(pcb_hid_cfg_keys_t *km)
{
	htpp_init(&km->keys, keyb_hash, keyb_eq);
	return 0;
}

int pcb_hid_cfg_keys_uninit(pcb_hid_cfg_keys_t *km)
{
TODO(": recursive free of nodes")
	htpp_uninit(&km->keys);
	return 0;
}

pcb_hid_cfg_keyseq_t *pcb_hid_cfg_keys_add_under(pcb_hid_cfg_keys_t *km, pcb_hid_cfg_keyseq_t *parent, pcb_hid_cfg_mod_t mods, unsigned short int key_raw, unsigned short int key_tr, int terminal, const char **errmsg)
{
	pcb_hid_cfg_keyseq_t *ns;
	hid_cfg_keyhash_t addr;
	htpp_t *phash = (parent == NULL) ? &km->keys : &parent->seq_next;

	/* do not grow the tree under actions */
	if ((parent != NULL) && (parent->action_node != NULL)) {
		if (*errmsg != NULL)
			*errmsg = "unreachable multikey combo: prefix of the multikey stroke already has an action";
		return NULL;
	}

	addr.mods = mods;
	addr.key_raw = key_raw;
	addr.key_tr = key_tr;

	/* already in the tree */
	ns = htpp_get(phash, &addr);
	if (ns != NULL) {
		if (terminal) {
			if (*errmsg != NULL)
				*errmsg = "duplicate: already registered";
			return NULL; /* full-path-match is collision */
		}
		return ns;
	}

	/* new node on this level */
	ns = calloc(sizeof(pcb_hid_cfg_keyseq_t), 1);
	if (!terminal)
		htpp_init(&ns->seq_next, keyb_hash, keyb_eq);

	ns->addr.mods = mods;
	ns->addr.key_raw = key_raw;
	ns->addr.key_tr = key_tr;

	htpp_set(phash, &ns->addr, ns);
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
		pcb_message(PCB_MSG_ERROR, "key sym name too long\n");
		return 0;
	}
	strncpy(tmp, desc, len);
	tmp[len] = '\0';

	if (km->auto_tr != NULL) {
		const pcb_hid_cfg_keytrans_t *t;
		for(t = km->auto_tr; t->name != NULL; t++) {
			if (pcb_strcasecmp(tmp, t->name) == 0) {
				tmp[0] = t->sym;
				tmp[1] = '\0';
				len = 1;
				break;
			}
		}
	}

	return km->translate_key(tmp, len);
}

static int parse_keydesc(pcb_hid_cfg_keys_t *km, const char *keydesc, pcb_hid_cfg_mod_t *mods, unsigned short int *key_raws, unsigned short int *key_trs, int arr_len, const lht_node_t *loc)
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
			pcb_message(PCB_MSG_ERROR, "Missing <key> in the key description: '%s' at %s:%ld\n", keydesc, loc->file_name, loc->line+1);
			return -1;
		}
		len -= k-last;
		k++; len--;
		if (pcb_strncasecmp(k, "key>", 4) == 0) {
			k+=4; len-=4;
			key_raws[slen] = translate_key(km, k, len);
			key_trs[slen] = 0;
		}
		else if (pcb_strncasecmp(k, "char>", 5) == 0) {
			k+=5; len-=5;
			key_raws[slen] = 0;
			if (!isalnum(*k))
				key_trs[slen] = *k;
			else
				key_trs[slen] = 0;
			k++; len--;
		}
		else {
			pcb_message(PCB_MSG_ERROR, "Missing <key> or <char> in the key description starting at %s at %s:%ld\n", k-1, loc->file_name, loc->line+1);
			return -1;
		}

		if ((key_raws[slen] == 0) && (key_trs[slen] == 0)) {
			char *s;
			s = malloc(len+1);
			memcpy(s, k, len);
			s[len] = '\0';
			pcb_message(PCB_MSG_ERROR, "Unrecognised key symbol in key description: %s at %s:%ld\n", s, loc->file_name, loc->line+1);
			free(s);
			return -1;
		}

		slen++;
		curr = next;
	} while(curr != NULL);
	return slen;
}

int pcb_hid_cfg_keys_add_by_strdesc_(pcb_hid_cfg_keys_t *km, const char *keydesc, const lht_node_t *action_node, pcb_hid_cfg_keyseq_t **out_seq, int out_seq_len)
{
	pcb_hid_cfg_mod_t mods[HIDCFG_MAX_KEYSEQ_LEN];
	unsigned short int key_raws[HIDCFG_MAX_KEYSEQ_LEN];
	unsigned short int key_trs[HIDCFG_MAX_KEYSEQ_LEN];
	pcb_hid_cfg_keyseq_t *lasts;
	int slen, n;
	const char *errmsg;

	slen = parse_keydesc(km, keydesc, mods, key_raws, key_trs, HIDCFG_MAX_KEYSEQ_LEN, action_node);
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

		s = pcb_hid_cfg_keys_add_under(km, lasts, mods[n], key_raws[n], key_trs[n], terminal, &errmsg);
		if (s == NULL) {
			pcb_message(PCB_MSG_ERROR, "Failed to add hotkey binding: %s: %s\n", keydesc, errmsg);
TODO(": free stuff?")
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

int pcb_hid_cfg_keys_add_by_strdesc(pcb_hid_cfg_keys_t *km, const char *keydesc, const lht_node_t *action_node)
{
	return pcb_hid_cfg_keys_add_by_strdesc_(km, keydesc, action_node, NULL, 0);
}

int pcb_hid_cfg_keys_add_by_desc_(pcb_hid_cfg_keys_t *km, const lht_node_t *keydescn, const lht_node_t *action_node, pcb_hid_cfg_keyseq_t **out_seq, int out_seq_len)
{
	switch(keydescn->type) {
		case LHT_TEXT: return pcb_hid_cfg_keys_add_by_strdesc_(km, keydescn->data.text.value, action_node, out_seq, out_seq_len);
		case LHT_LIST:
		{
			int ret = -1, cnt;
			lht_node_t *n;
			for(n = keydescn->data.list.first, cnt = 0; n != NULL; n = n->next, cnt++) {
				if (n->type != LHT_TEXT)
					break;
				if (cnt == 0)
					ret = pcb_hid_cfg_keys_add_by_strdesc_(km, n->data.text.value, action_node, out_seq, out_seq_len);
				else
					pcb_hid_cfg_keys_add_by_strdesc_(km, n->data.text.value, action_node, NULL, 0);
			}
			return ret;
		}
		default:;
	}
	return -1;
}

int pcb_hid_cfg_keys_add_by_desc(pcb_hid_cfg_keys_t *km, const lht_node_t *keydescn, const lht_node_t *action_node)
{
	return pcb_hid_cfg_keys_add_by_desc_(km, keydescn, action_node, NULL, 0);
}

static void gen_accel(gds_t *s, pcb_hid_cfg_keys_t *km, const char *keydesc, int *cnt, const char *sep, const lht_node_t *loc)
{
	pcb_hid_cfg_mod_t mods[HIDCFG_MAX_KEYSEQ_LEN];
	unsigned short int key_raws[HIDCFG_MAX_KEYSEQ_LEN];
	unsigned short int key_trs[HIDCFG_MAX_KEYSEQ_LEN];
	int slen, n;

	slen = parse_keydesc(km, keydesc, mods, key_raws, key_trs, HIDCFG_MAX_KEYSEQ_LEN, loc);
	if (slen <= 0)
		return;

	if (*cnt > 0)
		gds_append_str(s, sep);

	for(n = 0; n < slen; n++) {
		char buff[64];

		if (n > 0)
			gds_append(s, ' ');

		
		if (key_raws[n]) {
			if (km->key_name(key_raws[n], buff, sizeof(buff)) != 0)
				strcpy(buff, "<unknown>");
		}
		else if (key_trs[n] != 0) {
			buff[0] = key_trs[n];
			buff[1] ='\0';
		}
		else
			strcpy(buff, "<empty?>");

		if (mods[n] & PCB_M_Alt)   gds_append_str(s, "Alt-");
		if (mods[n] & PCB_M_Ctrl)  gds_append_str(s, "Ctrl-");
		if (mods[n] & PCB_M_Shift) gds_append_str(s, "Shift-");
		gds_append_str(s, buff);
	}
}

char *pcb_hid_cfg_keys_gen_accel(pcb_hid_cfg_keys_t *km, const lht_node_t *keydescn, unsigned long mask, const char *sep)
{
	gds_t s;
	int cnt = 0;

	memset(&s, 0, sizeof(s));

	switch(keydescn->type) {
		case LHT_TEXT:
			if (mask & 1)
				gen_accel(&s, km, keydescn->data.text.value, &cnt, sep, keydescn);
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
				gen_accel(&s, km, n->data.text.value, &cnt, sep, keydescn);
			}
		}
		default:;
	}
	return s.array;
}


int pcb_hid_cfg_keys_input_(pcb_hid_cfg_keys_t *km, pcb_hid_cfg_mod_t mods, unsigned short int key_raw, unsigned short int key_tr, pcb_hid_cfg_keyseq_t **seq, int *seq_len)
{
	pcb_hid_cfg_keyseq_t *ns;
	hid_cfg_keyhash_t addr;
	htpp_t *phash = (*seq_len == 0) ? &km->keys : &((seq[(*seq_len)-1])->seq_next);

	if (key_raw == 0) {
		static int warned = 0;
		if (!warned) {
			pcb_message(PCB_MSG_ERROR, "Keyboard translation error: probably the US layout is not enabled. Some hotkeys may not work properly\n");
			warned = 1;
		}
		/* happens if only the translated version is available. Try to reverse
		   engineer it as good as we can. */
		if (isalpha(key_tr)) {
			if (isupper(key_tr)) {
				key_raw = tolower(key_tr);
				mods |= PCB_M_Shift;
			}
			else
				key_raw = key_tr;
		}
		else if (isdigit(key_tr))
			key_raw = key_tr;
		/* the rest: punctuations should be handled by key_tr; special keys: well... */
	}

	/* first check for base key + mods */
	addr.mods = mods;
	addr.key_raw = key_raw;
	addr.key_tr = 0;
	
	ns = htpp_get(phash, &addr);

	/* if not found, try with translated key + limited mods */
	if (ns == NULL) {
		addr.mods = mods & PCB_M_Ctrl;
		addr.key_raw = 0;
		addr.key_tr = key_tr;
		ns = htpp_get(phash, &addr);
	}

	/* already in the tree */
	if (ns == NULL) {
		(*seq_len) = 0;
		return -1;
	}

	seq[*seq_len] = ns;
	(*seq_len)++;

	/* found a terminal node with an action */
	if (ns->action_node != NULL) {
		km->seq_len_action = *seq_len;
		(*seq_len) = 0;
		pcb_event(NULL, PCB_EVENT_USER_INPUT_KEY, NULL);
		return km->seq_len_action;
	}

	pcb_event(NULL, PCB_EVENT_USER_INPUT_KEY, NULL);
	return 0;
}

int pcb_hid_cfg_keys_input(pcb_hid_cfg_keys_t *km, pcb_hid_cfg_mod_t mods, unsigned short int key_raw, unsigned short int key_tr)
{
	return pcb_hid_cfg_keys_input_(km, mods, key_raw, key_tr, km->seq, &km->seq_len);
}

int pcb_hid_cfg_keys_action_(pcb_hid_cfg_keyseq_t **seq, int seq_len)
{
	int res;

	if (seq_len < 1)
		return -1;

	res = pcb_hid_cfg_action(seq[seq_len-1]->action_node);
	pcb_event(NULL, PCB_EVENT_USER_INPUT_POST, NULL);
	return res;
}

int pcb_hid_cfg_keys_action(pcb_hid_cfg_keys_t *km)
{
	int ret = pcb_hid_cfg_keys_action_(km->seq, km->seq_len_action);
	km->seq_len_action = 0;
	return ret;
}

int pcb_hid_cfg_keys_seq_(pcb_hid_cfg_keys_t *km, pcb_hid_cfg_keyseq_t **seq, int seq_len, char *dst, int dst_len)
{
	int n, sum = 0;
	char *end = dst;

	dst_len -= 25; /* make room for a full key with modifiers, the \0 and the potential ellipsis */

	for(n = 0; n < seq_len; n++) {
		int k = seq[n]->addr.key_raw, mods = seq[n]->addr.mods, ll = 0, l;

		if (n != 0) {
			*end = ' ';
			end++;
			ll = 1;
		}

		if (mods & PCB_M_Alt)   { strncpy(end, "Alt-", dst_len); end += 4; ll += 4; }
		if (mods & PCB_M_Ctrl)  { strncpy(end, "Ctrl-", dst_len); end += 5; ll += 5; }
		if (mods & PCB_M_Shift) { strncpy(end, "Shift-", dst_len); end += 6; ll += 6; }

		if (k == 0)
			k = seq[n]->addr.key_tr;

		if (km->key_name(k, end, dst_len) == 0) {
			l = strlen(end);
		}
		else {
			strncpy(end, "<unknown>", dst_len);
			l = 9;
		}

		ll += l;

		sum += ll;
		dst_len -= ll;
		end += l;

		if (dst_len <= 1) {
			strcpy(dst, " ...");
			sum += 4;
			dst_len -= 4;
			end += 4;
			break;
		}
	}
	*end = '\0';
	return sum;
}

int pcb_hid_cfg_keys_seq(pcb_hid_cfg_keys_t *km, char *dst, int dst_len)
{
	if (km->seq_len_action > 0)
		return pcb_hid_cfg_keys_seq_(km, km->seq, km->seq_len_action, dst, dst_len);
	else
		return pcb_hid_cfg_keys_seq_(km, km->seq, km->seq_len, dst, dst_len);
}
